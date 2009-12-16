/*
 * ps2fs/super.c
 *
 * Copyright (c) 2002 Andrew Church <achurch@achurch.org>
 */

#include <linux/config.h>
#include <linux/module.h>
#include <linux/string.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/blkdev.h>
#include <asm/uaccess.h>

#include "ps2fs_fs.h"
#include "ps2fs_fs_sb.h"
#include "ps2_partition.h"

/*************************************************************************/

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,4,0)
#define strspn my_strspn
static int strspn(const char *s, const char *accept)
{
    int count = 0;
    while (strchr(accept, *s++))
	count++;
    return count;
}
#endif

/*************************************************************************/

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,4,0)
static int ps2fs_statfs(struct super_block *sb, struct statfs *sf);
#else
static int ps2fs_statfs(struct super_block *sb, struct statfs *sf, int bufsiz);
#endif
static int ps2fs_remount_fs(struct super_block *sb, int *flags, char *data);

static struct super_operations ps2fs_sops = {
    read_inode:		ps2fs_read_inode,
    write_inode:	ps2fs_write_inode,
    put_inode:		ps2fs_put_inode,
    delete_inode:	ps2fs_delete_inode,
    statfs:		ps2fs_statfs,
    remount_fs:		ps2fs_remount_fs,
};

/*************************************************************************/
/*************************************************************************/

/* Parse options into variables; return 1 on success, or 0 if an
 * unsupported option is found.
 */

static int parse_options(char *options, char opt_partition[PS2_PART_IDMAX+2],
			 int *opt_tzoffset)
{
    char *s;

    while (options && *options) {
	if (strncmp(options, "partition=", 10) == 0) {
	    options += 10;
	    if (!*options || *options == ',') {
		printk("ps2fs: partition ID missing\n");
		return 0;
	    }
	    s = strpbrk(options, ",");
	    if (s)
		*s++ = 0;
	    else
		s = options + strlen(options);
	    strncpy(opt_partition, options, PS2_PART_IDMAX+1);
	    opt_partition[PS2_PART_IDMAX+1] = 0;
	    if (strlen(opt_partition) > PS2_PART_IDMAX) {
		printk("ps2fs: partition ID too long (%d chars max)",
		       PS2_PART_IDMAX);
		return 0;
	    }
	    options = s;
	} else if (strncmp(options, "tzoffset=", 9) == 0) {
	    options += 9;
	    if (!*options || *options == ',') {
		printk("ps2fs: time zone offset missing\n");
		return 0;
	    }
	    *opt_tzoffset = simple_strtol(options, &options, 0);
	    if (*options && *options != ',') {
		printk("ps2fs: tzoffset option requires an integer\n");
		return 0;
	    }
	} else {
	    s = strpbrk(options, "=");
	    if (s)
		*s = 0;
	    printk("ps2fs: unrecognized option `%s'\n", options);
	    return 0;
	}
	/* PS2 Linux (beta) chokes on the next line if *options == 0.
	 * This is believed to be due to a compiler bug:
         * strspn("\0\0\0\0\0\0\0\0\0\0\0\0\0\0\x91", ",") returns 14,
	 * not 0 as it should.  So make an explicit check for end-of-
	 * string. */
	if (*options)
	    options += strspn(options, ",");
    }
    return 1;
}

/*************************************************************************/

/* Read the superblock of a to-be-mounted filesystem.  Returns the
 * superblock structure passed in on success, NULL on failure.
 */

struct super_block *ps2fs_read_super(struct super_block *sb, void *data,
				     int silent)
{
    kdev_t dev = sb->s_dev;
    int blocksize;
    struct buffer_head *bh;
    struct ps2fs_super_block *ps2sb;
    struct ps2fs_sb_info *sbinfo = PS2FS_SB(sb);
    char opt_partition[PS2_PART_IDMAX+2] = "";
    int opt_tzoffset = 0;
    int i;
    char *s;

    /* Parse options */
    if (!parse_options((char *)data, opt_partition, &opt_tzoffset))
	return NULL;

    /* Get the hardware block size */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,4,0)
    blocksize = get_hardsect_size(dev);
#else
    blocksize = get_hardblocksize(dev);
    if (blocksize < BLOCK_SIZE)
	blocksize = BLOCK_SIZE;
#endif

    /* Locate the partition given by the "partition=" option, if any */
    if (*opt_partition)
	s = opt_partition;
    else
	s = NULL;
    i = find_partition(dev, blocksize, s, sbinfo->first_sector, sbinfo->size);
    if (i < 0)
	return NULL;
    sbinfo->n_subparts = i;

    /* Read the super block from address 0x400000 in the first subpart */
    bh = bread(dev, sbinfo->first_sector[0] + (0x400000/blocksize), blocksize);
    if (!bh) {
	printk("ps2fs: unable to read superblock\n");
	return NULL;
    }
    ps2sb = (struct ps2fs_super_block *)(bh->b_data);

    /* Check the magic value and save other values */
    if (le32_to_cpu(ps2sb->magic) != PS2FS_SUPER_MAGIC) {
	printk("ps2fs: bad magic number in superblock\n");
	brelse(bh);
	return NULL;
    }
    i = le32_to_cpu(ps2sb->blocksize) / blocksize;
    sbinfo->block_shift = 0;
    while (i > 1) {
	sbinfo->block_shift++;
	i >>= 1;
    }
    sbinfo->root_inode = le32_to_cpu(ps2sb->rootdir);
    if (sbinfo->root_inode < (0x400000 / le32_to_cpu(ps2sb->blocksize)) + 2) {
	ps2fs_warning(sb, "ps2fs_read_super", "root inode number (%d) too"
		      " small", sbinfo->root_inode);
    }
    sbinfo->tzoffset = opt_tzoffset*60;

    /* Free the superblock data */
    brelse(bh);

    /* Set various superblock entries */
    sb->s_blocksize = blocksize;
    sb->s_blocksize_bits = 0;
    i = blocksize;
    while (i > 1) {
	sb->s_blocksize_bits++;
	i >>= 1;
    }

    /* Retrieve root inode */
    sb->s_op = &ps2fs_sops;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,4,0)
    sb->s_root = d_alloc_root(iget(sb, sbinfo->root_inode));
#else
    sb->s_root = d_alloc_root(iget(sb, sbinfo->root_inode), NULL);
#endif
    if (!sb->s_root
     || !S_ISDIR(sb->s_root->d_inode->i_mode)
     || !sb->s_root->d_inode->i_blocks
     || !sb->s_root->d_inode->i_size
    ) {
	if (sb->s_root) {
	    ps2fs_error(sb, "ps2fs_read_super", "root inode corrupt!"
			" (mode=0%o blocks=%d size=%ld)",
			sb->s_root->d_inode->i_mode,
			sb->s_root->d_inode->i_blocks,
			sb->s_root->d_inode->i_size);
	    dput(sb->s_root);
	    sb->s_root = NULL;
	} else {
	    ps2fs_error(sb, "ps2fs_read_super", "unable to read root inode");
	}
	return NULL;
    }

    /* Successful completion */
    return sb;
}

/*************************************************************************/

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,4,0)
static int ps2fs_statfs(struct super_block *sb, struct statfs *sf)
#else
static int ps2fs_statfs(struct super_block *sb, struct statfs *sf, int bufsiz)
#endif
{
    struct ps2fs_sb_info *sbinfo = PS2FS_SB(sb);
    long count, size;
    int i;

    size = 0;
    for (i = 0; i < sbinfo->n_subparts; i++)
	size += sbinfo->size[i];
    count = count_used_blocks(sb);
    if (count < 0)
	return count;
    sf->f_type = PS2FS_SUPER_MAGIC;
    sf->f_bsize = sb->s_blocksize;
    sf->f_blocks = size - (sbinfo->root_inode << sbinfo->block_shift);
    sf->f_bfree = sf->f_blocks - count;
    sf->f_bavail = sf->f_bfree;
    sf->f_files = -1;
    sf->f_ffree = -1;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,4,0)
    sf->f_fsid.val[0] = -1;
    sf->f_fsid.val[1] = -1;
#endif
    sf->f_namelen = PS2FS_NAME_LEN;
    return 0;
}

/*************************************************************************/

static int ps2fs_remount_fs(struct super_block *sb, int *flags, char *data)
{
    return -ENOSYS;  /*FIXME*/
}

/*************************************************************************/
/*************************************************************************/

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,4,0)

static DECLARE_FSTYPE_DEV(ps2fs_fs_type, "ps2fs", ps2fs_read_super);

static int __init init_ps2fs_fs(void)
{
    return register_filesystem(&ps2fs_fs_type);
}

static void __exit exit_ps2fs_fs(void)
{
    unregister_filesystem(&ps2fs_fs_type);
}

#else

static struct file_system_type ps2fs_fs_type = {
    "ps2fs", FS_REQUIRES_DEV, ps2fs_read_super, NULL
};

__initfunc(int init_ps2fs_fs(void))
{
    return register_filesystem(&ps2fs_fs_type);
}

#endif

/*************************************************************************/

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,4,0)
MODULE_AUTHOR("Andrew Church <achurch@achurch.org>");
MODULE_DESCRIPTION("Playstation 2 filesystem");
MODULE_LICENSE("GPLish");

EXPORT_NO_SYMBOLS;

module_init(init_ps2fs_fs)
module_exit(exit_ps2fs_fs)

#else

EXPORT_NO_SYMBOLS;

int init_module(void) { return init_ps2fs_fs(); }

void cleanup_module(void) { unregister_filesystem(&ps2fs_fs_type); }

#endif

/*************************************************************************/

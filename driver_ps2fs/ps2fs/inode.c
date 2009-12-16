/*
 * ps2fs/inode.c
 *
 * Copyright (c) 2002 Andrew Church <achurch@achurch.org>
 */

#include <linux/config.h>
#include <linux/version.h>
#include <linux/string.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/locks.h>
#include <linux/blkdev.h>
#include <asm/uaccess.h>

#include "ps2fs_fs.h"
#include "ps2fs_fs_sb.h"
#include "ps2fs_fs_i.h"

/*************************************************************************/

/* for inode versions */
static unsigned long version = 0;

/*************************************************************************/

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,4,0)
static int ps2fs_readpage(struct file *file, struct page *page);
static int ps2fs_writepage(struct page *page);
static int ps2fs_prepare_write(struct file *file, struct page *page,
			       unsigned int from, unsigned int to);
#else /* < 2.4.0 */
static int ps2fs_file_write(struct file *file, const char *buf, size_t count,
			    loff_t *ppos);
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,4,0)
struct address_space_operations ps2fs_aops = {
    readpage:		ps2fs_readpage,
    writepage:		ps2fs_writepage,
    sync_page:		block_sync_page,
    prepare_write:	ps2fs_prepare_write,
    commit_write:	generic_commit_write,
    /* bmap:		ps2fs_bmap, */
};
#endif

struct file_operations ps2fs_fops = {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,4,0)
    llseek:	generic_file_llseek,
    read:	generic_file_read,
    write:	generic_file_write,
    mmap:	generic_file_mmap,
    open:	generic_file_open,
#else
    read:	generic_file_read,
    write:	ps2fs_file_write,
    mmap:	generic_file_mmap,
#endif
};

struct inode_operations ps2fs_file_iops = {
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,4,0)
    default_file_ops: &ps2fs_fops,
    readpage:	generic_readpage,
    bmap:	ps2fs_bmap,
#endif
    truncate:	ps2fs_truncate,
};

/*************************************************************************/
/*************************************************************************/

/* Read an inode into the given buffer (ps2inode) and check its magic
 * number and checksum.  Returns 0 on success, negative on error.
 */

static int ps2fs_read_raw_inode(struct inode *inode,
				struct ps2fs_inode *ps2inode)
{
    __u32 *inodebuf = (__u32 *)ps2inode;
    struct buffer_head *bh;
    unsigned long sector;
    __u32 checksum;
    unsigned int i;

    /* Calculate sector number */
    sector = PS2FS_SB(inode->i_sb)->first_sector[PS2FS_SUBPART(inode->i_ino)]
	   + (PS2FS_BNUM(inode->i_ino) << PS2FS_SB(inode->i_sb)->block_shift);

    /* Try to read the inode data in */
    if (inode->i_sb->s_blocksize >= 1024 /*sizeof(struct ps2fs_inode)*/) {
	bh = bread(inode->i_dev, sector, inode->i_sb->s_blocksize);
	if (!bh)
	    goto bad_block;
	memcpy(inodebuf, bh->b_data, 1024);
	brelse(bh);
    } else if (inode->i_sb->s_blocksize == 512) {
	bh = bread(inode->i_dev, sector, 512);
	if (!bh)
	    goto bad_block;
	memcpy(inodebuf, bh->b_data, 512);
	brelse(bh);
	sector++;
	bh = bread(inode->i_dev, sector, 512);
	if (!bh)
	    goto bad_block;
	memcpy(inodebuf+128, bh->b_data, 512);
	brelse(bh);
    } else {
	ps2fs_error(inode->i_sb, "ps2fs_read_raw_inode", "unsupported block"
		    " size %lu", inode->i_sb->s_blocksize);
	return -EIO;
    }

    /* Sanity-check the read-in data */
    if (le32_to_cpu(ps2inode->magic) != PS2FS_INODE_MAGIC) {
	ps2fs_error(inode->i_sb, "ps2fs_read_raw_inode", "bad magic number"
		    " for inode 0x%08lX", inode->i_ino);
	return -EIO;
    }
    checksum = 0;
    for (i = 1; i < 255; i++)
	checksum += le32_to_cpu(inodebuf[i]);
    if (checksum != le32_to_cpu(ps2inode->checksum)) {
	ps2fs_error(inode->i_sb, "ps2fs_read_raw_inode", "bad checksum for"
		    " inode 0x%08lX", inode->i_ino);
	return -EIO;
    }

    /* Success */
    return 0;

  bad_block:
    ps2fs_error(inode->i_sb, "ps2fs_read_raw_inode", "unable to read inode"
		" block (%lu)", sector);
    return -EIO;
}

/*************************************************************************/

/* Write an inode to disk (really to a bh), setting the checksum
 * appropriately.  The raw data is taken from ps2fs_inode_info.rawinode.
 * If `sync' is nonzero, physically write the data to disk.  Returns zero
 * on success, else error code.
 */

static int ps2fs_write_raw_inode(struct inode *inode, int sync)
{
    struct ps2fs_inode *ps2inode = PS2FS_INODE(inode)->rawinode;
    __u32 *inodebuf = (__u32 *)ps2inode;
    struct buffer_head *bh;
    unsigned long sector;
    __u32 checksum;
    unsigned int i;

    /* Sanity-check the data */
    if (le32_to_cpu(ps2inode->magic) != PS2FS_INODE_MAGIC) {
	ps2fs_error(inode->i_sb, "ps2fs_write_raw_inode", "bad magic number"
		    " for inode 0x%08lX", inode->i_ino);
	return -EIO;
    }

    /* Calculate checksum */
    checksum = 0;
    for (i = 1; i < 255; i++)
	checksum += le32_to_cpu(inodebuf[i]);
    ps2inode->checksum = cpu_to_le32(checksum);

    /* Calculate sector number */
    sector = PS2FS_SB(inode->i_sb)->first_sector[PS2FS_SUBPART(inode->i_ino)]
	   + (PS2FS_BNUM(inode->i_ino) << PS2FS_SB(inode->i_sb)->block_shift);

    /* Try to write the inode data out */
    if (inode->i_sb->s_blocksize >= 1024 /*sizeof(struct ps2fs_inode)*/) {
	bh = bread(inode->i_dev, sector, inode->i_sb->s_blocksize);
	if (!bh)
	    goto bad_block;
	memcpy(bh->b_data, inodebuf, 1024);
	mark_buffer_dirty_inode(bh, inode);
	if (sync) {
dprintk("syncing! 0x%08lX at 0x%08lX\n",inode->i_ino,bh->b_blocknr);
	    ll_rw_block(WRITE, 1, &bh);
	    wait_on_buffer(bh);
	    if (!buffer_uptodate(bh))
		goto bad_write;
	}
	brelse(bh);
    } else if (inode->i_sb->s_blocksize == 512) {
	struct buffer_head *bh2;
	__u8 savebuf[512];  /* to save bh contents in case of bh2 write fail */

	bh = bread(inode->i_dev, sector, 512);
	if (!bh)
	    goto bad_block;
	sector++;
	bh2 = bread(inode->i_dev, sector, 512);
	if (!bh2)
	    goto bad_block;
	memcpy(savebuf, bh->b_data, 512);
	memcpy(bh->b_data, inodebuf, 512);
	memcpy(bh2->b_data, inodebuf+128, 512);
	mark_buffer_dirty(bh);
	mark_buffer_dirty_inode(bh2, inode);
	if (sync) {
dprintk("syncing 2! 0x%08lX at 0x%08lX size=%d\n",inode->i_ino,bh->b_blocknr,ps2inode->size);
	    ll_rw_block(WRITE, 1, &bh);
	    wait_on_buffer(bh);
	    if (!buffer_uptodate(bh)) {
		brelse(bh2);
		goto bad_write;
	    }
	    ll_rw_block(WRITE, 1, &bh2);
	    wait_on_buffer(bh2);
	    if (!buffer_uptodate(bh2)) {
		memcpy(bh->b_data, savebuf, 512);
		mark_buffer_dirty(bh);
		ll_rw_block(WRITE, 1, &bh);
		wait_on_buffer(bh);
		if (!buffer_uptodate(bh)) {
		    ps2fs_error(inode->i_sb, "ps2fs_write_raw_inode",
				"unable to recover half-written inode 0x%08lX",
				inode->i_ino);
		}
		brelse(bh2);
		goto bad_write;
	    }
	}
	brelse(bh);
	brelse(bh2);
    } else {
	ps2fs_error(inode->i_sb, "ps2fs_write_raw_inode", "unsupported"
		    " block size %lu", inode->i_sb->s_blocksize);
	return -EIO;
    }

    /* Success */
    return 0;

  bad_block:
    ps2fs_error(inode->i_sb, "ps2fs_write_raw_inode", "unable to read inode"
		" block (%lu)", sector);
    return -EIO;
  bad_write:
    ps2fs_error(inode->i_sb, "ps2_write_raw_inode", "write of sector %lu"
		" failed", sector);
    brelse(bh);
    return -EIO;
}

/*************************************************************************/
/*************************************************************************/

/* Create a new inode on the given device.  Returns the inode or an
 * ERR_PTR error value.  If successful, i_mode is set up for a normal file
 * (0644); it should be changed later if the inode is a directory.
 */

struct inode *ps2fs_new_inode(struct super_block *sb)
{
    struct inode *inode;
    struct ps2fs_inode *ps2inode;
    long block;
    int err;

    /* Allocate memory for the raw inode buffer */
    ps2inode = kmalloc(sizeof(*ps2inode), GFP_KERNEL);
dprintk("+++ kmalloced *NEW*\n");
    if (!ps2inode) {
	ps2fs_error(sb, "ps2fs_new_inode", "no memory for raw inode");
	return ERR_PTR(-ENOMEM);
    }

    /* Create the new inode's in-memory structure */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,4,0)
    inode = get_empty_inode();
#else
    inode = new_inode(sb);
#endif
    if (!inode) {
	err = -ENOMEM;
	goto fail_kfree;
    }

    /* Obtain a free block, if possible */
    block = get_new_block(sb, 0);
    if (block < 0) {
	err = (int)block;
	goto fail_inode;
    }

    /* Set up the inode structure */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,4,0)
    inode->i_sb = sb;
    inode->i_dev = sb->s_dev;
#endif
    inode->i_ino = block;
    inode->i_blksize = PAGE_SIZE;
    inode->i_uid = 0;
    inode->i_gid = 0;
    inode->i_mode = S_IFREG | S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
    inode->i_size = 0;
    inode->i_blocks = 0;
    inode->i_atime = inode->i_ctime = inode->i_mtime = CURRENT_TIME;
    inode->i_version = ++version;
    PS2FS_INODE(inode)->rawinode = ps2inode;
    memset(ps2inode, 0, sizeof(*ps2inode));
    ps2inode->unknown1[0].number = cpu_to_le32(PS2FS_BNUM(block));
    ps2inode->unknown1[0].subpart = PS2FS_SUBPART(block);
    ps2inode->unknown1[0].count = cpu_to_le16(1);
    ps2inode->unknown1[2] = ps2inode->unknown1[0];
    ps2inode->unknown1[4] = ps2inode->unknown1[0];

    /* Mark the inode dirty, to ensure it gets written back later */
    mark_inode_dirty(inode);

    /* Success, return the inode */
    return inode;

    /* Failure cleanup */
  fail_inode:
    make_bad_inode(inode);
    iput(inode);
  fail_kfree:
dprintk("... kfreeing %08lX\n",inode->i_ino);
    kfree(ps2inode);
    return ERR_PTR(err);
}

/*************************************************************************/

/* Read in the given inode. */

void ps2fs_read_inode(struct inode *inode)
{
    struct super_block *sb = inode->i_sb;
    struct ps2fs_sb_info *sbinfo = PS2FS_SB(sb);
    struct ps2fs_inode *ps2inode = NULL;
    unsigned long blocksize;

dprintk("read_inode(%08lX) mode=%07o\n",inode->i_ino,(int)inode->i_mode);
    /* Sanity check on inode number */
    if (inode->i_ino < sbinfo->root_inode
     || PS2FS_SUBPART(inode->i_ino) >= sbinfo->n_subparts
     || PS2FS_BNUM(inode->i_ino) >=
	    sbinfo->size[PS2FS_SUBPART(inode->i_ino)]>>sbinfo->block_shift
    ) {
	ps2fs_error(sb, "ps2fs_read_inode", "bad inode number: %lu",
		    inode->i_ino);
	goto bad_inode;
    }

    /* Allocate memory for the inode buffer */
    ps2inode = kmalloc(sizeof(*ps2inode), GFP_KERNEL);
dprintk("*** kmalloced 0x%08lX\n",inode->i_ino);
    if (!ps2inode) {
	ps2fs_error(sb, "ps2fs_read_inode", "0x%08lX: no memory for raw inode",
		    inode->i_ino);
	goto bad_inode;
    }
    /* Read the inode in */
    if (ps2fs_read_raw_inode(inode, ps2inode) < 0)
	goto bad_inode;

    /* Copy/convert data into inode structure */
    PS2FS_INODE(inode)->rawinode = ps2inode;
    inode->i_atime = from_ps2time(&ps2inode->atime) - sbinfo->tzoffset;
    inode->i_ctime = from_ps2time(&ps2inode->ctime) - sbinfo->tzoffset;
    inode->i_mtime = from_ps2time(&ps2inode->mtime) - sbinfo->tzoffset;
    inode->i_size = le32_to_cpu(ps2inode->size);
    blocksize = 1 << (9+sbinfo->block_shift);
    inode->i_blocks = (((inode->i_size + (blocksize-1)) & -blocksize) >> 9)
                    + (blocksize>>9);  /* count inode block as well */
    inode->i_mode = S_IRUSR | S_IRGRP | S_IROTH;
    if (!(sb->s_flags & MS_RDONLY))
	inode->i_mode |= S_IWUSR;
    if (inode->i_ino == PS2FS_SB(sb)->root_inode
     || 0  /* FIXME: how to determine this? */
    ) {
	inode->i_mode |= S_IFDIR | S_IXUSR | S_IXGRP | S_IXOTH;
	inode->i_op = &ps2fs_dir_iops;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,4,0)
	inode->i_fop = &ps2fs_dops;
	inode->i_mapping->a_ops = &ps2fs_aops;
#endif
    } else {
	inode->i_mode |= S_IFREG;
	inode->i_op = &ps2fs_file_iops;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,4,0)
	inode->i_fop = &ps2fs_fops;
	inode->i_mapping->a_ops = &ps2fs_aops;
#endif
    }
    inode->i_uid = 0;
    inode->i_gid = 0;
    inode->i_blksize = PAGE_SIZE;  /* optimal I/O size */
    inode->i_version = ++version;

    /* Success */
    return;

  bad_inode:
    if (ps2inode) {
dprintk("... kfreeing 0x%08lX\n",inode->i_ino);
	kfree(ps2inode);
    }
    make_bad_inode(inode);
}

/*************************************************************************/

/* Write out a modified inode, possibly with syncing afterwards.  Returns
 * zero on success, else error code.
 */

int ps2fs_update_inode(struct inode *inode, int sync)
{
    struct super_block *sb = inode->i_sb;
    struct ps2fs_sb_info *sbinfo = PS2FS_SB(sb);
    struct ps2fs_inode *ps2inode = PS2FS_INODE(inode)->rawinode;

dprintk("update_inode(0x%08lX,%d) size=%lu\n",inode->i_ino,sync,(long)inode->i_size);

    /* Don't do anything to inodes with zero link count, they're going to
     * go away anyway */
    if (!inode->i_nlink)
	return 0;

    /* Sanity check on inode number and raw inode pointer */
    if (inode->i_ino < sbinfo->root_inode
     || PS2FS_SUBPART(inode->i_ino) >= sbinfo->n_subparts
     || PS2FS_BNUM(inode->i_ino) >=
	    sbinfo->size[PS2FS_SUBPART(inode->i_ino)]>>sbinfo->block_shift
    ) {
	ps2fs_error(sb, "ps2fs_update_inode", "bad inode number: 0x%08lX",
		    inode->i_ino);
	return -EINVAL;
    }
    if (!ps2inode) {
	ps2fs_error(sb, "ps2fs_update_inode",
		    "inode 0x%08lX: raw inode missing", inode->i_ino);
	return -EINVAL;
    }

    if (sb->s_flags & MS_RDONLY)
	return -EROFS;

    /* Copy/convert data into raw inode structure */
    to_ps2time(&ps2inode->atime, inode->i_atime + sbinfo->tzoffset);
    to_ps2time(&ps2inode->ctime, inode->i_ctime + sbinfo->tzoffset);
    to_ps2time(&ps2inode->mtime, inode->i_mtime + sbinfo->tzoffset);
    ps2inode->size = cpu_to_le32(inode->i_size);

    /* Write the raw inode out */
    return ps2fs_write_raw_inode(inode, sync);
}

/*************************************************************************/

/* Write out a modified inode (callback for superblock ops--return is void
 * rather than int, no second parameter in 2.2).
 */

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,4,0)
void ps2fs_write_inode(struct inode *inode)
{
    ps2fs_update_inode(inode, 0);
}
#else
void ps2fs_write_inode(struct inode *inode, int wait)
{
    ps2fs_update_inode(inode, wait);
}
#endif

/*************************************************************************/

/* iput() callback; called just before an inode's usage count is
 * decremented.  If this is the last user of the inode, we free the raw
 * inode memory.
 */

void ps2fs_put_inode(struct inode *inode)
{
    if (atomic_read(&inode->i_count) == 1) {
	struct ps2fs_inode *ps2inode = PS2FS_INODE(inode)->rawinode;
	if (ps2inode) {
dprintk("--- kfreeing 0x%08lX\n",inode->i_ino);
	    kfree(ps2inode);
	} else {
	    ps2fs_error(inode->i_sb, "ps2fs_put_inode",
			"inode 0x%08lX: rawinode is NULL", inode->i_ino);
	}
    }
}

/*************************************************************************/

/* Callback for deleting an inode (i.e. i_nlink zero at last iput()). */

void ps2fs_delete_inode(struct inode *inode)
{
    inode->i_size = 0;
    ps2fs_truncate(inode);
    if (free_block(inode->i_sb, inode->i_ino) < 0) {
	ps2fs_error(inode->i_sb, "ps2fs_delete_inode",
		    "cannot free inode block 0x%08lX", inode->i_ino);
    }
    clear_inode(inode);  /* must be called by delete_inode() callback */
}

/*************************************************************************/
/*************************************************************************/

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,4,0)

/* Read in a page of a file. */
static int ps2fs_readpage(struct file *file, struct page *page)
{
    return block_read_full_page(page, ps2fs_get_block);
}

/* Write out a page of a file. */
static int ps2fs_writepage(struct page *page)
{
    return block_write_full_page(page, ps2fs_get_block);
}

/* Prepare to write a page. */
static int ps2fs_prepare_write(struct file *file, struct page *page,
			       unsigned int from, unsigned int to)
{
    return block_prepare_write(page, from, to, ps2fs_get_block);
}

#else  /* kernel < 2.4.0 */

/* Return the sector number for the given inode and block. */
int ps2fs_bmap(struct inode *inode, int iblock)
{
    struct buffer_head result;
    if (ps2fs_get_block(inode, iblock, &result, 0) < 0)
	return 0;
    return result.b_blocknr;
}

/* Write to a file. */
static int ps2fs_file_write(struct file *file, const char *buf, size_t count,
			    loff_t *ppos)
{
    struct inode *inode = file->f_dentry->d_inode;
    loff_t pos;
    struct super_block *sb = inode->i_sb;
    int blocksize = 1 << (sb->s_blocksize_bits + PS2FS_SB(sb)->block_shift);
    int block, lastblock;
    int error = -EIO;
    int written = 0;

    if (!count)
	return 0;
    if (sb->s_flags & MS_RDONLY)
	return -EROFS;
    pos = *ppos;
    if (pos > (__u32)(pos+count)) {
	count = ~pos;	/* 0xFFFFFFFF-pos */
	if (!count)
	    return -EFBIG;
    }
    lastblock = -1;
    while (count > 0) {
	struct buffer_head *bh;
	struct buffer_head tmpbh;  /* to receive block numbers */
	int to_write, secofs;

	block = pos / blocksize;
	if (block != lastblock) {
	    error = ps2fs_get_block(inode, block, &tmpbh, 0);
	    if (error)
		error = ps2fs_get_block(inode, block, &tmpbh, 1);
	    if (error)
		goto ioerr;
	    lastblock = block;
	}
	tmpbh.b_blocknr += (pos & (blocksize-1)) >> sb->s_blocksize_bits;
	bh = bread(tmpbh.b_dev, tmpbh.b_blocknr, sb->s_blocksize);
	if (!bh) {
	    error = -EIO;
	    goto ioerr;
	}
	secofs = pos & (sb->s_blocksize-1);
	to_write = sb->s_blocksize-secofs;
	if (to_write > count)
	    to_write = count;
	to_write -= copy_from_user(bh->b_data+secofs, buf, to_write);
	if (!to_write) {
	    /* Couldn't copy any data from userspace */
	    brelse(bh);
	    error = -EFAULT;
	    goto ioerr;
	}
	update_vm_cache(inode, pos, bh->b_data+secofs, to_write);
	mark_buffer_uptodate(bh, 1);
	mark_buffer_dirty(bh);
	if (file->f_flags & O_SYNC) {
	    ll_rw_block(WRITE, 1, &bh);
	    wait_on_buffer(bh);
	    if (!buffer_uptodate(bh)) {
		brelse(bh);
		error = -EIO;
		goto ioerr;
	    }
	}
	brelse(bh);
	written += to_write;
	pos += to_write;
	buf += to_write;
	count -= to_write;
    }
    *ppos = pos;
    if (pos > inode->i_size)  /* not currently possible */
	inode->i_size = pos;
    inode->i_ctime = inode->i_mtime = CURRENT_TIME;
    mark_inode_dirty(inode);
    if (file->f_flags & O_SYNC)
	ps2fs_update_inode(inode, 1);
    return written;

  ioerr:
    return written ? written : error;
}

#endif

/*************************************************************************/

/* Truncate a file to the size given in inode->i_size. */

void ps2fs_truncate(struct inode *inode)
{
    struct ps2fs_inode *ps2inode = PS2FS_INODE(inode)->rawinode;
    struct super_block *sb = inode->i_sb;
    __u32 blocksize = sb->s_blocksize << PS2FS_SB(sb)->block_shift;
    __u32 blocks = (inode->i_size + (blocksize-1))
		   >> (sb->s_blocksize_bits + PS2FS_SB(sb)->block_shift);
    __u32 i, j;

    for (i = 0; i < 113; i++) {  /* FIXME: 113 -> named constant */
	__u32 first = le32_to_cpu(ps2inode->data[i].number);
	__u32 count = le16_to_cpu(ps2inode->data[i].count);
	if (!count)
	    break;
	if (blocks < count) {
	    /* Free some or all blocks */
	    /* FIXME: fix free_block() to free multiple blocks at once */
	    for (j = blocks; j < count; j++)
		free_block(sb, first+j);
	    if (blocks) {
		ps2inode->data[i].count = cpu_to_le16(blocks);
	    } else {
		ps2inode->data[i].number = 0;  /* 0 is 0 in any endianness */
		*((__u32 *)(&ps2inode->data[i].number)+1) = 0;
	    }
	    blocks = 0;
	} else {
	    blocks -= count;
	}
    }
}

/*************************************************************************/
/*************************************************************************/

/* Find the hardware sector number corresponding to the given block of the
 * given file (inode), and set up `bh_result' accordingly.  Returns 0 on
 * success, negative on error.
 */

int ps2fs_get_block(struct inode *inode, long iblock,
		    struct buffer_head *bh_result, int create)
{
    struct ps2fs_inode *ps2inode = PS2FS_INODE(inode)->rawinode;
    int part = PS2FS_SUBPART(inode->i_ino);
    int i;

dprintk("getblock: ino=0x%08lX block=%ld create=%d\n",inode->i_ino,iblock,create);
    if (iblock < 0)
	return -EINVAL;

    for (i = 0; i < 113; i++) {  /* FIXME: 113 -> named constant */
	__u32 count = le16_to_cpu(ps2inode->data[i].count);
dprintk("   %d: %08X/%u\n",i,le32_to_cpu(ps2inode->data[i].number),count);
	if (!count)
	    break;
	count <<= PS2FS_SB(inode->i_sb)->block_shift;
	if (iblock < count) {
	    /* Found it! */
	    bh_result->b_dev = inode->i_dev;
	    bh_result->b_blocknr = PS2FS_SB(inode->i_sb)->first_sector[part]
				 + (le32_to_cpu(ps2inode->data[i].number)
				    << PS2FS_SB(inode->i_sb)->block_shift)
				 + iblock;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,4,0)
	    bh_result->b_state |= 1UL << BH_Mapped;
#endif
dprintk("   found! %08lX\n",bh_result->b_blocknr);
	    return 0;
	}
	/* Not found, try next fragment */
	iblock -= count;
    }

    /* Block does not exist */
    if (create) {
	long block;
	long nextblock;  /* Block after last block in file */

	if (iblock != 0) {
	    /* We don't support holes in files */
	    printk("ps2fs: holes in files not supported\n");
	    return -EIO;
	}
	/* FIXME: is 0x8000-0xFFFF okay in count field? */
	if (i > 0 && le16_to_cpu(ps2inode->data[i-1].count) < 0x7FFF) {
	    nextblock = le32_to_cpu(ps2inode->data[i-1].number)
		      + le16_to_cpu(ps2inode->data[i-1].count);
	} else {
	    nextblock = -1;  /* impossible value */
	}
	if (i == 113) {
	    /* Inode's block table is full.  If we can get the next block
	     * after the last entry, take it. */
	    block = get_new_block(inode->i_sb,
				  PS2FS_MAKE_INUM(nextblock,part));
	} else {
	    /* Any block will do. */
	    block = get_new_block(inode->i_sb, 0);
	}
	if (block < 0)
	    return block;
	if (i > 0 && block == nextblock) {
	    ps2inode->data[i-1].count =
		cpu_to_le16(le16_to_cpu(ps2inode->data[i-1].count) + 1);
	} else {
	    ps2inode->data[i].number = cpu_to_le32(block);
	    ps2inode->data[i].count = cpu_to_le16(1);
	}
	inode->i_ctime = CURRENT_TIME;  /* inode was changed */
	bh_result->b_dev = inode->i_dev;
	bh_result->b_blocknr = PS2FS_SB(inode->i_sb)->first_sector[part]
			     + (block << PS2FS_SB(inode->i_sb)->block_shift);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,4,0)
	bh_result->b_state |= (1UL << BH_New);
#endif
dprintk("   added! %08lX\n",bh_result->b_blocknr); 
	return 0;
    }

    /* !create */
    return -EIO;
}

/*************************************************************************/

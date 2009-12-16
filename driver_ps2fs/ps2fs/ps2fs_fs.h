#ifndef _LINUX_PS2FS_FS_H
#define _LINUX_PS2FS_FS_H

#include <linux/version.h>

/*
 * PS2 filesystem constants/structures
 * Note that "inode numbers" are just block numbers in the PS2 filesystem.
 */

/*************************************************************************/

/* Constants */
#define PS2FS_BLOCKSIZE 	0x2000
#define PS2FS_SUPER_MAGIC	0x50465300	/* "PFS\0" */
#define PS2FS_INODE_MAGIC	0x53454744	/* "SEGD" */

/* Maximum number of subparts we allow (see also ps2fs_fs_sb.h)
 * This MUST NOT be greater than 127 or block/inode numbers will go negative!
 * See PS2FS_SUBPART below.
 */
#define PS2FS_MAX_SUBPARTS	16

/* NOTE: if this is not the maximum possible value of the
 * ps2fs_dirent.namelen field, then buffer overflows can occur in dir.c! */
#define PS2FS_NAME_LEN		255

/* Block number/count pair (used in inodes) */
struct ps2fs_blockinfo {
    __u32 number;
    __u8 subpart;
    __u8 pad;
    __u16 count;
};

/* Date/time descriptor */
struct ps2fs_datetime {
    u_int8_t unused;
    u_int8_t sec;
    u_int8_t min;
    u_int8_t hour;
    u_int8_t day;
    u_int8_t month;
    u_int16_t year;
};

/*************************************************************************/

/* Superblock structure */
struct ps2fs_super_block {
    __u32 magic;
    __u32 unknown1[3];
    __u32 blocksize;
    __u32 unknown2;
    __u32 bitmap_end;	/* First block after the used-block bitmap */
    __u32 unknown3;
    __u32 rootdir;
};

/* Inode structure */
struct ps2fs_inode {
    __u32 checksum;	/* Sum of all other words in the inode */
    __u32 magic;
    struct ps2fs_blockinfo unknown1[5];  /* 0,2,4 point to inode itself */
    struct ps2fs_blockinfo data[113];
    __u32 unknown2[2];
    struct ps2fs_datetime atime;  /* FIXME: order needs to be confirmed */
    struct ps2fs_datetime ctime;  /* ctime seems to be correct */
    struct ps2fs_datetime mtime;
    __u32 size;
    __u32 unknown3[9];
}; /* 1024 bytes */

/* Directory entry structure */
struct ps2fs_dirent {
    __u32 inode;	/* Inode within subpart */
    __u8 subpart;	/* Subpart of partition where file is located */
    __u8 namelen;
    __u16 size_flags;	/* 10-bit entry length plus flags */
    char name[0];	/* Padded to a multiple of 4 bytes */
};
#define PS2FS_DIRENT_SIZEMASK	0x03FF
#define PS2FS_DIRENT_IFMT	0xF000
#define PS2FS_DIRENT_IFDIR	0x1000
#define PS2FS_DIRENT_IFREG	0x2000
#define PS2FS_DIRENT_NAMEMAX	255

#define ps2fs_next_dirent(de) \
    ((struct ps2fs_dirent *)((char *)(de) + (le16_to_cpu(de->size_flags) & PS2FS_DIRENT_SIZEMASK)))

/* We use (subpart<<24 | inode) as the system "inode number". */
#define PS2FS_MAKE_INUM(inode,subpart)	((inode) | (subpart)<<24)
#define PS2FS_BNUM(inode)		((inode) & 0xFFFFFF)
#define PS2FS_SUBPART(inode)		((__u32)(inode) >> 24)

/*************************************************************************/

#ifdef __KERNEL__

/* For debugging */
//#define dprintk(fmt,rest...) printk(KERN_DEBUG fmt , ## rest)
#define dprintk(fmt,rest...)

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,4,0)
# undef atomic_read
# define atomic_read(atomicptr) *(atomicptr)
# define mark_buffer_dirty(bh) mark_buffer_dirty(bh,0)
# define mark_buffer_dirty_inode(bh,inode) mark_buffer_dirty(bh)
# define BUG() do { printk("ps2fs: BUG()\n"); *(char *)0 = 0; } while (0)
#endif

extern struct inode_operations ps2fs_file_iops;
extern struct inode_operations ps2fs_dir_iops;
extern struct file_operations ps2fs_fops;
extern struct file_operations ps2fs_dops;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,4,0)
extern struct address_space_operations ps2fs_aops;
#endif

/**** bitmap.c ****/
extern long count_used_blocks(struct super_block *sb);
extern long get_new_block(struct super_block *sb, long desired);
extern int free_block(struct super_block *sb, long block);

/**** dir.c ****/
extern __s32 last_mode;

/**** inode.c ****/
struct inode *ps2fs_new_inode(struct super_block *sb);
extern void ps2fs_read_inode(struct inode *inode);
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,4,0)
extern void ps2fs_write_inode(struct inode *inode);
#else
extern void ps2fs_write_inode(struct inode *inode, int write);
#endif
extern void ps2fs_put_inode(struct inode *inode);
extern void ps2fs_delete_inode(struct inode *inode);
extern void ps2fs_truncate(struct inode *inode);
extern int ps2fs_get_block(struct inode *inode, long iblock,
			   struct buffer_head *bh_result, int create);
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,4,0)
int ps2fs_bmap(struct inode *inode, int iblock);
#endif

/**** misc.c ****/
extern void ps2fs_warning(struct super_block *sb, const char *function,
			  const char *fmt, ...);
extern void ps2fs_error(struct super_block *sb, const char *function,
			const char *fmt, ...);
extern void ps2fs_panic(struct super_block *sb, const char *function,
			const char *fmt, ...);
extern __s32 from_ps2time(const struct ps2fs_datetime *dt);
extern void to_ps2time(struct ps2fs_datetime *dt, __s32 time);

/**** super.c ****/
extern struct super_block *ps2fs_read_super(struct super_block *sb, void *data,
					    int silent);

/**** unicode.c ****/
extern void translate_from_unicode(unsigned char *buf);
extern int translate_to_unicode(const struct dentry *dentry,
				unsigned char *dest, int destlen);

#endif /* __KERNEL__ */

/*************************************************************************/

#endif  /* PS2FS_FS_H */

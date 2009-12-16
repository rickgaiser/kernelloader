/*
 * ps2fs/bitmap.c
 *
 * Copyright (c) 2002 Andrew Church <achurch@achurch.org>
 */

#include <linux/config.h>
#include <linux/string.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/locks.h>
#include <linux/blkdev.h>
#include <asm/uaccess.h>

#include "ps2fs_fs.h"
#include "ps2fs_fs_sb.h"

/*************************************************************************/

/* Returns the number of (hardware) blocks currently used in the
 * filesystem.  Returns an error code (negative value) on error.
 */

long count_used_blocks(struct super_block *sb)
{
    static int bitcount[256] = {
	0,1,1,2,1,2,2,3,1,2,2,3,2,3,3,4,1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,
	1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
	1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
	2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
	1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
	2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
	2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
	3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,4,5,5,6,5,6,6,7,5,6,6,7,6,7,7,8,
    };

    struct ps2fs_sb_info *sbinfo = PS2FS_SB(sb);
    int bitsperblock = sb->s_blocksize * 8;
    unsigned long count = 0;
    int part;

    for (part = 0; part < sbinfo->n_subparts; part++) {
	unsigned long start, nblocks, i;
	const unsigned char *s;

	/* FIXME: make this into a macro or inline function? */
	start = ((part?0:0x400000) + (sb->s_blocksize<<sbinfo->block_shift))
	        / sb->s_blocksize
	        + sbinfo->first_sector[part];
	nblocks = ((sbinfo->size[part]>>sbinfo->block_shift) + bitsperblock-1)
	          / bitsperblock;
	for (i = 0; i < nblocks; i++) {
	    struct buffer_head *bh = bread(sb->s_dev,start+i,sb->s_blocksize);
	    if (!bh) {
		ps2fs_error(sb, "count_used_blocks", "unable to retrieve"
			    " bitmap block %ld for subpart %d\n", i, part);
		return -EIO;
	    }
	    for (s = bh->b_data;
		 s < (unsigned char *)bh->b_data+sb->s_blocksize;
		 s++
	    ) {
		count += bitcount[*s];
	    }
	    brelse(bh);
	}
    }
    return (count - sbinfo->root_inode) << sbinfo->block_shift;
}

/*************************************************************************/

/* Attempts to allocate a new block on the filesystem; `desired' is the
 * block number desired, or 0 if any block is acceptable.  Returns the
 * block number if successful, <0 (error code) otherwise; if `desired' is
 * nonzero and the desired block is not available, returns -ENOSPC.
 * Note: bit 0 (0x01) is the first block in the byte.
 */

long get_new_block(struct super_block *sb, long desired)
{
    struct ps2fs_sb_info *sbinfo = PS2FS_SB(sb);
    int bitsperblock = sb->s_blocksize * 8;
    int part;
    unsigned long start, nblocks, i;
    __u8 *s;
    struct buffer_head *bh;

    if (desired < 0)
	return -ENOSPC;
    part = PS2FS_SUBPART(desired);
    desired = PS2FS_BNUM(desired);
    if (part >= sbinfo->n_subparts)
	return -ENOSPC;
    start = ((part?0:0x400000) + (sb->s_blocksize<<sbinfo->block_shift))
	    / sb->s_blocksize
	    + sbinfo->first_sector[part];
    nblocks = ((sbinfo->size[part] >> sbinfo->block_shift) + bitsperblock-1)
	      / bitsperblock;

    lock_super(sb);

    if (desired) {
	i = desired / bitsperblock;
	bh = bread(sb->s_dev, start+i, sb->s_blocksize);
	if (!bh) {
	    unlock_super(sb);
	    ps2fs_error(sb, "get_new_block",
			"unable to retrieve bitmap block %lu\n", i);
	    return -EIO;
	}
	s = (__u8 *)bh->b_data + (desired % bitsperblock) / 8;
	i = 1 << (desired & 7);
	if (*s & i) {
	    unlock_super(sb);
	    brelse(bh);
	    return -ENOSPC;
	}
	*s |= i;
	mark_buffer_dirty(bh);
	ll_rw_block(WRITE, 1, &bh);
	wait_on_buffer(bh);
	if (!buffer_uptodate(bh)) {
	    unlock_super(sb);
	    ps2fs_error(sb, "get_new_block",
			"write of sector %lu failed", bh->b_blocknr);
	    brelse(bh);
	    return -EIO;
	}
	unlock_super(sb);
	brelse(bh);
	return desired;
    }

    /* FIXME: check all open partitions */
    for (i = 0; i < nblocks; i++) {
	bh = bread(sb->s_dev, start+i, sb->s_blocksize);
	if (!bh) {
	    unlock_super(sb);
	    ps2fs_error(sb, "get_new_block",
			"unable to retrieve bitmap block %lu\n", i);
	    return -EIO;
	}
	for (s = bh->b_data; s < (__u8 *)bh->b_data+sb->s_blocksize; s++) {
	    if (*s != 0xFF) {
		int bit = 0, val = *s;
		while (val & 1) {
		    bit++;
		    val >>= 1;
		}
		if (bit > 7) {
		    unlock_super(sb);
		    ps2fs_error(sb, "get_new_block",
				"impossible: bit>7 (bitmap: %02X)\n", *s);
		    BUG();
		    return -EIO;
		}
		*s |= 1 << bit;
		mark_buffer_dirty(bh);
		ll_rw_block(WRITE, 1, &bh);
		wait_on_buffer(bh);
		if (!buffer_uptodate(bh)) {
		    unlock_super(sb);
		    ps2fs_error(sb, "get_new_block",
				"write of sector %lu failed", bh->b_blocknr);
		    brelse(bh);
		    return -EIO;
		}
		unlock_super(sb);
		brelse(bh);
		return i * bitsperblock
		       + (s - (unsigned char *)bh->b_data) * 8
		       + bit;
	    }
	}
	brelse(bh);
    }
    /* No free blocks... */
    unlock_super(sb);
    return -ENOSPC;
}

/*************************************************************************/

/* Marks the given block as free in the bitmap.  Returns 0 on success,
 * error code on failure.
 */

int free_block(struct super_block *sb, long block)
{
    struct ps2fs_sb_info *sbinfo = PS2FS_SB(sb);
    int bitsperblock = sb->s_blocksize * 8;
    int part;
    unsigned long start, i;
    __u8 *s;
    struct buffer_head *bh;

    if (block < 0) {
	ps2fs_error(sb, "free_block", "bad block number: 0x%08lX", block);
	return -EIO;
    }
    part = PS2FS_SUBPART(block);
    block = PS2FS_BNUM(block);
    if (part >= sbinfo->n_subparts) {
	ps2fs_error(sb, "free_block",
		    "bad block number (invalid subpart): 0x%08lX", block);
	return -EIO;
    }
    start = ((part?0:0x400000) + (sb->s_blocksize<<sbinfo->block_shift))
	    / sb->s_blocksize
	    + sbinfo->first_sector[part];

    lock_super(sb);
    i = block / bitsperblock;
    bh = bread(sb->s_dev, start+i, sb->s_blocksize);
    if (!bh) {
	unlock_super(sb);
	ps2fs_error(sb, "free_block",
		    "unable to retrieve bitmap block 0x%08X\n", i);
	return -EIO;
    }
    s = (__u8 *)bh->b_data + (block % bitsperblock) / 8;
    i = 1 << (block & 7);
    if (*s & i) {
	*s &= ~i;
	mark_buffer_dirty(bh);
	ll_rw_block(WRITE, 1, &bh);
	wait_on_buffer(bh);
	if (!buffer_uptodate(bh)) {
	    unlock_super(sb);
	    ps2fs_error(sb, "free_block",
			"write of sector 0x%08X failed", bh->b_blocknr);
	    brelse(bh);
	    return -EIO;
	}
    } else {
	ps2fs_warning(sb, "free_block",
		    "block 0x%08X already free in bitmap\n", block);
    }
    unlock_super(sb);
    brelse(bh);
    return 0;
}

/*************************************************************************/

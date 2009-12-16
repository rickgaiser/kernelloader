/*
 * ps2fs/super.c
 *
 * Copyright (c) 2002 Andrew Church <achurch@achurch.org>
 *
 * Parts taken from devices/block/genhd.c from PS2 Linux Beta Release 1
 */

#include <linux/config.h>
#include <linux/string.h>
#include <linux/blkdev.h>

#include "ps2fs_fs.h"
#include "ps2_partition.h"

/*************************************************************************/

#define PS2_PARTITION_MAGIC	0x00415041	/* "APA\0" */
#define PS2_PART_MAXSUB		64	/* Maximum # of sub-partitions */

#define PS2_PART_FLAG_SUB	0x0001	/* Is partition a sub-partition? */

#define PS2_MBR_VERSION		2	/* Current MBR version */

struct ps2_partition_header {
    __u32 checksum;	/* Sum of all 256 words, assuming checksum==0 */
    __u32 magic;	/* PS2_PARTITION_MAGIC */
    __u32 next;		/* Sector address of next partition */
    __u32 prev;		/* Sector address of previous partition */
    char id[PS2_PART_IDMAX];
    char rpwd[PS2_PART_PASSWDMAX];  /* ??? (from PS2 Linux) */
    char fpwd[PS2_PART_PASSWDMAX];  /* ??? (from PS2 Linux) */
    __u32 start;	/* Sector address of this partition */
    __u32 length;	/* Sector count */
    __u16 type;
    __u16 flags;	/* PS2_PART_FLAG_* */
    __u32 nsub;		/* No. of sub-partitions (stored in main partition) */
    struct ps2fs_datetime created;
    __u32 main;		/* For sub-partitions, main partition sector address */
    __u32 number;	/* For sub-partitions, sub-partition number */
    __u32 modver;	/* ??? (from PS2 Linux) */
    __u32 reserved[7];	/* Currently unused */
    char name[PS2_PART_NAMEMAX];
    struct {
	char magic[32];	/* Copyright message in MBR */
	__u32 version;	/* MBR version (PS2_MBR_VERSION) */
	__u32 nsector;	/* ??? (from PS2 Linux, apparently set to zero) */
	struct ps2fs_datetime modified;
	__u32 osd_start; /* ??? (from PS2 Linux) */
	__u32 osd_count; /* ??? (from PS2 Linux) */
	char reserved[200]; /* Currently unused */
    } mbr;
    struct {		/* Sub-partition data */
	__u32 start;	/* Sector address */
	__u32 length;	/* Sector count */
    } subs[PS2_PART_MAXSUB];
};

/*************************************************************************/

/* Find the partition with ID `id' on device `dev' with block size
 * `blocksize'.  Returns the number of subpartitions in the partition, or
 * -1 if no such partition is found.  If `id' is NULL, uses the first
 * partition on the device.  If the search is successful, the `start' and
 * `size' arrays are set to the starting block and size of each
 * subpartition in `blocksize' units.
 */

int find_partition(kdev_t dev, int blocksize, const char *id,
		   __u32 start[PS2FS_MAX_SUBPARTS],
		   __u32 size[PS2FS_MAX_SUBPARTS])
{
    unsigned long block = 0, next;

    if (id && strlen(id) > PS2_PART_IDMAX) {
	printk("ps2fs: find_partition: called with overlong ID\n");
	return -1;
    } else if (blocksize < 512) {
	printk("ps2fs: find_partition: blocksize < 512 not supported\n");
	return -1;
    }

    do {
	struct buffer_head *bh;
	__u32 data[256];
	struct ps2_partition_header *ph = (struct ps2_partition_header *)data;
	int i;
	__u32 checksum;

	bh = bread(dev, block, blocksize);
	if (!bh) {
	    printk("ps2fs: find_partition: bread(0x%08lX) failed\n", block);
	    return -1;
	}
	if (blocksize == 512) {
	    memcpy(data, bh->b_data, 512);
	    brelse(bh);
	    bh = bread(dev, block+1, 512);
	    memcpy(data+128, bh->b_data, 512);
	} else {
	    memcpy(data, bh->b_data, 1024);
	}
	brelse(bh);
	checksum = 0;
	for (i = 1; i < 256; i++)
	    checksum += le32_to_cpu(data[i]);
	if (checksum != le32_to_cpu(ph->checksum)) {
	    printk("ps2fs: find_partition: (warning) bad checksum for"
		   " partition header at sector 0x%08lX\n", block);
	}
	next = le32_to_cpu(ph->next) / (blocksize/512);
	if (!id || strncmp(id, ph->id, PS2_PART_IDMAX) == 0) {
	    start[0] = block;
	    size[0] = le32_to_cpu(ph->length) / (blocksize/512);
	    for (i = 0; i < PS2_PART_MAXSUB && ph->subs[i].length; i++) {
		if (i+1 > PS2FS_MAX_SUBPARTS) {
		    printk("ps2fs: find_partition: (warning) partition has"
			   " too many subparts (max %d)", PS2FS_MAX_SUBPARTS);
		    return PS2FS_MAX_SUBPARTS;
		}
		start[i+1] = le32_to_cpu(ph->subs[i].start) / (blocksize/512);
		size[i+1] = le32_to_cpu(ph->subs[i].length) / (blocksize/512);
	    }
	    return i+1;
	}
    } while ((block = next) != 0);
    return -1;
}

/*************************************************************************/

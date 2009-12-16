/* PS2 partition related constants and routines */

#ifndef PS2_PARTITION_H
#define PS2_PARTITION_H

#ifndef PS2FS_FS_H
# include "ps2fs_fs.h"
#endif

/*************************************************************************/

#define PS2_PART_IDMAX		32
#define PS2_PART_PASSWDMAX	8
#define PS2_PART_NAMEMAX	128

extern int find_partition(kdev_t dev, int blocksize, const char *id,
			  __u32 start[PS2FS_MAX_SUBPARTS],
			  __u32 size[PS2FS_MAX_SUBPARTS]);

/*************************************************************************/

#endif	/* PS2_PARTITION_H */

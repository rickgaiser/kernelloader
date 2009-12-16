#ifndef PS2FS_FS_SB_H
#define PS2FS_FS_SB_H

/* In-memory superblock information */
/* Maximum size = sizeof(ext2_sb_info) = 216 bytes (Linux 2.4.19) */
#if 12+PS2FS_MAX_SUBPARTS*8 > 216
# error sizeof(ps2fs_sb_info) too big, reduce PS2FS_MAX_SUBPARTS
#endif
struct ps2fs_sb_info {
    __u32 tzoffset;	/* Time zone offset in seconds (tzoffset option) */
    __u32 root_inode;
    __u32 block_shift;	/* log2(superblock block size / hardware block size) */
    __u32 n_subparts;	/* Number of subparts */
    __u32 first_sector[PS2FS_MAX_SUBPARTS];
    __u32 size[PS2FS_MAX_SUBPARTS];	/* In hardware blocks */
};

#define PS2FS_SB(sb)	((struct ps2fs_sb_info *)(&((sb)->u.generic_sbp)))

#endif

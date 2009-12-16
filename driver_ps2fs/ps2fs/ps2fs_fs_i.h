#ifndef PS2FS_FS_I_H
#define PS2FS_FS_I_H

/* In-memory inode information */
struct ps2fs_inode_info {
    struct ps2fs_inode *rawinode;  /* A copy of the raw inode data (1k) */
};

#define PS2FS_INODE(inode) \
    ((struct ps2fs_inode_info *)(&((inode)->u.generic_ip)))

#endif

/*
 * Copyright (c) 2003-2005 Erez Zadok
 * Copyright (c) 2003-2005 Charles P. Wright
 * Copyright (c) 2005      Arun M. Krishnakumar
 * Copyright (c) 2005      David P. Quigley
 * Copyright (c) 2003-2004 Mohammad Nayyer Zubair
 * Copyright (c) 2003-2003 Puja Gupta
 * Copyright (c) 2003-2003 Harikesavan Krishnan
 * Copyright (c) 2003-2005 Stony Brook University
 * Copyright (c) 2003-2005 The Research Foundation of State University of New York
 *
 * For specific licensing information, see the COPYING file distributed with
 * this package.
 *
 * This Copyright notice must be kept intact and distributed with all sources.
 */
/*
 *  $Id$
 */

#ifndef __UNIONFS_H_
#error This file should only be included from unionfs.h!
#endif

/* File to hidden file. */
#define ftohf(f) (fbstart(f) < UNIONFS_INLINE_OBJECTS ? ftopd(f)->ufi_file_i[fbstart(f)] : ftopd(f)->ufi_file_p[fbstart(f) - UNIONFS_INLINE_OBJECTS])
#define ftohf_index(f, index) (index < UNIONFS_INLINE_OBJECTS ? ftopd(f)->ufi_file_i[index] : ftopd(f)->ufi_file_p[index - UNIONFS_INLINE_OBJECTS])

#define set_ftohf_index(f, index, val) \
do { \
	struct file *f2 = f;\
	if (index < UNIONFS_INLINE_OBJECTS) \
		ftopd(f2)->ufi_file_i[index] = val; \
	else \
		ftopd(f2)->ufi_file_p[index - UNIONFS_INLINE_OBJECTS] = val; \
} while (0);
#define set_ftohf(f, val) \
do { \
	struct file *f2 = f;\
	int index = fbstart(f2); \
	if (index < UNIONFS_INLINE_OBJECTS) \
		ftopd(f2)->ufi_file_i[index] = val; \
	else \
		ftopd(f2)->ufi_file_p[index - UNIONFS_INLINE_OBJECTS] = val; \
} while (0);

/* Inode to hidden inode. */
#define itohi(i) (ibstart(i) < UNIONFS_INLINE_OBJECTS ? itopd(i)->uii_inode_i[ibstart(i)] : itopd(i)->uii_inode_p[ibstart(i) - UNIONFS_INLINE_OBJECTS])
#define itohi_index(i, index) (index < UNIONFS_INLINE_OBJECTS ? itopd(i)->uii_inode_i[index] : itopd(i)->uii_inode_p[index - UNIONFS_INLINE_OBJECTS])

#define set_itohi_index(i, index, val) \
do { \
	struct inode *i2 = i;\
	if (index < UNIONFS_INLINE_OBJECTS) \
		itopd(i2)->uii_inode_i[index] = val; \
	else \
		itopd(i2)->uii_inode_p[index - UNIONFS_INLINE_OBJECTS] = val; \
} while (0);
#define set_itohi(i, val) \
do { \
	struct inode *i2 = i;\
	int index = ibstart(i2); \
	if (index < UNIONFS_INLINE_OBJECTS) \
		itopd(i2)->uii_inode_i[index] = val; \
	else \
		itopd(i2)->uii_inode_p[index - UNIONFS_INLINE_OBJECTS] = val; \
} while (0);

/* Superblock to hidden superblock. */
#define stohs(o) (sbstart(o) < UNIONFS_INLINE_OBJECTS ? stopd(o)->usi_sb_i[sbstart(o)] : stopd(o)->usi_sb_p[sbstart(o) - UNIONFS_INLINE_OBJECTS])
#define stohs_index(o, index) (index < UNIONFS_INLINE_OBJECTS ? stopd(o)->usi_sb_i[index] : stopd(o)->usi_sb_p[index - UNIONFS_INLINE_OBJECTS])

#define set_stohs_index(o, index, val) \
do { \
	struct super_block *s2 = o;\
	if (index < UNIONFS_INLINE_OBJECTS) \
		stopd(s2)->usi_sb_i[index] = val; \
	else \
		stopd(s2)->usi_sb_p[index - UNIONFS_INLINE_OBJECTS] = val; \
} while (0);
#define set_stohs(o, val) \
do { \
	struct super_block *s2 = o;\
	int index = sbstart(s2); \
	if (index < UNIONFS_INLINE_OBJECTS) \
		stopd(s2)->usi_sb_i[index] = val; \
	else \
		stopd(s2)->usi_sb_p[index - UNIONFS_INLINE_OBJECTS] = val; \
} while (0);

/* Super to hidden mount. */
#define stohiddenmnt_index(o, index) (index < UNIONFS_INLINE_OBJECTS ? stopd(o)->usi_hidden_mnt_i[index] : stopd(o)->usi_hidden_mnt_p[index - UNIONFS_INLINE_OBJECTS])

#define set_stohiddenmnt_index(o, index, val) \
do { \
	struct super_block *s2 = o;\
	if (index < UNIONFS_INLINE_OBJECTS) \
		stopd(s2)->usi_hidden_mnt_i[index] = val; \
	else \
		stopd(s2)->usi_hidden_mnt_p[index - UNIONFS_INLINE_OBJECTS] = val; \
} while (0);

/* Branch count macros. */
#define branch_count(o, index) \
(index < UNIONFS_INLINE_OBJECTS ? \
atomic_read(&stopd(o)->usi_sbcount_i[index]) : \
atomic_read(&stopd(o)->usi_sbcount_p[index - UNIONFS_INLINE_OBJECTS]))

#define set_branch_count(o, index, val) \
do { \
	if (index < UNIONFS_INLINE_OBJECTS) \
		atomic_set(&stopd(o)->usi_sbcount_i[index], val); \
	else \
		atomic_set(&stopd(o)->usi_sbcount_p[index - UNIONFS_INLINE_OBJECTS], val); \
} while (0);
#define branchget(o, index) \
do { \
	if (index < UNIONFS_INLINE_OBJECTS) \
		atomic_inc(&stopd(o)->usi_sbcount_i[index]); \
	else \
		atomic_inc(&stopd(o)->usi_sbcount_p[index - UNIONFS_INLINE_OBJECTS]); \
} while (0);
#define branchput(o, index) \
do { \
	if (index < UNIONFS_INLINE_OBJECTS) \
		atomic_dec(&stopd(o)->usi_sbcount_i[index]); \
	else \
		atomic_dec(&stopd(o)->usi_sbcount_p[index - UNIONFS_INLINE_OBJECTS]); \
} while (0);

/* Dentry macros */
#define dtopd(dent) ((struct unionfs_dentry_info *)(dent)->d_fsdata)
#define dtopd_lhs(dent) ((dent)->d_fsdata)
#define dtopd_nocheck(dent) dtopd(dent)
#define dbstart(dent) (dtopd(dent)->udi_bstart)
#define set_dbstart(dent, val) do { dtopd(dent)->udi_bstart = val; } while(0)
#define dbend(dent) (dtopd(dent)->udi_bend)
#define set_dbend(dent, val) do { dtopd(dent)->udi_bend = val; } while(0)
#define dbopaque(dent) (dtopd(dent)->udi_bopaque)
#define set_dbopaque(dent, val) do { dtopd(dent)->udi_bopaque = val; } while (0)

#define set_dtohd_index(dent, index, val) \
do { \
	struct dentry *d2 = dent; \
	if (index < UNIONFS_INLINE_OBJECTS) \
		dtopd(d2)->udi_dentry_i[index] = val; \
	else \
		dtopd(d2)->udi_dentry_p[index - UNIONFS_INLINE_OBJECTS] = val; \
} while (0);

static inline struct dentry *dtohd_index(const struct dentry *dent, int index)
{
	struct dentry *d;
	if (index < UNIONFS_INLINE_OBJECTS)
		d = dtopd(dent)->udi_dentry_i[index];
	else
		d = dtopd(dent)->udi_dentry_p[index - UNIONFS_INLINE_OBJECTS];
	return d;
}

static inline struct dentry *dtohd(const struct dentry *dent)
{
	struct dentry *d;
	int index;
	index = dbstart(dent);
	if (index < UNIONFS_INLINE_OBJECTS)
		d = dtopd(dent)->udi_dentry_i[index];
	else
		d = dtopd(dent)->udi_dentry_p[index - UNIONFS_INLINE_OBJECTS];
	return d;
}

#define set_dtohd_index_nocheck(dent, index, val) set_dtohd_index(dent, index, val)
#define dtohd_index_nocheck(dent, index) dtohd_index(dent, index)

#define dtohd_ptr(dent) (dtopd_nocheck(dent)->udi_dentry_p)
#define dtohd_inline(dent) (dtopd_nocheck(dent)->udi_dentry_i)

/* Macros for locking a dentry. */
#define lock_dentry(d) down(&dtopd(d)->udi_sem)
#define unlock_dentry(d) up(&dtopd(d)->udi_sem)
#define verify_locked(d)
/*
 *
 * vim:shiftwidth=8
 * vim:tabstop=8
 *
 * For Emacs:
 * Local variables:
 * c-basic-offset: 8
 * c-comment-only-line-offset: 0
 * c-offsets-alist: ((statement-block-intro . +) (knr-argdecl-intro . 0)
 *              (substatement-open . 0) (label . 0) (statement-cont . +))
 * indent-tabs-mode: t
 * tab-width: 8
 * End:
 */

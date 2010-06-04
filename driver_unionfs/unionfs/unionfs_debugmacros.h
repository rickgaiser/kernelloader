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

/* File to hidden file */

static inline struct file *__ftohf_index(const struct file *f, int index,
					 const char *file, const char *function,
					 int line)
{
	struct file *hidden_file;
	if (index < UNIONFS_INLINE_OBJECTS)
		hidden_file = ftopd(f)->ufi_file_i[index];
	else
		hidden_file =
		    ftopd(f)->ufi_file_p[index - UNIONFS_INLINE_OBJECTS];

	if (hidden_file)
		ASSERT2((atomic_read(&hidden_file->f_count)) > 0);
	return hidden_file;
}
static inline struct file *__ftohf(const struct file *f, const char *file,
				   const char *function, int line)
{
	int branch;
	ASSERT2(ftopd(f)->b_start >= 0);
	branch = fbstart(f);
	return __ftohf_index(f, branch, file, function, line);
}
static inline struct file *__set_ftohf_index(struct file *f, int index,
					     struct file *val, const char *file,
					     const char *function, int line)
{
	ASSERT(index >= 0);
	if (val)
		ASSERT2((atomic_read(&val->f_count)) > 0);
	if (index < UNIONFS_INLINE_OBJECTS)
		ftopd(f)->ufi_file_i[index] = val;
	else
		ftopd(f)->ufi_file_p[index - UNIONFS_INLINE_OBJECTS] = val;
	return val;
}
static inline struct file *__set_ftohf(struct file *f, struct file *val,
				       const char *file, const char *function,
				       int line)
{
	int branch;
	ASSERT2(ftopd(f)->b_start >= 0);
	branch = fbstart(f);
	return __set_ftohf_index(f, branch, val, file, function, line);
}

#define ftohf(file) __ftohf(file, __FILE__, __FUNCTION__, __LINE__)
#define set_ftohf(file, val) __set_ftohf(file, val, __FILE__, __FUNCTION__, __LINE__)
#define ftohf_index(file, index) __ftohf_index(file, index, __FILE__, __FUNCTION__, __LINE__)
#define set_ftohf_index(file, index, val) __set_ftohf_index(file, index, val, __FILE__, __FUNCTION__, __LINE__)

/* Inode to hidden inode. */

static inline struct inode *__itohi_index(const struct inode *ino, int index,
					  const char *file,
					  const char *function, int line)
{
	struct inode *hidden_inode;
	ASSERT2(index >= 0);
	if (index < UNIONFS_INLINE_OBJECTS)
		hidden_inode = itopd(ino)->uii_inode_i[index];
	else
		hidden_inode =
		    itopd(ino)->uii_inode_p[index - UNIONFS_INLINE_OBJECTS];
	if (hidden_inode)
		ASSERT2((atomic_read(&hidden_inode->i_count)) > 0);
	return hidden_inode;
}
static inline struct inode *__itohi(const struct inode *ino, const char *file,
				    const char *function, int line)
{
	int index;
	ASSERT2(itopd(ino)->b_start >= 0);
	index = itopd(ino)->b_start;
	return __itohi_index(ino, index, file, function, line);
}
static inline struct inode *__set_itohi_index(struct inode *ino, int index,
					      struct inode *val,
					      const char *file,
					      const char *function, int line)
{
	ASSERT2(index >= 0);
	if (val)
		ASSERT2((atomic_read(&val->i_count)) > 0);
	if (index < UNIONFS_INLINE_OBJECTS)
		itopd(ino)->uii_inode_i[index] = val;
	else
		itopd(ino)->uii_inode_p[index - UNIONFS_INLINE_OBJECTS] = val;
	return val;
}

#define itohi(ino) __itohi(ino, __FILE__, __FUNCTION__, __LINE__)
#define itohi_index(ino, index) __itohi_index(ino, index, __FILE__, __FUNCTION__, __LINE__)
#define set_itohi_index(ino, index, val) __set_itohi_index(ino, index, val, __FILE__, __FUNCTION__, __LINE__);

/* Superblock to hidden superblock */
static inline struct super_block *__stohs_index(const struct super_block *f,
						int index, const char *file,
						const char *function, int line)
{
	struct super_block *hidden_sb;
	if (index < UNIONFS_INLINE_OBJECTS)
		hidden_sb = stopd(f)->usi_sb_i[index];
	else
		hidden_sb = stopd(f)->usi_sb_p[index - UNIONFS_INLINE_OBJECTS];
	return hidden_sb;
}
static inline struct super_block *__stohs(const struct super_block *sb,
					  const char *file,
					  const char *function, int line)
{
	int branch;
	ASSERT2(sbstart(sb) == 0);
	branch = sbstart(sb);
	return __stohs_index(sb, branch, file, function, line);
}
static inline struct super_block *__set_stohs_index(struct super_block *sb,
						    int index,
						    struct super_block *val,
						    const char *file,
						    const char *function,
						    int line)
{
	if (index < UNIONFS_INLINE_OBJECTS)
		stopd(sb)->usi_sb_i[index] = val;
	else
		stopd(sb)->usi_sb_p[index - UNIONFS_INLINE_OBJECTS] = val;
	return val;
}

#define stohs(file) __stohs(file, __FILE__, __FUNCTION__, __LINE__)
#define stohs_index(file, index) __stohs_index(file, index, __FILE__, __FUNCTION__, __LINE__)
#define set_stohs_index(file, index, val) __set_stohs_index(file, index, val, __FILE__, __FUNCTION__, __LINE__)

/* Superblock to hidden vfsmount  */
static inline struct vfsmount *stohiddenmnt_index(struct super_block *sb,
						  int index)
{
	ASSERT(index >= 0);
	if (index < UNIONFS_INLINE_OBJECTS)
		return stopd(sb)->usi_hidden_mnt_i[index];
	else
		return stopd(sb)->usi_hidden_mnt_p[index -
						   UNIONFS_INLINE_OBJECTS];
}

static inline void set_stohiddenmnt_index(struct super_block *sb, int index,
					  struct vfsmount *mnt)
{
	ASSERT(index >= 0);
	if (index < UNIONFS_INLINE_OBJECTS)
		stopd(sb)->usi_hidden_mnt_i[index] = mnt;
	else
		stopd(sb)->usi_hidden_mnt_p[index - UNIONFS_INLINE_OBJECTS] =
		    mnt;
}

/* Get and put branches on the superblock. */
static inline void set_branch_count(struct super_block *sb, int index,
				    int count)
{
	ASSERT(index >= 0);
	if (index < UNIONFS_INLINE_OBJECTS)
		atomic_set(&stopd(sb)->usi_sbcount_i[index], count);
	else
		atomic_set(&stopd(sb)->
			   usi_sbcount_p[index - UNIONFS_INLINE_OBJECTS],
			   count);
}
static inline int branch_count(struct super_block *sb, int index)
{
	int count;
	ASSERT(index >= 0);
	if (index < UNIONFS_INLINE_OBJECTS)
		count = atomic_read(&stopd(sb)->usi_sbcount_i[index]);
	else
		count =
		    atomic_read(&stopd(sb)->
				usi_sbcount_p[index - UNIONFS_INLINE_OBJECTS]);
	return count;
}
static inline void branchget(struct super_block *sb, int index)
{
	ASSERT(index >= 0);
	if (index < UNIONFS_INLINE_OBJECTS) {
		atomic_inc(&stopd(sb)->usi_sbcount_i[index]);
		ASSERT(atomic_read(&stopd(sb)->usi_sbcount_i[index]) >= 0);
	} else {
		atomic_inc(&stopd(sb)->
			   usi_sbcount_p[index - UNIONFS_INLINE_OBJECTS]);
		ASSERT(atomic_read(&stopd(sb)->
				   usi_sbcount_p[index -
						 UNIONFS_INLINE_OBJECTS]) >= 0);
	}
}
static inline void branchput(struct super_block *sb, int index)
{
	ASSERT(index >= 0);
	if (index < UNIONFS_INLINE_OBJECTS) {
		atomic_dec(&stopd(sb)->usi_sbcount_i[index]);
		ASSERT(atomic_read(&stopd(sb)->usi_sbcount_i[index]) >= 0);
	} else {
		atomic_dec(&stopd(sb)->
			   usi_sbcount_p[index - UNIONFS_INLINE_OBJECTS]);
		ASSERT(atomic_read(&stopd(sb)->
				   usi_sbcount_p[index -
						 UNIONFS_INLINE_OBJECTS]) >= 0);
	}
}

/* Dentry to Hidden Dentry  */
static inline struct unionfs_dentry_info *__dtopd(const struct dentry *dent,
						  int check, const char *file,
						  const char *function,
						  int line)
{
	struct unionfs_dentry_info *ret;

	PASSERT2(dent);
	ret = (struct unionfs_dentry_info *)(dent)->d_fsdata;
	/* We are really only interested in catching poison here. */
	if (ret) {
		PASSERT2(ret);
		if (check) {
			if ((ret->udi_bend > ret->udi_bcount)
			    || (ret->udi_bend > sbmax(dent->d_sb))) {
				printk(KERN_EMERG
				       "udi_bend = %d, udi_count = %d, sbmax = %d\n",
				       ret->udi_bend, ret->udi_bcount,
				       sbmax(dent->d_sb));
			}
			ASSERT2(ret->udi_bend <= ret->udi_bcount);
			ASSERT2(ret->udi_bend <= sbmax(dent->d_sb));
		}
	}

	return ret;
}

#define dtopd(dent) __dtopd(dent, 1, __FILE__, __FUNCTION__, __LINE__)
#define dtopd_nocheck(dent) ((struct unionfs_dentry_info *)(dent)->d_fsdata)
#define dtopd_lhs(dent) ((dent)->d_fsdata)

/* Macros for locking a dentry. */
static inline void lock_dentry(struct dentry *d)
{
	PASSERT(d);
	PASSERT(dtopd(d));
#ifdef TRACKLOCK
	printk("LOCK:%p\n", d);
#endif
	down(&dtopd(d)->udi_sem);
}
static inline void unlock_dentry(struct dentry *d)
{
	PASSERT(d);
	PASSERT(dtopd(d));
#ifdef TRACKLOCK
	printk("UNLOCK:%p\n", d);
#endif
	up(&dtopd(d)->udi_sem);
}

#define verify_locked2(dentry) __verify_locked((dentry), (file), (function), (line))
#define verify_locked(dentry) __verify_locked((dentry), __FILE__, __FUNCTION__, __LINE__)
static inline void __verify_locked(const struct dentry *d, const char *file,
				   const char *function, int line)
{
	PASSERT2(d);
	PASSERT2(dtopd(d));
#ifdef TRACKLOCK
	printk("MUST BE LOCKED:%p\n", d);
#endif
	ASSERT2(down_trylock(&dtopd(d)->udi_sem) != 0);
}

#define dbend(dentry) __dbend(dentry, __FILE__, __FUNCTION__, __LINE__)
static inline int __dbend(const struct dentry *dentry, const char *file,
			  const char *function, int line)
{
	verify_locked2(dentry);
	return dtopd(dentry)->udi_bend;
}

#define set_dbend(dentry, val) __set_dbend(dentry, val, __FILE__, __FUNCTION__, __LINE__)
static inline int __set_dbend(const struct dentry *dentry, int val,
			      const char *file, const char *function, int line)
{
	verify_locked2(dentry);
	if (val < 0) {
		ASSERT2(val == -1);
	}
	ASSERT2(val <= __dtopd(dentry, 0, file, function, line)->udi_bcount);
	ASSERT2(val <= sbmax(dentry->d_sb));
	dtopd(dentry)->udi_bend = val;
	return __dtopd(dentry, 1, file, function, line)->udi_bend;
}

#define dbstart(dentry) __dbstart(dentry, __FILE__, __FUNCTION__, __LINE__)
static inline int __dbstart(const struct dentry *dentry, const char *file,
			    const char *function, int line)
{
	verify_locked2(dentry);
	return dtopd(dentry)->udi_bstart;
}

#define set_dbstart(dentry, val) __set_dbstart(dentry, val, __FILE__, __FUNCTION__, __LINE__)
static inline int __set_dbstart(const struct dentry *dentry, int val,
				const char *file, const char *function,
				int line)
{
	verify_locked2(dentry);
	if (val < 0) {
		ASSERT2(val == -1);
	}
	ASSERT2(val <= __dtopd(dentry, 0, file, function, line)->udi_bcount);
	ASSERT2(val <= sbmax(dentry->d_sb));
	__dtopd(dentry, 0, file, function, line)->udi_bstart = val;
	return __dtopd(dentry, 1, file, function, line)->udi_bstart;
}

#define dbopaque(dentry) __dbopaque(dentry, __FILE__, __FUNCTION__, __LINE__)
static inline int __dbopaque(const struct dentry *dentry, const char *file,
			     const char *function, int line)
{
	verify_locked2(dentry);
	return dtopd(dentry)->udi_bopaque;
}

#define set_dbopaque(dentry, val) __set_dbopaque(dentry, val, __FILE__, __FUNCTION__, __LINE__)
static inline int __set_dbopaque(const struct dentry *dentry, int val,
				 const char *file, const char *function,
				 int line)
{
	verify_locked2(dentry);
	if (val < 0) {
		ASSERT2(val == -1);
	}
	ASSERT2(val <= __dtopd(dentry, 0, file, function, line)->udi_bcount);
	ASSERT2(val <= sbmax(dentry->d_sb));
	ASSERT2(val <= dbend(dentry));
	__dtopd(dentry, 0, file, function, line)->udi_bopaque = val;
	return __dtopd(dentry, 1, file, function, line)->udi_bopaque;
}

/* Dentry to hidden dentry functions */
#define dtohd_index(dent, index) __dtohd_index(dent, index, __FILE__, __FUNCTION__, __LINE__,1)
#define dtohd_index_nocheck(dent, index) __dtohd_index(dent, index, __FILE__, __FUNCTION__, __LINE__,0)
/* This pointer should not be generally used except for maintainence functions. */
#define dtohd_ptr(dent) (dtopd_nocheck(dent)->udi_dentry_p)
#define dtohd_inline(dent) (dtopd_nocheck(dent)->udi_dentry_i)
static inline struct dentry *__dtohd_index(const struct dentry *dent, int index,
					   const char *file,
					   const char *function, int line,
					   int docheck)
{
	struct dentry *d;

	PASSERT2(dent);
	PASSERT2(dent->d_sb);
	PASSERT2(dtopd(dent));
	verify_locked2(dent);
	if (index >= UNIONFS_INLINE_OBJECTS)
		PASSERT2(dtopd(dent)->udi_dentry_p);
	ASSERT2(index >= 0);
	if (docheck) {
		if (index > sbend(dent->d_sb)) {
			printk
			    ("Dentry index out of super bounds: index=%d, sbend=%d\n",
			     index, sbend(dent->d_sb));
			ASSERT2(index <= sbend(dent->d_sb));
		}
		if (index > dtopd(dent)->udi_bcount) {
			printk
			    ("Dentry index out of array bounds: index=%d, count=%d\n",
			     index, dtopd(dent)->udi_bcount);
			printk("Generation of dentry: %d\n",
			       atomic_read(&dtopd(dent)->udi_generation));
			printk("Generation of sb: %d\n",
			       atomic_read(&stopd(dent->d_sb)->usi_generation));
			ASSERT2(index <= dtopd(dent)->udi_bcount);
		}
	}
	if (index < UNIONFS_INLINE_OBJECTS)
		d = dtopd(dent)->udi_dentry_i[index];
	else
		d = dtopd(dent)->udi_dentry_p[index - UNIONFS_INLINE_OBJECTS];
	if (d) {
		PASSERT2(d);
		ASSERT2((atomic_read(&d->d_count)) > 0);
	}
	return d;
}

#define dtohd(dent) __dtohd(dent, __FILE__, __FUNCTION__, __LINE__)
static inline struct dentry *__dtohd(const struct dentry *dent,
				     const char *file, const char *function,
				     int line)
{
	struct dentry *d;
	int index;

	verify_locked2(dent);
	ASSERT2(dbstart(dent) >= 0);
	index = dbstart(dent);
	d = __dtohd_index(dent, index, file, function, line, 1);

	return d;
}

// Dentry to Hidden Dentry based on index
#define set_dtohd_index(dent, index, val) __set_dtohd_index(dent, index, val, __FILE__, __FUNCTION__, __LINE__, 1)
#define set_dtohd_index_nocheck(dent, index, val) __set_dtohd_index(dent, index, val, __FILE__, __FUNCTION__, __LINE__, 0)
static inline struct dentry *__set_dtohd_index(struct dentry *dent, int index,
					       struct dentry *val,
					       const char *file,
					       const char *function, int line,
					       int docheck)
{
#ifdef FIST_MALLOC_DEBUG
	struct dentry *old;
#endif

	PASSERT2(dent);

	PASSERT2(dent->d_sb);
	PASSERT2(dtopd(dent));
	verify_locked2(dent);
	if (index >= UNIONFS_INLINE_OBJECTS)
		PASSERT2(dtopd(dent)->udi_dentry_p);
	ASSERT2(index >= 0);
	if (docheck) {
		if (index > sbend(dent->d_sb)) {
			printk
			    ("Dentry index out of super bounds: index=%d, sbend=%d\n",
			     index, sbend(dent->d_sb));
			ASSERT2(index <= sbend(dent->d_sb));
		}
		if (index > dtopd(dent)->udi_bcount) {
			printk
			    ("Dentry index out of array bounds: index=%d, count=%d\n",
			     index, dtopd(dent)->udi_bcount);
			printk("Generation of dentry: %d\n",
			       atomic_read(&dtopd(dent)->udi_generation));
			printk("Generation of sb: %d\n",
			       atomic_read(&stopd(dent->d_sb)->usi_generation));
			ASSERT2(index <= dtopd(dent)->udi_bcount);
		}
	}
	if (val)
		PASSERT2(val);
#ifdef FIST_MALLOC_DEBUG
	if (index < UNIONFS_INLINE_OBJECTS)
		old = dtopd(dent)->udi_dentry_i[index];
	else
		old = dtopd(dent)->udi_dentry_p[index - UNIONFS_INLINE_OBJECTS];
	record_set(dent, index, val, old, line, file);
#endif
	if (index < UNIONFS_INLINE_OBJECTS)
		dtopd(dent)->udi_dentry_i[index] = val;
	else
		dtopd(dent)->udi_dentry_p[index - UNIONFS_INLINE_OBJECTS] = val;

	return val;
}

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

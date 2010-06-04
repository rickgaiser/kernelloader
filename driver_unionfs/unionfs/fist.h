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

#ifndef __FIST_H_
#define __FIST_H_

/*
 * KERNEL ONLY CODE:
 */
#ifdef __KERNEL__
#include <linux/config.h>
#include <linux/version.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
#ifdef CONFIG_MODVERSIONS
# define MODVERSIONS
# include <linux/modversions.h>
#endif				/* CONFIG_MODVERSIONS */
#endif				/* KERNEL_VERSION < 2.6.0 */
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/string.h>
#include <linux/stat.h>
#include <linux/errno.h>
#include <linux/wait.h>
#include <linux/limits.h>
#include <linux/random.h>
#include <linux/poll.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
#include <linux/locks.h>
#include <linux/spinlock.h>
#include "compat24.h"
#else
#include <linux/buffer_head.h>
#include <linux/pagemap.h>
#include <linux/namei.h>
#include <linux/module.h>
#include <linux/mount.h>
#include <linux/page-flags.h>
#include <linux/writeback.h>
#include <linux/page-flags.h>
#include <linux/statfs.h>
#endif
#include <linux/smp.h>
#include <linux/smp_lock.h>
#include <linux/file.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/poll.h>
#include <linux/list.h>
#include <linux/init.h>
#if defined(UNIONFS_XATTR) && (LINUX_VERSION_CODE >= KERNEL_VERSION(2,4,20))
#include <linux/xattr.h>
#endif
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
#include <linux/security.h>
#include <linux/compat.h>
#endif

#include <linux/swap.h>

#include <asm/system.h>
#include <asm/segment.h>
#include <asm/mman.h>
#include <linux/seq_file.h>
#include <linux/dcache.h>
#include <linux/poll.h>

/*
 * MACROS:
 */

#ifndef SEEK_SET
#define SEEK_SET 0
#endif				/* not SEEK_SET */

#ifndef SEEK_CUR
#define SEEK_CUR 1
#endif				/* not SEEK_CUR */

#ifndef SEEK_END
#define SEEK_END 2
#endif				/* not SEEK_END */

#ifndef DEFAULT_POLLMASK
#define DEFAULT_POLLMASK (POLLIN | POLLOUT | POLLRDNORM | POLLWRNORM)
#endif

/*
 * EXTERNALS:
 */

#ifdef FIST_MALLOC_DEBUG
extern void *unionfs_kmalloc(size_t len, int flag, int line, const char *file);
extern void unionfs_kfree(void *ptr, int line, const char *file);

extern struct dentry *unionfs_dget_parent(struct dentry *child, int line,
					  const char *file);
extern struct dentry *unionfs_dget(struct dentry *ptr, int line,
				   const char *file);
extern void unionfs_dput(struct dentry *ptr, int line, const char *file);
extern struct dentry *unionfs_lookup_one_len(const char *name,
					     struct dentry *parent, int len,
					     int line, const char *file);
void record_path_lookup(struct nameidata *nd, int line, const char *file);
void record_path_release(struct nameidata *nd, int line, const char *file);
struct file *unionfs_dentry_open(struct dentry *ptr, struct vfsmount *mnt,
				 int flags, int line, const char *file);
void record_set(struct dentry *upper, int index, struct dentry *ptr,
		struct dentry *old, int line, const char *file);

#define KMALLOC(size,flag) unionfs_kmalloc((size),(flag),__LINE__,__FILE__)
#define KFREE(ptr) unionfs_kfree((ptr),__LINE__,__FILE__)
#define DGET(d) unionfs_dget((d),__LINE__,__FILE__)
#define DPUT(d) unionfs_dput((d),__LINE__,__FILE__)
#define LOOKUP_ONE_LEN(name,parent,len) unionfs_lookup_one_len((name),(parent),(len),__LINE__,__FILE__)
# define RECORD_PATH_LOOKUP(nd)	record_path_lookup((nd),__LINE__,__FILE__)
# define RECORD_PATH_RELEASE(nd) record_path_release((nd),__LINE__,__FILE__)
/* This has the effect of reducing the reference count sooner or later,
 * if the file is closed.  If it isn't then the mount will be busy and
 * you can't unmount.
 */
# define DENTRY_OPEN(d,m,f) unionfs_dentry_open((d),(m),(f),__LINE__,__FILE__)
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
# define GET_PARENT(dentry) unionfs_dget_parent((dentry),__LINE__,__FILE__)
#endif
#else				/* not FIST_MALLOC_DEBUG */
# define KMALLOC(a,b)		kmalloc((a),(b))
# define KFREE(a)		kfree((a))
# define DPUT(a)		dput((a))
# define DGET(a)		dget((a))
# define LOOKUP_ONE_LEN(a,b,c)	lookup_one_len((a),(b),(c))
# define RECORD_PATH_LOOKUP(a)
# define RECORD_PATH_RELEASE(a)
# define DENTRY_OPEN(d,m,f)	dentry_open((d),(m),(f))
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
# define GET_PARENT(d)		dget_parent(d)
#else
# define GET_PARENT(d)		get_parent(d)
#endif
#endif				/* not FIST_MALLOC_DEBUG */

/* This needs to go so low so that we can bring in DGET/DPUT. */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
#include "missing_vfs_funcs.h"
#endif

#ifdef UNIONFS_NDEBUG
/* All of these should be noops. */
static inline int fist_get_debug_value(void)
{
	return 0;
}
static inline int fist_set_debug_value(int val)
{
	return -ENOTSUPP;
}

#define fist_print_dentry(msg, o)
#define __fist_print_dentry(msg, o, i)
#define fist_print_generic_dentry(msg, o)
#define fist_print_generic_dentry3(msg, o)
#define __fist_print_generic_dentry(msg, o, i)
#define fist_print_inode(msg, o)
#define fist_print_generic_inode(msg, o)
#define fist_print_file(msg, o)
#define fist_checkinode(o, msg)
#define fist_print_sb(msg, o)
#else
extern int fist_get_debug_value(void);
extern int fist_set_debug_value(int val);
extern void fist_dprint_internal(const char *file, const char *function,
				 int line, int level, char *str, ...)
    __attribute__ ((format(__printf__, 5, 6)));

extern void fist_print_dentry(const char *, const struct dentry *);
extern void __fist_print_dentry(const char *, const struct dentry *, int);
extern void fist_print_generic_dentry(const char *, const struct dentry *);
extern void fist_print_generic_dentry3(const char *, const char *,
				       const struct dentry *);
extern void __fist_print_generic_dentry(const char *, const char *, const
					struct dentry *, int);
extern void fist_print_inode(const char *, const struct inode *);
extern void fist_print_generic_inode(const char *, const struct inode *);
extern void fist_print_file(const char *, const struct file *);
extern void fist_checkinode(const struct inode *, const char *);
extern void fist_print_sb(const char *str, const struct super_block *);

extern char *add_indent(void);
extern char *del_indent(void);
#endif

/* The poison pointer.  This needs to be changed on an ia64. */
#define POISON    ((void *)0x5a5a5a5a)

#define WHEREAMI() \
do { \
	printk("HERE: %s:%s:%d\n", __FILE__, __FUNCTION__, __LINE__); \
} while (0)

#ifndef UNIONFS_NDEBUG

/* Call if you encounter a bug. */
#define FISTBUG(msg) \
do { \
	printk("<0>FISTBUG at %s:%s:%d %s", __FILE__, __FUNCTION__, __LINE__, msg); \
	(*((char *)0))=0;	\
} while (0);

/* The if (0 ...) is so that we can make sure that you don't pass this
 * define a non-pointer.  gcc should optimize it away. */
#define PASSERT(EX)	\
do {	\
    void *p = (void *)(EX);\
    if (0 && ((EX) == (void *)4)) { /* do nothing */ }; \
    if (!(p) || (p == POISON)) {	\
	printk(KERN_CRIT "ASSERTION FAILED: %s %s at %s:%d (%s)\n", #EX,	\
	        (p == POISON) ? "(poisoned)" : "(null)", \
                __FILE__, __LINE__, __FUNCTION__);	\
	(*((char *)0))=0;	\
    }	\
} while (0)

/* The if (0 ...) is so that we can make sure that you don't pass this
 * define a non-pointer.  gcc should optimize it away. */
/* same PASSERT, but tell me who was the caller of the function */
#define PASSERT2(EX)	\
do {	\
    void *p = (void *)(EX);\
    if (0 && ((EX) ==  (void *)4)) { /* do nothing */ }; \
    if (!(p) || (p == POISON)) {	\
	printk(KERN_CRIT "ASSERTION FAILED %s %s at %s:%d (%s) called by %s:%d (%s)\n", #EX, (p == POISON) ? "(poisoned)" : "(null)",	\
	       __FILE__, __LINE__, __FUNCTION__, file, line, function);\
	(*((char *)0))=0;	\
    }	\
} while (0)

/* The if (0 ...) is so that we can make sure that you don't pass this
 * define a pointer.  gcc should optimize it away. */
#define ASSERT(EX)	\
do {	\
    if (0 && ((EX) == 1)) { /* do nothing */ }; \
    if ((EX) == 0) {	\
	printk(KERN_CRIT "ASSERTION FAILED: %s at %s:%d (%s)\n", #EX,	\
	       __FILE__, __LINE__, __FUNCTION__);	\
	(*((char *)0))=0;	\
    }	\
} while (0)

/* same ASSERT, but tell me who was the caller of the function */
#define ASSERT2(EX)	\
do {	\
    if (0 && ((EX) == 1)) { /* do nothing */ }; \
    if ((EX) == 0) {	\
	printk(KERN_CRIT "ASSERTION FAILED %s at %s:%d (%s) called by %s:%d (%s)\n", #EX,	\
	       __FILE__, __LINE__, __FUNCTION__, file, line, function);\
	(*((char *)0))=0;	\
    }	\
} while (0)

#define fist_dprint(level, str, args...) fist_dprint_internal(__FILE__, __FUNCTION__, __LINE__, level, KERN_DEBUG str, ## args)
#define print_entry(format, args...) fist_dprint(4, "%sIN:  %s %s:%d " format "\n", add_indent(), __FUNCTION__, __FILE__, __LINE__, ##args)
#define print_entry_location() fist_dprint(4, "%sIN:  %s %s:%d\n", add_indent(), __FUNCTION__, __FILE__, __LINE__)
#define print_exit_location() fist_dprint(5, "%s OUT: %s %s:%d\n", del_indent(), __FUNCTION__, __FILE__, __LINE__)
#define print_exit_status(status) fist_dprint(5, "%s OUT: %s %s:%d, STATUS: %d\n", del_indent(), __FUNCTION__, __FILE__, __LINE__, status)
#define print_exit_pointer(status) \
do { \
  if (IS_ERR(status)) \
    fist_dprint(5, "%s OUT: %s %s:%d, RESULT: %ld\n", del_indent(), __FUNCTION__, __FILE__, __LINE__, PTR_ERR(status)); \
  else \
    fist_dprint(5, "%s OUT: %s %s:%d, RESULT: 0x%p\n", del_indent(), __FUNCTION__, __FILE__, __LINE__, status); \
} while (0)

#define print_util_entry(format, args...) fist_dprint(6, "%sIN:  %s %s:%d" format "\n", add_indent(), __FUNCTION__, __FILE__, __LINE__, ##args)
#define print_util_entry_location() fist_dprint(6, "%sIN:  %s %s:%d\n", add_indent(), __FUNCTION__, __FILE__, __LINE__)
#define print_util_exit_location() fist_dprint(7, "%s OUT: %s %s:%d\n", del_indent(), __FUNCTION__, __FILE__, __LINE__)
#define print_util_exit_status(status) fist_dprint(7, "%s OUT: %s %s:%d, STATUS: %d\n", del_indent(), __FUNCTION__, __FILE__, __LINE__, status)
#define print_util_exit_pointer(status) \
do { \
  if (IS_ERR(status)) \
    fist_dprint(7, "%s OUT: %s %s:%d, RESULT: %ld\n", del_indent(), __FUNCTION__, __FILE__, __LINE__, PTR_ERR(status)); \
  else \
    fist_dprint(5, "%s OUT: %s %s:%d, RESULT: 0x%x\n", del_indent(), __FUNCTION__, __FILE__, __LINE__, PTR_ERR(status)); \
} while (0)

#else
#define ASSERT(ex)
#define ASSERT2(ex)
#define PASSERT(ex)
#define PASSERT2(ex)
#define FISTBUG(args...)
#define fist_dprint(args...)
#define print_entry(args...)
#define print_entry_location()
#define print_exit_location()
#define print_exit_status(status)
#define print_exit_pointer(status)
#define print_util_entry(args...)
#define print_util_entry_location()
#define print_util_exit_location()
#define print_util_exit_status(status)
#define print_util_exit_pointer(status)
#endif

#endif				/* __KERNEL__ */

/*
 * DEFINITIONS FOR USER AND KERNEL CODE:
 * (Note: ioctl numbers 1--9 are reserved for fistgen, the rest
 *  are auto-generated automatically based on the user's .fist file.)
 */
# define FIST_IOCTL_GET_DEBUG_VALUE	_IOR(0x15, 1, int)
# define FIST_IOCTL_SET_DEBUG_VALUE	_IOW(0x15, 2, int)
# define UNIONFS_IOCTL_BRANCH_COUNT	_IOR(0x15, 10, int)
# define UNIONFS_IOCTL_INCGEN		_IOR(0x15, 11, int)
# define UNIONFS_IOCTL_ADDBRANCH	_IOW(0x15, 12, int)
# define UNIONFS_IOCTL_DELBRANCH	_IOW(0x15, 13, int)
# define UNIONFS_IOCTL_RDWRBRANCH	_IOW(0x15, 14, int)
# define UNIONFS_IOCTL_QUERYFILE	_IOR(0x15, 15, int)

/* We don't support normal remount, but unionctl uses it. */
# define UNIONFS_REMOUNT_MAGIC		0x4a5a4380

struct unionfs_addbranch_args {
	unsigned int ab_branch;
	char *ab_path;
	unsigned int ab_perms;
};

struct unionfs_rdwrbranch_args {
	unsigned int rwb_branch;
	unsigned int rwb_perms;
};

#endif				/* not __FIST_H_ */
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

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

#include "fist.h"
#include "unionfs.h"

#if defined(UNIONFS_XATTR) && (LINUX_VERSION_CODE >= KERNEL_VERSION(2,4,20))
/* This is lifted from fs/xattr.c */
void *xattr_alloc(size_t size, size_t limit)
{
	void *ptr;

	if (size > limit)
		return ERR_PTR(-E2BIG);

	if (!size)		/* size request, no buffer is needed */
		return NULL;
	else if (size <= PAGE_SIZE)
		ptr = KMALLOC((unsigned long)size, GFP_KERNEL);
	else
		ptr = vmalloc((unsigned long)size);
	if (!ptr)
		return ERR_PTR(-ENOMEM);
	return ptr;
}

void xattr_free(void *ptr, size_t size)
{
	if (!size)		/* size request, no buffer was needed */
		return;
	else if (size <= PAGE_SIZE)
		KFREE(ptr);
	else
		vfree(ptr);
}

/* BKL held by caller.
 * dentry->d_inode->i_sem down
 */
int unionfs_getxattr(struct dentry *dentry, const char *name, void *value,
		     size_t size)
{
	struct dentry *hidden_dentry = NULL;
	int err = -EOPNOTSUPP;
	/* Define these anyway so we don't need as much ifdef'ed code. */
	char *encoded_name = NULL;
	char *encoded_value = NULL;

	print_entry_location();

	lock_dentry(dentry);

	hidden_dentry = dtohd(dentry);

	PASSERT(hidden_dentry);
	PASSERT(hidden_dentry->d_inode);
	PASSERT(hidden_dentry->d_inode->i_op);

	fist_dprint(8, "getxattr: name=\"%s\", value %lu bytes\n", name,
		    (unsigned long)size);

	if (hidden_dentry->d_inode->i_op->getxattr) {
		encoded_name = (char *)name;

		encoded_value = (char *)value;

		down(&hidden_dentry->d_inode->i_sem);
		/* lock_kernel() already done by caller. */
		err =
		    hidden_dentry->d_inode->i_op->getxattr(hidden_dentry,
							   encoded_name,
							   encoded_value, size);
		/* unlock_kernel() will be done by caller. */
		up(&hidden_dentry->d_inode->i_sem);

	}

	unlock_dentry(dentry);
	print_exit_status(err);
	return err;
}

/* BKL held by caller.
 * dentry->d_inode->i_sem down
 */
int
#if defined(FIST_SETXATTR_CONSTVOID) || (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0))
unionfs_setxattr(struct dentry *dentry, const char *name, const void *value,
		 size_t size, int flags)
#else
unionfs_setxattr(struct dentry *dentry, const char *name, void *value,
		 size_t size, int flags)
#endif
{
	struct dentry *hidden_dentry = NULL;
	int err = -EOPNOTSUPP;

	print_entry_location();

	lock_dentry(dentry);
	hidden_dentry = dtohd(dentry);

	PASSERT(hidden_dentry);
	PASSERT(hidden_dentry->d_inode);
	PASSERT(hidden_dentry->d_inode->i_op);

	fist_dprint(8, "setxattr: name=\"%s\", value %lu bytes, flags=%x\n",
		    name, (unsigned long)size, flags);

	if (hidden_dentry->d_inode->i_op->setxattr) {
		down(&hidden_dentry->d_inode->i_sem);
		/* lock_kernel() already done by caller. */
		err = hidden_dentry->d_inode->i_op->
		    setxattr(hidden_dentry, name, value, size, flags);
		/* unlock_kernel() will be done by caller. */
		up(&hidden_dentry->d_inode->i_sem);
	}

	unlock_dentry(dentry);
	print_exit_status(err);
	return err;
}

/* BKL held by caller.
 * dentry->d_inode->i_sem down
 */
int unionfs_removexattr(struct dentry *dentry, const char *name)
{
	struct dentry *hidden_dentry = NULL;
	int err = -EOPNOTSUPP;
	char *encoded_name;
	print_entry_location();

	lock_dentry(dentry);
	hidden_dentry = dtohd(dentry);

	PASSERT(hidden_dentry);
	PASSERT(hidden_dentry->d_inode);
	PASSERT(hidden_dentry->d_inode->i_op);

	fist_dprint(8, "removexattr: name=\"%s\"\n", name);

	if (hidden_dentry->d_inode->i_op->removexattr) {
		encoded_name = (char *)name;

		down(&hidden_dentry->d_inode->i_sem);
		/* lock_kernel() already done by caller. */
		err =
		    hidden_dentry->d_inode->i_op->removexattr(hidden_dentry,
							      encoded_name);
		/* unlock_kernel() will be done by caller. */
		up(&hidden_dentry->d_inode->i_sem);
	}

	unlock_dentry(dentry);
	print_exit_status(err);
	return err;
}

/* BKL held by caller.
 * dentry->d_inode->i_sem down
 */
int unionfs_listxattr(struct dentry *dentry, char *list, size_t size)
{
	struct dentry *hidden_dentry = NULL;
	int err = -EOPNOTSUPP;
	char *encoded_list = NULL;

	print_entry_location();
	lock_dentry(dentry);

	hidden_dentry = dtohd(dentry);

	PASSERT(hidden_dentry);
	PASSERT(hidden_dentry->d_inode);
	PASSERT(hidden_dentry->d_inode->i_op);

	if (hidden_dentry->d_inode->i_op->listxattr) {
		encoded_list = list;
		down(&hidden_dentry->d_inode->i_sem);
		/* lock_kernel() already done by caller. */
		err =
		    hidden_dentry->d_inode->i_op->listxattr(hidden_dentry,
							    encoded_list, size);
		/* unlock_kernel() will be done by caller. */
		up(&hidden_dentry->d_inode->i_sem);
	}

	unlock_dentry(dentry);
	print_exit_status(err);
	return err;
}
# endif				/* UNIONFS_XATTR && (LINUX_VERSION_CODE >= KERNEL_VERSION(2,4,20)) */
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

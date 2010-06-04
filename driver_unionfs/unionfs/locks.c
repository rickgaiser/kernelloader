#if 0
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

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
static inline void __locks_delete_block(struct file_lock *waiter)
{
	list_del_init(&waiter->fl_block);
	list_del_init(&waiter->fl_link);
	waiter->fl_next = NULL;
}

static void locks_delete_block(struct file_lock *waiter)
{
	lock_kernel();
	__locks_delete_block(waiter);
	unlock_kernel();
}
#endif

/*The difference in code between 2.4 and 2.6 is large enough that it is
 * very complicated to keep it in one function.
 */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
int unionfs_setlk(struct file *file, int cmd, struct file_lock *fl)
{
	int error;
	struct inode *inode = NULL;

	print_entry_location();

	error = -EINVAL;
	inode = file->f_dentry->d_inode;
	/* Don't allow mandatory locks on files that may be memory mapped
	 * and shared.
	 */
	if (IS_MANDLOCK(inode) &&
	    (inode->i_mode & (S_ISGID | S_IXGRP)) == S_ISGID) {

		struct address_space *mapping = inode->i_mapping;

		if (mapping->i_mmap_shared != NULL) {
			error = -EAGAIN;
			goto out;
		}
	}
	error = -EBADF;
	switch (fl->fl_type) {
	case F_RDLCK:
		if (!(file->f_mode & FMODE_READ))
			goto out;
		break;
	case F_WRLCK:
		if (!(file->f_mode & FMODE_WRITE))
			goto out;
		break;
	case F_UNLCK:
		break;
	case F_SHLCK:
	case F_EXLCK:
	default:
		error = -EINVAL;
		goto out;
	}
	if (file->f_op && file->f_op->lock != NULL) {
		error = file->f_op->lock(file, cmd, fl);
		if (error < 0)
			goto out;
	}
#ifdef F_SETLKW64
	error = posix_lock_file(file, fl, cmd == F_SETLKW || cmd == F_SETLKW64);
#else
	error = posix_lock_file(file, fl, cmd == F_SETLKW);
#endif

      out:

	print_exit_status(error);
	return error;
}
#else
int unionfs_setlk(struct file *file, int cmd, struct file_lock *fl)
{
	int error;
	struct inode *inode = NULL;

	print_entry_location();

	error = -EINVAL;
	inode = file->f_dentry->d_inode;
	/* Don't allow mandatory locks on files that may be memory mapped
	 * and shared.
	 */
	if (IS_MANDLOCK(inode) &&
	    (inode->i_mode & (S_ISGID | S_IXGRP)) == S_ISGID &&
	    mapping_writably_mapped(file->f_mapping)) {
		error = -EAGAIN;
		goto out;
	}

	error = -EBADF;
	switch (fl->fl_type) {
	case F_RDLCK:
		if (!(file->f_mode & FMODE_READ))
			goto out;
		break;
	case F_WRLCK:
		if (!(file->f_mode & FMODE_WRITE))
			goto out;
		break;
	case F_UNLCK:
		break;
	default:
		error = -EINVAL;
		goto out;
	}

	error = security_file_lock(file, fl->fl_type);
	if (error)
		goto out;
	if (file->f_op && file->f_op->lock != NULL) {
		error = file->f_op->lock(file, cmd, fl);
		goto out;
	}
	for (;;) {
		error = posix_lock_file(file, fl);
		if ((error != -EAGAIN) || (cmd == F_SETLK))
			break;
#ifdef F_SETLK64
		if (cmd == F_SETLK64)
			break;
#endif

		error = wait_event_interruptible(fl->fl_wait, !fl->fl_next);
		if (!error)
			continue;

		locks_delete_block(fl);
		break;
	}
      out:
	print_exit_status(error);
	return error;
}

#endif				/* endif on unionfs_setlk */

int unionfs_getlk(struct file *file, struct file_lock *fl)
{
	int error = 0;
	struct file_lock *tempfl = NULL;

	if (file->f_op && file->f_op->lock) {
		error = file->f_op->lock(file, F_GETLK, fl);
		if (error < 0)
			goto out;
#if LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,10)
		else if (error == LOCK_USE_CLNT)
			/* Bypass for NFS with no locking - 2.0.36 compat */
			tempfl = posix_test_lock(file, fl);
#endif
		else
			tempfl = (fl->fl_type == F_UNLCK ? NULL : fl);
	} else {
		tempfl = posix_test_lock(file, fl);
	}

	if (!tempfl)
		fl->fl_type = F_UNLCK;
	else
		memcpy(fl, tempfl, sizeof(struct file_lock));

      out:
	return error;
}
#endif
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

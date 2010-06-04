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

/*******************
 * File Operations *
 *******************/

static loff_t unionfs_llseek(struct file *file, loff_t offset, int origin)
{
	loff_t err;
	struct file *hidden_file = NULL;

	print_entry_location();

	fist_dprint(6, "unionfs_llseek: file=%p, offset=0x%llx, origin=%d\n",
		    file, offset, origin);

	if ((err = unionfs_file_revalidate(file, 0)))
		goto out;

	PASSERT(ftopd(file));
	hidden_file = ftohf(file);
	PASSERT(hidden_file);
	/* always set hidden position to this one */
	hidden_file->f_pos = file->f_pos;

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
	/* update readahead information, all the time, because the old file
	 * could have read-ahead information that doesn't match.
	 */
	if (file->f_reada) {
		hidden_file->f_reada = file->f_reada;
		hidden_file->f_ramax = file->f_ramax;
		hidden_file->f_raend = file->f_raend;
		hidden_file->f_ralen = file->f_ralen;
		hidden_file->f_rawin = file->f_rawin;
	}
#else
	memcpy(&(hidden_file->f_ra), &(file->f_ra),
	       sizeof(struct file_ra_state));
#endif

	if (hidden_file->f_op && hidden_file->f_op->llseek)
		err = hidden_file->f_op->llseek(hidden_file, offset, origin);
	else
		err = generic_file_llseek(hidden_file, offset, origin);

	if (err < 0)
		goto out;
	if (err != file->f_pos) {
		file->f_pos = err;
		// ION maybe this?
		//      file->f_pos = hidden_file->f_pos;

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
		file->f_reada = 0;
#endif
		file->f_version++;
	}
      out:
	print_exit_status((int)err);
	return err;
}

static ssize_t unionfs_read(struct file *file, char *buf, size_t count,
			    loff_t * ppos)
{
	int err = -EINVAL;
	struct file *hidden_file = NULL;
	loff_t pos = *ppos;

	print_entry_location();

	if ((err = unionfs_file_revalidate(file, 0)))
		goto out;

	fist_print_file("entering read()", file);

	PASSERT(ftopd(file));
	hidden_file = ftohf(file);
	PASSERT(hidden_file);

	if (!hidden_file->f_op || !hidden_file->f_op->read)
		goto out;

	err = hidden_file->f_op->read(hidden_file, buf, count, &pos);
	*ppos = pos;
	if (err >= 0) {
		/* atime should also be updated for reads of size zero or more */
		fist_copy_attr_atime(file->f_dentry->d_inode,
				     hidden_file->f_dentry->d_inode);
	}
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
	/*
	 * because pread() does not have any way to tell us that it is
	 * our caller, then we don't know for sure if we have to update
	 * the file positions.  This hack relies on read() having passed us
	 * the "real" pointer of its struct file's f_pos field.
	 */
	if (ppos == &file->f_pos)
		hidden_file->f_pos = *ppos = pos;
	if (hidden_file->f_reada) {	/* update readahead information if needed */
		file->f_reada = hidden_file->f_reada;
		file->f_ramax = hidden_file->f_ramax;
		file->f_raend = hidden_file->f_raend;
		file->f_ralen = hidden_file->f_ralen;
		file->f_rawin = hidden_file->f_rawin;
	}
#else
	memcpy(&(file->f_ra), &(hidden_file->f_ra),
	       sizeof(struct file_ra_state));
#endif

      out:
	fist_print_file("leaving read()", file);
	print_exit_status(err);
	return err;
}

#if defined(SUPPORT_BROKEN_LOSETUP) && LINUX_VERSION_CODE > KERNEL_VERSION(2,6,0)
static ssize_t unionfs_sendfile(struct file *file, loff_t * ppos,
				size_t count, read_actor_t actor, void *target)
{
	ssize_t err;
	struct file *hidden_file = NULL;

	print_entry_location();

	if ((err = unionfs_file_revalidate(file, 0)))
		goto out;

	hidden_file = ftohf(file);

	err = -EINVAL;
	if (!hidden_file->f_op || !hidden_file->f_op->sendfile)
		goto out;

	err = hidden_file->f_op->sendfile(hidden_file, ppos, count, actor,
					  target);

      out:
	print_exit_status(err);
	return err;
}
#endif

/* this unionfs_write() does not modify data pages! */
static ssize_t unionfs_write(struct file *file, const char *buf, size_t count,
			     loff_t * ppos)
{
	int err = -EINVAL;
	struct file *hidden_file = NULL;
	struct inode *inode;
	struct inode *hidden_inode;
	loff_t pos = *ppos;
	int bstart, bend;

	print_entry_location();

	if ((err = unionfs_file_revalidate(file, 1)))
		goto out;

	inode = file->f_dentry->d_inode;

	bstart = fbstart(file);
	bend = fbend(file);

	ASSERT(bstart != -1);
	PASSERT(ftopd(file));
	PASSERT(ftohf(file));

	hidden_file = ftohf(file);
	hidden_inode = hidden_file->f_dentry->d_inode;

	if (!hidden_file->f_op || !hidden_file->f_op->write)
		goto out;

	/* adjust for append -- seek to the end of the file */
	if (file->f_flags & O_APPEND)
		pos = inode->i_size;

	err = hidden_file->f_op->write(hidden_file, buf, count, &pos);

	/*
	 * copy ctime and mtime from lower layer attributes
	 * atime is unchanged for both layers
	 */
	if (err >= 0)
		fist_copy_attr_times(inode, hidden_inode);

	*ppos = pos;
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
	/*
	 * XXX: MAJOR HACK
	 *
	 * because pwrite() does not have any way to tell us that it is
	 * our caller, then we don't know for sure if we have to update
	 * the file positions.  This hack relies on write() having passed us
	 * the "real" pointer of its struct file's f_pos field.
	 */
	if (ppos == &file->f_pos)
		hidden_file->f_pos = *ppos = pos;
#endif

	/* update this inode's size */
	if (pos > inode->i_size)
		inode->i_size = pos;

      out:
	print_exit_status(err);
	return err;
}

static int unionfs_file_readdir(struct file *file, void *dirent,
				filldir_t filldir)
{
	int err = -ENOTDIR;
	print_entry_location();
	print_exit_status(err);
	return err;
}

static unsigned int unionfs_poll(struct file *file, poll_table * wait)
{
	unsigned int mask = DEFAULT_POLLMASK;
	struct file *hidden_file = NULL;

	print_entry_location();

	if (unionfs_file_revalidate(file, 0)) {
		/* We should pretend an error happend. */
		mask = POLLERR | POLLIN | POLLOUT;
		goto out;
	}

	if (ftopd(file) != NULL)
		hidden_file = ftohf(file);

	if (!hidden_file->f_op || !hidden_file->f_op->poll)
		goto out;

	mask = hidden_file->f_op->poll(hidden_file, wait);

      out:
	print_exit_status(mask);
	return mask;
}

/* FIST-LITE special version of mmap */
static int unionfs_mmap(struct file *file, struct vm_area_struct *vma)
{
	int err = 0;
	struct file *hidden_file = NULL;
	int willwrite;

	print_entry_location();

	/* This might could be deferred to mmap's writepage. */
	willwrite = ((vma->vm_flags | VM_SHARED | VM_WRITE) == vma->vm_flags);
	if ((err = unionfs_file_revalidate(file, willwrite)))
		goto out;

	PASSERT(ftopd(file));
	hidden_file = ftohf(file);

	err = -ENODEV;
	if (!hidden_file->f_op || !hidden_file->f_op->mmap)
		goto out;

	PASSERT(hidden_file);
	PASSERT(hidden_file->f_op);
	PASSERT(hidden_file->f_op->mmap);

	vma->vm_file = hidden_file;
	err = hidden_file->f_op->mmap(hidden_file, vma);
	get_file(hidden_file);	/* make sure it doesn't get freed on us */
	fput(file);		/* no need to keep extra ref on ours */

      out:
	print_exit_status(err);
	return err;
}

static int unionfs_fsync(struct file *file, struct dentry *dentry, int datasync)
{
	int err;
	struct file *hidden_file = NULL;

	print_entry_location();

	if ((err = unionfs_file_revalidate(file, 1)))
		goto out;

	PASSERT(ftopd(file));
	hidden_file = ftohf(file);

	err = -EINVAL;
	if (!hidden_file->f_op && !hidden_file->f_op->fsync)
		goto out;

	down(&hidden_file->f_dentry->d_inode->i_sem);
	err = hidden_file->f_op->fsync(hidden_file, hidden_file->f_dentry,
				       datasync);
	up(&hidden_file->f_dentry->d_inode->i_sem);

      out:
	print_exit_status(err);
	return err;
}

static int unionfs_fasync(int fd, struct file *file, int flag)
{
	int err = 0;
	struct file *hidden_file = NULL;

	print_entry_location();

	if ((err = unionfs_file_revalidate(file, 1)))
		goto out;

	PASSERT(ftopd(file));
	hidden_file = ftohf(file);

	if (hidden_file->f_op && hidden_file->f_op->fasync)
		err = hidden_file->f_op->fasync(fd, hidden_file, flag);

      out:
	print_exit_status(err);
	return err;
}

/* This is broken dont use
 * 
static int unionfs_lock(struct file *file, int cmd, struct file_lock *fl)
{
	int err = 0;
	struct file *lower_file = NULL;
	int write = 0;

	print_entry_location();

	if (fl->fl_type == F_WRLCK && (cmd == F_SETLK || cmd == F_SETLKW
#ifdef F_SETLK64
				       || cmd == F_SETLK64 || cmd == F_SETLKW64
#endif
	    ))
		write = 1;

	if ((err = unionfs_file_revalidate(file, write)))
		goto out;

	PASSERT(ftopd(file));
	lower_file = ftohf(file);
	PASSERT(lower_file);

	err = -EINVAL;
	if (!fl)
		goto out;
	fl->fl_file = lower_file;

	//Since we are being send a posix lock and not a flock or flock64 we can
	//treat the 64bit and regular calls the same.
	switch (cmd) {
	case F_GETLK:
#ifdef F_GETLK64
	case F_GETLK64:
#endif
		err = unionfs_getlk(lower_file, fl);
		break;

	case F_SETLK:
	case F_SETLKW:
#ifdef F_SETLK64
	case F_SETLK64:
	case F_SETLKW64:
#endif
		err = unionfs_setlk(lower_file, cmd, fl);
		break;
	default:
		err = -EINVAL;
	}
	fl->fl_file = file;

      out:
	print_exit_status(err);
	return err;
}
*/
struct file_operations unionfs_main_fops = {
	.llseek = unionfs_llseek,
	.read = unionfs_read,
	.write = unionfs_write,
	.readdir = unionfs_file_readdir,
	.poll = unionfs_poll,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,11)
	.unlocked_ioctl = unionfs_ioctl,
#else
	.ioctl = unionfs_ioctl,
#endif
	.mmap = unionfs_mmap,
	.open = unionfs_open,
	.flush = unionfs_flush,
	.release = unionfs_file_release,
	.fsync = unionfs_fsync,
	.fasync = unionfs_fasync,
#if defined(SUPPORT_BROKEN_LOSETUP) && LINUX_VERSION_CODE > KERNEL_VERSION(2,6,0)
	.sendfile = unionfs_sendfile,
#endif
	//.lock = unionfs_lock, This is broken we will let the vfs handle 
	//locking on the upper filesystem
};

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

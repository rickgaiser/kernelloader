/*
 *  Adpated from linux/fs/bad_inode.c
 *
 *  Copyright (C) 1997, Stephen Tweedie
 *
 *  Provide stub functions for "stale" inodes, a bit friendlier than the
 *  -EIO that bad_inode.c does.
 */
/*
 *  $Id$
 */

#include <linux/config.h>
#include <linux/version.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
#ifdef CONFIG_MODVERSIONS
# define MODVERSIONS
# include <linux/modversions.h>
#endif				/* CONFIG_MODVERSIONS */
#endif				/* KERNEL_VERSION < 2.6.0 */
#include <linux/fs.h>
#include <linux/stat.h>
#include <linux/sched.h>

static struct address_space_operations unionfs_stale_aops;

/*
 * The follow_link operation is special: it must behave as a no-op
 * so that a stale root inode can at least be unmounted. To do this
 * we must dput() the base and return the dentry with a dget().
 */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,13)
static int stale_follow_link(struct dentry *dent, struct nameidata *nd)
#else
static void *stale_follow_link(struct dentry *dent, struct nameidata *nd)
#endif
{
	int err = vfs_follow_link(nd, ERR_PTR(-ESTALE));
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,13)
	return err;
#else
	return ERR_PTR(err);
#endif
}

static int return_ESTALE(void)
{
	return -ESTALE;
}

#define ESTALE_ERROR ((void *) (return_ESTALE))

static struct file_operations stale_file_ops = {
	.llseek = ESTALE_ERROR,
	.read = ESTALE_ERROR,
	.write = ESTALE_ERROR,
	.readdir = ESTALE_ERROR,
	.poll = ESTALE_ERROR,
	.ioctl = ESTALE_ERROR,
	.mmap = ESTALE_ERROR,
	.open = ESTALE_ERROR,
	.flush = ESTALE_ERROR,
	.release = ESTALE_ERROR,
	.fsync = ESTALE_ERROR,
	.fasync = ESTALE_ERROR,
	.lock = ESTALE_ERROR,
};

struct inode_operations stale_inode_ops = {
	.create = ESTALE_ERROR,
	.lookup = ESTALE_ERROR,
	.link = ESTALE_ERROR,
	.unlink = ESTALE_ERROR,
	.symlink = ESTALE_ERROR,
	.mkdir = ESTALE_ERROR,
	.rmdir = ESTALE_ERROR,
	.mknod = ESTALE_ERROR,
	.rename = ESTALE_ERROR,
	.readlink = ESTALE_ERROR,
	.follow_link = stale_follow_link,
	.truncate = ESTALE_ERROR,
	.permission = ESTALE_ERROR,
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
	.revalidate = ESTALE_ERROR,
#endif
};

/*
 * When a filesystem is unable to read an inode due to an I/O error in
 * its read_inode() function, it can call make_stale_inode() to return a
 * set of stubs which will return ESTALE errors as required.
 *
 * We only need to do limited initialisation: all other fields are
 * preinitialised to zero automatically.
 */

/**
 *	make_stale_inode - mark an inode stale due to an I/O error
 *	@inode: Inode to mark stale
 *
 *	When an inode cannot be read due to a media or remote network
 *	failure this function makes the inode "stale" and causes I/O operations
 *	on it to fail from this point on.
 */

void make_stale_inode(struct inode *inode)
{
	inode->i_mode = S_IFREG;
	inode->i_atime = inode->i_mtime = inode->i_ctime = CURRENT_TIME;
	inode->i_op = &stale_inode_ops;
	inode->i_fop = &stale_file_ops;
	inode->i_mapping->a_ops = &unionfs_stale_aops;
}

/*
 * This tests whether an inode has been flagged as stale. The test uses
 * &stale_inode_ops to cover the case of invalidated inodes as well as
 * those created by make_stale_inode() above.
 */

/**
 *	is_stale_inode - is an inode errored
 *	@inode: inode to test
 *
 *	Returns true if the inode in question has been marked as stale.
 */

int is_stale_inode(struct inode *inode)
{
	return (inode->i_op == &stale_inode_ops);
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

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

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
static int unionfs_create(struct inode *parent, struct dentry *dentry, int mode)
#else
static int unionfs_create(struct inode *parent, struct dentry *dentry,
			  int mode, struct nameidata *nd)
#endif
{
	int err = 0;
	struct dentry *hidden_dentry = NULL;
	struct dentry *whiteout_dentry = NULL;
	struct dentry *new_hidden_dentry;
	struct dentry *hidden_parent_dentry = NULL;
	int bindex = 0, bstart;
	char *name = NULL;

	print_entry_location();
	lock_dentry(dentry);
	fist_print_dentry("IN unionfs_create", dentry);

	/* We start out in the leftmost branch. */
	bstart = dbstart(dentry);
	hidden_dentry = dtohd(dentry);

	/* check if whiteout exists in this branch, i.e. lookup .wh.foo first */
	name = KMALLOC(dentry->d_name.len + sizeof(".wh"), GFP_UNIONFS);
	if (!name) {
		err = -ENOMEM;
		goto out;
	}
	strcpy(name, ".wh.");
	strncat(name, dentry->d_name.name, dentry->d_name.len);
	name[4 + dentry->d_name.len] = '\0';

	whiteout_dentry =
	    LOOKUP_ONE_LEN(name, hidden_dentry->d_parent,
			   dentry->d_name.len + 4);
	if (IS_ERR(whiteout_dentry)) {
		err = PTR_ERR(whiteout_dentry);
		whiteout_dentry = NULL;
		goto out;
	}
	PASSERT(whiteout_dentry);

	if (whiteout_dentry->d_inode) {
		/* .wh.foo has been found. */
		/* First truncate it and then rename it to foo (hence having
		 * the same overall effect as a normal create.
		 *
		 * XXX: This is not strictly correct.  If we have unlinked the
		 * file and it still has a reference count, then we should
		 * actually unlink the whiteout so that user's data isn't
		 * hosed over.
		 */
		struct dentry *hidden_dir_dentry;
		struct iattr newattrs;

		down(&whiteout_dentry->d_inode->i_sem);
		newattrs.ia_valid = ATTR_CTIME | ATTR_MODE;
		newattrs.ia_mode = mode;
		if (whiteout_dentry->d_inode->i_size != 0) {
			newattrs.ia_valid |= ATTR_SIZE;
			newattrs.ia_size = 0;
		}
		err = notify_change(whiteout_dentry, &newattrs);
		up(&whiteout_dentry->d_inode->i_sem);

		new_hidden_dentry = dtohd(dentry);
		DGET(new_hidden_dentry);

		/* They are in the same directory, but we need to get it twice. */
		hidden_dir_dentry = GET_PARENT(whiteout_dentry);
		GET_PARENT(whiteout_dentry);
		double_lock(hidden_dir_dentry, hidden_dir_dentry);

		if (!(err = is_robranch_super(dentry->d_sb, bstart))) {
			err =
			    vfs_rename(hidden_dir_dentry->d_inode,
				       whiteout_dentry,
				       hidden_dir_dentry->d_inode,
				       new_hidden_dentry);
		}
		if (!err) {
			fist_copy_attr_timesizes(parent,
						 new_hidden_dentry->d_parent->
						 d_inode);
			parent->i_nlink = get_nlinks(parent);
		}

		/* This will dput our double gotten parent. */
		double_unlock(hidden_dir_dentry, hidden_dir_dentry);

		DPUT(new_hidden_dentry);

		if (err) {
			/* exit if the error returned was NOT -EROFS */
			if (!IS_COPYUP_ERR(err))
				goto out;
			/* We were not able to create the file in this branch,
			 * so, we try to create it in one branch to left
			 */
			bstart--;
		} else {
			/* reset the unionfs dentry to point to the .wh.foo entry. */

			/* Discard any old reference. */
			PASSERT(dtohd(dentry));
			DPUT(dtohd(dentry));

			/* Trade one reference to another. */
			set_dtohd_index(dentry, bstart, whiteout_dentry);
			whiteout_dentry = NULL;

			err = unionfs_interpose(dentry, parent->i_sb, 0);
			goto out;
		}
	}

	for (bindex = bstart; bindex >= 0; bindex--) {
		hidden_dentry = dtohd_index(dentry, bindex);
		if (!hidden_dentry) {
			/* if hidden_dentry is NULL, create the entire
			 * dentry directory structure in branch 'bindex'.
			 * hidden_dentry will NOT be null when bindex == bstart
			 * because lookup passed as a negative unionfs dentry
			 * pointing to a lone negative underlying dentry */
			hidden_dentry = create_parents(parent, dentry, bindex);
			if (!hidden_dentry || IS_ERR(hidden_dentry)) {
				if (IS_ERR(hidden_dentry))
					err = PTR_ERR(hidden_dentry);
				continue;
			}
		}

		PASSERT(hidden_dentry);
		fist_checkinode(parent, "unionfs_create");

		hidden_parent_dentry = lock_parent(hidden_dentry);
		if (IS_ERR(hidden_parent_dentry)) {
			err = PTR_ERR(hidden_parent_dentry);
			goto out;
		}
		/* We shouldn't create things in a read-only branch. */
		if (!(err = is_robranch_super(dentry->d_sb, bindex))) {
			//DQ: vfs_create has a different prototype in 2.6
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
			err = vfs_create(hidden_parent_dentry->d_inode,
					 hidden_dentry, mode);
#else
			err = vfs_create(hidden_parent_dentry->d_inode,
					 hidden_dentry, mode, nd);
#endif

		}
		if (err || !hidden_dentry->d_inode) {
			unlock_dir(hidden_parent_dentry);

			/* break out of for loop if the error wasn't  -EROFS */
			if (!IS_COPYUP_ERR(err))
				break;
		} else {
			err = unionfs_interpose(dentry, parent->i_sb, 0);
			if (!err) {
				fist_copy_attr_timesizes(parent,
							 hidden_parent_dentry->
							 d_inode);
				/* update number of links on parent directory */
				parent->i_nlink = get_nlinks(parent);
			}
			unlock_dir(hidden_parent_dentry);
			break;
		}
	}

      out:
	DPUT(whiteout_dentry);
	KFREE(name);

	fist_print_dentry("OUT unionfs_create :", dentry);
	unlock_dentry(dentry);
	print_exit_status(err);
	return err;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
struct dentry *unionfs_lookup(struct inode *parent, struct dentry *dentry)
#else
struct dentry *unionfs_lookup(struct inode *parent, struct dentry *dentry,
			      struct nameidata *nd)
#endif
{
	/* The locking is done by unionfs_lookup_backend. */
	return unionfs_lookup_backend(dentry, INTERPOSE_LOOKUP);
}

static int unionfs_link(struct dentry *old_dentry, struct inode *dir,
			struct dentry *new_dentry)
{
	int err = 0;
	struct dentry *hidden_old_dentry = NULL;
	struct dentry *hidden_new_dentry = NULL;
	struct dentry *hidden_dir_dentry = NULL;
	struct dentry *whiteout_dentry;
	char *name = NULL;

	print_entry_location();
	double_lock_dentry(new_dentry, old_dentry);

	hidden_new_dentry = dtohd(new_dentry);

	/* check if whiteout exists in the branch of new dentry, i.e. lookup
	 * .wh.foo first. If present, delete it */
	name = KMALLOC(new_dentry->d_name.len + sizeof(".wh."), GFP_UNIONFS);
	if (!name) {
		err = -ENOMEM;
		goto out;
	}
	strcpy(name, ".wh.");
	strncat(name, new_dentry->d_name.name, new_dentry->d_name.len);
	name[4 + new_dentry->d_name.len] = '\0';

	whiteout_dentry =
	    LOOKUP_ONE_LEN(name, hidden_new_dentry->d_parent,
			   new_dentry->d_name.len + 4);
	if (IS_ERR(whiteout_dentry)) {
		err = PTR_ERR(whiteout_dentry);
		goto out;
	}

	PASSERT(whiteout_dentry);

	if (!whiteout_dentry->d_inode) {
		DPUT(whiteout_dentry);
		whiteout_dentry = NULL;
	} else {
		/* found a .wh.foo entry, unlink it and then call vfs_link() */
		hidden_dir_dentry = lock_parent(whiteout_dentry);
		if (!
		    (err =
		     is_robranch_super(new_dentry->d_sb,
				       dbstart(new_dentry)))) {
			err =
			    vfs_unlink(hidden_dir_dentry->d_inode,
				       whiteout_dentry);
		}
		fist_copy_attr_times(dir, hidden_dir_dentry->d_inode);
		dir->i_nlink = get_nlinks(dir);
		unlock_dir(hidden_dir_dentry);
		hidden_dir_dentry = NULL;
		DPUT(whiteout_dentry);
		if (err)
			goto out;
	}

	if (dbstart(old_dentry) != dbstart(new_dentry)) {
		hidden_new_dentry =
		    create_parents(dir, new_dentry, dbstart(old_dentry));
		err = PTR_ERR(hidden_new_dentry);
		if (IS_COPYUP_ERR(err))
			goto docopyup;
		if (!hidden_new_dentry || IS_ERR(hidden_new_dentry))
			goto out;
	}
	hidden_new_dentry = dtohd(new_dentry);
	hidden_old_dentry = dtohd(old_dentry);

	ASSERT(dbstart(old_dentry) == dbstart(new_dentry));
	hidden_dir_dentry = lock_parent(hidden_new_dentry);
	if (!(err = is_robranch(old_dentry)))
		err =
		    vfs_link(hidden_old_dentry, hidden_dir_dentry->d_inode,
			     hidden_new_dentry);
	unlock_dir(hidden_dir_dentry);

      docopyup:
	if (IS_COPYUP_ERR(err)) {
		int old_bstart = dbstart(old_dentry);
		int bindex;

		for (bindex = old_bstart - 1; bindex >= 0; bindex--) {
			err =
			    copyup_dentry(old_dentry->d_parent->
					  d_inode, old_dentry,
					  old_bstart, bindex, NULL,
					  old_dentry->d_inode->i_size);
			if (!err) {
				hidden_new_dentry =
				    create_parents(dir, new_dentry, bindex);
				hidden_old_dentry = dtohd(old_dentry);
				hidden_dir_dentry =
				    lock_parent(hidden_new_dentry);
				/* do vfs_link */
				err =
				    vfs_link(hidden_old_dentry,
					     hidden_dir_dentry->d_inode,
					     hidden_new_dentry);
				unlock_dir(hidden_dir_dentry);
				goto check_link;
			}
		}
		goto out;
	}
      check_link:
	if (err || !hidden_new_dentry->d_inode)
		goto out;

	/* Its a hard link, so use the same inode */
	new_dentry->d_inode = igrab(old_dentry->d_inode);
	PASSERT(new_dentry->d_inode);
	d_instantiate(new_dentry, new_dentry->d_inode);
	fist_copy_attr_all(dir, hidden_new_dentry->d_parent->d_inode);
	/* propagate number of hard-links */
	old_dentry->d_inode->i_nlink = get_nlinks(old_dentry->d_inode);

      out:
	if (!new_dentry->d_inode)
		d_drop(new_dentry);

	KFREE(name);

	unlock_dentry(new_dentry);
	unlock_dentry(old_dentry);

	print_exit_status(err);
	return err;
}

static int unionfs_symlink(struct inode *dir, struct dentry *dentry,
			   const char *symname)
{
	int err = 0;
	struct dentry *hidden_dentry = NULL;
	struct dentry *whiteout_dentry = NULL;
	struct dentry *hidden_dir_dentry = NULL;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
	umode_t mode;
#endif
	int bindex = 0, bstart;
	char *name = NULL;

	print_entry_location();
	lock_dentry(dentry);
	fist_print_dentry("IN unionfs_symlink", dentry);

	/* We start out in the leftmost branch. */
	bstart = dbstart(dentry);

	hidden_dentry = dtohd(dentry);

	/* check if whiteout exists in this branch, i.e. lookup .wh.foo first. If present, delete it */
	name = KMALLOC(dentry->d_name.len + sizeof(".wh"), GFP_UNIONFS);
	if (!name) {
		err = -ENOMEM;
		goto out;
	}
	strcpy(name, ".wh.");
	strncat(name, dentry->d_name.name, dentry->d_name.len);
	name[4 + dentry->d_name.len] = '\0';

	whiteout_dentry =
	    LOOKUP_ONE_LEN(name, hidden_dentry->d_parent,
			   dentry->d_name.len + 4);
	if (IS_ERR(whiteout_dentry)) {
		err = PTR_ERR(whiteout_dentry);
		goto out;
	}

	PASSERT(whiteout_dentry);
	if (!whiteout_dentry->d_inode) {
		DPUT(whiteout_dentry);
		whiteout_dentry = NULL;
	} else {
		/* found a .wh.foo entry, unlink it and then call vfs_symlink() */
		hidden_dir_dentry = lock_parent(whiteout_dentry);

		fist_print_generic_dentry("HDD", hidden_dir_dentry);
		fist_print_generic_dentry("WD", whiteout_dentry);

		if (!(err = is_robranch_super(dentry->d_sb, bstart))) {
			err =
			    vfs_unlink(hidden_dir_dentry->d_inode,
				       whiteout_dentry);
		}
		DPUT(whiteout_dentry);

		fist_copy_attr_times(dir, hidden_dir_dentry->d_inode);
		/* propagate number of hard-links */
		dir->i_nlink = get_nlinks(dir);

		unlock_dir(hidden_dir_dentry);

		if (err) {
			/* exit if the error returned was NOT -EROFS */
			if (!IS_COPYUP_ERR(err))
				goto out;
			/* should now try to create symlink in the another branch */
			bstart--;
		}
	}

	/* deleted whiteout if it was present, now do a normal vfs_symlink() with
	   possible recursive directory creation */
	for (bindex = bstart; bindex >= 0; bindex--) {
		hidden_dentry = dtohd_index(dentry, bindex);
		if (!hidden_dentry) {
			/* if hidden_dentry is NULL, create the entire
			 * dentry directory structure in branch 'bindex'. hidden_dentry will NOT be null when
			 * bindex == bstart because lookup passed as a negative unionfs dentry pointing to a
			 * lone negative underlying dentry */
			hidden_dentry = create_parents(dir, dentry, bindex);
			if (!hidden_dentry || IS_ERR(hidden_dentry)) {
				if (IS_ERR(hidden_dentry)) {
					err = PTR_ERR(hidden_dentry);
				}
				fist_dprint(8,
					    "hidden dentry NULL (or error) for bindex = %d\n",
					    bindex);
				continue;
			}
		}

		PASSERT(hidden_dentry);

		hidden_dir_dentry = lock_parent(hidden_dentry);

		if (!(err = is_robranch_super(dentry->d_sb, bindex))) {
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
			err =
			    vfs_symlink(hidden_dir_dentry->d_inode,
					hidden_dentry, symname);
#else
			mode = S_IALLUGO;
			err =
			    vfs_symlink(hidden_dir_dentry->d_inode,
					hidden_dentry, symname, mode);
#endif
		}
		unlock_dir(hidden_dir_dentry);

		if (err || !hidden_dentry->d_inode) {
			/* break out of for loop if error returned was NOT -EROFS */
			if (!IS_COPYUP_ERR(err))
				break;
		} else {
			err = unionfs_interpose(dentry, dir->i_sb, 0);
			if (!err) {
				fist_copy_attr_timesizes(dir,
							 hidden_dir_dentry->
							 d_inode);
				/* update number of links on parent directory */
				dir->i_nlink = get_nlinks(dir);
			}
			break;
		}
	}

      out:
	if (!dentry->d_inode)
		d_drop(dentry);

	KFREE(name);
	fist_print_dentry("OUT unionfs_symlink :", dentry);
	unlock_dentry(dentry);
	print_exit_status(err);
	return err;
}

static int unionfs_mkdir(struct inode *parent, struct dentry *dentry, int mode)
{
	int err = 0;
	struct dentry *hidden_dentry = NULL, *whiteout_dentry = NULL;
	struct dentry *hidden_parent_dentry = NULL;
	int bindex = 0, bstart;
	char *name = NULL;
	int whiteout_unlinked = 0;
	uid_t saved_uid = current->fsuid;
	gid_t saved_gid = current->fsgid;

	print_entry_location();
	lock_dentry(dentry);
	fist_print_dentry("IN unionfs_mkdir", dentry);
	bstart = dbstart(dentry);

	hidden_dentry = dtohd(dentry);

	// check if whiteout exists in this branch, i.e. lookup .wh.foo first
	name = KMALLOC(dentry->d_name.len + sizeof(".wh"), GFP_UNIONFS);
	if (!name) {
		err = -ENOMEM;
		goto out;
	}
	strcpy(name, ".wh.");
	strncat(name, dentry->d_name.name, dentry->d_name.len);
	name[4 + dentry->d_name.len] = '\0';

	whiteout_dentry =
	    LOOKUP_ONE_LEN(name, hidden_dentry->d_parent,
			   dentry->d_name.len + 4);
	if (IS_ERR(whiteout_dentry)) {
		err = PTR_ERR(whiteout_dentry);
		goto out;
	}
	PASSERT(whiteout_dentry);

	if (!whiteout_dentry->d_inode) {
		DPUT(whiteout_dentry);
		whiteout_dentry = NULL;
	} else {
		hidden_parent_dentry = lock_parent(whiteout_dentry);

		/* Set the uid and gid to trick the fs into allowing us to create
		 * the file */
		current->fsuid = hidden_parent_dentry->d_inode->i_uid;
		current->fsgid = hidden_parent_dentry->d_inode->i_gid;
		//found a.wh.foo entry, remove it then do vfs_mkdir
		if (!(err = is_robranch_super(dentry->d_sb, bstart))) {
			err =
			    vfs_unlink(hidden_parent_dentry->d_inode,
				       whiteout_dentry);
		}
		DPUT(whiteout_dentry);

		current->fsuid = saved_uid;
		current->fsgid = saved_gid;

		unlock_dir(hidden_parent_dentry);

		if (err) {
			/* exit if the error returned was NOT -EROFS */
			if (!IS_COPYUP_ERR(err))
				goto out;
			bstart--;
		} else {
			whiteout_unlinked = 1;
		}
	}

	for (bindex = bstart; bindex >= 0; bindex--) {
		hidden_dentry = dtohd_index(dentry, bindex);
		if (!hidden_dentry) {
			hidden_dentry = create_parents(parent, dentry, bindex);
			if (!hidden_dentry || IS_ERR(hidden_dentry)) {
				fist_dprint(8,
					    "hidden dentry NULL for bindex = %d\n",
					    bindex);
				continue;
			}
		}

		PASSERT(hidden_dentry);

		hidden_parent_dentry = lock_parent(hidden_dentry);
		if (IS_ERR(hidden_parent_dentry)) {
			err = PTR_ERR(hidden_parent_dentry);
			goto out;
		}
		if (!(err = is_robranch_super(dentry->d_sb, bindex))) {
			err =
			    vfs_mkdir(hidden_parent_dentry->d_inode,
				      hidden_dentry, mode);
		}
		unlock_dir(hidden_parent_dentry);

		/* XXX this could potentially return a negative hidden_dentry! */
		if (err || !hidden_dentry->d_inode) {
			/* break out of for loop if error returned was NOT -EROFS */
			if (!IS_COPYUP_ERR(err))
				break;
		} else {
			int i;
			int bend = dbend(dentry);
			for (i = bindex + 1; i < bend; i++) {
				if (dtohd_index(dentry, i)) {
					DPUT(dtohd_index(dentry, i));
					set_dtohd_index(dentry, i, NULL);
				}
			}
			bend = bindex;
			set_dbend(dentry, bend);

			err = unionfs_interpose(dentry, parent->i_sb, 0);
			if (!err) {
				fist_copy_attr_timesizes(parent,
							 hidden_parent_dentry->
							 d_inode);
				/* update number of links on parent directory */
				parent->i_nlink = get_nlinks(parent);
			}
			whiteout_dentry = LOOKUP_ONE_LEN(UNIONFS_DIR_OPAQUE,
							 hidden_dentry,
							 sizeof
							 (UNIONFS_DIR_OPAQUE) -
							 1);
			if (IS_ERR(whiteout_dentry)) {
				err = PTR_ERR(whiteout_dentry);
				goto out;
			}
			down(&hidden_dentry->d_inode->i_sem);
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
			err = vfs_create(hidden_dentry->d_inode,
					 whiteout_dentry, 0600);
#else
			err = vfs_create(hidden_dentry->d_inode,
					 whiteout_dentry, 0600, NULL);
#endif
			up(&hidden_dentry->d_inode->i_sem);
			DPUT(whiteout_dentry);

			if (err) {
				fist_dprint(8,
					    "mkdir: error creating directory override entry: %d\n",
					    err);
				goto out;
			}
			break;
		}
	}

      out:
	if (!dentry->d_inode)
		d_drop(dentry);

	KFREE(name);

	fist_print_dentry("OUT unionfs_mkdir :", dentry);
	unlock_dentry(dentry);
	print_exit_status(err);
	return err;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
static int unionfs_mknod(struct inode *dir, struct dentry *dentry, int mode,
			 int dev)
#else
static int unionfs_mknod(struct inode *dir, struct dentry *dentry, int mode,
			 dev_t dev)
#endif
{
	int err = 0;
	struct dentry *hidden_dentry = NULL, *whiteout_dentry = NULL;
	struct dentry *hidden_parent_dentry = NULL;
	int bindex = 0, bstart;
	char *name = NULL;
	int whiteout_unlinked = 0;

	print_entry_location();
	lock_dentry(dentry);
	fist_print_dentry("IN unionfs_mknod", dentry);
	bstart = dbstart(dentry);

	hidden_dentry = dtohd(dentry);

	// check if whiteout exists in this branch, i.e. lookup .wh.foo first
	name = KMALLOC(dentry->d_name.len + sizeof(".wh"), GFP_UNIONFS);
	if (!name) {
		err = -ENOMEM;
		goto out;
	}
	strcpy(name, ".wh.");
	strncat(name, dentry->d_name.name, dentry->d_name.len);
	name[4 + dentry->d_name.len] = '\0';

	whiteout_dentry =
	    LOOKUP_ONE_LEN(name, hidden_dentry->d_parent,
			   dentry->d_name.len + 4);
	if (IS_ERR(whiteout_dentry)) {
		err = PTR_ERR(whiteout_dentry);
		goto out;
	}

	if (!whiteout_dentry->d_inode) {
		DPUT(whiteout_dentry);
		whiteout_dentry = NULL;
	} else {
		/* found .wh.foo, unlink it */
		hidden_parent_dentry = lock_parent(whiteout_dentry);

		//found a.wh.foo entry, remove it then do vfs_mkdir
		if (!(err = is_robranch_super(dentry->d_sb, bstart)))
			err = vfs_unlink(hidden_parent_dentry->d_inode,
					 whiteout_dentry);
		DPUT(whiteout_dentry);

		unlock_dir(hidden_parent_dentry);

		if (err) {
			if (!IS_COPYUP_ERR(err))
				goto out;

			bstart--;
		} else {
			whiteout_unlinked = 1;
		}
	}

	for (bindex = bstart; bindex >= 0; bindex--) {
		hidden_dentry = dtohd_index(dentry, bindex);
		if (!hidden_dentry) {
			hidden_dentry = create_parents(dir, dentry, bindex);
			if (!hidden_dentry || IS_ERR(hidden_dentry)) {
				fist_dprint(8,
					    "hidden dentry NULL for bindex = %d\n",
					    bindex);
				continue;
			}
		}

		PASSERT(hidden_dentry);

		hidden_parent_dentry = lock_parent(hidden_dentry);
		if (IS_ERR(hidden_parent_dentry)) {
			err = PTR_ERR(hidden_parent_dentry);
			goto out;
		}
		if (!(err = is_robranch_super(dentry->d_sb, bindex))) {
			err = vfs_mknod(hidden_parent_dentry->d_inode,
					hidden_dentry, mode, dev);
		}
		/* XXX this could potentially return a negative hidden_dentry! */
		if (err || !hidden_dentry->d_inode) {
			unlock_dir(hidden_parent_dentry);
			/* break out of for, if error was NOT -EROFS */
			if (!IS_COPYUP_ERR(err))
				break;
		} else {
			err = unionfs_interpose(dentry, dir->i_sb, 0);
			if (!err) {
				fist_copy_attr_timesizes(dir,
							 hidden_parent_dentry->
							 d_inode);
				/* update number of links on parent directory */
				dir->i_nlink = get_nlinks(dir);
			}
			unlock_dir(hidden_parent_dentry);

			break;
		}
	}

      out:
	if (!dentry->d_inode)
		d_drop(dentry);

	if (name) {
		KFREE(name);
	}

	fist_print_dentry("OUT unionfs_mknod :", dentry);
	unlock_dentry(dentry);
	print_exit_status(err);
	return err;
}

static int unionfs_readlink(struct dentry *dentry, char *buf, int bufsiz)
{
	int err;
	struct dentry *hidden_dentry;

	print_entry_location();
	lock_dentry(dentry);
	hidden_dentry = dtohd(dentry);
	fist_print_dentry("unionfs_readlink IN", dentry);

	if (!hidden_dentry->d_inode->i_op ||
	    !hidden_dentry->d_inode->i_op->readlink) {
		err = -EINVAL;
		goto out;
	}

	err = hidden_dentry->d_inode->i_op->readlink(hidden_dentry,
						     buf, bufsiz);
	if (err > 0)
		fist_copy_attr_atime(dentry->d_inode, hidden_dentry->d_inode);

      out:
	unlock_dentry(dentry);
	print_exit_status(err);
	return err;
}

/* We don't lock the dentry here, because readlink does the heavy lifting. */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,13)
static int unionfs_follow_link(struct dentry *dentry, struct nameidata *nd)
#else
static void *unionfs_follow_link(struct dentry *dentry, struct nameidata *nd)
#endif
{
	char *buf;
	int len = PAGE_SIZE, err;
	mm_segment_t old_fs;

	print_entry_location();

	/* This is freed by the put_link method assuming a successful call. */
	buf = KMALLOC(len, GFP_UNIONFS);
	if (!buf) {
		err = -ENOMEM;
		goto out;
	}

	/* read the symlink, and then we will follow it */
	old_fs = get_fs();
	set_fs(KERNEL_DS);
	err = dentry->d_inode->i_op->readlink(dentry, buf, len);
	set_fs(old_fs);
	if (err < 0) {
		KFREE(buf);
		buf = NULL;
		goto out;
	}
	buf[err] = 0;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
	nd_set_link(nd, buf);
	err = 0;
#else
	err = vfs_follow_link(nd, buf);
#endif

      out:
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
	KFREE(buf);
#endif
	print_exit_status(err);
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,13)
	return err;
#else
	return ERR_PTR(err);
#endif
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,13)
void unionfs_put_link(struct dentry *dentry, struct nameidata *nd)
#else
void unionfs_put_link(struct dentry *dentry, struct nameidata *nd, void *cookie)
#endif
{
	char *link;
	print_entry_location();
	link = nd_get_link(nd);
	KFREE(link);
	print_exit_location();
}
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
static int unionfs_permission(struct inode *inode, int mask,
			      struct nameidata *nd)
#else
static int unionfs_permission(struct inode *inode, int mask)
#endif
{
	struct inode *hidden_inode = NULL;
	int err = 0;
	int bindex, bstart, bend;
	int is_file = 0;

	print_entry_location();

	bstart = ibstart(inode);
	bend = ibend(inode);

	fist_print_inode("IN unionfs_permission", inode);

	/* set if check is for file */
	if (!S_ISDIR(inode->i_mode))
		is_file = 1;

	for (bindex = bstart; bindex <= bend; bindex++) {
		hidden_inode = itohi_index(inode, bindex);
		if (!hidden_inode)
			continue;

		/* check the condition for D-F-D underlying files/directories,
		 * we dont have to check for files, if we are checking for
		 * directories.
		 */
		if (!S_ISDIR(hidden_inode->i_mode) && (!is_file))
			continue;
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
		err = permission(hidden_inode, mask);
#else
		err = permission(hidden_inode, mask, nd);
#endif
		/* If the leftmost branch is r/o we can't copyup.
		 * Otherwise we ignore EROFS. */
		if (IS_COPYUP_ERR(err) && (bindex != 0))
			err = 0;
		/* The permissions are an intersection of the overall directory
		 * permissions, so we fail if one fails. */
		if (err)
			goto out;
		/* only the leftmost file matters. */
		if (is_file)
			break;
	}

      out:
	print_exit_status(err);
	return err;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
static int unionfs_inode_revalidate(struct dentry *dentry)
{
	int err = 0;
	struct dentry *hidden_dentry;
	struct inode *hidden_inode;
	struct inode *inode;
	int bindex, bstart, bend;
	int sbgen, igen;
	mode_t savemode;
	int locked = 0;

	print_entry_location();
	PASSERT(dentry);
	lock_dentry(dentry);
	inode = dentry->d_inode;
	PASSERT(inode);

	/* Before anything else, we should check if the generation number is valid. */
      restart:
	PASSERT(inode->i_sb);
	PASSERT(itopd(inode));
	sbgen = atomic_read(&stopd(inode->i_sb)->usi_generation);
	igen = atomic_read(&itopd(inode)->uii_generation);
	if (sbgen != igen) {
		struct dentry *result;

		lock_super(inode->i_sb);
		locked = 1;

		savemode = inode->i_mode;

		/* The root entry should always be valid */
		fist_dprint(8, "revalidate: sbgen = %d, igen = %d\n", sbgen,
			    igen);

		ASSERT(!IS_ROOT(dentry));
		PASSERT(dentry->d_parent);
		PASSERT(dentry->d_parent->d_inode);

		/* We can't work correctly if our parent isn't valid. */
		if (atomic_read(&stopd(inode->i_sb)->usi_generation) !=
		    atomic_read(&dtopd(dentry->d_parent)->udi_generation)) {
			PASSERT(dentry->d_parent->d_inode);
			PASSERT(dentry->d_parent->d_inode->i_op);
			PASSERT(dentry->d_parent->d_inode->i_op->revalidate);
			unlock_super(inode->i_sb);

			err =
			    dentry->d_parent->d_inode->i_op->revalidate(dentry->
									d_parent);
			if (err) {
				goto out;
			}

			goto restart;
		}

		/* Free the pointers for our inodes and this dentry. */
		bstart = dbstart(dentry);
		bend = dbend(dentry);
		if (bstart >= 0) {
			struct dentry *hidden_dentry;
			for (bindex = bstart; bindex <= bend; bindex++) {
				hidden_dentry = dtohd_index(dentry, bindex);
				if (!hidden_dentry)
					continue;
				DPUT(hidden_dentry);
			}
		}
		set_dbstart(dentry, -1);
		set_dbend(dentry, -1);

		bstart = ibstart(inode);
		bend = ibend(inode);
		if (bstart >= 0) {
			struct inode *hidden_inode;
			for (bindex = bstart; bindex <= bend; bindex++) {
				hidden_inode = itohi_index(inode, bindex);
				if (!hidden_inode)
					continue;
				iput(hidden_inode);
			}
		}
		KFREE(itohi_ptr(dentry->d_inode));
		itohi_ptr(dentry->d_inode) = NULL;
		ibstart(dentry->d_inode) = -1;
		ibend(dentry->d_inode) = -1;

		result = unionfs_lookup_backend(dentry, INTERPOSE_REVAL);
		if (result && IS_ERR(result)) {
			err = PTR_ERR(result);
			goto out;
		}

		if (itopd(dentry->d_inode)->uii_stale) {
			make_stale_inode(dentry->d_inode);
			d_drop(dentry);
			err = -ESTALE;
			goto out;
		}
	}

	bstart = dbstart(dentry);
	bend = dbend(dentry);
	for (bindex = bstart; bindex <= bend; bindex++) {
		hidden_dentry = dtohd_index(dentry, bindex);
		if (!hidden_dentry) {
			continue;
		}

		hidden_inode = hidden_dentry->d_inode;
		PASSERT(hidden_inode);

		/* Now revalidate the lower level. */
		if (hidden_inode->i_op && hidden_inode->i_op->revalidate) {
			err = hidden_inode->i_op->revalidate(hidden_dentry);
		}
	}

	hidden_inode = NULL;
	for (bindex = bstart; bindex <= bend && !hidden_inode; bindex++) {
		hidden_inode = itohi_index(inode, bindex);
	}

	if (hidden_inode && !err) {
		fist_copy_attr_all(inode, hidden_inode);
	}

      out:
	if (locked) {
		unlock_super(inode->i_sb);
	}
	unlock_dentry(dentry);
	print_exit_status(err);
	return err;
}
#endif				/* End of revalidate */

static int unionfs_setattr(struct dentry *dentry, struct iattr *ia)
{
	int err = 0;
	struct dentry *hidden_dentry;
	struct inode *inode = NULL;
	struct inode *hidden_inode = NULL;
	int bstart, bend, bindex;
	int i;
	int copyup = 0;
	int locked = 0;

	print_entry_location();
	lock_dentry(dentry);
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
	if (!(ia->ia_valid & ATTR_SIZE)) {
		locked = 1;
		PASSERT(dentry->d_inode);
		down(&dentry->d_inode->i_sem);
	}
#endif
	bstart = dbstart(dentry);
	bend = dbend(dentry);
	inode = dentry->d_inode;

	for (bindex = bstart; (bindex <= bend) || (bindex == bstart); bindex++) {
		hidden_dentry = dtohd_index(dentry, bindex);
		if (!hidden_dentry)
			continue;
		ASSERT(hidden_dentry->d_inode != NULL);

		/* If the file is on a read only branch */
		if (is_robranch_super(dentry->d_sb, bindex)
		    || IS_RDONLY(hidden_dentry->d_inode)) {
			if (copyup || (bindex != bstart))
				continue;
			/* Only if its the leftmost file, copyup the file */
			for (i = bstart - 1; i >= 0; i--) {
				size_t size = dentry->d_inode->i_size;
				if (ia->ia_valid & ATTR_SIZE)
					size = ia->ia_size;
				err = copyup_dentry(dentry->d_parent->d_inode,
						    dentry, bstart, i, NULL,
						    size);

				if (!err) {
					copyup = 1;
					hidden_dentry = dtohd(dentry);
					break;
				}
				/* if error is in the leftmost f/s, pass it up */
				if (i == 0)
					goto out;
			}

		}
		err = notify_change(hidden_dentry, ia);
		if (err)
			goto out;
		break;
	}

	/* get the size from the first hidden inode */
	hidden_inode = itohi(dentry->d_inode);
	fist_checkinode(inode, "unionfs_setattr");
	fist_copy_attr_all(inode, hidden_inode);

      out:
	if (locked)
		up(&dentry->d_inode->i_sem);
	unlock_dentry(dentry);
	fist_checkinode(inode, "post unionfs_setattr");
	print_exit_status(err);
	return err;
}

struct inode_operations unionfs_symlink_iops = {
	.readlink = unionfs_readlink,
	.permission = unionfs_permission,
	.follow_link = unionfs_follow_link,
	.setattr = unionfs_setattr,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
	.put_link = unionfs_put_link,
#else
	.revalidate = unionfs_inode_revalidate,
#endif
};

struct inode_operations unionfs_dir_iops = {
	.create = unionfs_create,
	.lookup = unionfs_lookup,
	.link = unionfs_link,
	.unlink = unionfs_unlink,
	.symlink = unionfs_symlink,
	.mkdir = unionfs_mkdir,
	.rmdir = unionfs_rmdir,
	.mknod = unionfs_mknod,
	.rename = unionfs_rename,
	.permission = unionfs_permission,
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
	.revalidate = unionfs_inode_revalidate,
#endif
	.setattr = unionfs_setattr,
// If you have problems with these lines, try defining FIST_SETXATTR_CONSTVOID
# if defined(UNIONFS_XATTR) && LINUX_VERSION_CODE >= KERNEL_VERSION(2,4,20)
	.setxattr = unionfs_setxattr,
	.getxattr = unionfs_getxattr,
	.removexattr = unionfs_removexattr,
	.listxattr = unionfs_listxattr,
# endif
};

struct inode_operations unionfs_main_iops = {
	.permission = unionfs_permission,
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
	.revalidate = unionfs_inode_revalidate,
#endif
	.setattr = unionfs_setattr,
// If you have problems with these lines, try defining FIST_SETXATTR_CONSTVOID
# if defined(UNIONFS_XATTR) && LINUX_VERSION_CODE >= KERNEL_VERSION(2,4,20)
	.setxattr = unionfs_setxattr,
	.getxattr = unionfs_getxattr,
	.removexattr = unionfs_removexattr,
	.listxattr = unionfs_listxattr,
# endif
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

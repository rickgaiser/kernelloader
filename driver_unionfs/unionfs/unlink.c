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

static int unionfs_unlink_all(struct inode *dir, struct dentry *dentry)
{
	struct dentry *hidden_dentry;
	struct dentry *hidden_dir_dentry;
	int bstart, bend, bindex;
	int err = 0;
	int global_err = 0;

	print_entry_location();

	bstart = dbstart(dentry);
	err = unionfs_partial_lookup(dentry);
	if (err) {
		fist_dprint(8, "Error in partial lookup\n");
		goto out;
	}

	bstart = dbstart(dentry);
	bend = dbend(dentry);

	for (bindex = bend; bindex >= bstart; bindex--) {
		hidden_dentry = dtohd_index(dentry, bindex);
		if (!hidden_dentry)
			continue;

		hidden_dir_dentry = lock_parent(hidden_dentry);

		/* avoid destroying the hidden inode if the file is in use */
		DGET(hidden_dentry);
		if (!(err = is_robranch_super(dentry->d_sb, bindex))) {
			err =
			    vfs_unlink(hidden_dir_dentry->d_inode,
				       hidden_dentry);
		}
		DPUT(hidden_dentry);
		fist_copy_attr_times(dir, hidden_dir_dentry->d_inode);
		if (err) {
			if (!IS_COPYUP_ERR(err)) {
				/* passup the last error we got */
				unlock_dir(hidden_dir_dentry);
				goto out;
			}
			global_err = err;
		}

		unlock_dir(hidden_dir_dentry);
	}

	/* check if encountered error in the above loop */
	if (global_err) {
		/* If we failed in the leftmost branch, then err will be set
		 * and we should move one over to create the whiteout.
		 * Otherwise, we should try in the leftmost branch. */
		if (err) {
			if (dbstart(dentry) == 0) {
				goto out;
			}
			err = create_whiteout(dentry, dbstart(dentry) - 1);
		} else {
			err = create_whiteout(dentry, dbstart(dentry));
		}
	} else if (dbopaque(dentry) != -1) {
		/* There is a hidden lower-priority file with the same name. */
		err = create_whiteout(dentry, dbopaque(dentry));
	}
      out:
	/* propagate number of hard-links */
	if (dentry->d_inode->i_nlink != 0) {
		dentry->d_inode->i_nlink = get_nlinks(dentry->d_inode);
		if (!err && global_err)
			dentry->d_inode->i_nlink--;
	}
	/* We don't want to leave negative leftover dentries for revalidate. */
	if (!err && (global_err || dbopaque(dentry) != -1))
		update_bstart(dentry);

	print_exit_status(err);
	return err;
}

static int unionfs_unlink_whiteout(struct inode *dir, struct dentry *dentry)
{
	int err = 0;
	struct dentry *hidden_old_dentry;
	struct dentry *hidden_wh_dentry = NULL;
	struct dentry *hidden_old_dir_dentry, *hidden_new_dir_dentry;
	char *name = NULL;
	struct iattr newattrs;

	print_entry_location();
	fist_print_dentry("IN unionfs_unlink_whiteout: ", dentry);

	/* create whiteout, get the leftmost underlying dentry and rename it */
	hidden_old_dentry = dtohd(dentry);

	/* lookup .wh.foo first, MUST NOT EXIST */
	name = KMALLOC(dentry->d_name.len + sizeof(".wh."), GFP_UNIONFS);
	if (!name) {
		err = -ENOMEM;
		goto out;
	}
	strcpy(name, ".wh.");
	strncat(name, dentry->d_name.name, dentry->d_name.len);
	name[4 + dentry->d_name.len] = '\0';

	hidden_wh_dentry =
	    LOOKUP_ONE_LEN(name, hidden_old_dentry->d_parent,
			   dentry->d_name.len + 4);
	if (IS_ERR(hidden_wh_dentry)) {
		err = PTR_ERR(hidden_wh_dentry);
		goto out;
	}
	ASSERT(hidden_wh_dentry->d_inode == NULL);

	DGET(hidden_old_dentry);

	hidden_old_dir_dentry = GET_PARENT(hidden_old_dentry);
	hidden_new_dir_dentry = GET_PARENT(hidden_wh_dentry);
	double_lock(hidden_old_dir_dentry, hidden_new_dir_dentry);

	if (!(err = is_robranch(dentry))) {
		if (S_ISREG(hidden_old_dentry->d_inode->i_mode)) {
			err = vfs_rename(hidden_old_dir_dentry->d_inode,
					 hidden_old_dentry,
					 hidden_new_dir_dentry->d_inode,
					 hidden_wh_dentry);
		} else {
			err = vfs_unlink(hidden_old_dir_dentry->d_inode,
					 hidden_old_dentry);
			if (!err)
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
				err = vfs_create(hidden_new_dir_dentry->d_inode,
						 hidden_wh_dentry, 0666, NULL);
#else
				err = vfs_create(hidden_new_dir_dentry->d_inode,
						 hidden_wh_dentry, 0666);
#endif
		}
	}

	double_unlock(hidden_old_dir_dentry, hidden_new_dir_dentry);
	DPUT(hidden_old_dentry);
	DPUT(hidden_wh_dentry);

	if (!err) {
		hidden_wh_dentry = LOOKUP_ONE_LEN(name,
						  hidden_old_dentry->d_parent,
						  dentry->d_name.len + 4);
		if (IS_ERR(hidden_wh_dentry)) {
			err = PTR_ERR(hidden_wh_dentry);
			goto out;
		}
		PASSERT(hidden_wh_dentry->d_inode);
		down(&hidden_wh_dentry->d_inode->i_sem);
		newattrs.ia_valid = ATTR_CTIME;
		if (hidden_wh_dentry->d_inode->i_size != 0) {
			newattrs.ia_valid |= ATTR_SIZE;
			newattrs.ia_size = 0;
		}
		/* We discard this error, because the entry is whited out
		 * even if we fail here. */
		notify_change(hidden_wh_dentry, &newattrs);
		up(&hidden_wh_dentry->d_inode->i_sem);
		DPUT(hidden_wh_dentry);
	}

	if (err) {
		if (dbstart(dentry) == 0)
			goto out;
		/* exit if the error returned was NOT -EROFS */
		if (!IS_COPYUP_ERR(err))
			goto out;
		err = create_whiteout(dentry, dbstart(dentry) - 1);
	} else {
		fist_copy_attr_all(dir, hidden_new_dir_dentry->d_inode);
	}

      out:
	KFREE(name);
	print_exit_status(err);
	return err;
}

int unionfs_unlink(struct inode *dir, struct dentry *dentry)
{
	int err = 0;

	print_entry_location();
	lock_dentry(dentry);
	fist_print_dentry("IN unionfs_unlink", dentry);

	if (IS_SET(dir->i_sb, DELETE_WHITEOUT))
		err = unionfs_unlink_whiteout(dir, dentry);
	else
		err = unionfs_unlink_all(dir, dentry);

	/* call d_drop so the system "forgets" about us */
	if (!err)
		d_drop(dentry);

	unlock_dentry(dentry);
	print_exit_status(err);
	return err;
}

static int unionfs_rmdir_first(struct inode *dir, struct dentry *dentry,
			       struct unionfs_dir_state *namelist)
{
	int err;
	struct dentry *hidden_dentry;
	struct dentry *hidden_dir_dentry = NULL;

	print_entry_location();
	fist_print_dentry("IN unionfs_rmdir_first: ", dentry);

	/* Here we need to remove whiteout entries. */
	err = delete_whiteouts(dentry, dbstart(dentry), namelist);
	if (err) {
		goto out;
	}

	hidden_dentry = dtohd(dentry);
	PASSERT(hidden_dentry);

	hidden_dir_dentry = lock_parent(hidden_dentry);

	/* avoid destroying the hidden inode if the file is in use */
	DGET(hidden_dentry);
	if (!(err = is_robranch(dentry))) {
		err = vfs_rmdir(hidden_dir_dentry->d_inode, hidden_dentry);
	}
	DPUT(hidden_dentry);

	fist_copy_attr_times(dir, hidden_dir_dentry->d_inode);
	/* propagate number of hard-links */
	dentry->d_inode->i_nlink = get_nlinks(dentry->d_inode);

      out:
	if (hidden_dir_dentry) {
		unlock_dir(hidden_dir_dentry);
	}
	fist_print_dentry("OUT unionfs_rmdir_first: ", dentry);
	print_exit_status(err);
	return err;
}

static int unionfs_rmdir_all(struct inode *dir, struct dentry *dentry,
			     struct unionfs_dir_state *namelist)
{
	struct dentry *hidden_dentry;
	struct dentry *hidden_dir_dentry;
	int bstart, bend, bindex;
	int err = 0;
	int global_err = 0;

	print_entry_location();
	fist_print_dentry("IN unionfs_rmdir_all: ", dentry);

	err = unionfs_partial_lookup(dentry);
	if (err) {
		fist_dprint(8, "Error in partial lookup\n");
		goto out;
	}

	bstart = dbstart(dentry);
	bend = dbend(dentry);

	for (bindex = bend; bindex >= bstart; bindex--) {
		hidden_dentry = dtohd_index(dentry, bindex);
		if (!hidden_dentry)
			continue;

		hidden_dir_dentry = lock_parent(hidden_dentry);
		if (S_ISDIR(hidden_dentry->d_inode->i_mode)) {
			delete_whiteouts(dentry, bindex, namelist);
			if (!(err = is_robranch_super(dentry->d_sb, bindex))) {
				err =
				    vfs_rmdir(hidden_dir_dentry->d_inode,
					      hidden_dentry);
			}
		} else {
			err = -EISDIR;
		}

		fist_copy_attr_times(dir, hidden_dir_dentry->d_inode);
		unlock_dir(hidden_dir_dentry);
		if (err) {
			int local_err =
			    unionfs_refresh_hidden_dentry(dentry, bindex);
			if (local_err) {
				err = local_err;
				goto out;
			}

			if (!IS_COPYUP_ERR(err) && err != -ENOTEMPTY
			    && err != -EISDIR)
				goto out;

			global_err = err;
		}
	}

	/* check if encountered error in the above loop */
	if (global_err) {
		/* If we failed in the leftmost branch, then err will be set and we should
		 * move one over to create the whiteout.  Otherwise, we should try in the
		 * leftmost branch.
		 */
		if (err) {
			if (dbstart(dentry) == 0) {
				goto out;
			}
			err = create_whiteout(dentry, dbstart(dentry) - 1);
		} else {
			err = create_whiteout(dentry, dbstart(dentry));
		}
	}

      out:
	/* propagate number of hard-links */
	dentry->d_inode->i_nlink = get_nlinks(dentry->d_inode);

	fist_print_dentry("OUT unionfs_rmdir_all: ", dentry);
	print_exit_status(err);
	return err;
}

int unionfs_rmdir(struct inode *dir, struct dentry *dentry)
{
	int err = 0;
	struct unionfs_dir_state *namelist = NULL;

	print_entry_location();
	lock_dentry(dentry);
	fist_print_dentry("IN unionfs_rmdir: ", dentry);

	/* check if this unionfs directory is empty or not */
	err = check_empty(dentry, &namelist);
	if (err) {
		goto out;
	}

	if (IS_SET(dir->i_sb, DELETE_WHITEOUT)) {
		/* Delete the first directory. */
		err = unionfs_rmdir_first(dir, dentry, namelist);
		/* create whiteout */
		if (!err) {
			err = create_whiteout(dentry, dbstart(dentry));
		} else {
			int new_err;

			if (dbstart(dentry) == 0)
				goto out;

			/* exit if the error returned was NOT -EROFS */
			if (!IS_COPYUP_ERR(err))
				goto out;

			new_err = create_whiteout(dentry, dbstart(dentry) - 1);
			if (new_err != -EEXIST)
				err = new_err;
		}
	} else {
		/* delete all. */
		err = unionfs_rmdir_all(dir, dentry, namelist);
	}

      out:
	/* call d_drop so the system "forgets" about us */
	if (!err)
		d_drop(dentry);

	if (namelist)
		free_rdstate(namelist);

	unlock_dentry(dentry);
	print_exit_status(err);
	return err;
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

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

static int do_rename(struct inode *old_dir, struct dentry *old_dentry,
		     struct inode *new_dir, struct dentry *new_dentry,
		     int bindex)
{
	int err = 0;
	struct dentry *hidden_old_dentry;
	struct dentry *hidden_new_dentry;
	struct dentry *hidden_old_dir_dentry;
	struct dentry *hidden_new_dir_dentry;
	struct dentry *hidden_wh_dentry;
	struct dentry *hidden_wh_dir_dentry;
	char *wh_name = NULL;

	print_entry(" bindex=%d", bindex);

	fist_print_dentry("IN: do_rename, old_dentry", old_dentry);
	fist_print_dentry("IN: do_rename, new_dentry", new_dentry);
	fist_dprint(7, "do_rename for bindex = %d\n", bindex);

	hidden_new_dentry = dtohd_index(new_dentry, bindex);
	hidden_old_dentry = dtohd_index(old_dentry, bindex);
	PASSERT(hidden_old_dentry);

	if (!hidden_new_dentry) {
		hidden_new_dentry =
		    create_parents(new_dentry->d_parent->d_inode, new_dentry,
				   bindex);
		if (IS_ERR(hidden_new_dentry)) {
			fist_dprint(7,
				    "error creating directory tree for rename, bindex = $d\n");
			err = PTR_ERR(hidden_new_dentry);
			goto out;
		}
	}

	wh_name = KMALLOC(new_dentry->d_name.len + 5, GFP_UNIONFS);
	if (!wh_name) {
		err = -ENOMEM;
		goto out;
	}
	strcpy(wh_name, ".wh.");
	strncat(wh_name, new_dentry->d_name.name, new_dentry->d_name.len);
	wh_name[4 + new_dentry->d_name.len] = '\0';

	hidden_wh_dentry =
	    LOOKUP_ONE_LEN(wh_name, hidden_new_dentry->d_parent,
			   new_dentry->d_name.len + 4);
	if (IS_ERR(hidden_wh_dentry)) {
		err = PTR_ERR(hidden_wh_dentry);
		goto out;
	}

	if (hidden_wh_dentry->d_inode) {
		/* get rid of the whiteout that is existing */
		if (hidden_new_dentry->d_inode) {
			printk(KERN_WARNING
			       "Both a whiteout and a dentry exist when doing a rename!\n");
			err = -EIO;

			DPUT(hidden_wh_dentry);
			goto out;
		}

		hidden_wh_dir_dentry = lock_parent(hidden_wh_dentry);
		if (!(err = is_robranch_super(old_dentry->d_sb, bindex))) {
			err =
			    vfs_unlink(hidden_wh_dir_dentry->d_inode,
				       hidden_wh_dentry);
		}
		DPUT(hidden_wh_dentry);
		unlock_dir(hidden_wh_dir_dentry);
		if (err)
			goto out;
	} else {
		DPUT(hidden_wh_dentry);
	}

	DGET(hidden_old_dentry);
	hidden_old_dir_dentry = GET_PARENT(hidden_old_dentry);
	hidden_new_dir_dentry = GET_PARENT(hidden_new_dentry);

	double_lock(hidden_old_dir_dentry, hidden_new_dir_dentry);

	if (!(err = is_robranch_super(old_dentry->d_sb, bindex))) {
		PASSERT(hidden_old_dir_dentry->d_inode);
		PASSERT(hidden_old_dentry);
		PASSERT(hidden_old_dentry->d_inode);
		PASSERT(hidden_new_dir_dentry->d_inode);
		PASSERT(hidden_new_dentry);
		fist_print_dentry("NEWBEF", new_dentry);
		fist_print_dentry("OLDBEF", old_dentry);
		err =
		    vfs_rename(hidden_old_dir_dentry->d_inode,
			       hidden_old_dentry,
			       hidden_new_dir_dentry->d_inode,
			       hidden_new_dentry);
		fist_print_dentry("NEWAFT", new_dentry);
		fist_print_dentry("OLDAFT", old_dentry);
	}

	double_unlock(hidden_old_dir_dentry, hidden_new_dir_dentry);
	DPUT(hidden_old_dentry);

      out:
	if (!err) {
		/* Fixup the newdentry. */
		if (bindex < dbstart(new_dentry))
			set_dbstart(new_dentry, bindex);
		else if (bindex > dbend(new_dentry))
			set_dbend(new_dentry, bindex);
	}

	KFREE(wh_name);

	fist_print_dentry("OUT: do_rename, old_dentry", old_dentry);
	fist_print_dentry("OUT: do_rename, new_dentry", new_dentry);

	print_exit_status(err);
	return err;
}

static int unionfs_rename_whiteout(struct inode *old_dir,
				   struct dentry *old_dentry,
				   struct inode *new_dir,
				   struct dentry *new_dentry)
{
	int err = 0;
	int bindex;
	int old_bstart, old_bend;
	int new_bstart, new_bend;
	int do_copyup = -1;
	struct dentry *parent_dentry = NULL;
	int local_err = 0;
	int eio = 0;
	int revert = 0;

	print_entry_location();

	old_bstart = dbstart(old_dentry);
	old_bend = dbend(old_dentry);
	parent_dentry = old_dentry->d_parent;

	new_bstart = dbstart(new_dentry);
	new_bend = dbend(new_dentry);

	/* Rename source to destination. */
	err = do_rename(old_dir, old_dentry, new_dir, new_dentry, old_bstart);
	if (err) {
		if (!IS_COPYUP_ERR(err)) {
			goto out;
		}
		do_copyup = old_bstart - 1;
	} else {
		revert = 1;
	}

	/* Unlink all instances of destination that exist to the left of
	 * bstart of source. On error, revert back, goto out.
	 */
	for (bindex = old_bstart - 1; bindex >= new_bstart; bindex--) {
		struct dentry *unlink_dentry;
		struct dentry *unlink_dir_dentry;

		unlink_dentry = dtohd_index(new_dentry, bindex);
		if (!unlink_dentry) {
			continue;
		}

		unlink_dir_dentry = lock_parent(unlink_dentry);
		if (!(err = is_robranch_super(old_dir->i_sb, bindex))) {
			err =
			    vfs_unlink(unlink_dir_dentry->d_inode,
				       unlink_dentry);
		}

		fist_copy_attr_times(new_dentry->d_parent->d_inode,
				     unlink_dir_dentry->d_inode);
		/* propagate number of hard-links */
		new_dentry->d_parent->d_inode->i_nlink =
		    get_nlinks(new_dentry->d_parent->d_inode);

		unlock_dir(unlink_dir_dentry);
		if (!err) {
			if (bindex != new_bstart) {
				DPUT(unlink_dentry);
				set_dtohd_index(new_dentry, bindex, NULL);
			}
		} else if (IS_COPYUP_ERR(err)) {
			do_copyup = bindex - 1;
		} else if (revert) {
			goto revert;
		}
	}

	if (do_copyup != -1) {
		for (bindex = do_copyup; bindex >= 0; bindex--) {
			/* copyup the file into some left directory, so that you can rename it */
			err =
			    copyup_dentry(old_dentry->d_parent->d_inode,
					  old_dentry, old_bstart, bindex, NULL,
					  old_dentry->d_inode->i_size);
			if (!err) {
				parent_dentry = old_dentry->d_parent;
				err =
				    do_rename(old_dir, old_dentry, new_dir,
					      new_dentry, bindex);
			}
		}
	}

	/* Create whiteout for source, only if:
	 * (1) There is more than one underlying instance of source.
	 * (2) We did a copy_up
	 */
	if ((old_bstart != old_bend) || (do_copyup != -1)) {
		int start = (do_copyup == -1) ? old_bstart : do_copyup;
		/* we want to create a whiteout for name in  this parent dentry */
		local_err = create_whiteout(old_dentry, start);
		if (local_err) {
			/* We can't fix anything now, so we cop-out and use -EIO. */
			printk
			    ("<0>We can't create a whiteout for the source in rename!\n");
			err = -EIO;
			goto out;
		}
	}

      out:

	print_exit_status(err);
	return err;

      revert:
	/* Do revert here. */
	local_err = unionfs_refresh_hidden_dentry(new_dentry, old_bstart);
	if (local_err) {
		printk(KERN_WARNING
		       "Revert failed in rename: the new refresh failed.\n");
		eio = -EIO;
	}

	local_err = unionfs_refresh_hidden_dentry(old_dentry, old_bstart);
	if (local_err) {
		printk(KERN_WARNING
		       "Revert failed in rename: the old refresh failed.\n");
		eio = -EIO;
		goto revert_out;
	}

	if (!dtohd_index(new_dentry, bindex)
	    || !dtohd_index(new_dentry, bindex)->d_inode) {
		printk(KERN_WARNING
		       "Revert failed in rename: the object disappeared from under us!\n");
		eio = -EIO;
		goto revert_out;
	}

	if (dtohd_index(old_dentry, bindex)
	    && dtohd_index(old_dentry, bindex)->d_inode) {
		printk(KERN_WARNING
		       "Revert failed in rename: the object was created underneath us!\n");
		eio = -EIO;
		goto revert_out;
	}

	local_err =
	    do_rename(new_dir, new_dentry, old_dir, old_dentry, old_bstart);

	/* If we can't fix it, then we cop-out with -EIO. */
	if (local_err) {
		printk(KERN_WARNING "Revert failed in rename!\n");
		eio = -EIO;
	}

	local_err = unionfs_refresh_hidden_dentry(new_dentry, bindex);
	if (local_err)
		eio = -EIO;
	local_err = unionfs_refresh_hidden_dentry(old_dentry, bindex);
	if (local_err)
		eio = -EIO;

      revert_out:
	if (eio)
		err = eio;
	print_exit_status(err);
	return err;
}

/*
 * The function is nasty, nasty, nasty, but so is rename. :(
 *
 * This psuedo-code describes what the function should do.  Essentially we move
 * from right-to-left, renaming each item.  We skip the leftmost destination
 * (this is so we can always undo the rename if the reverts work), and then the
 * very last thing we do is fix up the leftmost destination (either through
 * renaming it or unlinking it).
 *
 * for i = S_R downto S_L
 *	if (i != L_D && exists(S[i]))
 *		err = rename(S[i], D[i])
 *		if (err == COPYUP)  {
 *			do_whiteout = i - 1;
 *			if (i == S_R) {
 *				do_copyup = i - 1
 *			}
 *		}
 *		else if (err)
 *			goto revert;
 *		else
 *			ok = i;
 *
 *
 * If we get to the leftmost source (S_L) and it is EROFS, we should do copyup
 *
 * if (err == COPYUP) {
 * 	do_copyup = i - 1;
 * }
 *
 * while (i > new_bstart) {
 * 	err = unlink(D[i]);
 *	if (err == COPYUP) {
 *		do_copyup = i - 1;
 *	} else if (err) {
 *		goto revert;
 *	}
 * }
 *
 * if (exists(S[L_D])) {
 *	err = rename(S[L_D], D[L_D]);
 *	if (err = COPYUP) {
 *		if (ok > L_D) {
 *			do_copyup = min(do_copyup, L_D - 1);
 *		}
 *		do_whiteout = min(do_whiteout, L_D - 1);
 *	} else {
 *		goto revert;
 *	}
 * } else {
 *	err = unlink(D[L_D]);
 *	if (err = COPYUP) {
 *		do_copyup = min(do_copyup, L_D - 1);
 *	} else {
 *		goto revert;
 *	}
 * }
 *
 * out:
 *	if (do_whiteout != -1) {
 *		create_whiteout(do_whiteout);
 *	}
 *	if (do_copyup != -1) {
 *		copyup(source to do_copyup)
 *		rename source to destination
 *	}
 *	return err;
 * out_revert:
 *	do the reverting;
 *	return err;
 * }
 */
static int unionfs_rename_all(struct inode *old_dir, struct dentry *old_dentry,
			      struct inode *new_dir, struct dentry *new_dentry)
{
	int old_bstart, old_bend;
	int new_bstart, new_bend;
	struct dentry *parent_dentry = NULL;
	int bindex;
	int err = 0;
	int eio = 0;		/* Used for revert. */
	int isdir;

	/* These variables control error handling. */
	int rename_ok = FD_SETSIZE;	/* The last rename that is ok. */
	int do_copyup = -1;	/* Where we should start copyup. */
	int do_whiteout = -1;	/* Where we should start whiteouts of the source. */
	int clobber;		/* Are we clobbering the destination. */
	fd_set success_mask;

	print_entry_location();

	old_bstart = dbstart(old_dentry);
	old_bend = dbend(old_dentry);
	parent_dentry = old_dentry->d_parent;
	new_bstart = dbstart(new_dentry);
	new_bend = dbend(new_dentry);
	ASSERT(new_bstart >= 0);
	ASSERT(old_bstart >= 0);

	/* The failure mask only can deal with FD_SETSIZE entries. */
	ASSERT(old_bend <= FD_SETSIZE);
	ASSERT(new_bend <= FD_SETSIZE);
	FD_ZERO(&success_mask);

	/* Life is simpler if the dentry doesn't exist. */
	clobber = (dtohd_index(new_dentry, new_bstart)->d_inode) ? 1 : 0;
	isdir = S_ISDIR(old_dentry->d_inode->i_mode);

	/* Loop through all the branches from right to left and rename all
	 * instances of old dentry to new dentry, except if they are
	 */
	for (bindex = old_bend; bindex >= old_bstart; bindex--) {
		/* We don't rename if there is no source. */
		if (dtohd_index(old_dentry, bindex) == NULL)
			continue;

		/* we rename the bstart of destination only at the last of
		 * all operations, so that we don't lose it on error
		 */
		if (clobber && (bindex == new_bstart))
			continue;

		/* We shouldn't have a handle on this if there is no inode. */
		PASSERT(dtohd_index(old_dentry, bindex)->d_inode);

		err =
		    do_rename(old_dir, old_dentry, new_dir, new_dentry, bindex);
		if (!err) {
			/* For reverting. */
			FD_SET(bindex, &success_mask);
			/* So we know not to copyup on failures the right */
			rename_ok = bindex;
		} else if (IS_COPYUP_ERR(err)) {
			if (isdir) {
				err = -EXDEV;
				goto revert;
			}
			do_whiteout = bindex - 1;
			if (bindex == old_bstart)
				do_copyup = bindex - 1;
		} else {
			goto revert;
		}
	}

	while (bindex > new_bstart) {
		struct dentry *unlink_dentry;
		struct dentry *unlink_dir_dentry;

		unlink_dentry = dtohd_index(new_dentry, bindex);
		if (!unlink_dentry) {
			bindex--;
			continue;
		}

		unlink_dir_dentry = lock_parent(unlink_dentry);
		if (!(err = is_robranch_super(old_dir->i_sb, bindex)))
			err = vfs_unlink(unlink_dir_dentry->d_inode,
					 unlink_dentry);

		fist_copy_attr_times(new_dentry->d_parent->d_inode,
				     unlink_dir_dentry->d_inode);
		new_dentry->d_parent->d_inode->i_nlink =
		    get_nlinks(new_dentry->d_parent->d_inode);

		unlock_dir(unlink_dir_dentry);

		if (!err) {
			if (bindex != new_bstart) {
				DPUT(unlink_dentry);
				set_dtohd_index(new_dentry, bindex, NULL);
			}
		}

		if (IS_COPYUP_ERR(err)) {
			if (isdir) {
				err = -EXDEV;
				goto revert;
			}
			do_copyup = bindex - 1;
		} else if (err) {
			goto revert;
		}

		bindex--;
	}			// while bindex

	/* Now we need to handle the leftmost of the destination. */
	if (clobber && dtohd_index(old_dentry, new_bstart)) {
		err =
		    do_rename(old_dir, old_dentry, new_dir, new_dentry,
			      new_bstart);
		if (IS_COPYUP_ERR(err)) {
			if (isdir) {
				err = -EXDEV;
				goto revert;
			}
			if (rename_ok > new_bstart) {
				if ((do_copyup == -1)
				    || (new_bstart - 1 < do_copyup))
					do_copyup = new_bstart - 1;
			}
			if ((do_whiteout == -1)
			    || (new_bstart - 1 < do_whiteout))
				do_whiteout = new_bstart - 1;
		} else if (err) {
			goto revert;
		}
	} else if (clobber && (new_bstart < old_bstart)) {
		struct dentry *unlink_dentry;
		struct dentry *unlink_dir_dentry;

		unlink_dentry = dtohd_index(new_dentry, new_bstart);
		PASSERT(unlink_dentry);
		PASSERT(unlink_dentry->d_inode);

		unlink_dir_dentry = lock_parent(unlink_dentry);
		if (!(err = is_robranch_super(old_dir->i_sb, new_bstart)))
			err = vfs_unlink(unlink_dir_dentry->d_inode,
					 unlink_dentry);

		fist_copy_attr_times(new_dentry->d_parent->d_inode,
				     unlink_dir_dentry->d_inode);
		new_dentry->d_parent->d_inode->i_nlink =
		    get_nlinks(new_dentry->d_parent->d_inode);

		unlock_dir(unlink_dir_dentry);

		if (IS_COPYUP_ERR(err)) {
			if (isdir) {
				err = -EXDEV;
				goto revert;
			}
			if ((do_copyup == -1) || (new_bstart - 1 < do_copyup))
				do_copyup = new_bstart - 1;
		} else if (err) {
			goto revert;
		}
	}

	/* Create a whiteout for the source. */
	if (do_whiteout != -1) {
		ASSERT(do_whiteout >= 0);
		err = create_whiteout(old_dentry, do_whiteout);
		if (err) {
			/* We can't fix anything now, so we -EIO. */
			printk(KERN_WARNING "We can't create a whiteout for the"
			       "source in rename!\n");
			err = -EIO;
			goto out;
		}
	}

	if (do_copyup != -1) {
		/* We can't copyup a directory, because it may involve huge
		 * numbers of children, etc.  Doing that in the kernel would
		 * be ungood.
		 */

		if (S_ISDIR(old_dentry->d_inode->i_mode)) {
			err = -EXDEV;
			goto out;
		}

		for (bindex = do_copyup; bindex >= 0; bindex--) {
			err =
			    copyup_dentry(old_dentry->d_parent->d_inode,
					  old_dentry, old_bstart, bindex, NULL,
					  old_dentry->d_inode->i_size);
			if (!err)
				err =
				    do_rename(old_dir, old_dentry, new_dir,
					      new_dentry, bindex);
		}
	}

	/* We are at the point where reverting doesn't happen. */
	goto out;

      revert:
	for (bindex = old_bstart; bindex <= old_bend; bindex++) {
		int local_err;

		if (FD_ISSET(bindex, &success_mask)) {
			local_err =
			    unionfs_refresh_hidden_dentry(new_dentry, bindex);
			if (local_err) {
				printk(KERN_WARNING "Revert failed in rename: "
				       "the new refresh failed.\n");
				eio = -EIO;
			}

			local_err =
			    unionfs_refresh_hidden_dentry(old_dentry, bindex);
			if (local_err) {
				printk(KERN_WARNING "Revert failed in rename: "
				       "the old refresh failed.\n");
				eio = -EIO;
				continue;
			}

			if (!dtohd_index(new_dentry, bindex)
			    || !dtohd_index(new_dentry, bindex)->d_inode) {
				printk(KERN_WARNING "Revert failed in rename: "
				       "the object disappeared from under us!\n");
				eio = -EIO;
				continue;
			}

			if (dtohd_index(old_dentry, bindex)
			    && dtohd_index(old_dentry, bindex)->d_inode) {
				printk(KERN_WARNING "Revert failed in rename: "
				       "the object was created underneath us!\n");
				eio = -EIO;
				continue;
			}

			local_err =
			    do_rename(new_dir, new_dentry, old_dir, old_dentry,
				      bindex);

			/* If we can't fix it, then we cop-out with -EIO. */
			if (local_err) {
				printk(KERN_WARNING
				       "Revert failed in rename!\n");
				eio = -EIO;
			}
			local_err =
			    unionfs_refresh_hidden_dentry(new_dentry, bindex);
			if (local_err)
				eio = -EIO;
			local_err =
			    unionfs_refresh_hidden_dentry(old_dentry, bindex);
			if (local_err)
				eio = -EIO;
		}
	}
	if (eio)
		err = eio;

      out:
	print_exit_status(err);
	return err;
}

int unionfs_rename(struct inode *old_dir, struct dentry *old_dentry,
		   struct inode *new_dir, struct dentry *new_dentry)
{
	int err = 0;
	struct dentry *hidden_old_dentry;
	struct dentry *hidden_new_dentry;

	print_entry_location();

	double_lock_dentry(old_dentry, new_dentry);

	fist_checkinode(old_dir, "unionfs_rename-old_dir");
	fist_checkinode(new_dir, "unionfs_rename-new_dir");
	fist_print_dentry("IN: unionfs_rename, old_dentry", old_dentry);
	fist_print_dentry("IN: unionfs_rename, new_dentry", new_dentry);

	err = unionfs_partial_lookup(old_dentry);
	if (err)
		goto out;
	err = unionfs_partial_lookup(new_dentry);
	if (err)
		goto out;

	hidden_new_dentry = dtohd(new_dentry);
	hidden_old_dentry = dtohd(old_dentry);

	if (new_dentry->d_inode) {
		if (S_ISDIR(old_dentry->d_inode->i_mode) !=
		    S_ISDIR(new_dentry->d_inode->i_mode)) {
			err =
			    S_ISDIR(old_dentry->d_inode->
				    i_mode) ? -ENOTDIR : -EISDIR;
			goto out;
		}

		if (S_ISDIR(old_dentry->d_inode->i_mode)) {
			/* check if this unionfs directory is empty or not */
			err = check_empty(new_dentry, NULL);
			if (err)
				goto out;
			/* Handle the case where we are overwriting directories
			 * that are not really empty because of whiteout or
			 * non-whiteout entries.
			 */
		}
	}

	if (IS_SET(old_dir->i_sb, DELETE_WHITEOUT)) {
		/* create whiteout */
		err = unionfs_rename_whiteout(old_dir, old_dentry, new_dir,
					      new_dentry);
	} else {
		/* delete all. */
		err = unionfs_rename_all(old_dir, old_dentry, new_dir,
					 new_dentry);
	}

      out:
	fist_checkinode(new_dir, "post unionfs_rename-new_dir");
	fist_print_dentry("OUT: unionfs_rename, old_dentry", old_dentry);
	fist_print_dentry("OUT: unionfs_rename, new_dentry", new_dentry);
	unlock_dentry(new_dentry);
	unlock_dentry(old_dentry);
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

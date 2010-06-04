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
/*Not Working Yet*/
static int copyup_xattrs(struct dentry *old_hidden_dentry,
			 struct dentry *new_hidden_dentry)
{
	int err = 0;
	ssize_t list_size = -1;
	char *name_list = NULL;
	char *attr_value = NULL;
	char *name_list_orig = NULL;

	print_entry_location();

	PASSERT(old_hidden_dentry);
	PASSERT(old_hidden_dentry->d_inode);
	PASSERT(old_hidden_dentry->d_inode->i_op);
	PASSERT(new_hidden_dentry);
	PASSERT(new_hidden_dentry->d_inode);
	PASSERT(new_hidden_dentry->d_inode->i_op);

	if (!old_hidden_dentry->d_inode->i_op->getxattr ||
	    !old_hidden_dentry->d_inode->i_op->listxattr ||
	    !new_hidden_dentry->d_inode->i_op->setxattr) {
		err = -ENOTSUPP;
		goto out;
	}

	list_size =
	    old_hidden_dentry->d_inode->i_op->listxattr(old_hidden_dentry, NULL,
							0);
	if (list_size <= 0) {
		err = list_size;
		goto out;
	}

	name_list = xattr_alloc(list_size + 1, XATTR_LIST_MAX);
	if (!name_list || IS_ERR(name_list)) {
		err = PTR_ERR(name_list);
		goto out;
	}
	list_size =
	    old_hidden_dentry->d_inode->i_op->listxattr(old_hidden_dentry,
							name_list, list_size);
	attr_value = xattr_alloc(XATTR_SIZE_MAX, XATTR_SIZE_MAX);
	if (!attr_value || IS_ERR(attr_value)) {
		err = PTR_ERR(name_list);
		goto out;
	}
	name_list_orig = name_list;
	while (*name_list) {
		ssize_t size;
		down(&old_hidden_dentry->d_inode->i_sem);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
		err = security_inode_getxattr(old_hidden_dentry, name_list);
		if (err)
			size = err;
		else
#endif
			size =
			    old_hidden_dentry->d_inode->i_op->
			    getxattr(old_hidden_dentry, name_list, attr_value,
				     XATTR_SIZE_MAX);
		up(&old_hidden_dentry->d_inode->i_sem);
		if (size < 0) {
			err = size;
			goto out;
		}

		if (size > XATTR_SIZE_MAX) {
			err = -E2BIG;
			goto out;
		}

		down(&new_hidden_dentry->d_inode->i_sem);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
		err =
		    security_inode_setxattr(old_hidden_dentry, name_list,
					    attr_value, size, 0);

		if (!err) {
#endif
			err =
			    new_hidden_dentry->d_inode->i_op->
			    setxattr(new_hidden_dentry, name_list, attr_value,
				     size, 0);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
			if (!err)
				security_inode_post_setxattr(old_hidden_dentry,
							     name_list,
							     attr_value, size,
							     0);
		}
#endif
		up(&new_hidden_dentry->d_inode->i_sem);

		if (err < 0)
			goto out;
		name_list += strlen(name_list) + 1;
	}
      out:
	name_list = name_list_orig;

	if (name_list)
		xattr_free(name_list, list_size + 1);
	if (attr_value)
		xattr_free(attr_value, XATTR_SIZE_MAX);
	/* It is no big deal if this fails, we just roll with the punches. */
	if (err == -ENOTSUPP)
		err = 0;
	return err;
}
#endif

/* Determine the mode based on the copyup flags, and the existing dentry. */
static int copyup_permissions(struct super_block *sb,
			      struct dentry *old_hidden_dentry,
			      struct dentry *new_hidden_dentry)
{
	struct iattr newattrs;
	int err;

	print_entry_location();

	newattrs.ia_valid = ATTR_CTIME;
	if (IS_SET(sb, COPYUP_FS_MOUNTER)) {
		/* f/s mounter */
		newattrs.ia_mode = stopd(sb)->copyupmode;
		newattrs.ia_valid |= ATTR_FORCE | ATTR_MODE;
		if (stopd(sb)->copyupuid != -1) {
			newattrs.ia_uid = stopd(sb)->copyupuid;
			newattrs.ia_valid |= ATTR_UID;
		}
		if (stopd(sb)->copyupgid != -1) {
			newattrs.ia_gid = stopd(sb)->copyupgid;
			newattrs.ia_valid |= ATTR_GID;
		}
	} else if (IS_SET(sb, COPYUP_CURRENT_USER)) {
		/* current file permission */
		newattrs.ia_mode = ~current->fs->umask & S_IRWXUGO;
		newattrs.ia_valid |= ATTR_FORCE | ATTR_MODE;
	} else {
		/* original mode of old file */
		newattrs.ia_mode = old_hidden_dentry->d_inode->i_mode;
		newattrs.ia_gid = old_hidden_dentry->d_inode->i_gid;
		newattrs.ia_uid = old_hidden_dentry->d_inode->i_uid;
		newattrs.ia_valid |=
		    ATTR_FORCE | ATTR_GID | ATTR_UID | ATTR_MODE;
	}
	if (newattrs.ia_valid & ATTR_MODE) {
		newattrs.ia_mode =
		    (newattrs.ia_mode & S_IALLUGO) | (old_hidden_dentry->
						      d_inode->
						      i_mode & ~S_IALLUGO);
	}

	err = notify_change(new_hidden_dentry, &newattrs);

	print_exit_status(err);
	return err;
}

int copyup_dentry(struct inode *dir, struct dentry *dentry,
		  int bstart, int new_bindex,
		  struct file **copyup_file, int len)
{
	return copyup_named_dentry(dir, dentry, bstart, new_bindex,
				   (char *)dentry->d_name.name,
				   dentry->d_name.len, copyup_file, len);
}

int copyup_named_dentry(struct inode *dir, struct dentry *dentry,
			int bstart, int new_bindex, char *name,
			int namelen, struct file **copyup_file, int len)
{
	struct dentry *new_hidden_dentry;
	struct dentry *old_hidden_dentry = NULL;
	struct super_block *sb;
	struct file *input_file = NULL;
	struct file *output_file = NULL;
	ssize_t read_bytes, write_bytes;
	mm_segment_t old_fs;
	int err = 0;
	char *buf;
	int old_bindex;
	int got_branch_input = -1;
	int got_branch_output = -1;
	int old_bstart;
	int old_bend;
	int size = len;
	struct dentry *new_hidden_parent_dentry;
	mm_segment_t oldfs;
	char *symbuf = NULL;
	uid_t saved_uid = current->fsuid;
	gid_t saved_gid = current->fsgid;

	print_entry_location();
	verify_locked(dentry);
	fist_print_dentry("IN: copyup_named_dentry", dentry);

	old_bindex = bstart;
	old_bstart = dbstart(dentry);
	old_bend = dbend(dentry);

	ASSERT(new_bindex >= 0);
	ASSERT(new_bindex < old_bindex);
	PASSERT(dir);
	PASSERT(dentry);

	sb = dir->i_sb;

	if ((err = is_robranch_super(sb, new_bindex)))
		goto out;

	/* Create the directory structure above this dentry. */
	new_hidden_dentry = create_parents_named(dir, dentry, name, new_bindex);
	PASSERT(new_hidden_dentry);
	if (IS_ERR(new_hidden_dentry)) {
		err = PTR_ERR(new_hidden_dentry);
		goto out;
	}

	fist_print_generic_dentry("Copyup Object", new_hidden_dentry);

	/* Now we actually create the object. */
	old_hidden_dentry = dtohd_index(dentry, old_bindex);
	PASSERT(old_hidden_dentry);
	PASSERT(old_hidden_dentry->d_inode);
	DGET(old_hidden_dentry);

	/* For symlinks, we must read the link before we lock the directory. */
	if (S_ISLNK(old_hidden_dentry->d_inode->i_mode)) {
		PASSERT(old_hidden_dentry->d_inode->i_op);
		PASSERT(old_hidden_dentry->d_inode->i_op->readlink);

		symbuf = KMALLOC(PATH_MAX, GFP_UNIONFS);
		if (!symbuf) {
			err = -ENOMEM;
			goto copyup_readlink_err;
		}

		oldfs = get_fs();
		set_fs(KERNEL_DS);
		err =
		    old_hidden_dentry->d_inode->i_op->
		    readlink(old_hidden_dentry, symbuf, PATH_MAX);
		set_fs(oldfs);
		if (err < 0)
			goto copyup_readlink_err;
		symbuf[err] = '\0';
	}

	/* Now we lock the parent, and create the object in the new branch. */
	new_hidden_parent_dentry = lock_parent(new_hidden_dentry);
	current->fsuid = new_hidden_parent_dentry->d_inode->i_uid;
	current->fsgid = new_hidden_parent_dentry->d_inode->i_gid;
	if (S_ISDIR(old_hidden_dentry->d_inode->i_mode)) {
		err = vfs_mkdir(new_hidden_parent_dentry->d_inode,
				new_hidden_dentry, S_IRWXU);
	} else if (S_ISLNK(old_hidden_dentry->d_inode->i_mode)) {
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
		err = vfs_symlink(new_hidden_parent_dentry->d_inode,
				  new_hidden_dentry, symbuf);
#else
		err = vfs_symlink(new_hidden_parent_dentry->d_inode,
				  new_hidden_dentry, symbuf, S_IRWXU);
#endif
	} else if (S_ISBLK(old_hidden_dentry->d_inode->i_mode)
		   || S_ISCHR(old_hidden_dentry->d_inode->i_mode)
		   || S_ISFIFO(old_hidden_dentry->d_inode->i_mode)
		   || S_ISSOCK(old_hidden_dentry->d_inode->i_mode)) {
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
		err = vfs_mknod(new_hidden_parent_dentry->d_inode,
				new_hidden_dentry,
				old_hidden_dentry->d_inode->i_mode,
				kdev_t_to_nr(old_hidden_dentry->d_inode->
					     i_rdev));
#else
		err = vfs_mknod(new_hidden_parent_dentry->d_inode,
				new_hidden_dentry,
				old_hidden_dentry->d_inode->i_mode,
				old_hidden_dentry->d_inode->i_rdev);
#endif
	} else if (S_ISREG(old_hidden_dentry->d_inode->i_mode)) {
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
		err = vfs_create(new_hidden_parent_dentry->d_inode,
				 new_hidden_dentry, S_IRWXU);
#else
		err = vfs_create(new_hidden_parent_dentry->d_inode,
				 new_hidden_dentry, S_IRWXU, NULL);
#endif
	} else {
		char diemsg[100];
		snprintf(diemsg, sizeof(diemsg), "Unknown inode type %d\n",
			 old_hidden_dentry->d_inode->i_mode);
		FISTBUG(diemsg);
	}
	current->fsuid = saved_uid;
	current->fsgid = saved_gid;
	unlock_dir(new_hidden_parent_dentry);
      copyup_readlink_err:
	KFREE(symbuf);
	if (err) {
		/* get rid of the hidden dentry and all its traces */
		DPUT(new_hidden_dentry);
		set_dtohd_index(dentry, new_bindex, NULL);
		set_dbstart(dentry, old_bstart);
		set_dbend(dentry, old_bend);
		goto out;
	}

	/* We actually copyup the file here. */
	if (S_ISREG(old_hidden_dentry->d_inode->i_mode)) {
		mntget(stohiddenmnt_index(sb, old_bindex));
		branchget(sb, old_bindex);
		got_branch_input = old_bindex;
		input_file =
		    DENTRY_OPEN(old_hidden_dentry,
				stohiddenmnt_index(sb, old_bindex), O_RDONLY);
		if (IS_ERR(input_file)) {
			err = PTR_ERR(input_file);
			goto out;
		}
		if (!input_file->f_op || !input_file->f_op->read) {
			err = -EINVAL;
			goto out;
		}

		/* copy the new file */
		DGET(new_hidden_dentry);
		mntget(stohiddenmnt_index(sb, new_bindex));
		branchget(sb, new_bindex);
		got_branch_output = new_bindex;
		output_file =
		    DENTRY_OPEN(new_hidden_dentry,
				stohiddenmnt_index(sb, new_bindex), O_WRONLY);
		if (IS_ERR(output_file)) {
			err = PTR_ERR(output_file);
			goto out;
		}
		if (!output_file->f_op || !output_file->f_op->write) {
			err = -EINVAL;
			goto out;
		}

		/* allocating a buffer */
		buf = (char *)KMALLOC(PAGE_SIZE, GFP_UNIONFS);
		if (!buf) {
			err = -ENOMEM;
			goto out;
		}

		/* now read PAGE_SIZE bytes from offset 0 in a loop */
		old_fs = get_fs();

		input_file->f_pos = 0;
		output_file->f_pos = 0;

		set_fs(KERNEL_DS);
		do {
			if (len >= PAGE_SIZE)
				size = PAGE_SIZE;
			else if ((len < PAGE_SIZE) && (len > 0))
				size = len;

			len -= PAGE_SIZE;

			read_bytes =
			    input_file->f_op->read(input_file, buf, size,
						   &input_file->f_pos);
			if (read_bytes <= 0) {
				err = read_bytes;
				break;
			}

			write_bytes =
			    output_file->f_op->write(output_file, buf,
						     read_bytes,
						     &output_file->f_pos);
			if (write_bytes < 0 || (write_bytes < read_bytes)) {
				err = -EIO;
				break;
			}
		} while ((read_bytes > 0) && (len > 0));
		set_fs(old_fs);
		KFREE(buf);
	}

	/* Set permissions. */
	if ((err =
	     copyup_permissions(sb, old_hidden_dentry, new_hidden_dentry)))
		goto out;
	/* Selinux uses extended attributes for permissions. */
#if defined(UNIONFS_XATTR) && (LINUX_VERSION_CODE >= KERNEL_VERSION(2,4,20))
	if ((err = copyup_xattrs(old_hidden_dentry, new_hidden_dentry)))
		goto out;
#endif

	/* do not allow files getting deleted to be reinterposed */
	if (!d_deleted(dentry))
		unionfs_reinterpose(dentry);

      out:
	if (input_file && !IS_ERR(input_file)) {
		fput(input_file);
	} else {
		/* since input file was not opened, we need to explicitly
		 * dput the old_hidden_dentry
		 */
		DPUT(old_hidden_dentry);
	}

	/* in any case, we have to branchput */
	if (got_branch_input >= 0)
		branchput(sb, got_branch_input);

	if (output_file) {
		if (copyup_file && !err) {
			*copyup_file = output_file;
		} else {
			fput(output_file);
			branchput(sb, got_branch_output);
		}
	}

	fist_print_dentry("OUT: copyup_dentry", dentry);
	fist_print_inode("OUT: copyup_dentry", dentry->d_inode);

	print_exit_status(err);
	return err;
}

/* This function creates a copy of a file represented by 'file' which currently
 * resides in branch 'bstart' to branch 'new_bindex.  The copy will be named
 * "name".  */
int copyup_named_file(struct inode *dir, struct file *file, char *name,
		      int bstart, int new_bindex, int len)
{
	int err = 0;
	struct file *output_file = NULL;

	print_entry_location();

	err = copyup_named_dentry(dir, file->f_dentry, bstart,
				  new_bindex, name, strlen(name), &output_file,
				  len);
	if (!err) {
		fbstart(file) = new_bindex;
		set_ftohf_index(file, new_bindex, output_file);
	}

	print_exit_status(err);
	return err;
}

/* This function creates a copy of a file represented by 'file' which currently
 * resides in branch 'bstart' to branch 'new_bindex.
 */
int copyup_file(struct inode *dir, struct file *file, int bstart,
		int new_bindex, int len)
{
	int err = 0;
	struct file *output_file = NULL;

	print_entry_location();

	err = copyup_dentry(dir, file->f_dentry, bstart, new_bindex,
			    &output_file, len);
	if (!err) {
		fbstart(file) = new_bindex;
		set_ftohf_index(file, new_bindex, output_file);
	}

	print_exit_status(err);
	return err;
}

/* This function replicates the directory structure upto given dentry
 * in the bindex branch. Can create directory structure recursively to the right
 * also.
 */
struct dentry *create_parents(struct inode *dir, struct dentry *dentry,
			      int bindex)
{
	struct dentry *hidden_dentry;

	print_entry_location();
	hidden_dentry =
	    create_parents_named(dir, dentry, dentry->d_name.name, bindex);
	print_exit_location();

	return (hidden_dentry);
}

/* This function replicates the directory structure upto given dentry
 * in the bindex branch.  */
struct dentry *create_parents_named(struct inode *dir, struct dentry *dentry,
				    const char *name, int bindex)
{
	int err;
	struct dentry *child_dentry;
	struct dentry *parent_dentry;
	struct dentry *hidden_parent_dentry = NULL;
	struct dentry *hidden_dentry = NULL;
	const char *childname;
	unsigned int childnamelen;

	int old_kmalloc_size;
	int kmalloc_size;
	int num_dentry;
	int count;

	int old_bstart;
	int old_bend;
	struct dentry **path = NULL;
	struct dentry **tmp_path;

	print_entry_location();

	verify_locked(dentry);

	/* There is no sense allocating any less than the minimum. */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
	kmalloc_size = malloc_sizes[0].cs_size;
#else
	kmalloc_size = 32;
#endif
	num_dentry = kmalloc_size / sizeof(struct dentry *);

	if ((err = is_robranch_super(dir->i_sb, bindex))) {
		hidden_dentry = ERR_PTR(err);
		goto out;
	}

	fist_print_dentry("IN: create_parents_named", dentry);
	fist_dprint(8, "name = %s\n", name);

	old_bstart = dbstart(dentry);
	old_bend = dbend(dentry);

	path = (struct dentry **)KMALLOC(kmalloc_size, GFP_KERNEL);
	memset(path, 0, kmalloc_size);

	/* assume the negative dentry of unionfs as the parent dentry */
	parent_dentry = dentry;

	count = 0;
	/* This loop finds the first parent that exists in the given branch.
	 * We start building the directory structure from there.  At the end
	 * of the loop, the following should hold:
	 *      child_dentry is the first nonexistent child
	 *      parent_dentry is the first existent parent
	 *      path[0] is the = deepest child
	 *      path[count] is the first child to create
	 */
	do {
		child_dentry = parent_dentry;

		/* find the parent directory dentry in unionfs */
		parent_dentry = child_dentry->d_parent;
		lock_dentry(parent_dentry);

		/* find out the hidden_parent_dentry in the given branch */
		hidden_parent_dentry = dtohd_index(parent_dentry, bindex);

		/* store the child dentry */
		path[count++] = child_dentry;
		if (count == num_dentry) {
			old_kmalloc_size = kmalloc_size;
			kmalloc_size *= 2;
			num_dentry = kmalloc_size / sizeof(struct dentry *);

			tmp_path =
			    (struct dentry **)KMALLOC(kmalloc_size, GFP_KERNEL);
			if (!tmp_path) {
				hidden_dentry = ERR_PTR(-ENOMEM);
				goto out;
			}
			memset(tmp_path, 0, kmalloc_size);
			memcpy(tmp_path, path, old_kmalloc_size);
			KFREE(path);
			path = tmp_path;
			tmp_path = NULL;
		}

	} while (!hidden_parent_dentry);
	count--;

	/* This is basically while(child_dentry != dentry).  This loop is
	 * horrible to follow and should be replaced with cleaner code. */
	while (1) {
		PASSERT(child_dentry);
		PASSERT(parent_dentry);
		PASSERT(parent_dentry->d_inode);

		// get hidden parent dir in the current branch
		hidden_parent_dentry = dtohd_index(parent_dentry, bindex);
		unlock_dentry(parent_dentry);
		PASSERT(hidden_parent_dentry);
		PASSERT(hidden_parent_dentry->d_inode);

		// init the values to lookup
		childname = child_dentry->d_name.name;
		childnamelen = child_dentry->d_name.len;

		if (child_dentry != dentry) {
			// lookup child in the underlying file system
			hidden_dentry =
			    LOOKUP_ONE_LEN(childname, hidden_parent_dentry,
					   childnamelen);
			if (IS_ERR(hidden_dentry))
				goto out;
		} else {
			int loop_start;
			int loop_end;
			int new_bstart = -1;
			int new_bend = -1;
			int i;

			/* is the name a whiteout of the childname ? */
			//lookup the whiteout child in the underlying file system
			hidden_dentry =
			    LOOKUP_ONE_LEN(name, hidden_parent_dentry,
					   strlen(name));
			if (IS_ERR(hidden_dentry))
				goto out;

			/* Replace the current dentry (if any) with the new one. */
			DPUT(dtohd_index(dentry, bindex));
			set_dtohd_index(dentry, bindex, hidden_dentry);

			loop_start =
			    (old_bstart < bindex) ? old_bstart : bindex;
			loop_end = (old_bend > bindex) ? old_bend : bindex;

			/* This loop sets the bstart and bend for the new
			 * dentry by traversing from left to right.
			 * It also dputs all negative dentries except
			 * bindex (the newly looked dentry
			 */
			for (i = loop_start; i <= loop_end; i++) {
				if (!dtohd_index(dentry, i))
					continue;

				if (i == bindex) {
					new_bend = i;
					if (new_bstart < 0)
						new_bstart = i;
					continue;
				}

				if (!dtohd_index(dentry, i)->d_inode) {
					DPUT(dtohd_index(dentry, i));
					set_dtohd_index(dentry, i, NULL);
				} else {
					if (new_bstart < 0)
						new_bstart = i;
					new_bend = i;
				}
			}

			if (new_bstart < 0)
				new_bstart = bindex;
			if (new_bend < 0)
				new_bend = bindex;
			set_dbstart(dentry, new_bstart);
			set_dbend(dentry, new_bend);
			break;
		}

		if (hidden_dentry->d_inode) {
			/* since this already exists we dput to avoid
			 * multiple references on the same dentry */
			DPUT(hidden_dentry);
		} else {
			uid_t saved_uid = current->fsuid;
			gid_t saved_gid = current->fsgid;

			/* its a negative dentry, create a new dir */
			hidden_parent_dentry = lock_parent(hidden_dentry);
			current->fsuid = hidden_parent_dentry->d_inode->i_uid;
			current->fsgid = hidden_parent_dentry->d_inode->i_gid;
			err = vfs_mkdir(hidden_parent_dentry->d_inode,
					hidden_dentry, S_IRWXUGO);
			current->fsuid = saved_uid;
			current->fsgid = saved_gid;
			unlock_dir(hidden_parent_dentry);
			if (err || !hidden_dentry->d_inode) {
				DPUT(hidden_dentry);
				hidden_dentry = ERR_PTR(err);
				goto out;
			}
			err =
			    copyup_permissions(dir->i_sb, child_dentry,
					       hidden_dentry);
			if (err) {
				DPUT(hidden_dentry);
				hidden_dentry = ERR_PTR(err);
				goto out;
			}
			set_itohi_index(child_dentry->d_inode, bindex,
					igrab(hidden_dentry->d_inode));
			if (ibstart(child_dentry->d_inode) > bindex)
				ibstart(child_dentry->d_inode) = bindex;
			if (ibend(child_dentry->d_inode) < bindex)
				ibend(child_dentry->d_inode) = bindex;

			set_dtohd_index(child_dentry, bindex, hidden_dentry);
			if (dbstart(child_dentry) > bindex)
				set_dbstart(child_dentry, bindex);
			if (dbend(child_dentry) < bindex)
				set_dbend(child_dentry, bindex);
		}

		parent_dentry = child_dentry;
		child_dentry = path[--count];
	}
      out:
	KFREE(path);
	fist_print_dentry("OUT: create_parents_named", dentry);
	print_exit_pointer(hidden_dentry);
	return hidden_dentry;
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

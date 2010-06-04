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
#include <linux/module.h>

/* sb we pass is unionfs's super_block */
int unionfs_interpose(struct dentry *dentry, struct super_block *sb, int flag)
{
	struct inode *hidden_inode;
	struct dentry *hidden_dentry;
	int err = 0;
	struct inode *inode;
	int is_negative_dentry = 1;
	int bindex, bstart, bend;

	print_entry("flag = %d", flag);

	verify_locked(dentry);

	fist_print_dentry("In unionfs_interpose", dentry);

	bstart = dbstart(dentry);
	bend = dbend(dentry);

	/* Make sure that we didn't get a negative dentry. */
	for (bindex = bstart; bindex <= bend; bindex++) {
		if (dtohd_index(dentry, bindex) &&
		    dtohd_index(dentry, bindex)->d_inode) {
			is_negative_dentry = 0;
			break;
		}
	}
	ASSERT(!is_negative_dentry);

	/* We allocate our new inode below, by calling iget.
	 * iget will call our read_inode which will initialize some
	 * of the new inode's fields
	 */

	/* On revalidate we've already got our own inode and just need
	 * to fix it up. */
	if (flag == INTERPOSE_REVAL) {
		inode = dentry->d_inode;
		PASSERT(inode);
		itopd(inode)->b_start = -1;
		itopd(inode)->b_end = -1;
		atomic_set(&itopd(inode)->uii_generation,
			   atomic_read(&stopd(sb)->usi_generation));

		if (sbmax(sb) > UNIONFS_INLINE_OBJECTS) {
			int size =
			    (sbmax(sb) -
			     UNIONFS_INLINE_OBJECTS) * sizeof(struct inode *);
			itohi_ptr(inode) = KMALLOC(size, GFP_UNIONFS);
			if (!itohi_ptr(inode)) {
				err = -ENOMEM;
				goto out;
			}
			memset(itohi_ptr(inode), 0, size);
		}
		memset(itohi_inline(inode), 0,
		       UNIONFS_INLINE_OBJECTS * sizeof(struct inode *));
	} else {
		ino_t ino;
		/* get unique inode number for unionfs */
		if (stopd(sb)->usi_persistent)
			ino =
			    get_uin(sb, bindex,
				    dtohd_index(dentry, bindex)->d_inode->i_ino,
				    O_CREAT);
		else
			ino = iunique(sb, UNIONFS_ROOT_INO);

		inode = iget(sb, ino);
		if (!inode) {
			err = -EACCES;	/* should be impossible??? */
			goto out;
		}
	}

	for (bindex = bstart; bindex <= bend; bindex++) {
		hidden_dentry = dtohd_index(dentry, bindex);
		if (!hidden_dentry) {
			set_itohi_index(inode, bindex, NULL);
			continue;
		}
		/* Initialize the hidden inode to the new hidden inode. */
		if (!hidden_dentry->d_inode)
			continue;
		set_itohi_index(inode, bindex, igrab(hidden_dentry->d_inode));
	}

	ibstart(inode) = dbstart(dentry);
	ibend(inode) = dbend(dentry);

	/* Use attributes from the first branch. */
	hidden_inode = itohi(inode);
	PASSERT(hidden_inode);

	/* Use different set of inode ops for symlinks & directories */
	if (S_ISLNK(hidden_inode->i_mode))
		inode->i_op = &unionfs_symlink_iops;
	else if (S_ISDIR(hidden_inode->i_mode))
		inode->i_op = &unionfs_dir_iops;

	/* Use different set of file ops for directories */
	if (S_ISDIR(hidden_inode->i_mode))
		inode->i_fop = &unionfs_dir_fops;

	/* properly initialize special inodes */
	if (S_ISBLK(hidden_inode->i_mode) || S_ISCHR(hidden_inode->i_mode) ||
	    S_ISFIFO(hidden_inode->i_mode) || S_ISSOCK(hidden_inode->i_mode))
		init_special_inode(inode, hidden_inode->i_mode,
				   hidden_inode->i_rdev);

	/* Fix our inode's address operations to that of the lower inode (Unionfs is FiST-Lite) */
	if (inode->i_mapping->a_ops != hidden_inode->i_mapping->a_ops) {
		fist_dprint(7, "fixing inode 0x%p a_ops (0x%p -> 0x%p)\n",
			    inode, inode->i_mapping->a_ops,
			    hidden_inode->i_mapping->a_ops);
		inode->i_mapping->a_ops = hidden_inode->i_mapping->a_ops;
	}

	/* all well, copy inode attributes */
	fist_copy_attr_all(inode, hidden_inode);

	/* only (our) lookup wants to do a d_add */
	switch (flag) {
	case INTERPOSE_DEFAULT:
	case INTERPOSE_REVAL_NEG:
		d_instantiate(dentry, inode);
		break;
	case INTERPOSE_LOOKUP:
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
		err = PTR_ERR(d_splice_alias(inode, dentry));
#else
		d_add(dentry, inode);
#endif
		break;
	case INTERPOSE_REVAL:
		/* Do nothing. */
		break;
	default:
		FISTBUG("Invalid interpose flag passed!");
	}

	PASSERT(dtopd(dentry));

	fist_print_dentry("Leaving unionfs_interpose", dentry);
	fist_print_inode("Leaving unionfs_interpose", inode);

      out:
	print_exit_status(err);
	return err;
}

void unionfs_reinterpose(struct dentry *dentry)
{
	struct dentry *hidden_dentry;
	struct inode *inode;
	int bindex, bstart, bend;

	print_entry_location();
	verify_locked(dentry);
	fist_print_dentry("IN: unionfs_reinterpose: ", dentry);

	/* This is pre-allocated inode */
	inode = dentry->d_inode;
	PASSERT(inode);

	bstart = dbstart(dentry);
	bend = dbend(dentry);
	for (bindex = bstart; bindex <= bend; bindex++) {
		hidden_dentry = dtohd_index(dentry, bindex);
		if (!hidden_dentry)
			continue;

		if (!hidden_dentry->d_inode)
			continue;
		if (itohi_index(inode, bindex))
			continue;
		set_itohi_index(inode, bindex, igrab(hidden_dentry->d_inode));
	}
	ibstart(inode) = dbstart(dentry);
	ibend(inode) = dbend(dentry);

	fist_print_dentry("OUT: unionfs_reinterpose: ", dentry);
	fist_print_inode("OUT: unionfs_reinterpose: ", inode);

	print_exit_location();
}

int check_branch(struct nameidata *nd)
{
	PASSERT(nd);
	PASSERT(nd->dentry);
	PASSERT(nd->dentry->d_sb);
	PASSERT(nd->dentry->d_sb->s_type);
	if (!strcmp(nd->dentry->d_sb->s_type->name, "unionfs"))
		return -EINVAL;
	if (!nd->dentry->d_inode)
		return -ENOENT;
	if (!S_ISDIR(nd->dentry->d_inode->i_mode))
		return -ENOTDIR;
	return 0;
}

static int parse_dirs_option(struct super_block *sb, struct unionfs_dentry_info
			     *hidden_root_info, char *options)
{
	struct nameidata nd;
	char *name;
	int pobjects = 0;
	int err = 0;
	int branches = 1;
	int bindex = 0;
	int i;

	if (options[0] == '\0') {
		printk(KERN_WARNING "unionfs: no branches specified\n");
		err = -EINVAL;
		goto out;
	}

	/* Each colon means we have a separator, this is really just a rough
	 * guess, since strsep will handle empty fields for us. */
	for (i = 0; options[i]; i++) {
		if (options[i] == ':')
			branches++;
	}

	/* allocate space for  underlying pointers to hidden dentry */
	if (branches > UNIONFS_INLINE_OBJECTS)
		pobjects = branches - UNIONFS_INLINE_OBJECTS;

	if (pobjects) {
		err = -ENOMEM;

		hidden_root_info->udi_dentry_p =
		    KMALLOC(sizeof(struct dentry *) * pobjects, GFP_UNIONFS);
		if (!hidden_root_info->udi_dentry_p)
			goto out;

		memset(hidden_root_info->udi_dentry_p, 0,
		       sizeof(struct dentry *) * pobjects);

		stohs_ptr(sb) =
		    KMALLOC(sizeof(struct super_block *) *
			    pobjects, GFP_UNIONFS);
		if (!stohs_ptr(sb))
			goto out;

		memset(stohs_ptr(sb), 0,
		       pobjects * sizeof(struct super_block *));

		stopd(sb)->usi_sbcount_p =
		    KMALLOC(sizeof(atomic_t) * pobjects, GFP_UNIONFS);
		if (!stopd(sb)->usi_sbcount_p)
			goto out;

		stopd(sb)->usi_branchperms_p =
		    KMALLOC(sizeof(int) * pobjects, GFP_UNIONFS);
		if (!stopd(sb)->usi_branchperms_p)
			goto out;

		stohiddenmnt_ptr(sb) =
		    KMALLOC(sizeof(struct vfsmount *) * pobjects, GFP_UNIONFS);
		if (!stohiddenmnt_ptr(sb))
			goto out;

		/* We're done with allocating memory. */
		err = 0;
	}

	/* now parsing the string b1:b2=rw:b3=ro:b4 */
	branches = 0;
	while ((name = strsep(&options, ":")) != NULL) {
		int perms;
		int l;

		if (!*name)
			continue;

		branches++;

		/* strip off =rw or =ro if it is specified. */
		l = strlen(name);
		if (!strcmp(name + l - 3, "=ro")) {
			perms = MAY_READ;
			name[l - 3] = '\0';
		} else if (!strcmp(name + l - 3, "=rw")) {
			perms = MAY_READ | MAY_WRITE;
			name[l - 3] = '\0';
		} else {
			perms = MAY_READ | MAY_WRITE;
		}

		fist_dprint(4, "unionfs: using directory: %s (%c%c)\n",
			    name, perms & MAY_READ ? 'r' : '-',
			    perms & MAY_WRITE ? 'w' : '-');

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
		if (path_init(name, LOOKUP_FOLLOW, &nd))
			err = path_walk(name, &nd);
#else
		err = path_lookup(name, LOOKUP_FOLLOW, &nd);
#endif
		RECORD_PATH_LOOKUP(&nd);
		if (err) {
			printk(KERN_WARNING "unionfs: error accessing "
			       "hidden directory '%s' (error %d)\n", name, err);
			goto out;
		}

		if ((err = check_branch(&nd))) {
			printk(KERN_WARNING "unionfs: hidden directory "
			       "'%s' is not a valid branch\n", name);
			path_release(&nd);
			RECORD_PATH_RELEASE(&nd);
			goto out;
		}

		if (bindex < UNIONFS_INLINE_OBJECTS)
			hidden_root_info->udi_dentry_i[bindex] = nd.dentry;
		else
			hidden_root_info->udi_dentry_p[bindex -
						       UNIONFS_INLINE_OBJECTS]
			    = nd.dentry;

		set_stohiddenmnt_index(sb, bindex, nd.mnt);
		set_branchperms(sb, bindex, perms);
		set_branch_count(sb, bindex, 0);

		if (hidden_root_info->udi_bstart < 0)
			hidden_root_info->udi_bstart = bindex;
		hidden_root_info->udi_bend = bindex;
		bindex++;
	}
	if (branches == 0) {
		printk(KERN_WARNING "unionfs: no branches specified\n");
		err = -EINVAL;
		goto out;
	}

	ASSERT(branches == (hidden_root_info->udi_bend + 1));
      out:
	return err;
}

/*
 * Parse mount options.  See the manual page for usage instructions.
 *
 * Returns the dentry object of the lower-level (hidden) directory;
 * We want to mount our stackable file system on top of that hidden directory.
 *
 * Sets default debugging level to N, if any.
 */
struct unionfs_dentry_info *unionfs_parse_options(struct super_block *sb,
						  char *options)
{
	struct unionfs_dentry_info *hidden_root_info;
	char *optname;
	int err = 0;
	int bindex;
	int dirsfound = 0;
	int imapfound = 0;
	int mounter_f = 0, copyupuid_f = 0, copyupgid_f = 0, copyupmode_f = 0;
	print_entry_location();

	/* allocate private data area */
	err = -ENOMEM;
	hidden_root_info =
	    KMALLOC(sizeof(struct unionfs_dentry_info), GFP_UNIONFS);
	if (!hidden_root_info)
		goto out_error;
	memset(hidden_root_info, 0, sizeof(struct unionfs_dentry_info));
	hidden_root_info->udi_bstart = -1;
	hidden_root_info->udi_bend = -1;
	hidden_root_info->udi_bopaque = -1;

	while ((optname = strsep(&options, ",")) != NULL) {
		char *optarg;
		char *endptr;
		int intval;

		if (!*optname) {
			continue;
		}

		optarg = strchr(optname, '=');
		if (optarg) {
			*optarg++ = '\0';
		}

		/* All of our options take an argument now. Insert ones that
		 * don't, above this check.  */
		if (!optarg) {
			printk("unionfs: %s requires an argument.\n", optname);
			err = -EINVAL;
			goto out_error;
		}

		if (!strcmp("dirs", optname)) {
			if (++dirsfound > 1) {
				printk(KERN_WARNING
				       "unionfs: multiple dirs specified\n");
				err = -EINVAL;
				goto out_error;
			}
			err = parse_dirs_option(sb, hidden_root_info, optarg);
			if (err)
				goto out_error;
			continue;
		}
		if (!strcmp("imap", optname)) {
			if (++imapfound > 1) {
				printk(KERN_WARNING
				       "unionfs: multiple imap specified\n");
				err = -EINVAL;
				goto out_error;
			}
			err = parse_imap_option(sb, hidden_root_info, optarg);
			if (err)
				goto out_error;
			continue;
		}
		if (!strcmp("delete", optname)) {
			if (!strcmp("all", optarg)) {
				/* default */
			} else if (!strcmp("whiteout", optarg)) {
				MOUNT_FLAG(sb) |= DELETE_WHITEOUT;
			} else {
				printk(KERN_WARNING
				       "unionfs: invalid delete option '%s'\n",
				       optarg);
				err = -EINVAL;
				goto out_error;
			}
			continue;
		}
		if (!strcmp("copyup", optname)) {
			if (!strcmp("preserve", optarg)) {
				/* default */
			} else if (!strcmp("currentuser", optarg)) {
				MOUNT_FLAG(sb) |= COPYUP_CURRENT_USER;
			} else if (!strcmp("mounter", optarg)) {
				MOUNT_FLAG(sb) |= COPYUP_FS_MOUNTER;
				mounter_f = 1;
			} else {
				printk(KERN_WARNING
				       "unionfs: could not parse copyup option value '%s'\n",
				       optarg);
				err = -EINVAL;
				goto out_error;
			}
			continue;
		}

		/* All of these options require an integer argument. */
		intval = simple_strtoul(optarg, &endptr, 0);
		if (*endptr) {
			printk(KERN_WARNING
			       "unionfs: invalid %s option '%s'\n",
			       optname, optarg);
			err = -EINVAL;
			goto out_error;
		}

		if (!strcmp("debug", optname)) {
			fist_set_debug_value(intval);
			continue;
		}
		if (!strcmp("copyupuid", optname)) {
			stopd(sb)->copyupuid = intval;
			copyupuid_f = 1;
			continue;
		}
		if (!strcmp("copyupgid", optname)) {
			stopd(sb)->copyupgid = intval;
			copyupgid_f = 1;
			continue;
		}
		if (!strcmp("copyupmode", optname)) {
			if (intval & ~0777) {
				err = -EINVAL;
				printk(KERN_WARNING
				       "unionfs: copyupmode invalid '%o' (note: you need to preface octal values with 0.\n",
				       intval);
				goto out_error;
			}
			stopd(sb)->copyupmode = intval;
			copyupmode_f = 1;
			continue;
		}

		err = -EINVAL;
		printk(KERN_WARNING
		       "unionfs: unrecognized option '%s'\n", optname);
		goto out_error;
	}
	if (dirsfound != 1) {
		printk(KERN_WARNING "unionfs: dirs option required\n");
		err = -EINVAL;
		goto out_error;
	}
	if (mounter_f && !(copyupuid_f && copyupgid_f && copyupmode_f)) {
		printk(KERN_WARNING
		       "unionfs: "
		       "copyup=mounter all copyup options must be set\n");

		err = -EINVAL;
		goto out_error;
	}
	if ((copyupuid_f || copyupgid_f || copyupmode_f) && !mounter_f) {
		printk(KERN_WARNING
		       "unionfs: "
		       "copyup!=mounter and a copyup option is set\n");
		err = -EINVAL;
		goto out_error;
	}
	goto out;

      out_error:
	for (bindex = hidden_root_info->udi_bstart;
	     bindex >= 0 && bindex <= hidden_root_info->udi_bend; bindex++) {
		struct dentry *d;
		if (bindex < UNIONFS_INLINE_OBJECTS)
			d = hidden_root_info->udi_dentry_i[bindex];
		else
			d = hidden_root_info->udi_dentry_p[bindex -
							   UNIONFS_INLINE_OBJECTS];
		DPUT(d);
		if (stohiddenmnt_index(sb, bindex))
			mntput(stohiddenmnt_index(sb, bindex));
	}
	KFREE(hidden_root_info->udi_dentry_p);
	KFREE(hidden_root_info);

	KFREE(stohiddenmnt_ptr(sb));
	stohiddenmnt_ptr(sb) = NULL;
	KFREE(stopd(sb)->usi_sbcount_p);
	stopd(sb)->usi_sbcount_p = NULL;
	KFREE(stopd(sb)->usi_branchperms_p);
	stopd(sb)->usi_branchperms_p = NULL;
	KFREE(stohs_ptr(sb));
	stohs_ptr(sb) = NULL;

	hidden_root_info = ERR_PTR(err);
      out:
	print_exit_location();
	return hidden_root_info;
}

#ifdef FIST_MALLOC_DEBUG
/* for malloc debugging */
atomic_t unionfs_malloc_counter;
atomic_t unionfs_mallocs_outstanding;
atomic_t unionfs_dget_counter;
atomic_t unionfs_dgets_outstanding;

void *unionfs_kmalloc(size_t len, int flag, int line, const char *file)
{
	void *ptr = (void *)kmalloc(len, flag);
	if (ptr) {
		atomic_inc(&unionfs_malloc_counter);
		atomic_inc(&unionfs_mallocs_outstanding);
		printk("KM:%d:%d:%p:%d:%s\n",
		       atomic_read(&unionfs_malloc_counter),
		       atomic_read(&unionfs_mallocs_outstanding), ptr, line,
		       file);
	}
	return ptr;
}

void unionfs_kfree(void *ptr, int line, const char *file)
{
	atomic_inc(&unionfs_malloc_counter);
	if (ptr) {
		ASSERT(!IS_ERR(ptr));
		atomic_dec(&unionfs_mallocs_outstanding);
	}
	printk("KF:%d:%d:%p:%d:%s\n", atomic_read(&unionfs_malloc_counter),
	       atomic_read(&unionfs_mallocs_outstanding), ptr, line, file);
	kfree(ptr);
}

void record_set(struct dentry *upper, int index, struct dentry *ptr,
		struct dentry *old, int line, const char *file)
{
	atomic_inc(&unionfs_dget_counter);
	printk("DD:%d:%d:%d:%p:%d:%s %p, %d\n",
	       atomic_read(&unionfs_dget_counter),
	       atomic_read(&unionfs_dgets_outstanding),
	       old ? atomic_read(&old->d_count) : 0, old, line, file, upper,
	       index);
	atomic_inc(&unionfs_dget_counter);
	printk("DS:%d:%d:%d:%p:%d:%s %p, %d\n",
	       atomic_read(&unionfs_dget_counter),
	       atomic_read(&unionfs_dgets_outstanding),
	       ptr ? atomic_read(&ptr->d_count) : 0, ptr, line, file, upper,
	       index);
}

void record_path_lookup(struct nameidata *nd, int line, const char *file)
{
	struct dentry *ptr = nd->dentry;
	if (ptr) {
		atomic_inc(&unionfs_dget_counter);
		atomic_inc(&unionfs_dgets_outstanding);
		printk("DL:%d:%d:%d:%p:%d:%s\n",
		       atomic_read(&unionfs_dget_counter),
		       atomic_read(&unionfs_dgets_outstanding),
		       atomic_read(&ptr->d_count), ptr, line, file);
	}
}

void record_path_release(struct nameidata *nd, int line, const char *file)
{
	struct dentry *ptr = nd->dentry;

	atomic_inc(&unionfs_dget_counter);
	if (ptr)
		atomic_dec(&unionfs_dgets_outstanding);
	printk("DP:%d:%d:%d:%p:%d:%s\n", atomic_read(&unionfs_dget_counter),
	       atomic_read(&unionfs_dgets_outstanding),
	       ptr ? atomic_read(&ptr->d_count) : 0, ptr, line, file);
}

struct file *unionfs_dentry_open(struct dentry *ptr, struct vfsmount *mnt,
				 int flags, int line, const char *file)
{
	atomic_inc(&unionfs_dget_counter);
	if (ptr)
		atomic_dec(&unionfs_dgets_outstanding);
	printk("DO:%d:%d:%d:%p:%d:%s\n", atomic_read(&unionfs_dget_counter),
	       atomic_read(&unionfs_dgets_outstanding),
	       ptr ? atomic_read(&ptr->d_count) : 0, ptr, line, file);
	return dentry_open(ptr, mnt, flags);
}

struct dentry *unionfs_dget(struct dentry *ptr, int line, const char *file)
{
	ptr = dget(ptr);
	if (ptr) {
		atomic_inc(&unionfs_dget_counter);
		atomic_inc(&unionfs_dgets_outstanding);
		printk("DG:%d:%d:%d:%p:%d:%s\n",
		       atomic_read(&unionfs_dget_counter),
		       atomic_read(&unionfs_dgets_outstanding),
		       atomic_read(&ptr->d_count), ptr, line, file);
	}
	return ptr;
}

struct dentry *unionfs_dget_parent(struct dentry *child, int line,
				   const char *file)
{
	struct dentry *ptr;

	PASSERT(child);
	ptr = dget_parent(child);
	PASSERT(ptr);
	atomic_inc(&unionfs_dget_counter);
	atomic_inc(&unionfs_dgets_outstanding);
	printk("DG:%d:%d:%d:%p:%d:%s\n",
	       atomic_read(&unionfs_dget_counter),
	       atomic_read(&unionfs_dgets_outstanding),
	       atomic_read(&ptr->d_count), ptr, line, file);

	return ptr;
}

struct dentry *unionfs_lookup_one_len(const char *name, struct dentry *parent,
				      int len, int line, const char *file)
{
	struct dentry *ptr = lookup_one_len(name, parent, len);
	if (ptr && !IS_ERR(ptr)) {
		atomic_inc(&unionfs_dget_counter);
		atomic_inc(&unionfs_dgets_outstanding);
		printk("DL:%d:%d:%d:%p:%d:%s\n",
		       atomic_read(&unionfs_dget_counter),
		       atomic_read(&unionfs_dgets_outstanding),
		       atomic_read(&ptr->d_count), ptr, line, file);
	}
	return ptr;
}

void unionfs_dput(struct dentry *ptr, int line, const char *file)
{
	atomic_inc(&unionfs_dget_counter);
	if (ptr) {
		ASSERT(!IS_ERR(ptr));
		atomic_dec(&unionfs_dgets_outstanding);
	}
	printk("DP:%d:%d:%d:%p:%d:%s\n", atomic_read(&unionfs_dget_counter),
	       atomic_read(&unionfs_dgets_outstanding),
	       ptr ? atomic_read(&ptr->d_count) : 0, ptr, line, file);
	dput(ptr);
}

#endif				/* FIST_MALLOC_DEBUG */

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
struct super_block *
#else
static int
#endif
unionfs_read_super(struct super_block *sb, void *raw_data, int silent)
{
	int err = 0;

	struct unionfs_dentry_info *hidden_root_info = NULL;
	int bindex, bstart, bend;
	unsigned long long maxbytes;

	print_entry_location();

	if (!raw_data) {
		printk(KERN_WARNING
		       "unionfs_read_super: missing data argument\n");
		err = -EINVAL;
		goto out;
	}

	/*
	 * Allocate superblock private data
	 */
	stopd_lhs(sb) = KMALLOC(sizeof(struct unionfs_sb_info), GFP_UNIONFS);
	if (!stopd(sb)) {
		printk(KERN_WARNING "%s: out of memory\n", __FUNCTION__);
		err = -ENOMEM;
		goto out;
	}
	memset(stopd(sb), 0, sizeof(struct unionfs_sb_info));
	stopd(sb)->b_end = -1;
	atomic_set(&stopd(sb)->usi_generation, 1);

	hidden_root_info = unionfs_parse_options(sb, raw_data);
	if (IS_ERR(hidden_root_info)) {
		printk(KERN_WARNING
		       "unionfs_read_super: error while parsing options (err = %ld)\n",
		       PTR_ERR(hidden_root_info));
		err = PTR_ERR(hidden_root_info);
		hidden_root_info = NULL;
		goto out_free;
	}
	if (hidden_root_info->udi_bstart == -1) {
		err = -ENOENT;
		goto out_free;
	}

	/* set the hidden superblock field of upper superblock */
	bstart = hidden_root_info->udi_bstart;
	ASSERT(bstart == 0);
	sbend(sb) = bend = hidden_root_info->udi_bend;
	for (bindex = bstart; bindex <= bend; bindex++) {
		struct dentry *d;

		if (bindex < UNIONFS_INLINE_OBJECTS)
			d = hidden_root_info->udi_dentry_i[bindex];
		else
			d = hidden_root_info->udi_dentry_p[bindex -
							   UNIONFS_INLINE_OBJECTS];

		set_stohs_index(sb, bindex, d->d_sb);
	}

	/* Unionfs: Max Bytes is the maximum bytes from among all the branches */
	maxbytes = -1;
	for (bindex = bstart; bindex <= bend; bindex++)
		if (maxbytes < stohs_index(sb, bindex)->s_maxbytes)
			maxbytes = stohs_index(sb, bindex)->s_maxbytes;
	sb->s_maxbytes = maxbytes;

	sb->s_op = &unionfs_sops;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
	sb->s_export_op = &unionfs_export_ops;
#endif

	/*
	 * we can't use d_alloc_root if we want to use
	 * our own interpose function unchanged,
	 * so we simply replicate *most* of the code in d_alloc_root here
	 */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
	sb->s_root = d_alloc(NULL, &(const struct qstr) {
			     "/", 1, 0});
#else
	sb->s_root = d_alloc(NULL, &(const struct qstr) {
      hash: 0, name: "/", len:1});
#endif
	if (IS_ERR(sb->s_root)) {
		printk(KERN_WARNING "unionfs_read_super: d_alloc failed\n");
		err = PTR_ERR(sb->s_root);
		goto out_dput;
	}

	sb->s_root->d_op = &unionfs_dops;
	sb->s_root->d_sb = sb;
	sb->s_root->d_parent = sb->s_root;

	/* link the upper and lower dentries */
	dtopd_lhs(sb->s_root) = NULL;
	if ((err = new_dentry_private_data(sb->s_root)))
		goto out_freedpd;

	/* Set the hidden dentries for s_root */
	for (bindex = bstart; bindex <= bend; bindex++) {
		struct dentry *d;

		if (bindex < UNIONFS_INLINE_OBJECTS)
			d = hidden_root_info->udi_dentry_i[bindex];
		else
			d = hidden_root_info->udi_dentry_p[bindex -
							   UNIONFS_INLINE_OBJECTS];

		set_dtohd_index(sb->s_root, bindex, d);
	}
	set_dbstart(sb->s_root, bstart);
	set_dbend(sb->s_root, bend);

	/* Set the generation number to one, since this is for the mount. */
	atomic_set(&dtopd(sb->s_root)->udi_generation, 1);

	/* call interpose to create the upper level inode */
	if ((err = unionfs_interpose(sb->s_root, sb, 0)))
		goto out_freedpd;
	unlock_dentry(sb->s_root);
	goto out;

      out_freedpd:
	if (dtopd(sb->s_root)) {
		KFREE(dtohd_ptr(sb->s_root));
		free_dentry_private_data(dtopd(sb->s_root));
	}
	DPUT(sb->s_root);
      out_dput:
	if (hidden_root_info && !IS_ERR(hidden_root_info)) {
		for (bindex = hidden_root_info->udi_bstart;
		     bindex <= hidden_root_info->udi_bend; bindex++) {
			struct dentry *d;

			if (bindex < UNIONFS_INLINE_OBJECTS)
				d = hidden_root_info->udi_dentry_i[bindex];
			else if (!hidden_root_info->udi_dentry_p)
				break;
			else
				d = hidden_root_info->udi_dentry_p[bindex -
								   UNIONFS_INLINE_OBJECTS];

			if (d)
				DPUT(d);

			if (stopd(sb) && stohiddenmnt_index(sb, bindex))
				mntput(stohiddenmnt_index(sb, bindex));
		}
		KFREE(hidden_root_info->udi_dentry_p);
		KFREE(hidden_root_info);
		hidden_root_info = NULL;
	}
      out_free:
	KFREE(stohiddenmnt_ptr(sb));
	KFREE(stopd(sb)->usi_sbcount_p);
	KFREE(stopd(sb)->usi_branchperms_p);
	KFREE(stohs_ptr(sb));
	KFREE(stopd(sb));
	stopd_lhs(sb) = NULL;
      out:
	if (hidden_root_info && !IS_ERR(hidden_root_info)) {
		KFREE(hidden_root_info->udi_dentry_p);
		KFREE(hidden_root_info);
	}
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
	if (err)
		sb = NULL;
	print_exit_pointer(sb);
	return sb;
#else
	print_exit_status(err);
	return err;
#endif
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
/*----*/
// this structure *must* be static!!! (or the memory for 'name' gets
// corrupted in 2.3...)
static DECLARE_FSTYPE(unionfs_fs_type, "unionfs", unionfs_read_super, 0);
#else
static struct super_block *unionfs_get_sb(struct file_system_type *fs_type,
					  int flags, const char *dev_name,
					  void *raw_data)
{
	return get_sb_nodev(fs_type, flags, raw_data, unionfs_read_super);
}

void unionfs_kill_block_super(struct super_block *sb)
{
	generic_shutdown_super(sb);
}

static struct file_system_type unionfs_fs_type = {
	.owner = THIS_MODULE,
	.name = "unionfs",
	.get_sb = unionfs_get_sb,
	.kill_sb = unionfs_kill_block_super,
	.fs_flags = FS_REVAL_DOT,
};

#endif
static int __init init_unionfs_fs(void)
{
	int err;
	printk("Registering unionfs " UNIONFS_VERSION "\n");

#ifdef FIST_MALLOC_DEBUG
	atomic_set(&unionfs_malloc_counter, 0);
	atomic_set(&unionfs_mallocs_outstanding, 0);
#endif				/* FIST_MALLOC_DEBUG */

	if ((err = init_filldir_cache()))
		goto out;
	if ((err = init_inode_cache()))
		goto out;
	if ((err = init_dentry_cache()))
		goto out;
	err = register_filesystem(&unionfs_fs_type);
      out:
	if (err) {
		destroy_filldir_cache();
		destroy_inode_cache();
		destroy_dentry_cache();
	}
	return err;
}
static void __exit exit_unionfs_fs(void)
{
	destroy_filldir_cache();
	destroy_inode_cache();
	destroy_dentry_cache();
	unregister_filesystem(&unionfs_fs_type);
	printk("Completed unionfs module unload.\n");
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
EXPORT_NO_SYMBOLS;
#endif
MODULE_AUTHOR("Erez Zadok <ezk@cs.columbia.edu>");
MODULE_DESCRIPTION("FiST-generated unionfs filesystem");
MODULE_LICENSE("GPL");

module_init(init_unionfs_fs);
module_exit(exit_unionfs_fs);
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

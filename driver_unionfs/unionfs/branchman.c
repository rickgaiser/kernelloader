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

static void fixputmaps(struct super_block *sb)
{
	struct unionfs_sb_info *spd;
	struct putmap *cur;
	int gen;
	int i;

	print_entry_location();

	spd = stopd(sb);
	cur = spd->usi_putmaps[spd->usi_lastputmap - spd->usi_firstputmap];

	for (gen = 0; gen < spd->usi_lastputmap - spd->usi_firstputmap; gen++) {
		if (!spd->usi_putmaps[gen])
			continue;
		for (i = 0; i <= spd->usi_putmaps[gen]->bend; i++)
			spd->usi_putmaps[gen]->map[i] =
			    cur->map[spd->usi_putmaps[gen]->map[i]];
	}

	print_exit_location();
}

static int newputmap(struct super_block *sb)
{
	struct unionfs_sb_info *spd;
	struct putmap *newmap;
	int count = 0;
	int i;

	print_entry_location();

	spd = stopd(sb);

	i = sizeof(int) * (sbend(sb) + 1);
	newmap = KMALLOC(sizeof(struct putmap) + i, GFP_UNIONFS);
	if (!newmap) {
		print_exit_status(-ENOMEM);
		return -ENOMEM;
	}

	if (!spd->usi_firstputmap) {
		spd->usi_firstputmap = 1;
		spd->usi_lastputmap = 1;

		spd->usi_putmaps =
		    KMALLOC(sizeof(struct putmap *), GFP_UNIONFS);
		if (!spd->usi_putmaps) {
			KFREE(newmap);
			print_exit_status(-ENOMEM);
			return -ENOMEM;
		}
	} else {
		struct putmap **newlist;
		int newfirst = spd->usi_firstputmap;

		while (!spd->usi_putmaps[newfirst - spd->usi_firstputmap] &&
		       newfirst <= spd->usi_lastputmap) {
			newfirst++;
		}

		newlist =
		    KMALLOC(sizeof(struct putmap *) *
			    (1 + spd->usi_lastputmap - newfirst), GFP_UNIONFS);
		if (!newlist) {
			KFREE(newmap);
			print_exit_status(-ENOMEM);
			return -ENOMEM;
		}

		for (i = newfirst; i <= spd->usi_lastputmap; i++) {
			newlist[i - newfirst] =
			    spd->usi_putmaps[i - spd->usi_firstputmap];
		}

		KFREE(spd->usi_putmaps);
		spd->usi_putmaps = newlist;
		spd->usi_firstputmap = newfirst;
		spd->usi_lastputmap++;
	}

	newmap->bend = sbend(sb);
	for (i = 0; i <= sbend(sb); i++) {
		count += branch_count(sb, i);
		newmap->map[i] = i;
	}
	for (i = spd->usi_firstputmap; i < spd->usi_lastputmap; i++) {
		struct putmap *cur;
		cur = spd->usi_putmaps[i - spd->usi_firstputmap];
		if (!cur)
			continue;
		count -= atomic_read(&cur->count);
	}
	atomic_set(&newmap->count, count);
	spd->usi_putmaps[spd->usi_lastputmap - spd->usi_firstputmap] = newmap;

	print_exit_status(0);
	return 0;
}

int unionfs_ioctl_branchcount(struct file *file, unsigned int cmd,
			      unsigned long arg)
{
	int err = 0;
	int bstart, bend;
	int i;
	struct super_block *sb = file->f_dentry->d_sb;

	print_entry_location();

	bstart = sbstart(sb);
	bend = sbend(sb);

	err = bend + 1;
	if (!arg)
		goto out;

	for (i = bstart; i <= bend; i++) {
		if (put_user(branch_count(sb, i), ((int *)arg) + i)) {
			err = -EFAULT;
			goto out;
		}
	}

      out:
	print_exit_status(err);
	return err;
}

int unionfs_ioctl_incgen(struct file *file, unsigned int cmd, unsigned long arg)
{
	int err = 0;
	struct super_block *sb;

	print_entry_location();

	sb = file->f_dentry->d_sb;

	lock_super(sb);
	if ((err = newputmap(sb)))
		goto out;

	atomic_inc(&stopd(sb)->usi_generation);
	err = atomic_read(&stopd(sb)->usi_generation);

	atomic_set(&dtopd(sb->s_root)->udi_generation, err);
	atomic_set(&itopd(sb->s_root->d_inode)->uii_generation, err);

	print_exit_status(err);

      out:
	unlock_super(sb);
	return err;
}

int unionfs_ioctl_addbranch(struct inode *inode, unsigned int cmd,
			    unsigned long arg)
{
	int err;
	struct unionfs_addbranch_args *addargs = NULL;
	struct nameidata nd;
	char *path = NULL;
	int gen;
	int i;
	int count;

	int pobjects;

	struct vfsmount **new_hidden_mnt = NULL;
	struct inode **new_uii_inode = NULL;
	struct dentry **new_udi_dentry = NULL;
	struct super_block **new_usi_sb = NULL;
	int *new_branchperms = NULL;
	atomic_t *new_counts = NULL;

	print_entry_location();

	err = -ENOMEM;
	addargs = KMALLOC(sizeof(struct unionfs_addbranch_args), GFP_UNIONFS);
	if (!addargs)
		goto out;

	err = -EFAULT;
	if (copy_from_user
	    (addargs, (void *)arg, sizeof(struct unionfs_addbranch_args)))
		goto out;

	err = -EINVAL;
	if (addargs->ab_perms & ~(MAY_READ | MAY_WRITE))
		goto out;
	if (!(addargs->ab_perms & MAY_READ))
		goto out;

	err = -E2BIG;
	if (sbend(inode->i_sb) > FD_SETSIZE)
		goto out;

	err = -ENOMEM;
	if (!(path = getname(addargs->ab_path)))
		goto out;

//DQ: 2.6 has a different way of doing this
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
	/* Look it up */
	if (path_init(path, LOOKUP_FOLLOW, &nd))
		err = path_walk(path, &nd);
#else
	err = path_lookup(path, LOOKUP_FOLLOW, &nd);
#endif
	RECORD_PATH_LOOKUP(&nd);
	if (err)
		goto out;
	if ((err = check_branch(&nd))) {
		path_release(&nd);
		RECORD_PATH_RELEASE(&nd);
		goto out;
	}

	lock_super(inode->i_sb);
	lock_dentry(inode->i_sb->s_root);

	err = -EINVAL;
	if (addargs->ab_branch < 0
	    || (addargs->ab_branch > (sbend(inode->i_sb) + 1)))
		goto out;

	if ((err = newputmap(inode->i_sb)))
		goto out;

	stopd(inode->i_sb)->b_end++;
	dtopd(inode->i_sb->s_root)->udi_bcount++;
	set_dbend(inode->i_sb->s_root, dbend(inode->i_sb->s_root) + 1);
	itopd(inode->i_sb->s_root->d_inode)->b_end++;

	atomic_inc(&stopd(inode->i_sb)->usi_generation);
	gen = atomic_read(&stopd(inode->i_sb)->usi_generation);

	pobjects = (sbend(inode->i_sb) + 1) - UNIONFS_INLINE_OBJECTS;
	if (pobjects > 0) {
		/* Reallocate the dynamic structures. */
		new_hidden_mnt =
		    KMALLOC(sizeof(struct vfsmount *) * pobjects, GFP_UNIONFS);
		new_udi_dentry =
		    KMALLOC(sizeof(struct dentry *) * pobjects, GFP_UNIONFS);
		new_uii_inode =
		    KMALLOC(sizeof(struct inode *) * pobjects, GFP_UNIONFS);
		new_usi_sb =
		    KMALLOC(sizeof(struct super_block *) * pobjects,
			    GFP_UNIONFS);
		new_counts = KMALLOC(sizeof(atomic_t) * pobjects, GFP_UNIONFS);
		new_branchperms = KMALLOC(sizeof(int) * pobjects, GFP_UNIONFS);
		if (!new_hidden_mnt || !new_udi_dentry || !new_uii_inode
		    || !new_counts || !new_usi_sb || !new_branchperms) {
			err = -ENOMEM;
			goto out;
		}
		memset(new_hidden_mnt, 0, sizeof(struct vfsmount *) * pobjects);
		memset(new_udi_dentry, 0, sizeof(struct dentry *) * pobjects);
		memset(new_uii_inode, 0, sizeof(struct inode *) * pobjects);
		memset(new_usi_sb, 0, sizeof(struct super_block *) * pobjects);
		memset(new_branchperms, 0, sizeof(int) * pobjects);
	}

	/* Copy the in-place values to our new structure. */
	for (i = UNIONFS_INLINE_OBJECTS; i < addargs->ab_branch; i++) {
		int j = i - UNIONFS_INLINE_OBJECTS;

		count = branch_count(inode->i_sb, i);
		atomic_set(&(new_counts[j]), count);

		new_branchperms[j] = branchperms(inode->i_sb, i);
		new_hidden_mnt[j] = stohiddenmnt_index(inode->i_sb, i);

		new_usi_sb[j] = stohs_index(inode->i_sb, i);
		new_udi_dentry[j] = dtohd_index(inode->i_sb->s_root, i);
		new_uii_inode[j] = itohi_index(inode->i_sb->s_root->d_inode, i);
	}

	/* Shift the ends to the right (only handle reallocated bits). */
	for (i = sbend(inode->i_sb) - 1; i >= (int)addargs->ab_branch; i--) {
		int j = i + 1;
		int perms;
		struct vfsmount *hm;
		struct super_block *hs;
		struct dentry *hd;
		struct inode *hi;
		int pmindex;

		count = branch_count(inode->i_sb, i);
		perms = branchperms(inode->i_sb, i);
		hm = stohiddenmnt_index(inode->i_sb, i);
		hs = stohs_index(inode->i_sb, i);
		hd = dtohd_index(inode->i_sb->s_root, i);
		hi = itohi_index(inode->i_sb->s_root->d_inode, i);

		/* Update the newest putmap, so it is correct for later. */
		pmindex = stopd(inode->i_sb)->usi_lastputmap;
		pmindex -= stopd(inode->i_sb)->usi_firstputmap;
		stopd(inode->i_sb)->usi_putmaps[pmindex]->map[i] = j;

		if (j >= UNIONFS_INLINE_OBJECTS) {
			j -= UNIONFS_INLINE_OBJECTS;
			atomic_set(&(new_counts[j]), count);
			new_branchperms[j] = perms;
			new_hidden_mnt[j] = hm;
			new_usi_sb[j] = hs;
			new_udi_dentry[j] = hd;
			new_uii_inode[j] = hi;
		} else {
			set_branch_count(inode->i_sb, j, count);
			set_branchperms(inode->i_sb, j, perms);
			set_stohiddenmnt_index(inode->i_sb, j, hm);
			set_stohs_index(inode->i_sb, j, hs);
			set_dtohd_index(inode->i_sb->s_root, j, hd);
			set_itohi_index(inode->i_sb->s_root->d_inode, j, hi);
		}
	}

	/* Now we can free the old ones. */
	KFREE(dtopd(inode->i_sb->s_root)->udi_dentry_p);
	KFREE(itopd(inode->i_sb->s_root->d_inode)->uii_inode_p);
	KFREE(stopd(inode->i_sb)->usi_hidden_mnt_p);
	KFREE(stopd(inode->i_sb)->usi_sb_p);
	KFREE(stopd(inode->i_sb)->usi_sbcount_p);
	KFREE(stopd(inode->i_sb)->usi_branchperms_p);

	/* Update the real pointers. */
	dtohd_ptr(inode->i_sb->s_root) = new_udi_dentry;
	itohi_ptr(inode->i_sb->s_root->d_inode) = new_uii_inode;
	stohiddenmnt_ptr(inode->i_sb) = new_hidden_mnt;
	stohs_ptr(inode->i_sb) = new_usi_sb;
	stopd(inode->i_sb)->usi_sbcount_p = new_counts;
	stopd(inode->i_sb)->usi_branchperms_p = new_branchperms;

	/* Re-NULL the new ones so we don't try to free them. */
	new_hidden_mnt = NULL;
	new_udi_dentry = NULL;
	new_usi_sb = NULL;
	new_uii_inode = NULL;
	new_counts = NULL;
	new_branchperms = NULL;

	/* Put the new dentry information into it's slot. */
	set_dtohd_index(inode->i_sb->s_root, addargs->ab_branch, nd.dentry);
	set_itohi_index(inode->i_sb->s_root->d_inode, addargs->ab_branch,
			igrab(nd.dentry->d_inode));
	set_branchperms(inode->i_sb, addargs->ab_branch, addargs->ab_perms);
	set_branch_count(inode->i_sb, addargs->ab_branch, 0);
	set_stohiddenmnt_index(inode->i_sb, addargs->ab_branch, nd.mnt);
	set_stohs_index(inode->i_sb, addargs->ab_branch, nd.dentry->d_sb);

	atomic_set(&dtopd(inode->i_sb->s_root)->udi_generation, gen);
	atomic_set(&itopd(inode->i_sb->s_root->d_inode)->uii_generation, gen);

	fixputmaps(inode->i_sb);

      out:
	unlock_dentry(inode->i_sb->s_root);
	unlock_super(inode->i_sb);

	KFREE(new_hidden_mnt);
	KFREE(new_udi_dentry);
	KFREE(new_uii_inode);
	KFREE(new_usi_sb);
	KFREE(new_counts);
	KFREE(new_branchperms);
	KFREE(addargs);
	if (path)
		putname(path);

	print_exit_status(err);

	return err;
}

/* This must be called with the super block already locked. */
int unionfs_ioctl_delbranch(struct super_block *sb, unsigned long arg)
{
	struct dentry *hidden_dentry;
	struct inode *hidden_inode;
	struct super_block *hidden_sb;
	struct vfsmount *hidden_mnt;
	struct dentry *root_dentry;
	struct inode *root_inode;
	int err = 0;
	int pmindex, i, gen;

	print_entry("branch = %lu ", arg);
	lock_dentry(sb->s_root);

	err = -EBUSY;
	if (sbmax(sb) == 1)
		goto out;
	err = -EINVAL;
	if (arg < 0 || arg > stopd(sb)->b_end)
		goto out;
	err = -EBUSY;
	if (branch_count(sb, arg))
		goto out;

	if ((err = newputmap(sb)))
		goto out;

	pmindex = stopd(sb)->usi_lastputmap;
	pmindex -= stopd(sb)->usi_firstputmap;

	atomic_inc(&stopd(sb)->usi_generation);
	gen = atomic_read(&stopd(sb)->usi_generation);

	root_dentry = sb->s_root;
	root_inode = sb->s_root->d_inode;

	hidden_dentry = dtohd_index(root_dentry, arg);
	hidden_mnt = stohiddenmnt_index(sb, arg);
	hidden_inode = itohi_index(root_inode, arg);
	hidden_sb = stohs_index(sb, arg);

	DPUT(hidden_dentry);
	iput(hidden_inode);
	mntput(hidden_mnt);

	for (i = arg; i <= (sbend(sb) - 1); i++) {
		set_branch_count(sb, i, branch_count(sb, i + 1));
		set_stohiddenmnt_index(sb, i, stohiddenmnt_index(sb, i + 1));
		set_stohs_index(sb, i, stohs_index(sb, i + 1));
		set_branchperms(sb, i, branchperms(sb, i + 1));
		set_dtohd_index(root_dentry, i,
				dtohd_index(root_dentry, i + 1));
		set_itohi_index(root_inode, i, itohi_index(root_inode, i + 1));
		stopd(sb)->usi_putmaps[pmindex]->map[i + 1] = i;
	}

	set_dtohd_index(root_dentry, sbend(sb), NULL);
	set_itohi_index(root_inode, sbend(sb), NULL);
	set_stohiddenmnt_index(sb, sbend(sb), NULL);
	set_stohs_index(sb, sbend(sb), NULL);

	stopd(sb)->b_end--;
	set_dbend(root_dentry, dbend(root_dentry) - 1);
	dtopd(root_dentry)->udi_bcount--;
	itopd(root_inode)->b_end--;

	atomic_set(&dtopd(root_dentry)->udi_generation, gen);
	atomic_set(&itopd(root_inode)->uii_generation, gen);

	fixputmaps(sb);

	/* This doesn't open a file, so we might have to free the map here. */
	if (atomic_read(&stopd(sb)->usi_putmaps[pmindex]->count) == 0) {
		KFREE(stopd(sb)->usi_putmaps[pmindex]);
		stopd(sb)->usi_putmaps[pmindex] = NULL;
	}

      out:
	unlock_dentry(sb->s_root);
	print_exit_status(err);

	return err;
}

int unionfs_ioctl_rdwrbranch(struct inode *inode, unsigned int cmd,
			     unsigned long arg)
{
	int err;
	struct unionfs_rdwrbranch_args *rdwrargs = NULL;
	int gen;

	print_entry_location();

	lock_super(inode->i_sb);
	lock_dentry(inode->i_sb->s_root);

	if ((err = newputmap(inode->i_sb)))
		goto out;

	err = -ENOMEM;
	rdwrargs = KMALLOC(sizeof(struct unionfs_rdwrbranch_args), GFP_UNIONFS);
	if (!rdwrargs)
		goto out;

	err = -EFAULT;
	if (copy_from_user
	    (rdwrargs, (void *)arg, sizeof(struct unionfs_rdwrbranch_args)))
		goto out;

	err = -EINVAL;
	if (rdwrargs->rwb_branch < 0
	    || (rdwrargs->rwb_branch > (sbend(inode->i_sb) + 1)))
		goto out;
	if (rdwrargs->rwb_perms & ~(MAY_READ | MAY_WRITE))
		goto out;
	if (!(rdwrargs->rwb_perms & MAY_READ))
		goto out;

	set_branchperms(inode->i_sb, rdwrargs->rwb_branch, rdwrargs->rwb_perms);

	atomic_inc(&stopd(inode->i_sb)->usi_generation);
	gen = atomic_read(&stopd(inode->i_sb)->usi_generation);
	atomic_set(&dtopd(inode->i_sb->s_root)->udi_generation, gen);
	atomic_set(&itopd(inode->i_sb->s_root->d_inode)->uii_generation, gen);

	err = 0;

      out:
	unlock_super(inode->i_sb);
	unlock_dentry(inode->i_sb->s_root);
	KFREE(rdwrargs);

	print_exit_status(err);

	return err;
}

int unionfs_ioctl_queryfile(struct file *file, unsigned int cmd,
			    unsigned long arg)
{
	int err = 0;
	fd_set branchlist;

	int bstart = 0, bend = 0, bindex = 0;
	struct dentry *dentry, *hidden_dentry;

	print_entry_location();

	dentry = file->f_dentry;
	lock_dentry(dentry);
	if ((err = unionfs_partial_lookup(dentry)))
		goto out;
	bstart = dbstart(dentry);
	bend = dbend(dentry);

	FD_ZERO(&branchlist);

	for (bindex = bstart; bindex <= bend; bindex++) {
		hidden_dentry = dtohd_index(dentry, bindex);
		if (!hidden_dentry)
			continue;
		if (hidden_dentry->d_inode)
			FD_SET(bindex, &branchlist);
	}

	err = copy_to_user((void *)arg, &branchlist, sizeof(fd_set));
	if (err) {
		err = -EFAULT;
		goto out;
	}

      out:
	unlock_dentry(dentry);
	err = err < 0 ? err : bend;
	print_exit_status(err);
	return (err);
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

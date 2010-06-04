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

/* The inode cache is used with alloc_inode for both our inode info and the
 * vfs inode.  */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
static kmem_cache_t *unionfs_inode_cachep;
#endif

static void unionfs_read_inode(struct inode *inode)
{
	static struct address_space_operations unionfs_empty_aops;

	print_entry_location();

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
	ASSERT(sizeof(struct unionfs_inode_info) <
	       sizeof(inode->u) - sizeof(void *));
	itopd_lhs(inode) = (&(inode->u.generic_ip) + 1);
#endif
	if (!itopd(inode)) {
		FISTBUG
		    ("No kernel memory when allocating inode private data!\n");
	}

	PASSERT(inode->i_sb);
	memset(itopd(inode), 0, sizeof(struct unionfs_inode_info));
	itopd(inode)->b_start = -1;
	itopd(inode)->b_end = -1;
	atomic_set(&itopd(inode)->uii_generation,
		   atomic_read(&stopd(inode->i_sb)->usi_generation));
	itopd(inode)->uii_rdlock = SPIN_LOCK_UNLOCKED;
	itopd(inode)->uii_rdcount = 1;
	itopd(inode)->uii_hashsize = -1;
	INIT_LIST_HEAD(&itopd(inode)->uii_readdircache);

	if (sbmax(inode->i_sb) > UNIONFS_INLINE_OBJECTS) {
		int size =
		    (sbmax(inode->i_sb) -
		     UNIONFS_INLINE_OBJECTS) * sizeof(struct inode *);
		itohi_ptr(inode) = KMALLOC(size, GFP_UNIONFS);
		if (!itohi_ptr(inode)) {
			FISTBUG
			    ("No kernel memory when allocating lower-pointer array!\n");
		}
		memset(itohi_ptr(inode), 0, size);
	}
	memset(itohi_inline(inode), 0,
	       UNIONFS_INLINE_OBJECTS * sizeof(struct inode *));

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
	inode->i_version = ++event;	/* increment inode version */
#else
	inode->i_version++;
#endif
	inode->i_op = &unionfs_main_iops;
	inode->i_fop = &unionfs_main_fops;
	/* I don't think ->a_ops is ever allowed to be NULL */
	inode->i_mapping->a_ops = &unionfs_empty_aops;
	fist_dprint(7, "setting inode 0x%p a_ops to empty (0x%p)\n",
		    inode, inode->i_mapping->a_ops);

	print_exit_location();
}

static void unionfs_put_inode(struct inode *inode)
{
	print_entry_location();
	fist_dprint(8, "%s i_count = %d, i_nlink = %d\n", __FUNCTION__,
		    atomic_read(&inode->i_count), inode->i_nlink);
	/*
	 * This is really funky stuff:
	 * Basically, if i_count == 1, iput will then decrement it and this inode will be destroyed.
	 * It is currently holding a reference to the hidden inode.
	 * Therefore, it needs to release that reference by calling iput on the hidden inode.
	 * iput() _will_ do it for us (by calling our clear_inode), but _only_ if i_nlink == 0.
	 * The problem is, NFS keeps i_nlink == 1 for silly_rename'd files.
	 * So we must for our i_nlink to 0 here to trick iput() into calling our clear_inode.
	 */
	if (atomic_read(&inode->i_count) == 1)
		inode->i_nlink = 0;
	print_exit_location();
}

/*
 * we now define delete_inode, because there are two VFS paths that may
 * destroy an inode: one of them calls clear inode before doing everything
 * else that's needed, and the other is fine.  This way we truncate the inode
 * size (and its pages) and then clear our own inode, which will do an iput
 * on our and the lower inode.
 */
static void unionfs_delete_inode(struct inode *inode)
{
	print_entry_location();

	fist_checkinode(inode, "unionfs_delete_inode IN");
	inode->i_size = 0;	/* every f/s seems to do that */
	clear_inode(inode);

	print_exit_location();
}

/* final actions when unmounting a file system */
static void unionfs_put_super(struct super_block *sb)
{
	int bindex, bstart, bend;
	struct unionfs_sb_info *spd;

	print_entry_location();

	if ((spd = stopd(sb))) {
		/* XXX: Free persistent inode stuff. */

		bstart = sbstart(sb);
		bend = sbend(sb);
		for (bindex = bstart; bindex <= bend; bindex++)
			mntput(stohiddenmnt_index(sb, bindex));

		/* Make sure we have no leaks of branchget/branchput. */
		for (bindex = bstart; bindex <= bend; bindex++)
			ASSERT(branch_count(sb, bindex) == 0);

		KFREE(stohs_ptr(sb));
		KFREE(stohiddenmnt_ptr(sb));
		KFREE(spd->usi_sbcount_p);
		KFREE(spd->usi_branchperms_p);
		KFREE(spd->usi_putmaps);
		KFREE(spd);
		stopd_lhs(sb) = NULL;
	}
	fist_dprint(6, "unionfs: released super\n");

	print_exit_location();
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
static int unionfs_statfs(struct super_block *sb, struct statfs *buf)
#else
static int unionfs_statfs(struct super_block *sb, struct kstatfs *buf)
#endif
{
	int err = 0;
	struct super_block *hidden_sb;
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
	struct statfs rsb;
#else
	struct kstatfs rsb;
#endif
	int bindex, bindex1, bstart, bend;

	print_entry_location();
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
	memset(buf, 0, sizeof(struct statfs));
#else
	memset(buf, 0, sizeof(struct kstatfs));
#endif
	buf->f_type = UNIONFS_SUPER_MAGIC;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
	buf->f_frsize = 0;
#endif
	buf->f_namelen = 0;

	bstart = sbstart(sb);
	bend = sbend(sb);

	for (bindex = bstart; bindex <= bend; bindex++) {
		int dup = 0;

		hidden_sb = stohs_index(sb, bindex);
		/* Ignore duplicate super blocks. */
		for (bindex1 = bstart; bindex1 < bindex; bindex1++) {
			if (hidden_sb == stohs_index(sb, bindex1)) {
				dup = 1;
				break;
			}
		}
		if (dup) {
			continue;
		}

		err = vfs_statfs(hidden_sb, &rsb);
		fist_dprint(8,
			    "adding values for bindex:%d bsize:%d blocks:%d bfree:%d bavail:%d\n",
			    bindex, (int)rsb.f_bsize, (int)rsb.f_blocks,
			    (int)rsb.f_bfree, (int)rsb.f_bavail);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
		if (!buf->f_frsize)
			buf->f_frsize = rsb.f_frsize;
#endif
		if (!buf->f_namelen) {
			buf->f_namelen = rsb.f_namelen;
		} else {
			if (buf->f_namelen > rsb.f_namelen)
				buf->f_namelen = rsb.f_namelen;
		}
		if (!buf->f_bsize) {
			buf->f_bsize = rsb.f_bsize;
		} else {
			if (buf->f_bsize < rsb.f_bsize) {
				int shifter = 0;
				while (buf->f_bsize < rsb.f_bsize) {
					shifter++;
					rsb.f_bsize >>= 1;
				}
				rsb.f_blocks <<= shifter;
				rsb.f_bfree <<= shifter;
				rsb.f_bavail <<= shifter;
			} else {
				int shifter = 0;
				while (buf->f_bsize > rsb.f_bsize) {
					shifter++;
					rsb.f_bsize <<= 1;
				}
				rsb.f_blocks >>= shifter;
				rsb.f_bfree >>= shifter;
				rsb.f_bavail >>= shifter;
			}
		}
		buf->f_blocks += rsb.f_blocks;
		buf->f_bfree += rsb.f_bfree;
		buf->f_bavail += rsb.f_bavail;
		buf->f_files += rsb.f_files;
		buf->f_ffree += rsb.f_ffree;
	}
	//DQ: we subtract 4 here since we can add .wh. to any file we want for whiteout 
	//files so we need to make sure there is room for it.
	buf->f_namelen -= 4;

	memset(&buf->f_fsid, 0, sizeof(__kernel_fsid_t));
	//DQ: The 6 and 5 in the below code are constants defined in the statfs struct. 
	//In 2.6 there is only room for 4 more variables in the struct while in 2.4
	//there is room for 5.
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
	memset(&buf->f_spare, 0, sizeof(long) * 6);
#else
	memset(&buf->f_spare, 0, sizeof(long) * 5);
#endif
	print_exit_status(err);
	return err;
}

static int do_binary_remount(struct super_block *sb, int *flags, char *data)
{
	unsigned long *uldata = (unsigned long *)data;
	int err;

	uldata++;

	switch (*uldata) {
	case UNIONFS_IOCTL_DELBRANCH:
		err = unionfs_ioctl_delbranch(sb, *(uldata + 1));
		break;
	default:
		err = -ENOTTY;
	}

	return err;
}

/* We don't support a standard text remount, but we do have a magic remount
 * for unionctl.  The idea is that you can remove a branch without opening
 * the union.  Eventually it would be nice to support a full-on remount, so 
 * that you can have all of the directories change at once, but that would
 * require some pretty complicated matching code. */
static int unionfs_remount_fs(struct super_block *sb, int *flags, char *data)
{
	if (data && *((unsigned long *)data) == UNIONFS_REMOUNT_MAGIC)
		return do_binary_remount(sb, flags, data);
	return -ENOSYS;
}

/*
 * Called by iput() when the inode reference count reached zero
 * and the inode is not hashed anywhere.  Used to clear anything
 * that needs to be, before the inode is completely destroyed and put
 * on the inode free list.
 */
static void unionfs_clear_inode(struct inode *inode)
{
	int bindex, bstart, bend;
	struct inode *hidden_inode;
	struct list_head *pos, *n;
	struct unionfs_dir_state *rdstate;

	print_entry_location();

	fist_checkinode(inode, "unionfs_clear_inode IN");

	list_for_each_safe(pos, n, &itopd(inode)->uii_readdircache) {
		rdstate = list_entry(pos, struct unionfs_dir_state, uds_cache);
		list_del(&rdstate->uds_cache);
		free_rdstate(rdstate);
	}

	/* Decrement a reference to a hidden_inode, which was incremented
	 * by our read_inode when it was created initially.  */
	bstart = ibstart(inode);
	bend = ibend(inode);
	if (bstart >= 0) {
		for (bindex = bstart; bindex <= bend; bindex++) {
			hidden_inode = itohi_index(inode, bindex);
			if (!hidden_inode)
				continue;
			iput(hidden_inode);
		}
	}
	// XXX: why this assertion fails?
	// because it doesn't like us
	// ASSERT((inode->i_state & I_DIRTY) == 0);
	KFREE(itohi_ptr(inode));
	itohi_ptr(inode) = NULL;
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
	itopd_lhs(inode) = NULL;
#endif

	print_exit_location();
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
static struct inode *unionfs_alloc_inode(struct super_block *sb)
{
	struct unionfs_inode_container *c;

	print_entry_location();

	c = (struct unionfs_inode_container *)
	    kmem_cache_alloc(unionfs_inode_cachep, SLAB_KERNEL);
	if (!c) {
		print_exit_pointer(NULL);
		return NULL;
	}

	memset(&c->info, 0, sizeof(c->info));

	c->vfs_inode.i_version = 1;
	print_exit_pointer(&c->vfs_inode);
	return &c->vfs_inode;
}

static void unionfs_destroy_inode(struct inode *inode)
{
	print_entry("inode = %p", inode);
	kmem_cache_free(unionfs_inode_cachep, itopd(inode));
	print_exit_location();
}

static void init_once(void *v, kmem_cache_t * cachep, unsigned long flags)
{
	struct unionfs_inode_container *c = (struct unionfs_inode_container *)v;

	print_entry_location();

	if ((flags & (SLAB_CTOR_VERIFY | SLAB_CTOR_CONSTRUCTOR)) ==
	    SLAB_CTOR_CONSTRUCTOR)
		inode_init_once(&c->vfs_inode);

	print_exit_location();
}

int init_inode_cache()
{
	int err = 0;

	print_entry_location();

	unionfs_inode_cachep =
	    kmem_cache_create("unionfs_inode_cache",
			      sizeof(struct unionfs_inode_container), 0,
			      SLAB_RECLAIM_ACCOUNT, init_once, NULL);
	if (!unionfs_inode_cachep)
		err = -ENOMEM;
	print_exit_status(err);
	return err;
}

void destroy_inode_cache()
{
	print_entry_location();
	if (!unionfs_inode_cachep)
		goto out;
	if (kmem_cache_destroy(unionfs_inode_cachep))
		printk(KERN_ERR
		       "unionfs_inode_cache: not all structures were freed\n");
      out:
	print_exit_location();
	return;
}
#else
int init_inode_cache()
{
	print_entry_location();
	print_exit_status(0);
	return 0;
}

void destroy_inode_cache()
{
	print_entry_location();
	print_exit_location();
}
#endif

/* Called when we have a dirty inode, right here we only throw out
 * parts of our readdir list that are too old.
 */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,9)
static int unionfs_write_inode(struct inode *inode, int sync)
{
#else
static void unionfs_write_inode(struct inode *inode, int sync)
{
#endif
	struct list_head *pos, *n;
	struct unionfs_dir_state *rdstate;

	print_entry_location();

	PASSERT(inode);
	PASSERT(itopd(inode));
	spin_lock(&itopd(inode)->uii_rdlock);
	list_for_each_safe(pos, n, &itopd(inode)->uii_readdircache) {
		rdstate = list_entry(pos, struct unionfs_dir_state, uds_cache);
		/* We keep this list in LRU order. */
		if ((rdstate->uds_access + RDCACHE_JIFFIES) > jiffies)
			break;
		itopd(inode)->uii_rdcount--;
		list_del(&rdstate->uds_cache);
		free_rdstate(rdstate);
	}
	spin_unlock(&itopd(inode)->uii_rdlock);

	print_exit_location();
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,9)
	return 0;
#endif
}

/*
 * Called in do_umount() if the MNT_FORCE flag was used and this
 * function is defined.  See comment in linux/fs/super.c:do_umount().
 * Used only in nfs, to kill any pending RPC tasks, so that subsequent
 * code can actually succeed and won't leave tasks that need handling.
 *
 * PS. I wonder if this is somehow useful to undo damage that was
 * left in the kernel after a user level file server (such as amd)
 * dies.
 */
static void unionfs_umount_begin(struct super_block *sb)
{
	struct super_block *hidden_sb;
	int bindex, bstart, bend;

	print_entry_location();

	bstart = sbstart(sb);
	bend = sbend(sb);
	for (bindex = bstart; bindex <= bend; bindex++) {
		hidden_sb = stohs_index(sb, bindex);
		if (hidden_sb && hidden_sb->s_op &&
		    hidden_sb->s_op->umount_begin)
			hidden_sb->s_op->umount_begin(hidden_sb);
	}
	print_exit_location();
}

static int unionfs_show_options(struct seq_file *m, struct vfsmount *mnt)
{
	struct super_block *sb = mnt->mnt_sb;
	int ret = 0;
	unsigned long tmp = 0;
	char *hidden_path;
	int bindex, bstart, bend;
	int perms;

	lock_dentry(sb->s_root);

	tmp = __get_free_page(GFP_UNIONFS);
	if (!tmp) {
		ret = -ENOMEM;
		goto out;
	}

	bindex = bstart = sbstart(sb);
	bend = sbend(sb);

	seq_printf(m, ",dirs=");
	for (bindex = bstart; bindex <= bend; bindex++) {
		hidden_path =
		    d_path(dtohd_index(sb->s_root, bindex),
			   stohiddenmnt_index(sb, bindex), (char *)tmp,
			   PAGE_SIZE);
		perms = branchperms(sb, bindex);
		seq_printf(m, "%s=%s", hidden_path,
			   perms & MAY_WRITE ? "rw" : "ro");
		if (bindex != bend) {
			seq_printf(m, ":");
		}
	}

	seq_printf(m, ",debug=%d", fist_get_debug_value());

	if (IS_SET(sb, DELETE_WHITEOUT))
		seq_printf(m, ",delete=whiteout");
	else
		seq_printf(m, ",delete=all");

	if (IS_SET(sb, COPYUP_CURRENT_USER))
		seq_printf(m, ",copyup=currentuser");
	else if (IS_SET(sb, COPYUP_FS_MOUNTER))
		seq_printf(m, ",copyup=mounter");
	else
		seq_printf(m, ",copyup=preserve");
      out:
	if (tmp)
		free_page(tmp);
	unlock_dentry(sb->s_root);
	return ret;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
/**
 * The entry given by dentry here is always a directory.
 * We can't just do dentry->d_parent, because it may
 * not be there, since this dentry could have been
 * created by calling d_alloc_anon.
 */
static struct dentry *unionfs_get_parent(struct dentry *dentry)
{

	struct dentry *ret;
	struct inode *childinode;
	childinode = dentry->d_inode;
	ret = d_find_alias(childinode);
	return ret;
}

struct export_operations unionfs_export_ops = {
	.get_parent = unionfs_get_parent
};
#endif

struct super_operations unionfs_sops = {
	.read_inode = unionfs_read_inode,
	.put_inode = unionfs_put_inode,
	.delete_inode = unionfs_delete_inode,
	.put_super = unionfs_put_super,
	.statfs = unionfs_statfs,
	.remount_fs = unionfs_remount_fs,
	.clear_inode = unionfs_clear_inode,
	.umount_begin = unionfs_umount_begin,
	.show_options = unionfs_show_options,
	.write_inode = unionfs_write_inode,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
	.alloc_inode = unionfs_alloc_inode,
	.destroy_inode = unionfs_destroy_inode,
#endif
#ifdef SPLIT_VIEW_CACHES
	.select_super = unionfs_select_super,
#endif
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

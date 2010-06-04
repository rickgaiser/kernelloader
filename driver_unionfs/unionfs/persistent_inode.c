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
#include <linux/kdev_t.h>
#endif

/*
 * verify_forwardmap(super_block *sb)
 * sb: pointer to a superblock containing the forwardmap.
 * returns: 0 on success EINVAL or ENOMEM on failure;
 */
static int verify_forwardmap(struct super_block *sb)
{
	int err = 0, bytesread = 0, bindex = 0, mallocsize = 0, i = 0;
	loff_t readpos = 0;
	struct file *forwardmap = NULL;
	struct fmaphdr header;
	mm_segment_t oldfs;
	print_entry_location();

	forwardmap = stopd(sb)->usi_forwardmap;
	if (!forwardmap) {
		err = -EINVAL;
		goto out;
	}
	oldfs = get_fs();
	set_fs(KERNEL_DS);
	bytesread =
	    forwardmap->f_op->read(forwardmap, (char *)&header,
				   sizeof(struct fmaphdr), &readpos);
	set_fs(oldfs);
	if (bytesread < sizeof(struct fmaphdr)) {
		err = -EINVAL;
		goto out;
	}
	if (header.magic != FORWARDMAP_MAGIC
	    || header.version != FORWARDMAP_VERSION) {
		err = -EINVAL;
		goto out;
	}
	stopd(sb)->usi_bmap =
	    KMALLOC(sizeof(struct bmapent) * header.usedbranches, GFP_UNIONFS);
	stopd(sb)->usi_num_bmapents = header.usedbranches;
	if (!stopd(sb)->usi_bmap) {
		err = -ENOMEM;
		goto out;
	}
	oldfs = get_fs();
	set_fs(KERNEL_DS);
	while (bindex < header.usedbranches) {
		bytesread =
		    forwardmap->f_op->read(forwardmap,
					   (char *)&stopd(sb)->usi_bmap[bindex],
					   sizeof(struct bmapent), &readpos);
		if (bytesread < sizeof(struct bmapent)) {
			err = -EINVAL;
			set_fs(oldfs);
			goto out_err;
		}
		bindex++;
	}
	set_fs(oldfs);
	mallocsize = sizeof(int) * header.usedbranches;
	stopd(sb)->usi_fsnum_table = KMALLOC(mallocsize, GFP_UNIONFS);
	if (!stopd(sb)->usi_fsnum_table) {
		err = -ENOMEM;
		goto out_err;
	}
	for (i = 0; i < header.usedbranches; i++) {
		stopd(sb)->usi_fsnum_table[i] = -1;
	}
	goto out;
      out_err:
	if (stopd(sb)->usi_bmap) {
		KFREE(stopd(sb)->usi_bmap);
	}
      out:
	print_exit_status(err);
	return err;
}

/* 
 * verify_reversemap(struct super_block sb, int rmapindex)
 *
 * sb: The unionfs superblock containing all of the current imap info
 * rmapindex: the index in the usi_reversemaps array that we wish to 
 * verify
 *
 * Assumes the reverse maps less than rmapindex are valid.
 * 
 * returns: 0 if the opperation succeds
 * 	-EINVAL if the map file does not belong to the forward map
 *
 */
static int verify_reversemap(struct super_block *sb, int rmapindex,
			     struct unionfs_dentry_info *hidden_root_info)
{
	int err = 0, i = 0, bindex = 0, found = 0, bytesread;
	loff_t readpos = 0;
	struct file *forwardmap, *reversemap;
	struct fmaphdr fheader;
	struct rmaphdr rheader;
	mm_segment_t oldfs;
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
	struct statfs st;
#else
	struct kstatfs st;
#endif

	print_entry_location();
	forwardmap = stopd(sb)->usi_forwardmap;
	if (!forwardmap) {
		err = -EINVAL;
		goto out;
	}
	reversemap = stopd(sb)->usi_reversemaps[rmapindex];
	if (!reversemap) {
		err = -EINVAL;
		goto out;
	}
	oldfs = get_fs();
	set_fs(KERNEL_DS);
	bytesread =
	    forwardmap->f_op->read(forwardmap, (char *)&fheader,
				   sizeof(struct fmaphdr), &readpos);
	if (bytesread < sizeof(struct fmaphdr)) {
		set_fs(oldfs);
		err = -EINVAL;
		goto out;
	}
	readpos = 0;
	bytesread =
	    reversemap->f_op->read(reversemap, (char *)&rheader,
				   sizeof(struct rmaphdr), &readpos);
	if (bytesread < sizeof(struct rmaphdr)) {
		set_fs(oldfs);
		err = -EINVAL;
		goto out;
	}
	set_fs(oldfs);
	if (rheader.magic != REVERSEMAP_MAGIC
	    || rheader.version != REVERSEMAP_VERSION) {
		err = -EINVAL;
		goto out;
	}
	if (memcmp(fheader.uuid, rheader.fwduuid, sizeof(fheader.uuid))) {
		err = -EINVAL;
		goto out;
	}

	/* XXX: Ok so here we take the new map and read the fsid from it. Then
	 * we go through all the branches in the union and see which ones it
	 * matches with*/
	for (i = 0; i < stopd(sb)->usi_num_bmapents && !found; i++) {
		if (memcmp
		    (rheader.revuuid, stopd(sb)->usi_bmap[i].uuid,
		     sizeof(rheader.revuuid))) {
			found = 1;
			for (bindex = 0; bindex <= hidden_root_info->udi_bend;
			     bindex++) {
				struct dentry *d;
				fsid_t fsid;
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
				kdev_t dev;
				memset(&st, 0, sizeof(struct statfs));
#else
				dev_t dev;
				memset(&st, 0, sizeof(struct kstatfs));
#endif
				if (bindex < UNIONFS_INLINE_OBJECTS)
					d = hidden_root_info->
					    udi_dentry_i[bindex];
				else
					d = hidden_root_info->
					    udi_dentry_p[bindex -
							 UNIONFS_INLINE_OBJECTS];
				err = d->d_sb->s_op->statfs(d->d_sb, &st);
				if (err) {
					goto out;
				}
				if (st.f_fsid.val[0] || st.f_fsid.val[1]) {
					fsid = st.f_fsid;
				} else {
					dev = d->d_sb->s_dev;
					fsid.val[0] = MAJOR(dev);
					fsid.val[1] = MINOR(dev);
				}
				if (!memcmp(&fsid, &rheader.fsid, sizeof(fsid))) {
					if (stopd(sb)->usi_fsnum_table[i] == -1)
						stopd(sb)->usi_fsnum_table[i] =
						    bindex;
					if (stopd(sb)->usi_bnum_table[bindex] ==
					    -1)
						stopd(sb)->
						    usi_bnum_table[bindex] = i;
					if (stopd(sb)->usi_map_table[bindex]) {
						printk(KERN_WARNING
						       "Two reverse maps share fsid %u%u!\n",
						       rheader.fsid.val[0],
						       rheader.fsid.val[1]);
						err = -EINVAL;
						goto out;
					} else {
						stopd(sb)->
						    usi_map_table[bindex] =
						    reversemap;

					}
				}
			}
		}
	}
	if (!found) {
		printk(KERN_WARNING
		       "Could not match the reversemap uuid with an entry in the forwardmap table\n");
		err = -EINVAL;
	}
      out:
	print_exit_status(err);
	return err;
}

int parse_imap_option(struct super_block *sb,
		      struct unionfs_dentry_info *hidden_root_info,
		      char *options)
{
	int mallocsize, count = 0, err = 0, i = 0;
	char *name;

	print_entry_location();

	//Make sure our file pointers are set to null initially
	stopd(sb)->usi_forwardmap = NULL;
	stopd(sb)->usi_reversemaps = NULL;
	stopd(sb)->usi_fsnum_table = NULL;
	stopd(sb)->usi_bnum_table = NULL;

	mallocsize = sizeof(struct file *) * (hidden_root_info->udi_bend + 1);
	stopd(sb)->usi_reversemaps = KMALLOC(mallocsize, GFP_UNIONFS);
	if (IS_ERR(stopd(sb)->usi_reversemaps)) {
		err = -ENOMEM;
		goto out_error;
	}
	memset(stopd(sb)->usi_reversemaps, 0, mallocsize);
	stopd(sb)->usi_map_table = KMALLOC(mallocsize, GFP_UNIONFS);
	if (IS_ERR(stopd(sb)->usi_map_table)) {
		err = -ENOMEM;
		goto out_error;
	}
	memset(stopd(sb)->usi_map_table, 0, mallocsize);
	mallocsize = sizeof(int) * (hidden_root_info->udi_bend + 1);
	stopd(sb)->usi_bnum_table = KMALLOC(mallocsize, GFP_UNIONFS);
	if (IS_ERR(stopd(sb)->usi_bnum_table)) {
		err = -ENOMEM;
		goto out_error;
	}

	for (i = 0; i <= hidden_root_info->udi_bend; i++) {
		stopd(sb)->usi_bnum_table[i] = -1;
	}
	while ((name = strsep(&options, ":")) != NULL) {
		if (!*name)
			continue;
		if (!stopd(sb)->usi_forwardmap) {
			stopd(sb)->usi_forwardmap = filp_open(name, O_RDWR, 0);
			if (IS_ERR(stopd(sb)->usi_forwardmap)) {
				err = PTR_ERR(stopd(sb)->usi_forwardmap);
				stopd(sb)->usi_forwardmap = NULL;
				goto out_error;
			}
			err = verify_forwardmap(sb);
			if (err)
				goto out_error;
		} else {
			stopd(sb)->usi_reversemaps[count] =
			    filp_open(name, O_RDWR, 0);
			if (IS_ERR(stopd(sb)->usi_reversemaps[count])) {
				err =
				    PTR_ERR(stopd(sb)->usi_reversemaps[count]);
				stopd(sb)->usi_reversemaps[count] = NULL;
				goto out_error;

			}
			count++;
			err =
			    verify_reversemap(sb, count - 1, hidden_root_info);
			if (err)
				goto out_error;
		}
	}
	if (count <= 0) {
		printk(KERN_WARNING "unionfs: no reverse maps specified.\n");
		err = -EINVAL;
	}
	if (err)
		goto out_error;

	/* Initialize the super block's next_avail field */
	/* Dave, you can't use 64-bit division here because the i386 doesn't
	 * support it natively.  Instead you need to punt if the size is
	 * greater than unsigned long, and then cast it down.  Then you should
	 * be able to assign to this value, without having these problems. */

	if (stopd(sb)->usi_forwardmap->f_dentry->d_inode->i_size > ULONG_MAX) {
		err = -EFBIG;
		goto out_error;
	}
	stopd(sb)->usi_next_avail =
	    ((unsigned long)(stopd(sb)->usi_forwardmap->f_dentry->d_inode->
			     i_size - (sizeof(struct fmaphdr) +
				       sizeof(struct bmapent[256])))
	     / sizeof(struct fmapent));
	if (stopd(sb)->usi_next_avail <= 2)
		stopd(sb)->usi_next_avail = 3;	//0 1 and 2 are reserved inode numbers.
	stopd(sb)->usi_num_bmapents = count;
	stopd(sb)->usi_persistent = 1;
	goto out;

      out_error:
	if (stopd(sb)->usi_forwardmap) {
		filp_close(stopd(sb)->usi_forwardmap, NULL);
	}
	while (count - 1 >= 0) {
		if (stopd(sb)->usi_reversemaps[count - 1]) {
			filp_close(stopd(sb)->usi_reversemaps[count - 1], NULL);
		}
		count--;
	}
      out:
	print_exit_status(err);
	return err;
}

/*
 * get_uin(struct super_block *sb, uint8_t branchnum, ino_t inode_number, int flag)
 * fsnum: branch to reference when getting the inode number
 * inode_number: lower level inode number use to reference the proper inode.
 * flag: if set to O_CREAT it will creat the entry if it doesent exist
 * 		 otherwise it will return the existing one.
 * returns: the unionfs inode number either created or retrieved based on
 * 			the information.
 */
ino_t get_uin(struct super_block * sb, uint8_t branchnum, ino_t inode_number,
	      int flag)
{
	struct file *reversemap;
	struct file *forwardmap;
	struct fmapent tmp_ent;
	mm_segment_t oldfs;
	loff_t seekpos = 0;
	loff_t isize = 0;
	int bytesread = 0, byteswritten = 0;
	ino_t err = 0;
	uint64_t uino = 0;

	print_entry_location();
	/* Make sure we have a valid super block */
	PASSERT(sb);
	PASSERT(stopd(sb)->usi_forwardmap);
	PASSERT(stopd(sb)->usi_reversemaps);
	/* Find appropriate reverse map and then read from the required position */
	/* get it from the array. */

	reversemap = stopd(sb)->usi_map_table[branchnum];
	seekpos = sizeof(struct rmaphdr) + (sizeof(uint64_t) * inode_number);
	isize = reversemap->f_dentry->d_inode->i_size;
	if (seekpos > isize) {
		if (flag & O_CREAT) {
			goto create;
		} else {
			err = -ENOENT;
			goto out;
		}
	}
	oldfs = get_fs();
	set_fs(KERNEL_DS);
	bytesread =
	    reversemap->f_op->read(reversemap, (char *)&uino, sizeof(uint64_t),
				   &seekpos);
	set_fs(oldfs);
	if (bytesread != sizeof(uint64_t)) {
		err = -EIO;
		goto out;
	}
	/* If we haven't found an entry and we have the O_CREAT flag set we want to 
	 * create a new entry write it out to the file and return its index
	 */
      create:
	if (uino <= 0 && (flag & O_CREAT)) {
		forwardmap = stopd(sb)->usi_forwardmap;
		tmp_ent.fsnum = stopd(sb)->usi_bnum_table[branchnum];
		tmp_ent.inode = inode_number;
		seekpos =
		    sizeof(struct fmaphdr) + sizeof(struct bmapent[256]) +
		    (sizeof(struct fmapent) * stopd(sb)->usi_next_avail);
		oldfs = get_fs();
		set_fs(KERNEL_DS);
		byteswritten =
		    forwardmap->f_op->write(forwardmap, (char *)&tmp_ent,
					    sizeof(struct fmapent), &seekpos);
		set_fs(oldfs);
		if (byteswritten != sizeof(struct fmapent)) {
			err = -EIO;
			goto out;
		}
		uino = stopd(sb)->usi_next_avail++;

		seekpos =
		    sizeof(struct rmaphdr) + (sizeof(uint64_t) * inode_number);
		oldfs = get_fs();
		set_fs(KERNEL_DS);

		byteswritten =
		    reversemap->f_op->write(reversemap, (char *)&uino,
					    sizeof(uint64_t), &seekpos);
		set_fs(oldfs);
		if (byteswritten != sizeof(uint64_t)) {
			err = -EIO;
			goto out;
		}
	} else if (uino >= 3)
		goto out;
	else
		err = -ENOENT;
      out:
	if (!err)
		err = uino;

	print_exit_status((int)err);
	return err;
}

/*
 * get_lin(ino_t inode_number)
 * inode_number : inode number for the unionfs inode
 * returns: the lower level inode# and branch#
 */
/* entry should use a poiner on the stack. should be staticly allocated one
 * level up*/
int get_lin(struct super_block *sb, ino_t inode_number, struct fmapent *entry)
{
	struct file *forwardmap;
	loff_t seek_size;
	mm_segment_t oldfs;
	int err = 0, bytesread = 0;

	print_entry_location();

	PASSERT(sb);
	if (!entry) {
		entry = ERR_PTR(-ENOMEM);
		goto out;
	}
	forwardmap = stopd(sb)->usi_forwardmap;
	seek_size =
	    sizeof(struct fmaphdr) + sizeof(struct bmapent[256]) +
	    (sizeof(struct fmapent) * inode_number);
	oldfs = get_fs();
	set_fs(KERNEL_DS);
	bytesread =
	    forwardmap->f_op->read(forwardmap, (char *)entry,
				   sizeof(struct fmapent), &seek_size);
	set_fs(oldfs);
	if (bytesread != sizeof(struct fmapent)) {
		entry = ERR_PTR(-EINVAL);
		goto out;
	}

      out:
	print_exit_location();
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

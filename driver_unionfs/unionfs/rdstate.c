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

/* This file contains the routines for maintaining readdir state. */
/* There are two structures here, rdstate which is a hash table
 * of the second structure which is a filldir_node. */

/* This is a kmem_cache_t for filldir nodes, because we allocate a lot of them
 * and they shouldn't waste memory.  If the node has a small name (as defined
 * by the dentry structure), then we use an inline name to preserve kmalloc
 * space. */
static kmem_cache_t *unionfs_filldir_cachep;
int init_filldir_cache()
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
	unionfs_filldir_cachep =
	    kmem_cache_create("unionfs_filldir", sizeof(struct filldir_node), 0,
			      SLAB_RECLAIM_ACCOUNT, NULL, NULL);
#else
	unionfs_filldir_cachep =
	    kmem_cache_create("unionfs_filldir", sizeof(struct filldir_node), 0,
			      0, NULL, NULL);
#endif
	if (!unionfs_filldir_cachep) {
		return -ENOMEM;
	}
	return 0;
}

void destroy_filldir_cache()
{
	if (!unionfs_filldir_cachep)
		return;
	if (kmem_cache_destroy(unionfs_filldir_cachep)) {
		printk(KERN_ERR
		       "unionfs_filldir_cache: not all structures were freed\n");
	}
	return;
}

/* This is a tuning parameter that tells us roughly how big to make the
 * hash table in directory entries per page.  This isn't perfect, but
 * at least we get a hash table size that shouldn't be too overloaded.
 * The following averages are based on my home directory.
 * 14.44693	Overall
 * 12.29	Single Page Directories
 * 117.93	Multi-page directories
 */
#define DENTPAGE 4096
#define DENTPERONEPAGE 12
#define DENTPERPAGE 118
#define MINHASHSIZE 1
int guesstimate_hash_size(struct inode *inode)
{
	struct inode *hidden_inode;
	int bindex;
	int hashsize = MINHASHSIZE;

	if (itopd(inode)->uii_hashsize > 0)
		return itopd(inode)->uii_hashsize;

	for (bindex = ibstart(inode); bindex <= ibend(inode); bindex++) {
		if (!(hidden_inode = itohi_index(inode, bindex)))
			continue;

		if (hidden_inode->i_size == DENTPAGE) {
			hashsize += DENTPERONEPAGE;
		} else {
			hashsize +=
			    (hidden_inode->i_size / DENTPAGE) * DENTPERPAGE;
		}
	}

	return hashsize;
}

int init_rdstate(struct file *file)
{
	ASSERT(sizeof(loff_t) == (sizeof(unsigned int) + sizeof(unsigned int)));
	PASSERT(file);
	ASSERT(ftopd(file)->rdstate == NULL);
	PASSERT(file->f_dentry);
	PASSERT(file->f_dentry->d_inode);

	ftopd(file)->rdstate =
	    alloc_rdstate(file->f_dentry->d_inode, fbstart(file));
	if (!ftopd(file)->rdstate)
		return -ENOMEM;
	return 0;
}

struct unionfs_dir_state *find_rdstate(struct inode *inode, loff_t fpos)
{
	struct unionfs_dir_state *rdstate = NULL;
	struct list_head *pos;

	print_entry("f_pos: %lld", fpos);
	spin_lock(&itopd(inode)->uii_rdlock);
	list_for_each(pos, &itopd(inode)->uii_readdircache) {
		struct unionfs_dir_state *r =
		    list_entry(pos, struct unionfs_dir_state, uds_cache);
		if (fpos == rdstate2offset(r)) {
			itopd(inode)->uii_rdcount--;
			list_del(&r->uds_cache);
			rdstate = r;
			break;
		}
	}
	spin_unlock(&itopd(inode)->uii_rdlock);
	print_exit_pointer(rdstate);
	return rdstate;
}

struct unionfs_dir_state *alloc_rdstate(struct inode *inode, int bindex)
{
	int i = 0;
	int hashsize;
	int mallocsize = sizeof(struct unionfs_dir_state);
	struct unionfs_dir_state *rdstate;

	hashsize = guesstimate_hash_size(inode);
	mallocsize += hashsize * sizeof(struct list_head);
	/* Round it up to the next highest power of two. */
	mallocsize--;
	mallocsize |= mallocsize >> 1;
	mallocsize |= mallocsize >> 2;
	mallocsize |= mallocsize >> 4;
	mallocsize |= mallocsize >> 8;
	mallocsize |= mallocsize >> 16;
	mallocsize++;

	/* This should give us about 500 entries anyway. */
	if (mallocsize > PAGE_SIZE)
		mallocsize = PAGE_SIZE;

	hashsize =
	    (mallocsize -
	     sizeof(struct unionfs_dir_state)) / sizeof(struct list_head);

	rdstate = KMALLOC(mallocsize, GFP_UNIONFS);
	if (!rdstate)
		return NULL;

	spin_lock(&itopd(inode)->uii_rdlock);
	if (itopd(inode)->uii_cookie >= (MAXRDCOOKIE - 1))
		itopd(inode)->uii_cookie = 1;
	else
		itopd(inode)->uii_cookie++;

	rdstate->uds_cookie = itopd(inode)->uii_cookie;
	spin_unlock(&itopd(inode)->uii_rdlock);
	rdstate->uds_offset = 1;
	rdstate->uds_access = jiffies;
	rdstate->uds_bindex = bindex;
	rdstate->uds_dirpos = 0;
	rdstate->uds_hashentries = 0;
	rdstate->uds_size = hashsize;
	for (i = 0; i < rdstate->uds_size; i++)
		INIT_LIST_HEAD(&rdstate->uds_list[i]);

	return rdstate;
}

void free_filldir_node(struct filldir_node *node)
{
	PASSERT(node);
	if (node->namelen >= DNAME_INLINE_LEN_MIN)
		KFREE(node->name);
	kmem_cache_free(unionfs_filldir_cachep, node);
}

void free_rdstate(struct unionfs_dir_state *state)
{
	struct filldir_node *tmp;
	int i;

	PASSERT(state);

	for (i = 0; i < state->uds_size; i++) {
		struct list_head *head = &(state->uds_list[i]);
		struct list_head *pos, *n;

		/* traverse the list and deallocate space */
		list_for_each_safe(pos, n, head) {
			tmp = list_entry(pos, struct filldir_node, file_list);
			list_del(&tmp->file_list);
			free_filldir_node(tmp);
		}
	}

	KFREE(state);
}

struct filldir_node *find_filldir_node(struct unionfs_dir_state *rdstate,
				       const char *name, int namelen)
{
	int index;
	unsigned int hash;
	struct list_head *head;
	struct list_head *pos;
	struct filldir_node *cursor = NULL;
	int found = 0;

	/* If we print entry, we end up with spurious data. */
	/* print_entry("name = %*s", namelen, name); */
	print_entry_location();

	PASSERT(rdstate);
	PASSERT(name);
	ASSERT(namelen > 0);

	hash = full_name_hash(name, namelen);
	index = hash % rdstate->uds_size;

	head = &(rdstate->uds_list[index]);
	list_for_each(pos, head) {
		cursor = list_entry(pos, struct filldir_node, file_list);

		if (cursor->namelen == namelen && cursor->hash == hash
		    && !strncmp(cursor->name, name, namelen)) {
			/* a duplicate exists, and hence no need to create entry to the list */
			found = 1;
			/* if the duplicate is in this branch, then the file system is corrupted. */
			if (cursor->bindex == rdstate->uds_bindex) {
				//buf->error = err = -EIO;
				fist_dprint(8,
					    "Possible I/O error unionfs_filldir: a file is duplicated in the same branch %d: %s\n",
					    rdstate->uds_bindex, cursor->name);
			}
			break;
		}
	}

	if (!found) {
		cursor = NULL;
	}
	print_exit_pointer(cursor);
	return cursor;
}

inline struct filldir_node *alloc_filldir_node(const char *name, int namelen,
					       unsigned int hash, int bindex)
{
	struct filldir_node *newnode;

	newnode =
	    (struct filldir_node *)kmem_cache_alloc(unionfs_filldir_cachep,
						    SLAB_KERNEL);
	if (!newnode)
		goto out;

      out:
	return newnode;
}

int add_filldir_node(struct unionfs_dir_state *rdstate, const char *name,
		     int namelen, int bindex, int whiteout)
{
	struct filldir_node *new;
	unsigned int hash;
	int index;
	int err = 0;
	struct list_head *head;

	/* We can't print this because we end up Oopsing. */
	/* print_entry("name = %*s", namelen, name); */
	print_entry_location();

	PASSERT(rdstate);
	PASSERT(name);
	ASSERT(namelen > 0);

	hash = full_name_hash(name, namelen);
	index = hash % rdstate->uds_size;
	head = &(rdstate->uds_list[index]);

	new = alloc_filldir_node(name, namelen, hash, bindex);
	if (!new) {
		err = -ENOMEM;
		goto out;
	}

	INIT_LIST_HEAD(&new->file_list);
	new->namelen = namelen;
	new->hash = hash;
	new->bindex = bindex;
	new->whiteout = whiteout;

	if (namelen < DNAME_INLINE_LEN_MIN) {
		new->name = new->iname;
	} else {
		new->name = (char *)KMALLOC(namelen + 1, GFP_UNIONFS);
		if (!new->name) {
			kmem_cache_free(unionfs_filldir_cachep, new);
			new = NULL;
			goto out;
		}
	}

	memcpy(new->name, name, namelen);
	new->name[namelen] = '\0';

	rdstate->uds_hashentries++;

	list_add(&(new->file_list), head);
      out:
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

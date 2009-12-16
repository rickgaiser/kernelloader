/*
 * ps2fs/dir.c
 *
 * Copyright (c) 2002 Andrew Church <achurch@achurch.org>
 */

#include <linux/config.h>
#include <linux/version.h>
#include <linux/string.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/locks.h>
#include <linux/blkdev.h>
#include <asm/uaccess.h>

#include "ps2fs_fs.h"
#include "ps2fs_fs_sb.h"
#include "ps2fs_fs_i.h"

/*************************************************************************/

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,4,0)
# define page buffer_head  /* "struct page" -> "struct buffer_head" */
# define page_address(bh) ((bh)->b_data)
# define PAGE_CACHE_SIZE 1024
# define PAGE_CACHE_SHIFT 10
# define PAGE_CACHE_MASK 0xFFFFFC00
# define filldir(dirent,name,namelen,dirofs,inode,type) \
    filldir(dirent,name,namelen,dirofs,inode)
static ssize_t generic_read_dir(struct file *file, char *buf, size_t count,
				loff_t *ppos) { return -EISDIR; }
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,4,0)
static struct dentry *ps2fs_lookup(struct inode *dir, struct dentry *dentry);
#define LOOKUP_RETURN(x) return ERR_PTR(x)
#else  /* < 2.4.0 */
static int ps2fs_lookup(struct inode *dir, struct dentry *dentry);
#define LOOKUP_RETURN(x) return (x)
#endif
static int ps2fs_readdir(struct file *file, void *dirent, filldir_t filldir);
static int ps2fs_create(struct inode *dir, struct dentry *dentry, int mode);
static int ps2fs_unlink(struct inode *dir, struct dentry *dentry);
static int ps2fs_mkdir(struct inode *dir, struct dentry *dentry, int mode);
static int ps2fs_rmdir(struct inode *dir, struct dentry *dentry);

struct file_operations ps2fs_dops = {
    read:	generic_read_dir,
    readdir:	ps2fs_readdir,
};

struct inode_operations ps2fs_dir_iops = {
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,4,0)
    default_file_ops: &ps2fs_dops,
    readpage:	generic_readpage,
    bmap:	ps2fs_bmap,
#endif
    lookup:	ps2fs_lookup,
    create:	ps2fs_create,
    unlink:	ps2fs_unlink,
    mkdir:	ps2fs_mkdir,
    rmdir:	ps2fs_rmdir,
/*    rename:	ps2fs_rename,*/
};

static int ps2fs_add_dirent(struct inode *dir, const char *name,
			    struct inode *inode);
static int ps2fs_remove_dirent(struct inode *dir, const char *name);
static ino_t ps2fs_inode_by_name(struct inode *dir, struct dentry *dentry,
				 int *mode);
static ino_t ps2fs_check_dir_empty(struct inode *dir);

static struct page *ps2fs_get_page(struct inode *dir, unsigned long pagenum);
static void ps2fs_put_page(struct page *page);

/*************************************************************************/
/*************************************************************************/

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,4,0)
static struct dentry *ps2fs_lookup(struct inode *dir, struct dentry *dentry)
#else
static int ps2fs_lookup(struct inode *dir, struct dentry *dentry)
#endif
{
    struct inode *inode = NULL;
    ino_t ino;
    int mode;

//dprintk("lookup(%08lX,%.*s)\n",dir->i_ino,(int)dentry->d_name.len,dentry->d_name.name);
    if (dentry->d_name.len > PS2FS_NAME_LEN)
	LOOKUP_RETURN(-ENAMETOOLONG);
    ino = ps2fs_inode_by_name(dir, dentry, &mode);
    if (ino) {
	inode = iget(dir->i_sb, ino);
//dprintk("*** lookup got inode\n");
	if (!inode)
	    LOOKUP_RETURN(-EACCES);
	if ((mode & PS2FS_DIRENT_IFMT) == PS2FS_DIRENT_IFDIR) {
	    inode->i_mode &= ~S_IFMT;
	    inode->i_mode |= S_IFDIR | S_IXUSR | S_IXGRP | S_IXOTH;
	    inode->i_op = &ps2fs_dir_iops;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,4,0)
	    inode->i_fop = &ps2fs_dops;
#endif
//dprintk("*** lookup sets dir: newmode=%07o\n",(int)inode->i_mode);
	}
    }
    d_add(dentry, inode);
    LOOKUP_RETURN(0);
}

/*************************************************************************/

static int ps2fs_readdir(struct file *file, void *dirent, filldir_t filldir)
{
    struct inode *inode = file->f_dentry->d_inode;
    loff_t pos = file->f_pos, size = inode->i_size;
    unsigned int pageofs = pos & ~PAGE_CACHE_MASK;
    unsigned long pagenum = pos >> PAGE_CACHE_SHIFT;
    unsigned long npages = (size + (PAGE_CACHE_SIZE-1)) >> PAGE_CACHE_SHIFT;

dprintk("readdir(%08lX) npages=%ld pos=%ld/%d\n",inode->i_ino,npages,pagenum,pageofs);
    if (pos > size - sizeof(struct ps2fs_dirent))
	return 0;

    for (; pagenum < npages; pagenum++, pageofs = 0) {
	struct page *page = ps2fs_get_page(inode, pagenum);
	struct ps2fs_dirent *ps2de;
	char *ptr, *end;

	if (IS_ERR(page))
	    continue;
	ptr = page_address(page);
	if (pagenum == npages-1 && (size&~PAGE_CACHE_MASK) != 0)
	    end = ptr + (size&~PAGE_CACHE_MASK) - sizeof(struct ps2fs_dirent);
	else
	    end = ptr + PAGE_CACHE_SIZE - sizeof(struct ps2fs_dirent);
	ps2de = (struct ps2fs_dirent *)(ptr + pageofs);
	for (; (char *)ps2de <= end; ps2de = ps2fs_next_dirent(ps2de)) {
	    char buf[PS2FS_NAME_LEN+1];
	    pageofs = (char *)ps2de - ptr;
	    if (!ps2de->inode || !ps2de->namelen)
		continue;
	    memcpy(buf, ps2de->name, ps2de->namelen);
	    buf[ps2de->namelen] = 0;
	    translate_from_unicode(buf);
	    if (filldir(dirent, buf, strlen(buf),
			pagenum<<PAGE_CACHE_SHIFT | pageofs,
			le32_to_cpu(ps2de->inode), DT_UNKNOWN)) {
		ps2fs_put_page(page);
		goto stop;
	    }
	}
	ps2fs_put_page(page);
    }

  stop:
    file->f_pos = pagenum<<PAGE_CACHE_SHIFT | pageofs;
    file->f_version = inode->i_version;
    return 0;
}

/*************************************************************************/

/* Create `dentry' in `dir' with mode `mode'. */

static int ps2fs_create(struct inode *dir, struct dentry *dentry, int mode)
{
    char unicode_buf[PS2FS_DIRENT_NAMEMAX+1];
    struct inode *inode;
    int err;

    err = translate_to_unicode(dentry, unicode_buf, sizeof(unicode_buf));
    if (err < 0)
	return err;
    inode = ps2fs_new_inode(dir->i_sb);
    if (IS_ERR(inode))
	return PTR_ERR(inode);
    inode->i_op = &ps2fs_file_iops;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,4,0)
    inode->i_fop = &ps2fs_fops;
    inode->i_mapping->a_ops = &ps2fs_aops;
#endif
    err = ps2fs_add_dirent(dir, unicode_buf, inode);
    if (err < 0) {
dprintk("!!! freeing inode 0x%08lX from create failure\n", inode->i_ino);
	kfree(PS2FS_INODE(inode)->rawinode);
	free_block(inode->i_sb, inode->i_ino);
	make_bad_inode(inode);
	iput(inode);
    } else {
	d_instantiate(dentry, inode);
    }
    return err;
}

/*************************************************************************/

/* Unlink `dentry' from `dir'. */

static int ps2fs_unlink(struct inode *dir, struct dentry *dentry)
{
    char unicode_buf[PS2FS_DIRENT_NAMEMAX+1];
    struct inode *inode = dentry->d_inode;
    int err;

    err = translate_to_unicode(dentry, unicode_buf, sizeof(unicode_buf));
    if (!err)
	err = ps2fs_remove_dirent(dir, unicode_buf);
    if (err < 0)
	return err;
    inode->i_nlink--;
    return 0;
}

/*************************************************************************/

/* Create directory `dentry' in `dir' with mode `mode'. */

static int ps2fs_mkdir(struct inode *dir, struct dentry *dentry, int mode)
{
    char unicode_buf[PS2FS_DIRENT_NAMEMAX+1];
    struct inode *inode;
    struct buffer_head tmpbh, *bh;
    struct ps2fs_dirent *ps2de;
    int err;

    /* Translate name to Unicode */
    err = translate_to_unicode(dentry, unicode_buf, sizeof(unicode_buf));
    if (err < 0)
	return err;

    /* Allocate new inode */
    inode = ps2fs_new_inode(dir->i_sb);
    if (IS_ERR(inode))
	return PTR_ERR(inode);

    /* Set up inode */
    inode->i_mode &= ~S_IFREG;
    inode->i_mode |= S_IFDIR | S_IXUSR | S_IXGRP | S_IXOTH;
    inode->i_op = &ps2fs_dir_iops;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,4,0)
    inode->i_fop = &ps2fs_dops;
    inode->i_mapping->a_ops = &ps2fs_aops;
#endif

    /* Allocate first directory block */
    if (ps2fs_get_block(inode, 0, &tmpbh, 1) < 0)
	goto fail_inode;

    /* Add . and .. entries to directory */
    if (inode->i_sb->s_blocksize < 0x200) {
	ps2fs_error(inode->i_sb, "ps2fs_mkdir", "internal error, bad blksize");
	BUG();
	goto fail_inode;
    }
    bh = bread(tmpbh.b_dev, tmpbh.b_blocknr, inode->i_sb->s_blocksize);
    if (!bh)
	goto fail_data;
    inode->i_size = 0x200;
    ps2de = (struct ps2fs_dirent *)((char *)bh->b_data);
    ps2de->inode = cpu_to_le32(PS2FS_BNUM(inode->i_ino));
    ps2de->subpart = PS2FS_SUBPART(inode->i_ino);
    ps2de->namelen = 1;
    ps2de->size_flags = cpu_to_le16(12 | PS2FS_DIRENT_IFDIR);
    ps2de->name[0] = '.';
    ps2de = (struct ps2fs_dirent *)((char *)bh->b_data+12);
    ps2de->inode = cpu_to_le32(PS2FS_BNUM(inode->i_ino));
    ps2de->subpart = PS2FS_SUBPART(inode->i_ino);
    ps2de->namelen = 1;
    ps2de->size_flags = cpu_to_le16(0x1E8 | PS2FS_DIRENT_IFDIR);
    ps2de->name[0] = '.';
    ps2de->name[1] = '.';
    mark_buffer_dirty(bh);
    brelse(bh);

    /* Add entry for directory to parent directory */
    err = ps2fs_add_dirent(dir, unicode_buf, inode);
    if (err < 0)
	goto fail_data;

    /* Success */
    d_instantiate(dentry, inode);
    return err;

  fail_data:
    inode->i_size = 0;
    ps2fs_truncate(inode);
  fail_inode:
dprintk("!!! freeing inode 0x%08lX from mkdir failure\n", inode->i_ino);
    kfree(PS2FS_INODE(inode)->rawinode);
    free_block(inode->i_sb, inode->i_ino);
    make_bad_inode(inode);
    iput(inode);
    return err;
}

/*************************************************************************/

/* Unlink `dentry' from `dir'.  The VFS layer guarantees that `dentry' is
 * a directory.
 */

static int ps2fs_rmdir(struct inode *dir, struct dentry *dentry)
{
    char unicode_buf[PS2FS_DIRENT_NAMEMAX+1];
    struct inode *inode = dentry->d_inode;
    int err;

    err = translate_to_unicode(dentry, unicode_buf, sizeof(unicode_buf));
    if (!err)
	err = ps2fs_check_dir_empty(inode);
    if (!err)
	err = ps2fs_remove_dirent(dir, unicode_buf);
    if (err < 0)
	return err;
    inode->i_nlink--;
    return 0;
}

/*************************************************************************/
/*************************************************************************/

/* Directory entry lookup/modification utility routines.  There's a lot of
 * duplicated code here. */

/*************************************************************************/

/* Add `name' (already translated to Unicode) to `dir' pointing to `inode',
 * which must have i_mode set correctly w.r.t. regular file/directory.
 * Assumes the name does not already exist.  Returns 0 or error.
 */

static int ps2fs_add_dirent(struct inode *dir, const char *name,
			    struct inode *inode)
{
    unsigned long pagenum;
    loff_t size = dir->i_size + 0x200;  /* because we may add a sector */
    unsigned long npages = (size + (PAGE_CACHE_SIZE-1)) >> PAGE_CACHE_SHIFT;
    int needed = sizeof(struct ps2fs_dirent) + ((strlen(name)+3)&~3);
    int len = 0;
    struct page *page = NULL;
    struct ps2fs_dirent *ps2de;
    char *ptr = NULL, *end;

    if (strlen(name) > PS2FS_DIRENT_NAMEMAX)
	return -ENAMETOOLONG;
    for (pagenum = 0; pagenum < npages; pagenum++) {
	page = ps2fs_get_page(dir, pagenum);
	if (IS_ERR(page)) {
	    if (pagenum == npages-1)
		return -EIO;
	    else
		continue;
	}
	ptr = page_address(page);
	if (pagenum == npages-1 && (size&~PAGE_CACHE_MASK) != 0)
	    end = ptr + (size&~PAGE_CACHE_MASK) - sizeof(struct ps2fs_dirent);
	else
	    end = ptr + PAGE_CACHE_SIZE - sizeof(struct ps2fs_dirent);
	if (pagenum == npages-1)
	    end -= 0x200;
	ps2de = (struct ps2fs_dirent *)ptr;
	len = 0x200;
	for (; (char *)ps2de <= end; ps2de = ps2fs_next_dirent(ps2de)) {
	    if (!ps2de->inode || !ps2de->namelen)
		continue;
	    len = sizeof(struct ps2fs_dirent) + ((ps2de->namelen+3)&~3);
	    if ((ps2de->size_flags & PS2FS_DIRENT_SIZEMASK) - len >= needed) {
		ptr += len;
		len = (ps2de->size_flags & PS2FS_DIRENT_SIZEMASK) - len;
		goto found;
	    }
	}
	if (pagenum != npages-1)
	    ps2fs_put_page(page);
	/* else fall through */
    }
  found:
    ps2de = (struct ps2fs_dirent *)ptr;
    ps2de->inode = cpu_to_le32(PS2FS_BNUM(inode->i_ino));
    ps2de->subpart = PS2FS_SUBPART(inode->i_ino);
    ps2de->namelen = strlen(name);
    ps2de->size_flags = len;
    strcpy(ps2de->name, name);  /* safe because we checked length at top */
    if (S_ISDIR(inode->i_mode))
	ps2de->size_flags |= PS2FS_DIRENT_IFDIR;
    else if (S_ISREG(inode->i_mode))
	ps2de->size_flags |= PS2FS_DIRENT_IFREG;
    else {
	ps2fs_error(dir->i_sb, "ps2fs_add_dirent",
		    "improper mode 0%o for target inode", inode->i_mode);
	BUG();
	return -EINVAL;
    }
    ps2fs_put_page(page);
    return 0;
}

/*************************************************************************/

/* Remove `name' (already translated to Unicode) from `dir'.  Will not
 * remove "." or "..".  Returns 0 or error.
 */

static int ps2fs_remove_dirent(struct inode *dir, const char *name)
{
    int namelen = strlen(name);
    unsigned long pagenum;
    loff_t size = dir->i_size;
    unsigned long npages = (size + (PAGE_CACHE_SIZE-1)) >> PAGE_CACHE_SHIFT;
    struct page *page;
    struct ps2fs_dirent *ps2de, *last;
    char *ptr, *end;

    if (*name == '.' && (!name[1] || (name[1] == '.' && !name[2]))) {
	/* Refuse "." and ".." */
	return -EPERM;
    }
    if (strlen(name) > PS2FS_DIRENT_NAMEMAX)
	return -ENAMETOOLONG;
    for (pagenum = 0; pagenum < npages; pagenum++) {
	page = ps2fs_get_page(dir, pagenum);
	if (IS_ERR(page))
	    continue;
	ptr = page_address(page);
	if (pagenum == npages-1 && (size&~PAGE_CACHE_MASK) != 0)
	    end = ptr + (size&~PAGE_CACHE_MASK) - sizeof(struct ps2fs_dirent);
	else
	    end = ptr + PAGE_CACHE_SIZE - sizeof(struct ps2fs_dirent);
	ps2de = (struct ps2fs_dirent *)ptr;
	last = ps2de;
	for (; (char *)ps2de <= end;
	       last = ps2de, ps2de = ps2fs_next_dirent(ps2de)
	) {
	    if (!ps2de->inode)
		break;
	    if ((((char *)ps2de - ptr) & 0x1FF) == 0) /*first entry in sector*/
		last = ps2de;
	    if (ps2de->namelen == namelen
	     && memcmp(ps2de->name, name, namelen) == 0
	    ) {
		if (ps2de == last) {  /* first entry in the sector */
		    ps2de->inode = 0;
		    ps2de->namelen = 0;
		} else {
		    int size = le16_to_cpu(ps2de->size_flags)
			       & PS2FS_DIRENT_SIZEMASK;
		    size += le16_to_cpu(last->size_flags);
		    last->size_flags = cpu_to_le16(size);
		}
		ps2fs_put_page(page);
		return 0;
	    }
	}
	ps2fs_put_page(page);
    }
    /* Not found */
    return -ENOENT;
}

/*************************************************************************/

/* Return the inode for `dentry' in `dir', or 0 if no such name exists.
 * If successful, the 16-bit mode value from the directory entry is stored
 * in `*mode' if `mode' is non-NULL.
 */

static ino_t ps2fs_inode_by_name(struct inode *dir, struct dentry *dentry,
				 int *mode)
{
    unsigned long pagenum;
    loff_t size = dir->i_size;
    unsigned long npages = (size + (PAGE_CACHE_SIZE-1)) >> PAGE_CACHE_SHIFT;

    for (pagenum = 0; pagenum < npages; pagenum++) {
	struct page *page = ps2fs_get_page(dir, pagenum);
	struct ps2fs_dirent *ps2de;
	char *ptr, *end;

	if (IS_ERR(page))
	    continue;
	ptr = page_address(page);
	if (pagenum == npages-1 && (size&~PAGE_CACHE_MASK) != 0)
	    end = ptr + (size&~PAGE_CACHE_MASK) - sizeof(struct ps2fs_dirent);
	else
	    end = ptr + PAGE_CACHE_SIZE - sizeof(struct ps2fs_dirent);
	ps2de = (struct ps2fs_dirent *)ptr;
	for (; (char *)ps2de <= end; ps2de = ps2fs_next_dirent(ps2de)) {
	    char buf[PS2FS_NAME_LEN+1];
	    if (!ps2de->inode || !ps2de->namelen)
		continue;
	    memcpy(buf, ps2de->name, ps2de->namelen);
	    buf[ps2de->namelen] = 0;
	    translate_from_unicode(buf);
	    if (buf[dentry->d_name.len] == 0
	     && memcmp(buf, dentry->d_name.name, dentry->d_name.len) == 0
	    ) {
		ps2fs_put_page(page);
		if (mode)
		    *mode = le16_to_cpu(ps2de->size_flags);
		return PS2FS_MAKE_INUM(le32_to_cpu(ps2de->inode),
				       ps2de->subpart);
	    }
	}
	ps2fs_put_page(page);
    }
    return 0;
}

/*************************************************************************/

/* Check whether `dir' is empty.  Returns 0 if so, -ENOTEMPTY if not.
 * Assumes `dir' is a directory.
 */

static ino_t ps2fs_check_dir_empty(struct inode *dir)
{
    unsigned long pagenum;
    loff_t size = dir->i_size;
    unsigned long npages = (size + (PAGE_CACHE_SIZE-1)) >> PAGE_CACHE_SHIFT;

    for (pagenum = 0; pagenum < npages; pagenum++) {
	struct page *page = ps2fs_get_page(dir, pagenum);
	struct ps2fs_dirent *ps2de;
	char *ptr, *end;

	if (IS_ERR(page))
	    continue;
	ptr = page_address(page);
	if (pagenum == npages-1 && (size&~PAGE_CACHE_MASK) != 0)
	    end = ptr + (size&~PAGE_CACHE_MASK) - sizeof(struct ps2fs_dirent);
	else
	    end = ptr + PAGE_CACHE_SIZE - sizeof(struct ps2fs_dirent);
	ps2de = (struct ps2fs_dirent *)ptr;
	for (; (char *)ps2de <= end; ps2de = ps2fs_next_dirent(ps2de)) {
	    if (!(ps2de->namelen==1 && ps2de->name[0]=='.')
	     && !(ps2de->namelen==2 && ps2de->name[0]=='.' && ps2de->name[1]=='.')
	    ) {
		ps2fs_put_page(page);
		return -ENOTEMPTY;
	    }
	}
	ps2fs_put_page(page);
    }
    return 0;
}

/*************************************************************************/
/*************************************************************************/

/* Version-specific utility and callback routines */

/*************************************************************************/

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,4,0)

static int ps2fs_check_page(struct page *page);

/* Read in and return a page of a directory. */
static struct page *ps2fs_get_page(struct inode *dir, unsigned long pagenum)
{
    struct address_space *mapping = dir->i_mapping;
    struct page *page;

    page = read_cache_page(mapping, pagenum,
			   (filler_t *)mapping->a_ops->readpage, NULL);
    if (!IS_ERR(page)) {
	wait_on_page(page);
	kmap(page);
	if (!Page_Uptodate(page)
	 || PageError(page)
	 || (!PageChecked(page) && !ps2fs_check_page(page))
	) {
	    ps2fs_put_page(page);
	    return ERR_PTR(-EIO);
	}
    }
    return page;
}

static int ps2fs_check_page(struct page *page)
{
    SetPageChecked(page);
    return 1;
}

static void ps2fs_put_page(struct page *page)
{
    kunmap(page);
    page_cache_release(page);
}

#else  /* kernel version < 2.4.0 */

static struct buffer_head *ps2fs_get_page(struct inode *dir,
					  unsigned long pagenum)
{
    struct buffer_head *bh;
    struct buffer_head tmpbh;  /* to receive block numbers */
    int err;

    if (dir->i_sb->s_blocksize != PAGE_CACHE_SIZE) {
	ps2fs_error(dir->i_sb, "ps2fs_get_page", "bad blocksize (%d)",
		    dir->i_sb->s_blocksize);
	return ERR_PTR(-EIO);
    }
    err = ps2fs_get_block(dir, pagenum, &tmpbh, 1);
    if (err)
	return ERR_PTR(err);
    bh = bread(tmpbh.b_dev, tmpbh.b_blocknr, PAGE_CACHE_SIZE);
    if (!bh)
	return ERR_PTR(-EIO);
    return bh;
}

static void ps2fs_put_page(struct buffer_head *bh)
{
    brelse(bh);
}

#endif  /* kernel version */

/*************************************************************************/

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

/* Print debugging functions */

#ifndef UNIONFS_NDEBUG

#include "fist.h"
#include "unionfs.h"

static int fist_debug_var = 0;

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
#define PageUptodate(page) Page_Uptodate(page)
#endif

/* get value of debugging variable */
int fist_get_debug_value(void)
{
	return fist_debug_var;
}

/* set debug level variable and return the previous value */
int fist_set_debug_value(int val)
{
	int prev = fist_debug_var;

	fist_debug_var = val;
	fist_dprint(1, "unionfs: setting debug level to %d\n", val);
	return prev;
}

/*
 * Utilities used by both client and server
 * Standard levels:
 * 0) no debugging
 * 1) hard failures
 * 2) soft failures
 * 3) current test software
 * 4) main procedure entry points
 * 5) main procedure exit points
 * 6) utility procedure entry points
 * 7) utility procedure exit points
 * 8) obscure procedure entry points
 * 9) obscure procedure exit points
 * 10) random stuff
 * 11) all <= 1
 * 12) all <= 2
 * 13) all <= 3
 * ...
 */

void fist_dprint_internal(const char *file, const char *function, int line,
			  int level, char *str, ...)
{
	va_list ap;
	int var = fist_get_debug_value();

	if (level >= 10 || level < 0) {
		printk(KERN_ERR "fist_dprint_internal: Invalid level passed"
		       "from %s:%s:%d\n", file, function, line);
	}

	if (var == level || (var > 10 && (var - 10) >= level)) {
		va_start(ap, str);
		vprintk(str, ap);
		va_end(ap);
	}
	return;
}

static int num_indents = 0;
char indent_buf[80] =
    "                                                                               ";
char *add_indent(void)
{
	indent_buf[num_indents] = ' ';
	num_indents++;
	if (num_indents > 79)
		num_indents = 79;
	indent_buf[num_indents] = '\0';
	return indent_buf;
}

char *del_indent(void)
{
	if (num_indents <= 0)
		return "<IBUG>";
	indent_buf[num_indents] = ' ';
	num_indents--;
	indent_buf[num_indents] = '\0';
	return indent_buf;
}

void fist_print_generic_inode3(const char *str, const char *str2,
			       const struct inode *inode)
{
	if (!inode) {
		printk("PI:%s%s: NULL INODE PASSED!\n", str, str2);
		return;
	}
	if (IS_ERR(inode)) {
		printk("PI:%s%s: ERROR INODE PASSED: %ld\n", str, str2,
		       PTR_ERR(inode));
		return;
	}
	PASSERT(inode);
	fist_dprint(8, "PI:%s%s: %s=%lu\n", str, str2, "i_ino", inode->i_ino);
	fist_dprint(8, "PI:%s%s: %s=%u\n", str, str2, "i_count",
		    atomic_read(&inode->i_count));
	fist_dprint(8, "PI:%s%s: %s=%u\n", str, str2, "i_nlink",
		    inode->i_nlink);
	fist_dprint(8, "PI:%s%s: %s=%o\n", str, str2, "i_mode", inode->i_mode);
	fist_dprint(8, "PI:%s%s: %s=%llu\n", str, str2, "i_size",
		    inode->i_size);
	fist_dprint(8, "PI:%s%s: %s=%p\n", str, str2, "i_op", inode->i_op);
	fist_dprint(8, "PI:%s%s: %s=%p (%s)\n", str, str2, "i_sb",
		    inode->i_sb, (inode->i_sb ? sbt(inode->i_sb) : "NullTypeSB")
	    );
}

void fist_print_generic_inode(const char *str, const struct inode *inode)
{
	fist_print_generic_inode3(str, "", inode);
}

void fist_print_inode(const char *str, const struct inode *inode)
{
	int bindex;

	if (!inode) {
		printk("PI:%s: NULL INODE PASSED!\n", str);
		return;
	}
	if (IS_ERR(inode)) {
		printk("PI:%s: ERROR INODE PASSED: %ld\n", str, PTR_ERR(inode));
		return;
	}
	PASSERT(inode);

	if (strcmp("unionfs", sbt(inode->i_sb))) {
		char msg[100];
		snprintf(msg, sizeof(msg), "Invalid inode passed to"
			 "fist_print_inode: %s\n", sbt(inode->i_sb));
		FISTBUG(msg);
	}

	fist_print_generic_inode(str, inode);

	if (!itopd(inode))
		return;
	PASSERT(itopd(inode));
	fist_dprint(8, "PI:%s: ibstart=%d, ibend=%d\n", str,
		    ibstart(inode), ibend(inode));

	if (ibstart(inode) == -1)
		return;

	for (bindex = ibstart(inode); bindex <= ibend(inode); bindex++) {
		struct inode *hidden_inode = itohi_index(inode, bindex);
		char newstr[10];
		if (!hidden_inode) {
			fist_dprint(8, "PI:%s: HI#%d: NULL\n", str, bindex);
			continue;
		}
		sprintf(newstr, ": HI%d", bindex);
		fist_print_generic_inode3(str, newstr, hidden_inode);
	}
}

void fist_print_pte_flags(char *str, const struct page *page)
{
	unsigned long address;

	PASSERT(page);
	address = page->index;
	fist_dprint(8, "PTE-FL:%s index=0x%lx\n", str, address);
}

void fist_print_generic_file3(const char *str, const char *str2,
			      const struct file *file)
{
	fist_dprint(8, "PF:%s%s: %s=0x%p\n", str, str2, "f_dentry",
		    file->f_dentry);
	fist_dprint(8, "PF:%s%s: name=%s\n", str, str2,
		    file->f_dentry->d_name.name);
	if (file->f_dentry->d_inode) {
		PASSERT(file->f_dentry->d_inode);
		fist_dprint(8, "PF:%s%s: %s=%lu\n", str, str2,
			    "f_dentry->d_inode->i_ino",
			    file->f_dentry->d_inode->i_ino);
		fist_dprint(8, "PF:%s%s: %s=%o\n", str, str2,
			    "f_dentry->d_inode->i_mode",
			    file->f_dentry->d_inode->i_mode);
	}
	fist_dprint(8, "PF:%s%s: %s=0x%p\n", str, str2, "f_op", file->f_op);
	fist_dprint(8, "PF:%s%s: %s=0x%x\n", str, str2, "f_mode", file->f_mode);
	fist_dprint(8, "PF:%s%s: %s=0x%llu\n", str, str2, "f_pos", file->f_pos);
	fist_dprint(8, "PF:%s%s: %s=%u\n", str, str2, "f_count",
		    atomic_read(&file->f_count));
	fist_dprint(8, "PF:%s%s: %s=0x%x\n", str, str2, "f_flags",
		    file->f_flags);
	fist_dprint(8, "PF:%s%s: %s=%lu\n", str, str2, "f_version",
		    file->f_version);
}

void fist_print_generic_file(const char *str, const struct file *file)
{
	fist_print_generic_file3(str, "", file);
}

void fist_print_file(const char *str, const struct file *file)
{
	struct file *hidden_file;

	if (!file) {
		fist_dprint(8, "PF:%s: NULL FILE PASSED!\n", str);
		return;
	}

	PASSERT(file->f_dentry);
	if (strcmp("unionfs", sbt(file->f_dentry->d_sb))) {
		char msg[100];
		snprintf(msg, sizeof(msg), "Invalid file passed to"
			 "fist_print_file: %s\n", sbt(file->f_dentry->d_sb));
		FISTBUG(msg);
	}

	fist_print_generic_file(str, file);

	if (ftopd(file)) {
		int bindex;

		fist_dprint(8, "PF:%s: fbstart=%d, fbend=%d\n", str,
			    fbstart(file), fbend(file));

		for (bindex = fbstart(file); bindex <= fbend(file); bindex++) {
			char newstr[10];
			hidden_file = ftohf_index(file, bindex);
			if (!hidden_file) {
				fist_dprint(8, "PF:%s: HF#%d is NULL\n", str,
					    bindex);
				continue;
			}
			sprintf(newstr, ": HF%d", bindex);
			fist_print_generic_file3(str, newstr, hidden_file);
		}
	}
}

static char mode_to_type(mode_t mode)
{
	if (S_ISDIR(mode))
		return 'd';
	if (S_ISLNK(mode))
		return 'l';
	if (S_ISCHR(mode))
		return 'c';
	if (S_ISBLK(mode))
		return 'b';
	if (S_ISREG(mode))
		return 'f';
	return '?';
}

void __fist_print_dentry(const char *str, const struct dentry *dentry,
			 int check)
{
	if (!dentry) {
		fist_dprint(8, "PD:%s: NULL DENTRY PASSED!\n", str);
		return;
	}
	if (IS_ERR(dentry)) {
		fist_dprint(8, "PD:%s: ERROR DENTRY (%ld)!\n", str,
			    PTR_ERR(dentry));
		return;
	}
	PASSERT(dentry);

	if (strcmp("unionfs", sbt(dentry->d_sb))) {
		char msg[100];
		snprintf(msg, sizeof(msg), "Invalid dentry passed to"
			 "fist_print_dentry: %s\n", sbt(dentry->d_sb));
		FISTBUG(msg);
	}

	__fist_print_generic_dentry(str, "", dentry, check);

	if (!dtopd(dentry))
		return;
	PASSERT(dtopd(dentry));
	fist_dprint(8, "PD:%s: dbstart=%d, dbend=%d, dbopaque=%d\n",
		    str, dbstart(dentry), dbend(dentry), dbopaque(dentry));
	if (dbstart(dentry) != -1) {
		int bindex;
		char newstr[10];
		struct dentry *hidden_dentry;
		PASSERT(dtopd(dentry));

		for (bindex = dbstart(dentry); bindex <= dbend(dentry);
		     bindex++) {
			hidden_dentry = dtohd_index(dentry, bindex);
			if (!hidden_dentry) {
				fist_dprint(8, "PD:%s: HD#%d: NULL\n", str,
					    bindex);
				continue;
			}
			sprintf(newstr, ": HD%d", bindex);
			fist_print_generic_dentry3(str, newstr, hidden_dentry);
		}
	}
}
void fist_print_dentry(const char *str, const struct dentry *dentry)
{
	__fist_print_dentry(str, dentry, 1);
}

void __fist_print_generic_dentry(const char *str, const char *str2, const
				 struct dentry *dentry, int check)
{
	if (!dentry) {
		fist_dprint(8, "PD:%s%s: NULL DENTRY PASSED!\n", str, str2);
		return;
	}
	if (IS_ERR(dentry)) {
		fist_dprint(8, "PD:%s%s: ERROR DENTRY (%ld)!\n", str, str2,
			    PTR_ERR(dentry));
		return;
	}
	PASSERT(dentry);

	fist_dprint(8, "PD:%s%s: dentry = %p\n", str, str2, dentry);
	fist_dprint(8, "PD:%s%s: %s=%d\n", str, str2, "d_count",
		    atomic_read(&dentry->d_count));
	fist_dprint(8, "PD:%s%s: %s=%x\n", str, str2, "d_flags",
		    (int)dentry->d_flags);
	fist_dprint(8, "PD:%s%s: %s=\"%s\" (len = %d)\n", str, str2,
		    "d_name.name", dentry->d_name.name, dentry->d_name.len);
	fist_dprint(8, "PD:%s%s: %s=%p (%s)\n", str, str2, "d_sb", dentry->d_sb,
		    sbt(dentry->d_sb));
	fist_dprint(8, "PD:%s%s: %s=%p\n", str, str2, "d_inode",
		    dentry->d_inode);
	if (dentry->d_inode) {
		fist_dprint(8, "PD:%s%s: %s=%ld (%s)\n", str, str2,
			    "d_inode->i_ino", dentry->d_inode->i_ino,
			    sbt(dentry->d_inode->i_sb));
		fist_dprint(8, "PD:%s%s: dentry->d_inode->i_mode: %c%o\n", str,
			    str2, mode_to_type(dentry->d_inode->i_mode),
			    dentry->d_inode->i_mode);
	}
	fist_dprint(8, "PD:%s%s: %s=%p (%s)\n", str, str2, "d_parent",
		    dentry->d_parent,
		    (dentry->d_parent ? sbt(dentry->d_parent->d_sb) : "nil"));
	fist_dprint(8, "PD:%s%s: %s=\"%s\"\n", str, str2,
		    "d_parent->d_name.name", dentry->d_parent->d_name.name);
	fist_dprint(8, "PD:%s%s: %s=%d\n", str, str2, "d_parent->d_count",
		    atomic_read(&dentry->d_parent->d_count));
	fist_dprint(8, "PD:%s%s: %s=%p\n", str, str2, "d_op", dentry->d_op);
	fist_dprint(8, "PD:%s%s: %s=%p\n", str, str2, "d_fsdata",
		    dentry->d_fsdata);
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
	fist_dprint(8, "PD:%s%s:%s=%d\n", str, str2, "list_empty(d_hash)",
		    list_empty(&((struct dentry *)dentry)->d_hash));
#else
	fist_dprint(8, "PD:%s%s: %s=%d\n", str, str2, "hlist_unhashed(d_hash)",
		    hlist_unhashed(&((struct dentry *)dentry)->d_hash));
#endif
	/* After we have printed it, we can assert something about it. */
	if (check)
		ASSERT(atomic_read(&dentry->d_count) > 0);
}

void fist_print_generic_dentry(const char *str, const struct dentry *dentry)
{
	__fist_print_generic_dentry(str, "", dentry, 1);
}
void fist_print_generic_dentry3(const char *str, const char *str2,
				const struct dentry *dentry)
{
	__fist_print_generic_dentry(str, str2, dentry, 1);
}

void fist_checkinode(const struct inode *inode, const char *msg)
{
	if (!inode) {
		printk(KERN_WARNING "fist_checkinode - inode is NULL! (%s)\n",
		       msg);
		return;
	}
	if (!itopd(inode)) {
		fist_dprint(8, "fist_checkinode(%ld) - no private data (%s)\n",
			    inode->i_ino, msg);
		return;
	}
	if ((itopd(inode)->b_start < 0) || !itohi(inode)) {
		fist_dprint(8,
			    "fist_checkinode(%ld) - underlying is NULL! (%s)\n",
			    inode->i_ino, msg);
		return;
	}
	if (!inode->i_sb) {
		fist_dprint(8,
			    "fist_checkinode(%ld) - inode->i_sb is NULL! (%s)\n",
			    inode->i_ino, msg);
		return;
	}
	fist_dprint(8, "inode->i_sb->s_type %p\n", inode->i_sb->s_type);
	if (!inode->i_sb->s_type) {
		fist_dprint(8,
			    "fist_checkinode(%ld) - inode->i_sb->s_type is NULL! (%s)\n",
			    inode->i_ino, msg);
		return;
	}
	fist_dprint(6,
		    "CI: %s: inode->i_count = %d, hidden_inode->i_count = %d, inode = %lu, sb = %s, hidden_sb = %s\n",
		    msg, atomic_read(&inode->i_count),
		    itopd(inode)->b_start >=
		    0 ? atomic_read(&itohi(inode)->i_count) : -1, inode->i_ino,
		    inode->i_sb->s_type->name,
		    itopd(inode)->b_start >=
		    0 ? itohi(inode)->i_sb->s_type->name : "(none)");
}

void fist_print_sb(const char *str, const struct super_block *sb)
{
	struct super_block *hidden_superblock;

	if (!sb) {
		fist_dprint(8, "PSB:%s: NULL SB PASSED!\n", str);
		return;
	}

	fist_dprint(8, "PSB:%s: %s=%u\n", str, "s_blocksize",
		    (int)sb->s_blocksize);
	fist_dprint(8, "PSB:%s: %s=%u\n", str, "s_blocksize_bits",
		    (int)sb->s_blocksize_bits);
	fist_dprint(8, "PSB:%s: %s=0x%x\n", str, "s_flags", (int)sb->s_flags);
	fist_dprint(8, "PSB:%s: %s=0x%x\n", str, "s_magic", (int)sb->s_magic);
	fist_dprint(8, "PSB:%s: %s=%llu\n", str, "s_maxbytes", sb->s_maxbytes);
	fist_dprint(8, "PSB:%s: %s=%d\n", str, "s_count", (int)sb->s_count);
	fist_dprint(8, "PSB:%s: %s=%d\n", str, "s_active",
		    (int)atomic_read(&sb->s_active));
	if (stopd(sb))
		fist_dprint(8, "sbstart=%d, sbend=%d\n", sbstart(sb),
			    sbend(sb));
	fist_dprint(8, "\n");

	if (stopd(sb)) {
		int bindex;
		for (bindex = sbstart(sb); bindex <= sbend(sb); bindex++) {
			hidden_superblock = stohs_index(sb, bindex);
			if (!hidden_superblock) {
				fist_dprint(8, "PSB:%s: HS#%d is NULL", str,
					    bindex);
				continue;
			}

			fist_dprint(8, "PSB:%s: HS#%d: %s=%u\n", str, bindex,
				    "s_blocksize",
				    (int)hidden_superblock->s_blocksize);
			fist_dprint(8, "PSB:%s: HS#%d: %s=%u\n", str, bindex,
				    "s_blocksize_bits",
				    (int)hidden_superblock->s_blocksize_bits);
			fist_dprint(8, "PSB:%s: HS#%d: %s=0x%x\n", str, bindex,
				    "s_flags", (int)hidden_superblock->s_flags);
			fist_dprint(8, "PSB:%s: HS#%d: %s=0x%x\n", str, bindex,
				    "s_magic", (int)hidden_superblock->s_magic);
			fist_dprint(8, "PSB:%s: HS#%d: %s=%llu\n", str, bindex,
				    "s_maxbytes",
				    hidden_superblock->s_maxbytes);
			fist_dprint(8, "PSB:%s: HS#%d: %s=%d\n", str, bindex,
				    "s_count", (int)hidden_superblock->s_count);
			fist_dprint(8, "PSB:%s: HS#%d: %s=%d\n", str, bindex,
				    "s_active",
				    (int)atomic_read(&hidden_superblock->
						     s_active));
		}
	}
}

#endif
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

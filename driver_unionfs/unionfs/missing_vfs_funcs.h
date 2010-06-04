#ifndef __MISSING_VFS_FUNCS_H_
#define __MISSING_VFS_FUNCS_H_
/*
 * DQ: These functions used to be in the actual vfs but now they are missing so we redefine them here if we are using 2.6 this code is used in the 2.6 templates
 * Common dentry functions for inclusion in the VFS
 * or in other stackable file systems.  Some of these
 * functions were in linux/fs/ C (VFS) files.
 *
 */

/*
 * Locking the parent is needed to:
 *  - serialize directory operations
 *  - make sure the parent doesn't change from
 *    under us in the middle of an operation.
 *
 * NOTE! Right now we'd rather use a "struct inode"
 * for this, but as I expect things to move toward
 * using dentries instead for most things it is
 * probably better to start with the conceptually
 * better interface of relying on a path of dentries.
 */
static inline struct dentry *lock_parent(struct dentry *dentry)
{
	struct dentry *dir = DGET(dentry->d_parent);

	down(&dir->d_inode->i_sem);
	return dir;
}

static inline void unlock_dir(struct dentry *dir)
{
	up(&dir->d_inode->i_sem);
	DPUT(dir);
}

/*
 * Whee.. Deadlock country. Happily there are only two VFS
 * operations that does this..
 */
static inline void double_down(struct semaphore *s1, struct semaphore *s2)
{
	if (s1 != s2) {
		if ((unsigned long)s1 < (unsigned long)s2) {
			struct semaphore *tmp = s2;
			s2 = s1;
			s1 = tmp;
		}
		down(s1);
	}
	down(s2);
}

/*
 * Ewwwwwwww... _triple_ lock. We are guaranteed that the 3rd argument is
 * not equal to 1st and not equal to 2nd - the first case (target is parent of
 * source) would be already caught, the second is plain impossible (target is
 * its own parent and that case would be caught even earlier). Very messy.
 * I _think_ that it works, but no warranties - please, look it through.
 * Pox on bloody lusers who mandated overwriting rename() for directories...
 */
/*Not used in templates */
static inline void triple_down(struct semaphore *s1,
			       struct semaphore *s2, struct semaphore *s3)
{
	if (s1 != s2) {
		if ((unsigned long)s1 < (unsigned long)s2) {
			if ((unsigned long)s1 < (unsigned long)s3) {
				struct semaphore *tmp = s3;
				s3 = s1;
				s1 = tmp;
			}
			if ((unsigned long)s1 < (unsigned long)s2) {
				struct semaphore *tmp = s2;
				s2 = s1;
				s1 = tmp;
			}
		} else {
			if ((unsigned long)s1 < (unsigned long)s3) {
				struct semaphore *tmp = s3;
				s3 = s1;
				s1 = tmp;
			}
			if ((unsigned long)s2 < (unsigned long)s3) {
				struct semaphore *tmp = s3;
				s3 = s2;
				s2 = tmp;
			}
		}
		down(s1);
	} else if ((unsigned long)s2 < (unsigned long)s3) {
		struct semaphore *tmp = s3;
		s3 = s2;
		s2 = tmp;
	}
	down(s2);
	down(s3);
}

static inline void double_up(struct semaphore *s1, struct semaphore *s2)
{
	up(s1);
	if (s1 != s2)
		up(s2);
}

/*not used in templates*/
static inline void triple_up(struct semaphore *s1,
			     struct semaphore *s2, struct semaphore *s3)
{
	up(s1);
	if (s1 != s2)
		up(s2);
	up(s3);
}

static inline void double_lock(struct dentry *d1, struct dentry *d2)
{
	double_down(&d1->d_inode->i_sem, &d2->d_inode->i_sem);
}

static inline void double_unlock(struct dentry *d1, struct dentry *d2)
{
	double_up(&d1->d_inode->i_sem, &d2->d_inode->i_sem);
	DPUT(d1);
	DPUT(d2);
}

#endif				//__MISSING_VFS_FUNCS_H_
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

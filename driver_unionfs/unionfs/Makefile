UNIONFS_VERSION  = 1.1.5
EXTRA_CFLAGS+=-DUNIONFS_VERSION=\"${UNIONFS_VERSION}\" -DUNIONFS_UNSUPPORTED
# This will disable debugging support
# EXTRA_CFLAGS+=-DUNIONFS_NDEBUG

O_TARGET := unionfs.o

obj-y := subr.o dentry.o file.o inode.o main.o super.o \
	stale_inode.o branchman.o xattr.o rdstate.o copyup.o  \
	dirhelper.o rename.o unlink.o lookup.o persistent_inode.o \
	commonfops.o dirfops.o print.o compat24.o

obj-m    := $(O_TARGET)

include $(TOPDIR)/Rules.make

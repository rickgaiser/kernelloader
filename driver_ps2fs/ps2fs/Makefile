#
# Makefile for the PS2 filesystem routines.
#
# Note! Dependencies are done automagically by 'make dep', which also
# removes any old dependencies. DON'T put your own dependencies here
# unless it's something special (not a .c file).
#
# Note 2! The CFLAGS definitions are now in the main makefile.

O_TARGET := ps2fs.o

obj-y    := dir.o inode.o misc.o partition.o super.o unicode.o bitmap.o
obj-m    := $(O_TARGET)

include $(TOPDIR)/Rules.make

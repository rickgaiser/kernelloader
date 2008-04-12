/*
# _____     ___ ____     ___ ____
#  ____|   |    ____|   |        | |____|
# |     ___|   |____ ___|    ____| |    \    PS2DEV Open Source Project.
#-----------------------------------------------------------------------
# Copyright 2001-2004, ps2dev - http://www.ps2dev.org
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.
#
# (c) 2007 Mega Man
#
# $Id$
# IOP heap handling prototypes
*/

#ifndef _IOP_HEAP_H_
#define _IOP_HEAP_H_

#ifdef __cplusplus
extern "C" {
#endif

int SifInitIopHeap(SifRpcEndFunc_t endfunc, void *efarg);
void SifExitIopHeap(void);

int SifAllocIopHeap(int size, SifRpcEndFunc_t endfunc, void *efarg, u32 *result);
int SifFreeIopHeap(void *addr, SifRpcEndFunc_t endfunc, void *efarg, int *result);

int SifLoadIopHeap(const char *path, void *addr, SifRpcEndFunc_t endfunc, void *efarg);

#ifdef __cplusplus
}
#endif

#endif

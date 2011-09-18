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
# Common SifDma prototypes and structures
*/

#ifndef _SIFDMA_H
#define _SIFDMA_H

#ifdef __cplusplus
extern "C" {
#endif

/** MSB is set for system commands. */
#define SYSTEM_CMD	0x80000000

#define SIF_DMA_INT_I	0x2
#define SIF_DMA_INT_O	0x4

u32 SifSetDma(tge_sifdma_transfer_t *sdd, s32 len);
u32 iSifSetDma(tge_sifdma_transfer_t *sdd, s32 len);
s32 SifDmaStat(u32 id);
u32 sif_dma_get_hw_index(void);
u32 sif_dma_get_sw_index(void);

#ifdef __cplusplus
}
#endif

#endif

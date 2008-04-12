/*
 * sif.h - SIF routines internal to the SBIOS.
 *
 * Copyright (c) 2003 Marcus R. Brown <mrbrown@0xd6.org>
 *
 * See the file LICENSE, located within this directory, for licensing terms.
 */

#ifndef TGE_SIF_H
#define TGE_SIF_H

#include "sifdma.h"

u32 SifSetDma(SifDmaTransfer_t *sdd, s32 len);
u32 iSifSetDma(SifDmaTransfer_t *sdd, s32 len);

void sif_set_dchain(void);

int sif_cmd_init(void);
int sif_cmd_exit(void);

u32 sif_cmd_send(u32 fid, u32 flags, void *packet, u32 packet_size, void *src, void *dest, u32 size);

void sif_cmd_add_handler(u32 fid, void *handler, void *harg);
u32 sif_cmd_get_sreg(u32 reg);

u32 SifSetReg(u32 reg, u32 val);
u32 SifGetReg(u32 reg);

#endif /* TGE_SIF_H */

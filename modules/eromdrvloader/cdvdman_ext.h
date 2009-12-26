/* Copyright 2009 Mega Man. */
#ifndef _CDVDMAN_EXT_H_
#define _CDVDMAN_EXT_H_

#include <tamtypes.h>
#include <cdvdman.h>

int sceCdReadNVM(u32 addr, u16 *data, u8 *stat);
#define I_sceCdReadNVM DECLARE_IMPORT(26, sceCdReadNVM)

#endif

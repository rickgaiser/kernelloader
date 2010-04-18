/*
# _____     ___ ____     ___ ____
#  ____|   |    ____|   |        | |____|
# |     ___|   |____ ___|    ____| |    \    PS2DEV Open Source Project.
#-----------------------------------------------------------------------
# Copyright (c) 2003  Marcus R. Brown <mrbrown@0xd6.org>
# Copyright (c) 2008 Mega Man
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.
#
# $Id$
# Sub-CPU module interface.
*/

#ifndef SBV_SMOD_H
#define SBV_SMOD_H

#include "stdint.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Module info entry.  Most of the fields are self-explanatory.  I don't know
   what the *flags fields do, and they don't seem to be important.  */
typedef struct _smod_mod_info {
	struct _smod_mod_info *next;
	uint8_t	*name;		/* A pointer to the name in IOP RAM, this must be smem_read().  */
	uint16_t	version;
	uint16_t	newflags;	/* For modload shipped with games.  */
	uint16_t	id;
	uint16_t	flags;		/* I believe this is where flags are kept for BIOS versions.  */
	uint32_t	entry;		/* _start */
	uint32_t	gp;
	uint32_t	text_start;
	uint32_t	text_size;
	uint32_t	data_size;
	uint32_t	bss_size;
	uint32_t	unused1;
	uint32_t	unused2;
} smod_mod_info_t;

/* Return the next module referenced in the global module list.  */
int smod_get_next_mod(smod_mod_info_t *cur_mod, smod_mod_info_t *next_mod);

/* Find and retreive a module by it's module name.  */
int smod_get_mod_by_name(const char *name, smod_mod_info_t *info);

#ifdef __cplusplus
}
#endif

#endif /* SBV_SMOD_H */

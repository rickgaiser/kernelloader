/*
 * sbios.c - SBIOS entrypoint and dispatch table.
 *
 * Copyright (c) 2003 Marcus R. Brown <mrbrown@0xd6.org>
 *
 * See the file LICENSE, located within this directory, for licensing terms.
 */

#include "sbcalls.h"
#include "iopmem.h"

#define SBCALL_MAX	256

static void *dispatch[SBCALL_MAX] __attribute__((section(".text"))) = {
	/* 0 */
	sbcall_getver,
	/* 1 */
	sbcall_halt,
	/* 2 */
	sbcall_setdve,
	/* 3 */
	sbcall_putc,
	/* 4 */
	sbcall_getc,
	/* 5 */
	sbcall_setgscrt,
	/* 6 */
	sbcall_setrgbyc,
	0, 0, 0, 0, 0, 0, 0, 0, 0,
	/* 16 */
	sbcall_sifinit,
	/* 17 */
	sbcall_sifexit,
	/* 18 */
	sbcall_sifsetdma,
	/* 19 */
	sbcall_sifdmastat,
	/* 20 */
	sbcall_sifsetdchain,
	/* 21 */
	sbcall_sifsetreg,
	/* 22 */
	sbcall_sifgetreg,
	/* 23 */
	sbcall_sifstopdma,
	0, 0, 0, 0, 0, 0, 0, 0,
	/* 32 */
	sbcall_sifinitcmd,
	/* 33 */
	sbcall_sifexitcmd,
	/* 34 */
	sbcall_sifsendcmd,
	/* 35 */
	sbcall_sifcmdintrhdlr,
	/* 36 */
	sbcall_sifaddcmdhandler,
	/* 37 */
	sbcall_sifremovecmdhandler,
	/* 38 */
	sbcall_sifsetcmdbuffer,
	/* 39 */
	sbcall_sifsetsreg,
	/* 40 */
	sbcall_sifgetsreg,
	/* 41 */
	sbcall_sifgetdatatable,
	/* 42 */
	sbcall_sifsetsyscmdbuffer,
	0, 0, 0, 0, 0,
};

int sbios(tge_sbcall_t sbcall, void *arg)
{
	int (*sbfunc)(void *) = dispatch[sbcall];

	iop_prints("sbios ");
	iop_printx(sbcall);
	iop_prints("\n");

	if (!sbfunc)
		return -1;

	return sbfunc(arg);
}

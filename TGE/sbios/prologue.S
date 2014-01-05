/*
 * prologue.S - SBIOS header and bootloader parameters.
 *
 * Copyright (c) 2003 Marcus R. Brown <mrbrown@0xd6.org>
 *
 * See the file LICENSE, located within this directory, for licensing terms.
 */

	.section .header
	.globl sbios_iopaddr
	.globl sbios_osdparam
	.globl sbios_header

sbios_header:
	.word	sbios	/* Entry address fo sbios calls. */

	.ascii	"PS2b"

sbios_iopaddr:
	.word	0
sbios_osdparam:
	.word	0

	.word	0, 0, 0, 0

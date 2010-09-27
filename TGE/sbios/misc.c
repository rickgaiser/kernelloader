/*
 * misc.c - Misc SBIOS calls
 *
 * Copyright (c) 2003 Marcus R. Brown <mrbrown@0xd6.org>
 * Copyright (c) 2008 Mega Man
 *
 * See the file LICENSE, located within this directory, for licensing terms.
 */


#include "tge_types.h"
#include "tge_defs.h"

#include "tge_sbios.h"

#include "hwreg.h"

#include "cache.h"
#include "iopmemdebug.h"

#include "core.h"
#include "cdvd.h"
#include "stdio.h"
#include "dve_reg.h"

/**
 * roment_t - ROMDIR entry
 * @name: entry name
 * @xi_size: size of information in "EXTINFO" for this entry
 * @size: entry size
 *
 * This data structure represents one entry in the ROMDIR entry table.  The
 * first entry is always "RESET", followed by "ROMDIR" and "EXTINFO".
 */
typedef struct {
	char		name[10];
	uint16_t	xi_size;
	uint32_t	size;
} __attribute__((packed)) romentry_t;

typedef struct {
	int	interlace;
	int	output_mode;
	int	field_frame_mode;
} crtmode_t;

typedef struct {
	int	output_mode;
	int	*dx1;
	int *dy1;
	int *dx2;
	int *dy2;
} crtoffsets_t;

typedef struct {
	const char filename[12];
	void (*setcrtmode)(crtmode_t *mode);
	void (*getcrtoffsets)(crtoffsets_t *offsets);
} __attribute__((packed)) romgscrt_t;

int strncmp(const char *str1, const char *str2, unsigned int len)
{
	unsigned int n;

	n = 0;
	while((*str1 != 0) && (*str2 != 0) && (n < len)) {
		if (*str1 != *str2) {
			break;
		}
		str1++;
		str2++;
		n++;
	}
	return *str2 - *str1;
}

romentry_t *get_romdir(unsigned int rom_addr)
{
	unsigned int offset;
	romentry_t *romdir = (void *) rom_addr;

	offset = 0;
	while (offset < 0x10000) {
		if (strncmp(romdir->name, "RESET", 6) == 0) {
			if (((romdir->size + sizeof(*romdir) - 1) & ~(sizeof(*romdir) - 1)) == offset) {
				return romdir;
			}
		}
		romdir++;
		offset += sizeof(*romdir);
	}
	return NULL;
}

romgscrt_t *get_romgscrt(void)
{
	romentry_t *romdir;
	unsigned int addr;

	addr = 0xBFC00000;

	/* Get ROMDIR of first ROM. */
	romdir = get_romdir(addr);

	if (romdir != NULL) {
		while (romdir->name[0] != 0) {
			if (strncmp(romdir->name, "ROMGSCRT", 9) == 0) {
				return (romgscrt_t *) addr;
			}
			/* Calculate address of next file in memory. */
			addr += (romdir->size + sizeof(*romdir) - 1) & ~(sizeof(*romdir) - 1);
			romdir++;
		}
	}
	return NULL;
}


int sbcall_getver()
{
	return TGE_SBIOS_VERSION;
}

static int sio_putc(int c)
{
	/* Wait for free space in the TX FIFO.  */
	while (_lw(EE_SIO_ISR) & 0x8000)
		;

	_sb(c, EE_SIO_TXFIFO);
	return c;
}

/* Output a character over the serial port.  */
int sbcall_putc(tge_sbcall_putc_arg_t *arg)
{
	iop_putc(arg->c);

	/* Translate \n to \r\n.  */
	if (arg->c == '\n') {
		sio_putc('\r');
		return sio_putc('\n');
	}

	return sio_putc(arg->c);
}

/* This MUST return 0 if there are no characters in the RX FIFO.  */
int sbcall_getc()
{
	if (_lw(EE_SIO_ISR) & 0xf00)
		return _lb(EE_SIO_RXFIFO);

	return 0;
}

int sbcall_halt(tge_sbcall_halt_arg_t *arg)
{
	u32 status;

	arg = arg;

	_sh(_lh(0xbf80146c) & 0xFFFE, 0xbf80146c);
	_sh(0, 0xbf801460);
	_sh(0, 0xbf80146c);

	//if (arg->mode == TGE_SB_HALT_MODE_POWEROFF) {
#ifdef NEW_ROM_MODULE_VERSION
		u32 result;
		
		if (cdPowerOff(&result) == 0) {
			printf("Power off failed.\n");
		}

#endif
		/* Power off manually. */
		_sb(0, 0xBF402017); /* SCMD */
		_sb(0xF, 0xBF402016); /* SCMD Power off */
	//}

	core_save_disable(&status);
	while (1) {
		/* Stop system or wait until reset. */
	}
	return -1;
}

int sbcall_setdve(tge_sbcall_setdve_arg_t *arg)
{
	romgscrt_t *romgscrt;

	printf("set DVE mode %d\n", arg->mode);

	/* The following is a workaround to get BSD working on newer consoles. */
	/* VGA should be selected in kernelloader to get it working with BSD. */
	romgscrt = get_romgscrt();
	if (romgscrt != NULL) {
		/* sbcall_setgscrt() should be used. */
		/* BSD uses sbcall_setdve(), I think this only works on the first PS2. */
		printf("Err: Cancel setting DVE mode.\n");
		return -1;
	}

	dve_prepare_bus();

	switch(arg->mode) {
		case 0: /* NTSC */
			break;

		case 1: /* PAL */
			break;

		case 2: /* VESA */
			dve_set_reg(0x77, 0x11);
			dve_set_reg(0x93, 0x01);
			dve_set_reg(0x91, 0x01);
			break;

		case 3: /* DTV 480P */
			dve_set_reg(0x77, 0x11);
			dve_set_reg(0x93, 0x01);
			dve_set_reg(0x91, 0x02);
			break;

		case 4: /* DTV 1080I */
			dve_set_reg(0x77, 0x11);
			dve_set_reg(0x93, 0x01);
			dve_set_reg(0x90, 0x04);
			dve_set_reg(0x91, 0x03);
			break;

		case 5: /* DTV 720P */
			dve_set_reg(0x77, 0x11);
			dve_set_reg(0x93, 0x01);
			dve_set_reg(0x90, 0x06);
			dve_set_reg(0x91, 0x03);
			break;

		default:
			break;
	}
	return 0;
}

int sbcall_setgscrt(tge_sbcall_setgscrt_arg_t *arg)
{
	romgscrt_t *romgscrt;
	crtmode_t crtmode;
	crtoffsets_t crtoffsets;

	romgscrt = get_romgscrt();
	if (romgscrt != NULL) {
		crtmode.interlace = arg->interlace;
		crtmode.output_mode = arg->output_mode;
		crtmode.field_frame_mode = arg->field_frame_mode;
		romgscrt->setcrtmode(&crtmode);

		crtoffsets.output_mode = arg->output_mode;
		crtoffsets.dx1 = arg->dx1;
		crtoffsets.dy1 = arg->dy1;
		crtoffsets.dx2 = arg->dx2;
		crtoffsets.dy2 = arg->dy2;
		romgscrt->getcrtoffsets(&crtoffsets);

		return 0;
	} else {
		/* sbcall_setdve() should be used instead. */
		printf("Err: ROMGSCRT is not available.\n");
		return -1;
	}
}

int sbcall_setrgbyc(tge_sbcall_setrgbyc_arg_t *arg)
{
	arg = arg;
	return 0;
}

void FlushCache(int operation)
{
	u32 status;

	core_save_disable(&status);
	switch(operation)
	{
		case 0:
			/* Writeback DCache .*/
			flushDCacheAll();
			break;
		case 1:
			/* Invalidate DCache. */
			printf("Inval DCache not supp1.\n");
			break;
		case 2:
			/* Invalidate ICache. */
			invalidateICacheAll();
			break;
		case 3:
			/* Invalidate Cache. */
			invalidateICacheAll();
			printf("Inval DCache not supp3.\n");
			break;
	}
	core_restore(status);
}

void SyncDCache(void *start, void *end)
{
	extern void _SyncDCache(void *start, void *end);

	u32 status;

	core_save_disable(&status);

	_SyncDCache((void *)((u32)start & 0xffffffc0), (void *)((u32)end & 0xffffffc0));

	core_restore(status);
}

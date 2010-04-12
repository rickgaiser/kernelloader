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
		_sb(0, 0xBF402017);
		_sb(0xF, 0xBF402016);
	//}

	core_save_disable(&status);
	while (1) {
		/* Stop system or wait until reset. */
	}
	return -1;
}

int sbcall_setdve(tge_sbcall_setdve_arg_t *arg)
{
	printf("set DVE mode %d\n", arg->mode);

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
	arg = arg;
	return -1;
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
			printf("Invalidate DCache not supported (1).\n");
			break;
		case 2:
			/* Invalidate ICache. */
			invalidateICacheAll();
			break;
		case 3:
			/* Invalidate Cache. */
			invalidateICacheAll();
			printf("Invalidate DCache not supported (3).\n");
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

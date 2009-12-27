/*
 * intrelay.c - TGE interupt relay
 *
 * Copyright (c) 2003 Marcus R. Brown <mrbrown@0xd6.org>
 *
 * See the file LICENSE, located within this directory, for licensing terms.
 */

#include "types.h"
#include "defs.h"
#include "irx.h"

#ifdef DEV9_SUPPORT
#include "speedregs.h"
#include "dev9.h"
#endif

#include "tge_hwdefs.h"
#include "usb.h"
#include "irx_imports.h"

IRX_ID("intrelay", 1, 1);

#ifdef RPC_IRQ_SUPPORT
#define SIF_CMD_INTERRUPT 0x20

#define IRQ_SBUS_AIF    40
#define IRQ_SBUS_PCIC   41
#define IRQ_SBUS_USB    42
#else
#define SIF_SMFLAG	0xbd000030
#endif

#ifdef RPC_IRQ_SUPPORT
typedef struct {
	struct t_SifCmdHeader    sifcmd;
	u32 data[16];
} iop_sifCmdBufferIrq_t;

static iop_sifCmdBufferIrq_t sifCmdBufferIrq __attribute__((aligned(64)));
#endif

int intr_usb_handler(void *unused)
{
#ifndef RPC_IRQ_SUPPORT
	_sw(TGE_SBUS_IRQ_USB, SIF_SMFLAG);

	_sw(_lw(0xbf801450) | 2, 0xbf801450);
	_sw(_lw(0xbf801450) & 0xfffffffd, 0xbf801450);
	_lw(0xbf801450);
#else
	sifCmdBufferIrq.data[0] = IRQ_SBUS_USB;
	isceSifSendCmd(SIF_CMD_INTERRUPT, &sifCmdBufferIrq, 64, NULL,
                NULL, 0);
#endif
	
	return 1;
}

int intr_dev9_handler(void *unused)
{
#ifndef RPC_IRQ_SUPPORT
	_sw(TGE_SBUS_IRQ_DEV9, SIF_SMFLAG);

	_sw(_lw(0xbf801450) | 2, 0xbf801450);
	_sw(_lw(0xbf801450) & 0xfffffffd, 0xbf801450);
	_lw(0xbf801450);
#else
	sifCmdBufferIrq.data[0] = IRQ_SBUS_PCIC;
	isceSifSendCmd(SIF_CMD_INTERRUPT, &sifCmdBufferIrq, 64, NULL,
                NULL, 0);
#endif
	
	return 1;
}

#ifdef DEV9_SUPPORT
int intr_dev9_handler_dev9(int flag)
{
#ifndef RPC_IRQ_SUPPORT
	_sw(TGE_SBUS_IRQ_DEV9, SIF_SMFLAG);

	_sw(_lw(0xbf801450) | 2, 0xbf801450);
	_sw(_lw(0xbf801450) & 0xfffffffd, 0xbf801450);
	_lw(0xbf801450);
#else
	sifCmdBufferIrq.data[0] = IRQ_SBUS_PCIC;
	isceSifSendCmd(SIF_CMD_INTERRUPT, &sifCmdBufferIrq, 64, NULL,
                NULL, 0);
#endif
	
	return 1;
}
#endif

int intr_ilink_handler(void *unused)
{
#ifndef RPC_IRQ_SUPPORT
	_sw(TGE_SBUS_IRQ_ILINK, SIF_SMFLAG);

	_sw(_lw(0xbf801450) | 2, 0xbf801450);
	_sw(_lw(0xbf801450) & 0xfffffffd, 0xbf801450);
	_lw(0xbf801450);
#else
	sifCmdBufferIrq.data[0] = IRQ_SBUS_AIF; /* XXX: Correct number??? */
	isceSifSendCmd(SIF_CMD_INTERRUPT, &sifCmdBufferIrq, 64, NULL,
                NULL, 0);
#endif
	
	return 1;
}

void nullthread(void *unused)
{
	while(1) {
		SleepThread();
	}
}

int _start(int argc, char *argv[])
{
	iop_thread_t thread;
	int res;
	int locked = 0;

#ifdef RPC_IRQ_SUPPORT
	if (!sceSifCheckInit())
		sceSifInit();
	sceSifInitRpc(0);
#endif

	if ((res = RegisterIntrHandler(IOP_IRQ_DEV9, 1, intr_dev9_handler, NULL))) {
		printf("intr 0x%02x, error %d\n", IOP_IRQ_DEV9, res);
		locked |= 1 << IOP_IRQ_DEV9;

#ifdef DEV9_SUPPORT
		/* ps2dev9 seems to be running use other way to get interrupts. */
		dev9RegisterIntrCb(1, intr_dev9_handler_dev9);
		dev9RegisterIntrCb(0, intr_dev9_handler_dev9);

		dev9IntrEnable(SPD_INTR_ATA0);
		dev9IntrEnable(SPD_INTR_ATA1);
#endif
	}

	if ((res = RegisterIntrHandler(IOP_IRQ_USB, 1, intr_usb_handler, NULL))) {
		printf("intr 0x%02x, error %d\n", IOP_IRQ_USB, res);
		locked |= 1 << IOP_IRQ_USB;
	}

	if ((res = RegisterIntrHandler(IOP_IRQ_ILINK, 1, intr_ilink_handler, NULL))) {
		printf("intr 0x%02x, error %d\n", IOP_IRQ_ILINK, res);
		locked |= 1 << IOP_IRQ_ILINK;
	}

	CpuEnableIntr();
	if (!(locked & (1 << IOP_IRQ_DEV9))) {
		EnableIntr(IOP_IRQ_DEV9);
	}
	if (!(locked & (1 << IOP_IRQ_USB))) {
		EnableIntr(IOP_IRQ_USB);
	}
	if (!(locked & (1 << IOP_IRQ_ILINK))) {
		EnableIntr(IOP_IRQ_ILINK);
	}

	thread.attr = TH_C;
	thread.option = 0;
	thread.thread = nullthread;
	thread.stacksize = 1024;
	thread.priority = 126;

	if ((res = CreateThread(&thread)) < 0) {
		printf("cannot create thread\n");
		return -1;
	}

	initUSB();

	StartThread(res, NULL);
	return 0;
}

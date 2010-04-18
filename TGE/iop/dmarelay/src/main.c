
/*
 * main.c - TGE DMA relay
 *
 * Copyright (c) 2003 Marcus R. Brown <mrbrown@0xd6.org>
 *
 * See the file LICENSE, located within this directory, for licensing terms.
 */

#include "dmarelay.h"

IRX_ID(MODNAME, 1, 2);

struct eng_args dma_struct = { -1, -1 };

static int dev9_dma_handler(void *arg)
{
	struct eng_args *dma_struct = (struct eng_args *) arg;

	iSetEventFlag(dma_struct->evflg, EF_DMA_DONE);
	return 1;
}

int _start(int argc, char *argv[])
{
	iop_event_t event;
	int res;

	M_PRINTF("IOP DMA relay module\n");

	if (!sceSifCheckInit()) {
		sceSifInit();
	}

	sceSifInitRpc(0);

	dma_struct.semid = CreateMutex(IOP_MUTEX_UNLOCKED);
	if (dma_struct.semid <= 0) {
		E_PRINTF("Unable to create %s (error %d).\n", "semaphore", dma_struct.semid);
		return 1;
	}

	event.attr = event.bits = event.option = 0;
	dma_struct.evflg = CreateEventFlag(&event);
	if (dma_struct.evflg <= 0) {
		E_PRINTF("Unable to create %s (error %d).\n", "event flag", dma_struct.evflg);
		return 1;
	}

	CpuEnableIntr();
	DisableIntr(IOP_IRQ_DMA_DEV9, NULL);
	res = RegisterIntrHandler(IOP_IRQ_DMA_DEV9, 1, dev9_dma_handler, &dma_struct);
	if (res != 0) {
		E_PRINTF("Unable to register 0x%02x intr handler (error %d).\n",
			IOP_IRQ_DMA_DEV9, res);
		return 1;
	}

	_sw(_lw(0xbf801570) | 0x80, 0xbf801570);

	res = ata_engine_init(&dma_struct);
	if (res <= 0) {
		E_PRINTF("Unable to initialize the %s DMA engine.\n", "ATA");
		return 1;
	}

	res = smap_engine_init(&dma_struct);
	if (res <= 0) {
		E_PRINTF("Unable to initialize the %s DMA engine.\n", "SMAP");
		return 1;
	}

	M_PRINTF("ATA/SMAP DMA relay module initialized.\n");
	return 0;
}

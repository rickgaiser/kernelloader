
/*
 * ata.c - TGE DMA relay for PS2 ATA
 *
 * Copyright (c) 2003 Marcus R. Brown <mrbrown@0xd6.org>
 *
 * See the file LICENSE, located within this directory, for licensing terms.
 */

#include "dmarelay.h"

static ata_dma_transfer_t dma_transfer;

static SifRpcDataQueue_t qd, end_qd;
static SifRpcServerData_t sd, end_sd;

static void *dma_buffer;

static int read_thid, write_thid, cur_thid;
static int rpc_result;

static void dma_setup(int val)
{
	USE_SPD_REGS;
	u8 if_ctrl;

	if_ctrl = SPD_REG8(SPD_R_IF_CTRL) & 0x01;

	if (!val)
		if_ctrl |= 0x7e;
	else
		if_ctrl |= 0x7c;

	SPD_REG8(SPD_R_IF_CTRL) = if_ctrl;

	SPD_REG8(SPD_R_XFR_CTRL) = val | 0x02;
	SPD_REG8(SPD_R_DMA_CTRL) = 0x06;
	SPD_REG8(0x38) = 0x03;

	_sw(0, DEV9_DMAC_CHCR);
}

static void dma_stop(int val)
{
	USE_SPD_REGS;
	u8 if_ctrl;

	_sw(0, DEV9_DMAC_CHCR);

	SPD_REG8(SPD_R_XFR_CTRL) = 0;
	SPD_REG8(SPD_R_IF_CTRL) = SPD_REG8(SPD_R_IF_CTRL) & 0xfb;

	if (val) {
		if_ctrl = SPD_REG8(SPD_R_IF_CTRL);
		SPD_REG8(SPD_R_IF_CTRL) = SPD_IF_ATA_RESET;
		DelayThread(100);
		SPD_REG8(SPD_R_IF_CTRL) = if_ctrl;
	}

	/*M_PRINTF("ATA DMA force break\n"); */
}

static void ata_dma_read_thread(void *arg)
{
	USE_SPD_REGS;
	volatile iop_dmac_chan_t *dev9_chan =
		(volatile iop_dmac_chan_t *) DEV9_DMAC_BASE;
	struct eng_args *args = (struct eng_args *) arg;
	ata_dma_transfer_t *t = &dma_transfer;	/* s7 */
	SifDmaTransfer_t *dmat;		/* s3 */
	u32 res;
	int c;						/* s1 */
	int k;						/* s2 */
	int init;					/* s4 */
	u32 did;					/* s6 */
	SifDmaTransfer_t *transfer;	/* s0 */

	did = -1;
	while (1) {
	  start_loop:
		while (SleepThread() || WaitSema(args->semid));

		dmat = t->dmat;
		transfer = NULL;
		init = 1;
		ClearEventFlag(args->evflg, 0);

		dma_setup(0);
		EnableIntr(IOP_IRQ_DMA_DEV9);

		/* Initiate the DMA transfer.  */
		dev9_chan->madr = (u32) dma_buffer;
		k = t->count;
		c = k;
		do {
			if (c > 0) {
				dev9_chan->bcr = ((dmat->size / 128) << 16) | 32;
				dev9_chan->chcr = 0x01000200;

				if (init > 0) {
					SPD_REG8(0x4e) = t->command;	/* ATA command register.  */
					SPD_REG8(SPD_R_PIO_DIR) = 1;
					SPD_REG8(SPD_R_PIO_DATA) = 0;
					DelayThread(1000);	/* XXX: Is this required? */
					SPD_REG8(SPD_R_XFR_CTRL) |= 0x80;
					init = 0;
				}
			}

			if (transfer != 0) {
				do {
					int intStatus;

					CpuSuspendIntr(&intStatus);
					did = SifSetDma(transfer, 1);
					CpuResumeIntr(intStatus);
				} while (did == 0);
			}
			if (c > 0) {
				WaitEventFlag(args->evflg, (EF_DMA_DONE | EF_ATA_DONE), 0x11,
					&res);
				dmat++;
				if (res & EF_ATA_DONE) {
					/* If we got the ATA end signal, force stop the transfer.  */
					DisableIntr(IOP_IRQ_DMA_DEV9, NULL);
					dma_stop(1);
					SignalSema(args->semid);
					goto start_loop;
				}
				c--;
			}
			if (transfer != NULL) {
				transfer++;
				k--;
			}
			else {
				transfer = t->dmat;
			}
		} while (k > 0);

		DisableIntr(IOP_IRQ_DMA_DEV9, NULL);
		do {
			if (SifDmaStat(did) < 0) {
				break;
			}
			PollEventFlag(args->evflg, EF_ATA_DONE, 0x11, &res);
		} while (!(res & EF_ATA_DONE));

		/* If we got the ATA end signal, force stop the transfer.  */
		if (res & EF_ATA_DONE)
			dma_stop(1);

		SPD_REG8(0x5c) = t->devctrl;

		SignalSema(args->semid);
	}
}

static void ata_dma_write_thread(void *arg)
{
	USE_SPD_REGS;
	volatile iop_dmac_chan_t *dev9_chan =
		(volatile iop_dmac_chan_t *) DEV9_DMAC_BASE;
	struct eng_args *args = (struct eng_args *) arg;
	ata_dma_transfer_t *t = &dma_transfer;
	u32 res;

	while (1) {
		while (SleepThread() || WaitSema(args->semid));

		ClearEventFlag(args->evflg, 0);

		dma_setup(1);
		EnableIntr(IOP_IRQ_DMA_DEV9);

		/* Initiate the DMA transfer.  */
		dev9_chan->madr = (u32) dma_buffer;
		dev9_chan->bcr = ((t->size / 128) << 16) | 32;
		dev9_chan->chcr = 0x01000201;

		SPD_REG8(0x4e) = t->command;	/* ATA command register.  */
		SPD_REG8(SPD_R_PIO_DIR) = 1;
		SPD_REG8(SPD_R_PIO_DATA) = 0;
		DelayThread(1000);		/* XXX: Is this required? */
		SPD_REG8(SPD_R_XFR_CTRL) |= 0x80;

		WaitEventFlag(args->evflg, (EF_DMA_DONE | EF_ATA_DONE), 0x11, &res);

#if 0
		/* XXX: Add this? */
		SPD_REG8(SPD_R_XFR_CTRL) &= 0x7f;
#endif

		DisableIntr(IOP_IRQ_DMA_DEV9, NULL);

		/* If we got the ATA end signal, force stop the transfer.  */
		if (res & EF_ATA_DONE)
			dma_stop(1);

		SignalSema(args->semid);
	}
}

static void *rpc_func(int fid, void *buf, int bufsize)
{
	int *res = &rpc_result;
	int thid = -1;

	switch (fid) {
	case RPCF_GetBufAddr:
		*res = (u32) dma_buffer;
		break;
	case RPCF_DmaRead:
		thid = read_thid;
		WakeupThread(thid);
		break;
	case RPCF_DmaWrite:
		thid = write_thid;
		WakeupThread(thid);
		break;

	default:
		M_PRINTF("rpc_func: invalid function code (%d).\n", fid);
		*res = -1;
	}

	if (thid >= 0) {
		cur_thid = thid;
		*res = 0;
	}

	return res;
}

static void ata_rpc_thread(void *unused)
{
	sceSifSetRpcQueue(&qd, GetThreadId());

	sceSifRegisterRpc(&sd, RPCS_ATA_DMA_BEGIN, rpc_func, &dma_transfer, NULL,
		NULL, &qd);

	sceSifRpcLoop(&qd);
}

static void *rpc_end_func(int fid, void *buf, int bufsize)
{
	SetEventFlag(dma_struct.evflg, EF_ATA_DONE);

	ReleaseWaitThread(cur_thid);
	return NULL;
}

static void ata_rpc_end_thread(void *unused)
{
	sceSifSetRpcQueue(&end_qd, GetThreadId());

	sceSifRegisterRpc(&end_sd, RPCS_ATA_DMA_END, rpc_end_func, NULL, NULL, NULL,
		&end_qd);

	sceSifRpcLoop(&end_qd);
}

int ata_engine_init(struct eng_args *args)
{
	iop_thread_t thread;
	int thid;

	/* DMA read thread.  */
	thread.attr = TH_C;
	thread.thread = ata_dma_read_thread;
	thread.stacksize = 4096;
	thread.priority = 21;
	thread.option = 0;
	read_thid = CreateThread(&thread);
	if (read_thid <= 0) {
		return read_thid;
	}

	StartThread(read_thid, args); // XXX: a1 = NULL

	/* DMA write thread.  */
	thread.attr = TH_C;
	thread.thread = ata_dma_write_thread;
	thread.stacksize = 4096;
	thread.priority = 21;
	thread.option = 0;
	write_thid = CreateThread(&thread);
	if (write_thid <= 0) {
		return write_thid;
	}

	StartThread(write_thid, args); // XXX: a1 = NULL

	/* RPC thread.  */
	thread.attr = TH_C;
	thread.thread = ata_rpc_thread;
	thread.stacksize = 4096;
	thread.priority = 20;
	thread.option = 0;
	thid = CreateThread(&thread);
	if (thid <= 0) {
		return thid;
	}

	StartThread(thid, NULL); // XXX: a1 = NULL

	/* RPC end thread.  */
	thread.attr = TH_C;
	thread.thread = ata_rpc_end_thread;
	thread.stacksize = 1024;
	thread.priority = 20;
	thread.option = 0;
	thid = CreateThread(&thread);
	if (thid <= 0) {
		return thid;
	}

	StartThread(thid, NULL); //XXX: a1 = NULL

	if (!(dma_buffer = AllocSysMemory(0, ATA_BUFFER_SIZE, NULL)))
		return -12;

	return 1;
}

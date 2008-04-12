/*
 * sifcmd.c - Native SIF Command interface.
 *
 * Copyright (c) 2003 Marcus R. Brown <mrbrown@0xd6.org>
 * Copyright (c) 2007 Mega Man
 *
 * See the file LICENSE, located within this directory, for licensing terms.
 */

#include "tge_types.h"
#include "tge_defs.h"

#include "tge_sbios.h"
#include "tge_sifdma.h"

#include "sbcalls.h"
#include "sif.h"
#include "hwreg.h"
#include "core.h"

typedef struct {
	void *handler;
	void *harg;
} sifCmdHandler_t;

extern u32 sbios_iopaddr;
static int initialized;		/* 0x80007fa0 */
static u32 unkP1[8];		/* 0x80009480 */
static u32 unkP2[8];		/* 0x80009500 */
static u32 sifCmdInitPkt[5] __attribute__((aligned(16))); /* 0x80009540 */
static u32 sifCmdCfg[8];	/* 0x80009554 */
static sifCmdHandler_t *sifCmdHandlerTable1;/* 0x80009560 */
static u32 sifCmdHandlerTable1size;
static sifCmdHandler_t *sifCmdHandlerTable2;/* 0x80009568 */
static u32 sifCmdHandlerTable2size;
static sifCmdHandler_t sifCmdSysBuffer[32];		/* 0x80009578 */
static u32 unkP4[32];		/* 0x80009678 - 0x800096f4 */

static u32 internal_sif_cmd_send(u32 fid, void *packet, u32 packet_size, void *src, void *dest, u32 size);

u32 sif_cmd_send(u32 fid, u32 flags, void *packet, u32 packet_size, void *src, void *dest, u32 size)
{
	tge_sifdma_transfer_t transfer[2];
	u32 count;
	u32 *pp = packet;

	count = 0;
	if (packet_size < 112) {
		if (size > 0) {
			count++;
			transfer[count].src = src;
			transfer[count].dest = dest;
			transfer[count].size = size;
			transfer[count].attr = 0;
			pp[1] = dest;
			pp[0] = *((u8 *) packet) | (size << 8);
			if ((flags & 4) != 0) {
				core_dcache_writeback(src, size);
			}
		} else {
			pp[4] = 0;
			pp[0] = *((u8 *) packet);
		}
		transfer[count].src = packet;
		transfer[count].dest = sifCmdCfg[2];
		transfer[count].size = packet_size;
		transfer[count].attr = 0x44;
		count++;

		pp[2] = fid;
		*((u8 *) packet) = packet_size;

		core_dcache_writeback(packet, packet_size);
		return sif_set_dma(transfer, count);
	} else {
		return 0;
	}
}

static void sif_cmd_interrupt()
{
}

/* Function is same as RTE. */
static u32 sif_cmd_set_sreg(u32 reg, u32 val)
{
	return 0;
}

/* Function is same as RTE. */
u32 sif_cmd_get_sreg(u32 reg)
{
	return 0;
}

/* Function is same as RTE. */
static void *sif_cmd_data_table()
{
	return NULL;
}

/* 0x80002950 */
static u32 handleSysCmd1(u32 *a0, u32 *a1)
{
	u32 idx;
	u32 *table;

	idx = a0[4];
	table = (u32 *) a1[7];
	table[idx] = a0[5];

	/* Did the function have a return value? */
	return &table[idx];
}

/* 0x80002970 */
static u32 handleSysCmd0(u32 *a0, u32 *a1)
{
	u32 ret;

	ret = a0[4];
	a1[2] = ret;

	/* Did the function have a return value? */
	return ret;
}

int sif_cmd_init()
{
	u32 status;
	int i;
	u32 *addr;

	core_save_disable(&status);
	if (initialized) {
		core_restore(status);
		return 0;
	}
	initialized = 1;

	sifCmdCfg[0] = KSEG1ADDR((u32 *)unkP1);
	sifCmdCfg[1] = KSEG1ADDR((u32 *)unkP2);
	sifCmdCfg[2] = 0;
	sifCmdHandlerTable1 = sifCmdSysBuffer;
	sifCmdHandlerTable1size = 32;
	sifCmdHandlerTable2 = NULL;
	sifCmdHandlerTable2size = 0;
	sifCmdCfg[7] = unkP4;

	for(i = 0; i < 32; i++) {
		sifCmdSysBuffer[i].handler = NULL;
		sifCmdSysBuffer[i].harg = NULL;
	}

	for(i = 0; i < 32; i++) {
		unkP4[i] = 0;
	}

	sifCmdSysBuffer[0].handler = handleSysCmd0;
	sifCmdSysBuffer[0].harg = sifCmdCfg;
	sifCmdSysBuffer[1].handler = handleSysCmd1;
	sifCmdSysBuffer[1].harg = sifCmdCfg;
	core_restore(status);

	sifCmdInitPkt[4] = PHYSADDR((u32 *)unkP1);
	internal_sif_cmd_send(0x80000000, sifCmdInitPkt, 0x14, NULL, NULL, 0);

	sif_set_reg(0x80000000, sbios_iopaddr);
	sif_set_reg(0x80000001, sbios_iopaddr);
	return 0;
}

static u32 internal_sif_cmd_send(u32 fid, void *packet, u32 packet_size, void *src, void *dest, u32 size)
{
	return sif_cmd_send(fid, 0, packet, packet_size, src,
			dest, size);
}

/* Functions is same as RTE. */
int sif_cmd_exit()
{
	initialized = 0;
	return 0;
}

/* Functions is same as RTE. */
void sif_cmd_add_handler(u32 fid, void *handler, void *harg)
{
	if (fid & 0x80000000)
	{
		sifCmdHandlerTable1[fid].handler = handler;
		sifCmdHandlerTable1[fid].harg = harg;
	}
	else
	{
		sifCmdHandlerTable2[fid].handler = handler;
		sifCmdHandlerTable2[fid].harg = harg;
	}
}

/* Functions is same as RTE. */
static void sif_cmd_del_handler(u32 fid)
{
	if (fid & 0x80000000)
	{
		sifCmdHandlerTable1[fid].handler = NULL;
	}
	else
	{
		sifCmdHandlerTable2[fid].handler = NULL;
	}
}

/* Functions is same as RTE. */
static void *sif_cmd_set_buffer(void *buf, u32 size)
{
	void *old;

	old = sifCmdHandlerTable2;

	sifCmdHandlerTable2 = buf;
	sifCmdHandlerTable2size = size;

	return old;
}

/* Functions is not same as RTE, original function does nothing and return NULL. */
static void *sif_cmd_set_sys_buffer(void *buf, u32 size)
{
	void *old;

	old = sifCmdHandlerTable1;

	sifCmdHandlerTable1 = buf;
	sifCmdHandlerTable1size = size;

	return old;
}

/* SBIOS interface.  */
int sbcall_sifinitcmd()
{
	return sif_cmd_init();
}

int sbcall_sifexitcmd()
{
	return sif_cmd_exit();
}

int sbcall_sifsendcmd(tge_sbcall_sifsendcmd_arg_t *arg)
{
	return sif_cmd_send(arg->fid, 0, arg->packet, arg->packet_size, arg->src,
			arg->dest, arg->size);
}

int sbcall_sifcmdintrhdlr()
{
	sif_cmd_interrupt();
	return 0;
}

int sbcall_sifaddcmdhandler(tge_sbcall_sifaddcmdhandler_arg_t *arg)
{
	sif_cmd_add_handler(arg->fid, arg->handler, arg->harg);
	return 0;
}

int sbcall_sifremovecmdhandler(tge_sbcall_sifremovecmdhandler_arg_t *arg)
{
	sif_cmd_del_handler(arg->fid);
	return 0;
}

int sbcall_sifsetcmdbuffer(tge_sbcall_sifsetcmdbuffer_arg_t *arg)
{
	return (int)sif_cmd_set_buffer(arg->buf, arg->size);
}

int sbcall_sifsetsreg(tge_sbcall_sifsetsreg_arg_t *arg)
{
	return sif_cmd_set_sreg(arg->reg, arg->val);
}

int sbcall_sifgetsreg(tge_sbcall_sifgetsreg_arg_t *arg)
{
	return sif_cmd_get_sreg(arg->reg);
}

int sbcall_sifgetdatatable()
{
	return (int)sif_cmd_data_table();
}

int sbcall_sifsetsyscmdbuffer(tge_sbcall_sifsetsyscmdbuffer_arg_t *arg)
{
	return (int)sif_cmd_set_sys_buffer(arg->buf, arg->size);
}

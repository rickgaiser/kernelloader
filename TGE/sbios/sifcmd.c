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
#include "iopmem.h"

/** Maximum packet data size when calling sif_cmd_send(). */
#define CMD_PACKET_DATA_MAX 112

/** MSB is set for system commands. */
#define SYSTEM_CMD	0x80000000
#define SIF_CMD_CHANGE_SADDR (SYSTEM_CMD | 0x00)
/** Set software register. */
#define SIF_CMD_SET_SREG (SYSTEM_CMD | 0x01)
/** Initialize command sif interface. */
#define SIF_CMD_INIT_CMD (SYSTEM_CMD | 0x02)
/** Reset IOP processor. */
#define SIF_CMD_RESET_CMD (SYSTEM_CMD | 0x03)

#define D5_CHCR 0x1000C000
#define D5_MADR 0x1000C010
#define D5_QWC 0x1000C020
#define D5_TADR 0x1000C030

typedef struct {
	void *handler;
	void *harg;
} sifCmdHandler_t;

typedef struct {
	/** Packet size (8 bit) + additional data size (24 Bit). */
	u32 size;
	/** Pointer to additional data. */
	void *dest;
	/** Command/function identifier. */
	int fid;
	/** Additional option (used by SIF_CMD_INIT_CMD). */
	u32 option;
} tge_sifcmd_header_t;

/* Even though I'm reluctant to do this, I've made this structure binary
 * compatible with the SCE libs and ps2lib.  In all implementations, a
 * pointer to this data is stored in SIF register 0x80000001.  Each routine
 * that relies on this data can then use the data referenced from that
 * register, so that even if a SIF library is initialized after this one, we
 * should still work exactly as expected.  */
typedef struct {
	void    *pktbuf;    /* Command packet received from the IOP */
	void    *cmdbuf;    /* Called "cmdbuf", but unused. */
	void    *iopbuf;    /* Address of IOP SIF DMA receive address */
	sifCmdHandler_t *sys_cmd_handlers;
	u32 nr_sys_handlers;
	sifCmdHandler_t *usr_cmd_handlers;
	u32 nr_usr_handlers;
	int *sregs;
} cmd_data_t;

/** DMA packet for command SIF_CMD_SET_SREG. */
typedef struct {
	tge_sifcmd_header_t header;
	u32 sreg;
	int val;
} sr_pkt_t;

//** DMA packet for command SIF_CMD_CHANGE_SADDR. */
typedef struct {
	tge_sifcmd_header_t header;
	void *buf;
} ca_pkt_t;


extern u32 sbios_iopaddr;
static int initialized;		/* 0x80007fa0 */
static u8 pktbuf[128] __attribute__((aligned(64)));		/* 0x80009480 */
/* Define this so that in the unlikely case another SIF implementation decides
 * to use it, it won't crash. Otherwise unused.
 */
static u8 cmdbuf[64] __attribute__((aligned(64))); /* 0x80009500 */
static u32 sifCmdInitPkt[5] __attribute__((aligned(64))); /* 0x80009540 */
cmd_data_t _sif_cmd_data; /* 0x80009554 */
static sifCmdHandler_t *sifCmdHandlerTable1; /* 0x80009560 */
static u32 sifCmdHandlerTable1size;
static sifCmdHandler_t *sifCmdHandlerTable2; /* 0x80009568 */
static u32 sifCmdHandlerTable2size;
static sifCmdHandler_t sifCmdSysBuffer[32]; /* 0x80009578 */
static u32 sregs[32]; /* 0x80009678 - 0x800096f4 */
sr_pkt_t testSr __attribute__((aligned(64)));

static u32 internal_sif_cmd_send(u32 fid, void *packet, u32 packet_size, void *src, void *dest, u32 size);

void dmac5_debug(void)
{
	volatile uint32_t *d5_chcr = KSEG1ADDR(D5_CHCR);
	volatile uint32_t *d5_qwc = KSEG1ADDR(D5_QWC);
	volatile uint32_t *d5_madr = KSEG1ADDR(D5_MADR);

	iop_prints("D5_CHCR 0x");
	iop_printx(*d5_chcr);
	iop_prints("\n");
	iop_prints("D5_MADR 0x");
	iop_printx(*d5_madr);
	iop_prints("\n");
	iop_prints("D5_QWC  0x");
	iop_printx(*d5_qwc);
	iop_prints("\n");
}

/**
 * Send SIF1 command from EE to IOP using DMA.
 * @param src EE memory source address.
 * @param dest IOP memory destination address.
 * @param size Size of data to be copied in bytes. 0, if nothing should be
 *   transfered.
 * @return 0, on error.
 * @return ID, describing DMA transfers.
 */
u32 sif_cmd_send(u32 fid, u32 flags, void *packet, u32 packet_size, void *src, void *dest, u32 size)
{
	tge_sifdma_transfer_t transfer[2];
	tge_sifcmd_header_t *header = packet;
	u32 count = 0;

	if (packet_size > CMD_PACKET_DATA_MAX) {
		return 0;
	}
	header = (tge_sifcmd_header_t *) packet;
	header->fid = fid;
	header->size = packet_size;
	header->dest = NULL;

	if (size > 0) {
		header->dest = dest;
		header->size = packet_size | (size << 8);
		if (flags & 4) {
			core_dcache_writeback(src, size);
		}
		transfer[count].src = src;
		transfer[count].dest = dest;
		transfer[count].size = size;
		transfer[count].attr = 0;
		count++;
	} else {
		header->option = 0;
	}
	transfer[count].src = packet;
	transfer[count].dest = _sif_cmd_data.iopbuf;
	transfer[count].size = packet_size;
	transfer[count].attr = TGE_SIFDMA_ATTR_ERT | TGE_SIFDMA_ATTR_INT_O;
	count++;

	core_dcache_writeback(packet, packet_size);
	return sif_set_dma(transfer, count);
}

static void sif_cmd_interrupt()
{
}

/* Function is not implemented in RTE. */
static u32 sif_cmd_set_sreg(u32 reg, u32 val)
{
	_sif_cmd_data.sregs[reg] = val;
	return 0;
}

/* Function is not implemented in RTE. */
u32 sif_cmd_get_sreg(u32 reg)
{
	iop_prints("sif_cmd_get_sreg 0x");
	iop_printx(reg);
	iop_prints(" = 0x");
	iop_printx(_sif_cmd_data.sregs[reg]);
	iop_prints("\nx");
	return _sif_cmd_data.sregs[reg];
}

/* Function is same as RTE. */
static void *sif_cmd_data_table()
{
	return NULL;
}

/* 0x80002950 */
static void set_sreg(void *packet, void *harg)
{
	cmd_data_t *cmd_data = (cmd_data_t *)harg;
	sr_pkt_t *pkt = (sr_pkt_t *)packet;

	cmd_data->sregs[pkt->sreg] = pkt->val;
}

/* 0x80002970 */
static void change_addr(void *packet, void *harg)
{
	cmd_data_t *cmd_data = (cmd_data_t *)harg;
	ca_pkt_t *pkt = (ca_pkt_t *)packet;
	cmd_data->iopbuf = pkt->buf;
}

int sif_cmd_init()
{
	u32 status;
	int i;

	iop_prints("sif_cmd_init()\n");

	core_save_disable(&status);
	if (initialized) {
		core_restore(status);
		return 0;
	}
	initialized = 1;

	_sif_cmd_data.pktbuf = KSEG1ADDR((u32 *)pktbuf);
	_sif_cmd_data.cmdbuf = KSEG1ADDR((u32 *)cmdbuf);
	_sif_cmd_data.iopbuf = 0;
	sifCmdHandlerTable1 = sifCmdSysBuffer;
	sifCmdHandlerTable1size = 32;
	sifCmdHandlerTable2 = NULL;
	sifCmdHandlerTable2size = 0;
	_sif_cmd_data.sregs = sregs;

	for(i = 0; i < 32; i++) {
		sifCmdSysBuffer[i].handler = NULL;
		sifCmdSysBuffer[i].harg = NULL;
	}

	for(i = 0; i < 32; i++) {
		sregs[i] = 0;
	}

	sifCmdSysBuffer[0].handler = change_addr;
	sifCmdSysBuffer[0].harg = &_sif_cmd_data;
	sifCmdSysBuffer[1].handler = set_sreg;
	sifCmdSysBuffer[1].harg = &_sif_cmd_data;
	core_restore(status);

	/* No check here if IOP is alredy initialized. Assumption is that it is
	 * already initialized
	 */
	/* give it our new receive address. */
	dmac5_debug();
	iop_prints("Send command 0x80000000\n");

	_sif_cmd_data.iopbuf = (void *) sbios_iopaddr; /* XXX: inserted for test. */
	((ca_pkt_t *)(sifCmdInitPkt))->buf = (void *) PHYSADDR((u32 *)_sif_cmd_data.pktbuf);
	internal_sif_cmd_send(SIF_CMD_CHANGE_SADDR, sifCmdInitPkt, sizeof(ca_pkt_t), NULL, NULL, 0);
	iop_prints("Back from sending.\n");
	dmac5_debug();

#if 0
	/* RTE does the following: */
	sif_set_reg(0x80000000, sbios_iopaddr);
	sif_set_reg(0x80000001, sbios_iopaddr);
#else
	/* XXX: PS2SDK code looks better: */
	sif_set_reg(0x80000000, (uint32_t) _sif_cmd_data.iopbuf);
	sif_set_reg(0x80000001, (uint32_t) &_sif_cmd_data);
#endif
#if 0
	iop_prints("test sreg 31\n");
	testSr.sreg = 31;
	testSr.val = 234;
	internal_sif_cmd_send(SIF_CMD_SET_SREG, &testSr, sizeof(testSr), NULL, NULL, 0);
	iop_prints("Command send.\n");
	dmac5_debug();
#endif
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
		sifCmdHandlerTable1[fid & 0x7FFFFFFF].handler = handler;
		sifCmdHandlerTable1[fid & 0x7FFFFFFF].harg = harg;
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
		sifCmdHandlerTable1[fid & 0x7FFFFFFF].handler = NULL;
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

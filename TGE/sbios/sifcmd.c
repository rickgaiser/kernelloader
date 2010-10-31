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

#include "sifdma.h"
#include "sifcmd.h"

#include "sbcalls.h"
#include "sif.h"
#include "hwreg.h"
#include "core.h"
#include "iopmemdebug.h"
#include "stdio.h"

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

#define CMD_HANDLER_MAX 32

typedef struct {
	void (*handler)( void *a, void *b);
	void *harg;
} sifCmdHandler_t;

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
static sifCmdHandler_t sifCmdSysBuffer[CMD_HANDLER_MAX]; /* 0x80009578 */
static u32 sregs[32]; /* 0x80009678 - 0x800096f4 */
//sr_pkt_t testSr __attribute__((aligned(64)));

void dmac5_debug(void)
{
#ifdef SHARED_MEM_DEBUG
	volatile uint32_t *d5_chcr = KSEG1ADDR(D5_CHCR);
	volatile uint32_t *d5_qwc = KSEG1ADDR(D5_QWC);
	volatile uint32_t *d5_madr = KSEG1ADDR(D5_MADR);
#endif

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
			SifWriteBackDCache(src, size);
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

	SifWriteBackDCache(packet, packet_size);
	return SifSetDma(transfer, count);
}

static void sif_cmd_interrupt()
{
	u128 packetbuf[8 + 1];
	u128 *packet;
	volatile unsigned int packet_aligned; /* volatile needed, because caller doesn't respect stack alignment. */
	u128 *pktbuf;
	cmd_data_t *cmd_data = &_sif_cmd_data;
	tge_sifcmd_header_t *header;
	sifCmdHandler_t *cmd_handlers = NULL;
	int size, pktquads, i = 0;
	uint32_t id;

	/* Align packet or u128 copy will fail. */
	packet_aligned = (unsigned int) packetbuf;
	packet_aligned += 16 - 1;
	packet_aligned &= ~(16 - 1);
	packet = (void *) packet_aligned;
	header = (tge_sifcmd_header_t *)cmd_data->pktbuf;

	if (!(size = (header->size & 0xff)))
		goto out;

	/* TODO: Don't copy anything extra */
	pktquads = (size + 30) >> 4;
	header->size = 0;
	if (pktquads) {
		pktbuf = (u128 *)cmd_data->pktbuf;
		while (pktquads--) {
			packet[i] = pktbuf[i];
			i++;
		}
	}

	sbcall_sifsetdchain();

	header = (tge_sifcmd_header_t *)packet;
	/* Get the command handler id and determine which handler list to
	   dispatch from.  */
#if defined(SBIOS_DEBUG) && defined(SHARED_MEM_DEBUG)
	printf("fid 0x%x\n", header->fid);
#endif
	id = header->fid & ~SYSTEM_CMD;

	if (header->fid & SYSTEM_CMD) {
		if (id < _sif_cmd_data.nr_sys_handlers) {
			cmd_handlers = cmd_data->sys_cmd_handlers;
		}
	} else {
		if (id < _sif_cmd_data.nr_usr_handlers) {
			cmd_handlers = cmd_data->usr_cmd_handlers;
		}
	}

	if ((cmd_handlers != NULL) && (cmd_handlers[id].handler != NULL)) {
#if defined(SBIOS_DEBUG) && defined(SHARED_MEM_DEBUG)
		printf("Callback 0x%x\n", (uint32_t) cmd_handlers[id].handler);
#endif
		cmd_handlers[id].handler(packet, cmd_handlers[id].harg);
	}

out:
	__asm__ volatile ("sync");
}

/* Function is not implemented in RTE. */
int	SifSetSreg(int reg, u32 val)
{
	_sif_cmd_data.sregs[(u32) reg] = val;
	return 0;
}

/* Function is not implemented in RTE. */
int	SifGetSreg(int reg)
{
	return _sif_cmd_data.sregs[(u32) reg];
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

/* RTE function has a return value of 0. */
void SifInitCmd(void)
{
	u32 status;
	int i;

	core_save_disable(&status);
	if (initialized) {
		core_restore(status);
		return;
	}
	initialized = 1;

	_sif_cmd_data.pktbuf = KSEG1ADDR((u32 *)pktbuf);
	_sif_cmd_data.cmdbuf = KSEG1ADDR((u32 *)cmdbuf);
	_sif_cmd_data.iopbuf = 0;
	_sif_cmd_data.sys_cmd_handlers = sifCmdSysBuffer;
	_sif_cmd_data.nr_sys_handlers = 32;
	_sif_cmd_data.usr_cmd_handlers = NULL;
	_sif_cmd_data.nr_usr_handlers = 0;
	_sif_cmd_data.sregs = sregs;

	for(i = 0; i < CMD_HANDLER_MAX; i++) {
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

	/* No check here if IOP is already initialized. Assumption is that it is
	 * already initialized
	 */

	/* give it our new receive address. */
	_sif_cmd_data.iopbuf = (void *) sbios_iopaddr; /* XXX: inserted for test. */
	((ca_pkt_t *)(sifCmdInitPkt))->buf = (void *) PHYSADDR((u32 *)_sif_cmd_data.pktbuf);
	SifSendCmd(SIF_CMD_CHANGE_SADDR, sifCmdInitPkt, sizeof(ca_pkt_t), NULL, NULL, 0);

#if 0
	/* RTE does the following: */
	SifSetReg(0x80000000, sbios_iopaddr);
	SifSetReg(0x80000001, sbios_iopaddr);
#else
	/* XXX: PS2SDK code looks better: */
	SifSetReg(0x80000000, (uint32_t) _sif_cmd_data.iopbuf);
	SifSetReg(0x80000001, (uint32_t) &_sif_cmd_data);
#endif
}

u32	SifSendCmd(int fid, void *packet, int packet_size, void *src, void *dest, int size)
{
	return sif_cmd_send(fid, 0, packet, packet_size, src,
			dest, size);
}

u32	iSifSendCmd(int fid, void *packet, int packet_size, void *src, void *dest, int size)
{
	return sif_cmd_send(fid, 0, packet, packet_size, src,
			dest, size);
}

/* Functions in RTE returns 0. */
void SifExitCmd(void)
{
	initialized = 0;
	return;
}

/* Functions is same as RTE, but with error handling. */
void SifAddCmdHandler(int fid, void (* handler)(void *, void *), void *harg)
{
	if (((u32) fid) & 0x80000000)
	{
		if (_sif_cmd_data.sys_cmd_handlers != NULL) {
			_sif_cmd_data.sys_cmd_handlers[((u32) fid) & 0x7FFFFFFFU].handler = handler;
			_sif_cmd_data.sys_cmd_handlers[((u32) fid) & 0x7FFFFFFFU].harg = harg;
		} else {
			printf("Err: sys_cmd_handlers not set.\n");
		}
	}
	else
	{
		if (_sif_cmd_data.usr_cmd_handlers != NULL) {
			_sif_cmd_data.usr_cmd_handlers[(u32) fid].handler = handler;
			_sif_cmd_data.usr_cmd_handlers[(u32) fid].harg = harg;
		} else {
			printf("Err: usr_cmd_handlers not set.\n");
		}
	}
}

/* Functions is same as RTE, but with error handling. */
static void sif_cmd_del_handler(u32 fid)
{
	if (fid & 0x80000000)
	{
		if (_sif_cmd_data.sys_cmd_handlers != NULL) {
			_sif_cmd_data.sys_cmd_handlers[fid & 0x7FFFFFFF].handler = NULL;
		}
	}
	else
	{
		if (_sif_cmd_data.usr_cmd_handlers != NULL) {
			_sif_cmd_data.usr_cmd_handlers[fid].handler = NULL;
		}
	}
}

/* Functions is same as RTE. */
static void *sif_cmd_set_buffer(void *buf, u32 size)
{
	void *old;

	old = _sif_cmd_data.usr_cmd_handlers;

	_sif_cmd_data.usr_cmd_handlers = buf;
	_sif_cmd_data.nr_usr_handlers = size;

	return old;
}

/* Functions is not same as RTE, original function does nothing and return NULL. */
static void *sif_cmd_set_sys_buffer(void *buf, u32 size)
{
	void *old;

	old = _sif_cmd_data.sys_cmd_handlers;

	_sif_cmd_data.sys_cmd_handlers = buf;
	_sif_cmd_data.nr_sys_handlers = size;

	return old;
}

/* SBIOS interface.  */
int sbcall_sifinitcmd()
{
	SifInitCmd();
	return 0;
}

int sbcall_sifexitcmd()
{
	SifExitCmd();
	return 0;
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
	SifAddCmdHandler(arg->fid, arg->handler, arg->harg);
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
	return SifSetSreg(arg->reg, arg->val);
}

int sbcall_sifgetsreg(tge_sbcall_sifgetsreg_arg_t *arg)
{
	return SifGetSreg(arg->reg);
}

int sbcall_sifgetdatatable()
{
	return (int)sif_cmd_data_table();
}

int sbcall_sifsetsyscmdbuffer(tge_sbcall_sifsetsyscmdbuffer_arg_t *arg)
{
	return (int)sif_cmd_set_sys_buffer(arg->buf, arg->size);
}

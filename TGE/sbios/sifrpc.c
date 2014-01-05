/*
# _____     ___ ____     ___ ____
#  ____|   |    ____|   |        | |____|
# |     ___|   |____ ___|    ____| |    \    PS2DEV Open Source Project.
#-----------------------------------------------------------------------
# (C)2001, Gustavo Scotti (gustavo@scotti.com)
# (c) 2003 Marcus R. Brown (mrbrown@0xd6.org)
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.
#
# (c) 2007 - 2012 Mega Man
#
# $Id$
# EE SIF RPC commands
# MRB: This file now contains the SIF routines included
# with libpsware.  Bug reports welcome.
*/

#include "tge_types.h"
#include "tge_defs.h"

#include "tge_sbios.h"
#include "tge_sifdma.h"

#include "core.h"

#include "sif.h"
#include "sifdma.h"
#include "sifcmd.h"
#include "sifrpc.h"
#include "sbcalls.h"
#include "ps2lib_err.h"
#include "iopmemdebug.h"
#include "stdio.h"
#ifdef FILEIO_DEBUG
#include "fileio.h"
#endif

#define RPC_PACKET_SIZE	64

/* Set if the packet has been allocated */
#define PACKET_F_ALLOC	0x01

struct rpc_data {
	int	pid;
	void	*pkt_table;
	int	pkt_table_len;
	int	unused1;
	int	unused2;
	u8	*rdata_table;
	int	rdata_table_len;
	u8	*client_table;
	int	client_table_len;
	int	rdata_table_idx;
	void	*active_queue;
};

extern struct rpc_data _sif_rpc_data;

void *_rpc_get_packet(struct rpc_data *rpc_data);
void *_rpc_get_fpacket(struct rpc_data *rpc_data);
static void rpc_packet_free(void *packet);

void *_rpc_get_packet(struct rpc_data *rpc_data)
{
	SifRpcPktHeader_t *packet;
	int len, pid, rid = 0;
	u32 status;

	core_save_disable(&status);

	len = rpc_data->pkt_table_len;
	if (len > 0) {
		packet = (SifRpcPktHeader_t *)rpc_data->pkt_table;

		for (rid = 0; rid < len; rid++, (u8 *)packet += 64) {
			if (!(packet->rec_id & PACKET_F_ALLOC))
				break;
		}
		if (rid == len) {
			core_restore(status);
			return NULL;
		}

		pid = rpc_data->pid;
		if (pid) {
			rpc_data->pid = ++pid;
		} else {
			rpc_data->pid = pid + 2;
			pid = 1;
		}
		
		packet->rec_id = (rid << 16) | 0x04 | PACKET_F_ALLOC;
		packet->rpc_id = pid;
		packet->pkt_addr = packet;
		core_restore(status);
		return packet;
	}
	core_restore(status);
	return NULL;
}

void *_rpc_get_fpacket(struct rpc_data *rpc_data)
{
	int index;

	index = rpc_data->rdata_table_idx % rpc_data->rdata_table_len;
	rpc_data->rdata_table_idx = index + 1;

	return (void *)(rpc_data->rdata_table + (index * RPC_PACKET_SIZE));
}

int SifBindRpc(SifRpcClientData_t *cd, int sid, int mode,
		SifRpcEndFunc_t endfunc, void *efarg)
{
	SifRpcBindPkt_t *bind;
	u32 status;

	bind = (SifRpcBindPkt_t *)_rpc_get_packet(&_sif_rpc_data);
	if (!bind) {
		return -E_SIF_PKT_ALLOC;
	}

	/* Callback is required by linux. */
	core_save_disable(&status);
	if (cd->hdr.pkt_addr != NULL) {
		core_restore(status);

		rpc_packet_free(bind);

		printf("SifBindRpc: Client 0x%x sid 0x%x is already in use.\n", (uint32_t) cd, sid);
		return -E_SIF_PKT_ALLOC;
	}
	cd->end_function  = endfunc;
	cd->end_param     = efarg;
	cd->command      = 0;
	cd->server       = NULL;
	cd->hdr.pkt_addr = bind;
	cd->hdr.rpc_id   = bind->rpc_id;
	cd->hdr.sema_id  = 0;
	core_restore(status);

	bind->sid        = sid;
	bind->pkt_addr   = bind;
	bind->client     = cd;

	if (mode & SIF_RPC_M_NOWAIT) {
		if (!SifSendCmd(0x80000009, bind, RPC_PACKET_SIZE, NULL, NULL, 0)) {
			rpc_packet_free(bind);
			return -E_SIF_PKT_SEND;
		}
		return 0;
	}

	/* The following code is normally not executed. */
	if (!SifSendCmd(0x80000009, bind, RPC_PACKET_SIZE, NULL, NULL, 0)) {
		rpc_packet_free(bind);
		return -E_SIF_PKT_SEND;
	}

	core_save_disable(&status);
	while(cd->hdr.sema_id == 0) {
		/* Wait until something is received. */
		core_restore(status);

		/* Preemption point. */

		core_save_disable(&status);
	}
	cd->hdr.sema_id--;
	core_restore(status);

	return 0;
}

int SifCallRpc(SifRpcClientData_t *cd, int rpc_number, int mode,
		void *sendbuf, int ssize, void *recvbuf, int rsize,
		SifRpcEndFunc_t endfunc, void *efarg)
{
	SifRpcCallPkt_t *call;
	u32 status;

	if ((((u32) recvbuf) & 0x0f) != 0) {
		/* This is fatal and will lead to write accesses on wrong memory addresses. */
		printf("SifCallRpc 0x%x is not aligned (cd 0x%08x rpc nr 0x%x).\n", (uint32_t) recvbuf, (uint32_t) cd, rpc_number);
		return -E_LIB_INVALID_ARG;
	}


	call = (SifRpcCallPkt_t *)_rpc_get_packet(&_sif_rpc_data);
	if (!call) {
		return -E_SIF_PKT_ALLOC;
	}

	core_save_disable(&status);
	if (cd->hdr.pkt_addr != 0) {
		core_restore(status);
		rpc_packet_free(call);

		printf("SifCallRpc: Client 0x%x rpc_number 0x%x is already in use.\n", (uint32_t) cd, rpc_number);
		return -E_SIF_PKT_ALLOC;
	}
	cd->end_function  = endfunc;
	cd->end_param     = efarg;
	cd->hdr.pkt_addr  = call;
	cd->hdr.rpc_id    = call->rpc_id;
	cd->hdr.sema_id   = 0;
	core_restore(status);

	call->rpc_number  = rpc_number;
	call->send_size   = ssize;
	call->receive     = recvbuf;
	call->recv_size   = rsize;
	call->rmode       = 1;
	call->pkt_addr    = call;
	call->client      = cd;
	call->server      = cd->server;

	if (!(mode & SIF_RPC_M_NOWBDC)) {
		if (ssize > 0)
			SifWriteBackDCache(sendbuf, ssize);
		if (rsize > 0)
			SifWriteBackDCache(recvbuf, rsize);
	}

	if (mode & SIF_RPC_M_NOWAIT) {
		if (!endfunc)
			call->rmode = 0;

		if (!SifSendCmd(0x8000000a, call, RPC_PACKET_SIZE, sendbuf,
					cd->buff, ssize)) {
			rpc_packet_free(call);
			return -E_SIF_PKT_SEND;
		}

		return 0;
	}

	/* The following code is normally not executed. */
	if (!SifSendCmd(0x8000000a, call, RPC_PACKET_SIZE, sendbuf,
				cd->buff, ssize)) {
		rpc_packet_free(call);
		return -E_SIF_PKT_SEND;
	}

	core_save_disable(&status);
	while(cd->hdr.sema_id == 0) {
		/* Wait until something is received. */
		core_restore(status);

		/* Preemption point. */

		core_save_disable(&status);
	}
	cd->hdr.sema_id--;
	core_restore(status);

	return 0;
}

#ifdef F_SifRpcGetOtherData
int SifRpcGetOtherData(SifRpcReceiveData_t *rd, void *src, void *dest,
		int size, int mode)
{
	ee_sema_t sema;
	SifRpcOtherDataPkt_t *other;

	other = (SifRpcOtherDataPkt_t *)_rpc_get_packet(&_sif_rpc_data);
	if (!other)
		return -E_SIF_PKT_ALLOC;

	rd->hdr.pkt_addr = other;
	rd->hdr.rpc_id   = other->rpc_id;
	rd->hdr.sema_id  = -1;

	other->src        = src;
	other->dest       = dest;
	other->size       = size;
	other->receive    = rd;

	if (mode & SIF_RPC_M_NOWAIT) {
		if (!SifSendCmd(0x8000000c, other, 64, NULL, NULL, 0))
			return -E_SIF_PKT_SEND;

		return 0;
	}

	sema.max_count  = 1;
	sema.init_count = 1;
	rd->hdr.sema_id = CreateSema(&sema);
	if (rd->hdr.sema_id < 0)
		return -E_LIB_SEMA_CREATE;

	if (!SifSendCmd(0x8000000c, other, 64, NULL, NULL, 0))
		return -E_SIF_PKT_SEND;

	WaitSema(rd->hdr.sema_id);
	DeleteSema(rd->hdr.sema_id);

	return 0;
}
#endif

/* The packets sent on EE RPC requests are allocated from this table.  */
static u8 pkt_table[2048] __attribute__((aligned(64)));
/* A ring buffer used to allocate packets sent on IOP requests.  */
static u8 rdata_table[2048];
static u8 client_table[2048];

struct rpc_data _sif_rpc_data = {
	pid:			1,
	pkt_table:		pkt_table,
	pkt_table_len:		sizeof(pkt_table)/RPC_PACKET_SIZE,
	rdata_table:		rdata_table,
	rdata_table_len:	sizeof(rdata_table)/RPC_PACKET_SIZE,
	client_table:		client_table,
	client_table_len:	sizeof(client_table)/RPC_PACKET_SIZE,
	rdata_table_idx:	0
};

static int init = 0;

static void rpc_packet_free(void *packet)
{
	u32 status;
	SifRpcRendPkt_t *rendpkt = (SifRpcRendPkt_t *)packet;

	core_save_disable(&status);
	rendpkt->rpc_id = 0;
	rendpkt->rec_id &= (~PACKET_F_ALLOC);

	core_restore(status);
}

/* Command 0x80000008 */
static void _request_end(SifRpcRendPkt_t *request, void *data)
{
	SifRpcClientData_t *client = request->client;
	void *pkt_addr;
	SifRpcEndFunc_t volatile end_function;
	void * volatile end_param;
	u32 status;

	data = data;

	/* Interrupts may not be disabled on Linux 2.6. */
	core_save_disable(&status);
	pkt_addr = client->hdr.pkt_addr;
	client->hdr.sema_id++;

	if (request->cid == 0x8000000a) {
		/* Response to RPC call. */
		end_function = client->end_function;
		end_param = client->end_param;
	} else if (request->cid == 0x80000009) {
		/* Response to Bind call. */
		client->server = request->server;
		client->buff   = request->buff;
		client->cbuff  = request->cbuff;

		/* Callback is not part of PS2SDK, but is required for linux. */
		end_function = client->end_function;
		end_param = client->end_param;
	} else {
		/* No callback. */
		end_function = NULL;
		end_param = NULL;
	}
	/* Set client free for use by next calls. */
	client->hdr.pkt_addr = NULL;
	core_restore(status);

#if defined(SBIOS_DEBUG)
	if (request->cid == 0x8000000a) {
		SifRpcCallPkt_t *call = pkt_addr;

		printf("request_end: client 0x%x cid 0x%x pkt_addr 0x%x rpc_number 0x%x\n", (uint32_t) client, (uint32_t) request->cid, (uint32_t) pkt_addr, call->rpc_number);
	} else if (request->cid == 0x80000009) {
		SifRpcBindPkt_t *bind = pkt_addr;

		printf("request_end: client 0x%x cid 0x%x pkt_addr 0x%x sid 0x%x\n", (uint32_t) client, (uint32_t) request->cid, (uint32_t) pkt_addr, bind->sid);
	} else {
		printf("request_end: client 0x%x cid 0x%x pkt_addr 0x%x\n", (uint32_t) client, (uint32_t) request->cid, (uint32_t) pkt_addr);
	}
#endif

	if (end_function != NULL) {
#if defined(SBIOS_DEBUG)
		printf("Calling 0x%x\n", (uint32_t) end_function);
#endif
		end_function(end_param);
	}

	rpc_packet_free(pkt_addr);

}

static void *search_svdata(u32 sid, struct rpc_data *rpc_data)
{
	SifRpcServerData_t *server;
	SifRpcDataQueue_t *queue = rpc_data->active_queue;

	if (!queue)
		return NULL;

	while (queue) {
		server = queue->link;
		while (server) {
			if (server->sid == sid)
				return server;

			server = server->link;
		}

		queue = queue->next;
	}

	return NULL;
}

/* Command 0x80000009 */
static void _request_bind(SifRpcBindPkt_t *bind, void *data)
{
	SifRpcRendPkt_t *rend;
	SifRpcServerData_t *server;

	rend = _rpc_get_fpacket(data);
	rend->pkt_addr = bind->pkt_addr;
	rend->client = bind->client;
	rend->cid = 0x80000009;

	server = search_svdata(bind->sid, data);
	if (!server) {
		rend->server = NULL;
		rend->buff   = NULL;
		rend->cbuff  = NULL;
	} else {
		rend->server = server;
		rend->buff   = server->buff;
		rend->cbuff  = server->cbuff;
	}

	iSifSendCmd(0x80000008, rend, 64, NULL, NULL, 0);
}

/* Command 0x8000000a */
static void _request_call(SifRpcCallPkt_t *request, void *data)
{
	SifRpcServerData_t *server = request->server;
	SifRpcDataQueue_t *base = server->base;

	data = data;

	if (base->start)
		base->end->link = server;
	else
		base->start = server;

	base->end          = server;
	server->pkt_addr   = request->pkt_addr;
	server->client     = request->client;
	server->rpc_number = request->rpc_number;
	server->size       = request->send_size;
	server->receive    = request->receive;
	server->rsize      = request->recv_size;
	server->rmode      = request->rmode;
	server->rid        = request->rec_id;

	/* XXX: Could be done easier without queue or in a thread? */
	/* XXX: The following is done in interrupt context, should not be done here. */
	server = SifGetNextRequest(base);
	if (server != NULL) {
		SifExecRequest(server);
	}
#if 0 /* XXX: This code is in PS2SDK. */
	if (base->thread_id < 0 || base->active == 0)
		return;

	iWakeupThread(base->thread_id);
#endif
}

/* Command 0x8000000c */
static void _request_rdata(SifRpcOtherDataPkt_t *rdata, void *data)
{
	SifRpcRendPkt_t *rend;

	rend = (SifRpcRendPkt_t *)_rpc_get_fpacket(data);
	rend->pkt_addr = rdata->pkt_addr;
	rend->client = (SifRpcClientData_t *)rdata->receive;
	rend->cid = 0x8000000c;

	iSifSendCmd(0x80000008, rend, 64, rdata->src, rdata->dest, rdata->size);
}

void SifInitRpc(void)
{
	u32 status;

	core_save_disable(&status);
	if (init) {
		core_restore(status);
		return;
	}
	init = 1;

	SifInitCmd();

	_sif_rpc_data.pkt_table    = UNCACHED_SEG(_sif_rpc_data.pkt_table);
	_sif_rpc_data.rdata_table  = UNCACHED_SEG(_sif_rpc_data.rdata_table);
	_sif_rpc_data.client_table = UNCACHED_SEG(_sif_rpc_data.client_table);
	
	SifAddCmdHandler(0x80000008, (void *)_request_end, &_sif_rpc_data);
	SifAddCmdHandler(0x80000009, (void *)_request_bind, &_sif_rpc_data);
	SifAddCmdHandler(0x8000000a, (void *)_request_call, &_sif_rpc_data);
	SifAddCmdHandler(0x8000000c, (void *)_request_rdata, &_sif_rpc_data);

#if 0 /* XXX: IOP is already initialized, we can't do this. */
	if (SifGetReg(0x80000002))
		return;

	cmdp = (u32 *)&pkt_table[64];
	cmdp[3] = 1;
	SifSendCmd(0x80000002, cmdp, 16, NULL, NULL, 0);

	while (!SifGetSreg(0))
		;
#endif
	SifSetReg(0x80000002, 1);
	core_restore(status);
}

void SifExitRpc(void)
{
    SifExitCmd();
    init = 0;
}

SifRpcServerData_t *
SifRegisterRpc(SifRpcServerData_t *sd, 
		int sid, SifRpcFunc_t func, void *buff, SifRpcFunc_t cfunc,
		void *cbuff, SifRpcDataQueue_t *qd)
{
	SifRpcServerData_t *server;
	u32 status;

	core_save_disable(&status);

	sd->link  = NULL;
	sd->next  = NULL;
	sd->sid   = sid;
	sd->func  = func;
	sd->buff  = buff;
	sd->cfunc = cfunc;
	sd->cbuff = cbuff;
	sd->base  = qd;

	if (!(server = qd->link)) {
		qd->link = sd;
	} else {
		while ((server = server->next))
			;

		server->next = sd;
	}

	core_restore(status);

	return server;
}

SifRpcDataQueue_t *
SifSetRpcQueue(SifRpcDataQueue_t *qd)
{
	SifRpcDataQueue_t *queue = NULL;
	u32 status;

	core_save_disable(&status);

#if 0
	qd->thread_id = thread_id;
#endif
	qd->active = 0;
	qd->link   = NULL;
	qd->start  = NULL;
	qd->end    = NULL;
	qd->next   = NULL;

	if (!_sif_rpc_data.active_queue) {
		_sif_rpc_data.active_queue = qd;
	} else {
		queue = _sif_rpc_data.active_queue;

		while ((queue = queue->next))
			;

		queue->next = qd;
	}

	core_restore(status);

	return queue;
}

SifRpcServerData_t *
SifGetNextRequest(SifRpcDataQueue_t *qd)
{
	SifRpcServerData_t *server;
	u32 status;

	core_save_disable(&status);

	server = qd->start;
	qd->active = 1;

	if (server) {
		qd->active = 0;
		qd->start  = server->next;
	}

	core_restore(status);

	return server;
}

static void *_rpc_get_fpacket2(struct rpc_data *rpc_data, int rid)
{
	if (rid < 0 || rid < rpc_data->client_table_len)
		return _rpc_get_fpacket(rpc_data);
	else
		return rpc_data->client_table + (rid * RPC_PACKET_SIZE);
}

void SifExecRequest(SifRpcServerData_t *sd)
{
	tge_sifdma_transfer_t dmat;
	SifRpcRendPkt_t *rend;
	void *rec = NULL;
	u32 status;

	rec = sd->func(sd->rpc_number, sd->buff, sd->size);

	if (sd->size)
		SifWriteBackDCache(sd->buff, sd->size);

	if (sd->rsize)
		SifWriteBackDCache(rec, sd->rsize);


	core_save_disable(&status);

	if (sd->rid & 4)
		rend = (SifRpcRendPkt_t *)
			_rpc_get_fpacket2(&_sif_rpc_data, (sd->rid >> 16) & 0xffff);
	else
		rend = (SifRpcRendPkt_t *)
			_rpc_get_fpacket(&_sif_rpc_data);

	core_restore(status);

	rend->client = sd->client;
	rend->cid    = 0x8000000a;
	rend->rpc_id = 0;  /* XXX: is this correct? */

	if (sd->rmode) {
		if (!SifSendCmd(0x80000008, rend, RPC_PACKET_SIZE, rec, sd->receive,
					sd->rsize))
			return;
	}

	rend->rec_id = 0;

	if (sd->rsize) {
		dmat.src = rec;
		dmat.dest = sd->receive;
		dmat.size = sd->rsize;
		dmat.attr = 0;
	} else {
		dmat.src = rend;
		dmat.dest = sd->pkt_addr;
		dmat.size = 64;
		dmat.attr = 0;
	}

	/* XXX: Caution this loops wait until a DMA entry is free. Called in interrupt handler. */
	while (!SifSetDma(&dmat, 1))
		;
}

#ifdef F_SifRpcLoop
void SifRpcLoop(SifRpcDataQueue_t *qd)
{
	SifRpcServerData_t *server;

	while (1) {
		while ((server = SifGetNextRequest(qd)))
			SifExecRequest(server);

		SleepThread();
	}
}
#endif

int SifCheckStatRpc(SifRpcClientData_t *cd)
{
	SifRpcPktHeader_t *packet = (SifRpcPktHeader_t *)cd->hdr.pkt_addr;

	if (!packet)
		return 0;

	if (cd->hdr.rpc_id != packet->rpc_id)
		return 0;

	if (!(packet->rec_id & PACKET_F_ALLOC))
		return 0;

	return 1;
}

int sbcall_sifinitrpc(void)
{
	SifInitRpc();
#ifdef FILEIO_DEBUG
	fioInit();
#endif
	return 0;
}

int sbcall_sifbindrpc(tge_sbcall_sifbindrpc_arg_t *arg)
{
	return SifBindRpc(arg->cd, arg->sid, arg->mode, arg->endfunc, arg->efarg);
}
int sbcall_sifcallrpc(tge_sbcall_sifcallrpc_arg_t *arg)
{
	return SifCallRpc(arg->cd, arg->func, arg->mode,
		arg->send, arg->send_size, arg->recv, arg->recv_size,
		arg->endfunc, arg->efarg);
}

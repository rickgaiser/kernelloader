/* Copyright (c) 2008 Mega Man */
#include "sbcalls.h"
#include "sifcmd.h"
#include "sifrpc.h"

/** RPC bind id for SDRDRV. */
#define SOUND_BIND_RPC_ID 0x80000701
#define SOUND_SERVER_INIT 0x8000
#define SOUND_GCORE_ATTR 0x8080
#define SOUND_SCORE_ATTR 0x8070
#define SOUND_SCORE_TRANS 0x80e0
#define SOUND_SCORE_TRANS_STAT 0x8100
#define SOUND_SCORE_TRANS_CALLBACK 0x8110

#define SOUND_SERVER_RPC_SID 0x80000704

static int soundInitCounter = 0;
static int soundInitialized = 0;

static SifRpcClientData_t sdrsif __attribute__((aligned(64)));
static SifRpcDataQueue_t soundRpcQueue __attribute__((aligned(64)));
static SifRpcServerData_t soundRpcServerData __attribute__((aligned(64)));
static u32 buffer[32] __attribute__((aligned(64)));
static u32 rpcBuffer[16] __attribute__((aligned(64)));
static u32 rpcResultBuffer[16] __attribute__((aligned(64)));

typedef int (sound_callback_fn)(void *, int);

static sound_callback_fn *callback1;
static void *callback1_arg;
static sound_callback_fn *callback2;
static void *callback2_arg;

static void *sound_rpc_server(int funcno, void *data, int size)
{
	uint32_t *param = data;

	funcno = funcno;
	size = size;

	switch (param[0]) {
		case 1:
			if(callback1 != NULL) {
				return (void *) callback1(callback1_arg, 0);
			}
			break;

		case 2:
			if(callback2 != NULL) {
				return (void *) callback2(callback2_arg, 1);
			}
			break;

		default:
			break;
	}
	return NULL;
}

static void soundInitCallback(void *rarg)
{
	tge_sbcall_rpc_arg_t *carg = (tge_sbcall_rpc_arg_t *) rarg;

	switch (soundInitCounter) {
		case 0:
			if (sdrsif.server != 0) {
				SifSetRpcQueue(&soundRpcQueue);

				SifRegisterRpc(&soundRpcServerData, SOUND_SERVER_RPC_SID, sound_rpc_server, buffer, NULL, NULL, &soundRpcQueue);

				soundInitCounter++;
				carg->result = -1;
			} else {
				carg->result = -2;
			}
			break;

		case 1:
			if (rpcResultBuffer[0] != 0) {
				/* Error */
				carg->result = -2;
			} else {
				if (soundInitialized) {
					carg->result = 0;
				} else {
					soundInitCounter++;
					carg->result = -1;
				}
			}
			break;

		case 2:
			/* Next init call will reinitialize it. */
			soundInitCounter = 1;
			soundInitialized = 1;
			/* Go to default */

		default:
			carg->result = 0;
			break;
	}
	carg->endfunc(carg->efarg, carg->result);
}

int sbcall_soundinit(tge_sbcall_rpc_arg_t *carg)
{
	tge_sbcall_soundinit_arg_t *arg = (tge_sbcall_soundinit_arg_t *) carg->sbarg;

	switch (soundInitCounter) {
		case 0:
			if (SifBindRpc(&sdrsif, SOUND_BIND_RPC_ID, SIF_RPC_M_NOWAIT, soundInitCallback, carg) < 0) {
				return -SIF_RPCE_SENDP;
			}
			break;

		case 1:
			rpcBuffer[0] = (uint32_t) &rpcResultBuffer;
			rpcBuffer[1] = arg->mode;
			if (SifCallRpc(&sdrsif, SOUND_SERVER_INIT, SIF_RPC_M_NOWAIT, rpcBuffer, 64, rpcResultBuffer, 16, soundInitCallback, carg) < 0) {
				return -SIF_RPCE_SENDP;
			}
			break;

		case 2:
			if (SifCallRpc(&sdrsif, 0xe620, SIF_RPC_M_NOWAIT, rpcBuffer, 64, rpcResultBuffer, 16, soundInitCallback, carg) < 0) {
				return -SIF_RPCE_SENDP;
			}
			break;

		default:
			soundInitCallback(carg);
			break;
	}
	return 0;
}

static void soundCallback(void *rarg)
{
	tge_sbcall_rpc_arg_t *carg = (tge_sbcall_rpc_arg_t *) rarg;

	carg->result = rpcResultBuffer[0];

	carg->endfunc(carg->efarg, carg->result);
}


int sbcall_soundend(tge_sbcall_rpc_arg_t *carg)
{
	if (soundInitCounter < 1) {
		return -1;
	}
	rpcBuffer[0] = (uint32_t) &rpcResultBuffer;
	rpcBuffer[1] = TGE_SB_SOUNDINIT_MODE_HOT;
	if (SifCallRpc(&sdrsif, SOUND_SERVER_INIT, SIF_RPC_M_NOWAIT, rpcBuffer, 64, rpcResultBuffer, 16, soundCallback, carg) < 0) {
		return -SIF_RPCE_SENDP;
	}
	return 0;
}

int sbcall_soundgetreg(tge_sbcall_soundgetreg_arg_t *arg)
{
	if (!soundInitialized) {
		return -1;
	}
	if (arg->index >= 0x1c) {
		return -1;
	}
	return -1;
}

int sbcall_soundsetreg(tge_sbcall_soundsetreg_arg_t *arg)
{
	if (arg->index >= 0x1c) {
		return -1;
	}
	return -1;
}

int sbcall_soundgcoreattr(tge_sbcall_rpc_arg_t *carg)
{
	tge_sbacall_soundgetcoreattr_arg_t *arg = (tge_sbacall_soundgetcoreattr_arg_t *) carg->sbarg;

	if (!soundInitialized) {
		return -1;
	}
	rpcBuffer[1] = arg->index;
	if (SifCallRpc(&sdrsif, SOUND_GCORE_ATTR, SIF_RPC_M_NOWAIT, rpcBuffer, 64, rpcResultBuffer, 16, soundCallback, carg) < 0) {
		return -SIF_RPCE_SENDP;
	}
	return 0;
}

static void soundCallbackOk(void *rarg)
{
	tge_sbcall_rpc_arg_t *carg = (tge_sbcall_rpc_arg_t *) rarg;

	carg->result = 0;

	carg->endfunc(carg->efarg, carg->result);
}

int sbcall_soundscoreattr(tge_sbcall_rpc_arg_t *carg)
{
	tge_sbacall_soundsetcoreattr_arg_t *arg = (tge_sbacall_soundsetcoreattr_arg_t *) carg->sbarg;

	if (!soundInitialized) {
		return -1;
	}
	rpcBuffer[1] = arg->index;
	rpcBuffer[2] = arg->attr;
	if (SifCallRpc(&sdrsif, SOUND_SCORE_ATTR, SIF_RPC_M_NOWAIT, rpcBuffer, 64, rpcResultBuffer, 16, soundCallbackOk, carg) < 0) {
		return -SIF_RPCE_SENDP;
	}
	return 0;
}

int sbcall_soundtrans(tge_sbcall_rpc_arg_t *carg)
{
	tge_sbcall_soundtransfer_arg_t *arg = (tge_sbcall_soundtransfer_arg_t *) carg->sbarg;

	if (!soundInitialized) {
		return -1;
	}
	rpcBuffer[1] = arg->channel;
	rpcBuffer[2] = arg->mode;
	rpcBuffer[3] = arg->addr;
	rpcBuffer[4] = arg->size;
	rpcBuffer[5] = arg->start_addr;
	if (SifCallRpc(&sdrsif, SOUND_SCORE_TRANS, SIF_RPC_M_NOWAIT, rpcBuffer, 64, rpcResultBuffer, 16, soundCallback, carg) < 0) {
		return -SIF_RPCE_SENDP;
	}
	return 0;
}

int sbcall_soundtransstat(tge_sbcall_rpc_arg_t *carg)
{
	tge_sbcall_soundtransferstat_arg_t *arg = (tge_sbcall_soundtransferstat_arg_t *) carg->sbarg;

	if (!soundInitialized) {
		return -1;
	}
	rpcBuffer[1] = arg->channel;
	rpcBuffer[2] = arg->flag;
	if (SifCallRpc(&sdrsif, SOUND_SCORE_TRANS, SIF_RPC_M_NOWAIT, rpcBuffer, 64, rpcResultBuffer, 16, soundCallback, carg) < 0) {
		return -SIF_RPCE_SENDP;
	}
	return 0;
}

int sbcall_soundtranscallback(tge_sbcall_rpc_arg_t *carg)
{
	tge_sbcall_soundtransfercallback_arg_t *arg = (tge_sbcall_soundtransfercallback_arg_t *) carg->sbarg;

	if (!soundInitialized) {
		return -1;
	}

	switch(arg->channel) {
		case 0:
			arg->old_cb = callback1;
			arg->oldarg = callback1_arg;
			callback1 = arg->transfer_cb;
			callback1_arg = arg->tcbarg;
			break;

		case 1:
			arg->old_cb = callback2;
			arg->oldarg = callback2_arg;
			callback2 = arg->transfer_cb;
			callback2_arg = arg->tcbarg;
			break;

		default:
			return -1;
	}
	rpcBuffer[1] = arg->channel;
	if (SifCallRpc(&sdrsif, SOUND_SCORE_TRANS_CALLBACK, SIF_RPC_M_NOWAIT, rpcBuffer, 64, rpcResultBuffer, 16, soundCallback, carg) < 0) {
		return -SIF_RPCE_SENDP;
	}
	return 0;
}

int sbcall_soundremote(tge_sbcall_rpc_arg_t *carg)
{
	if (!soundInitialized) {
		return -1;
	}
	carg = carg;
	return -1;
}


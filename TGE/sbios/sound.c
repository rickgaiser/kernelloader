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

typedef struct {
	void *addr;
	uint32_t type;
} sound_reg_t;

static int soundInitCounter = 0;
static int soundInitialized = 0;

static SifRpcClientData_t sdrsif __attribute__((aligned(64)));
static SifRpcDataQueue_t soundRpcQueue __attribute__((aligned(64)));
static SifRpcServerData_t soundRpcServerData __attribute__((aligned(64)));
static u32 buffer[32] __attribute__((aligned(64)));
static u32 rpcBuffer[16] __attribute__((aligned(64)));
static u32 rpcResultBuffer[16] __attribute__((aligned(64)));

typedef int (sound_callback_fn)(void *, int);

static sound_callback_fn *callback1 = NULL;
static void *callback1_arg = NULL;
static sound_callback_fn *callback2 = NULL;
static void *callback2_arg = NULL;

sound_reg_t soundreg[] = {
	/* Soundreg: 0 */
	{
		.addr = (void *) 0xbf8010c0,
		.type = 4,
	},
	/* Soundreg: 1 */
	{
		.addr = (void *) 0xbf801500,
		.type = 4,
	},
	/* Soundreg: 2 */
	{
		.addr = (void *) 0xbf8010c4,
		.type = 2
	},
	/* Soundreg: 3 */
	{
		.addr = (void *) 0xbf801504,
		.type = 2
	},
	/* Soundreg: 4 */
	{
		.addr = (void *) 0xbf8010c6,
		.type = 2
	},
	/* Soundreg: 5 */
	{
		.addr = (void *) 0x00000000,
		.type = 0
	},
	/* Soundreg: 6 */
	{
		.addr = (void *) 0xbf8010c8,
		.type = 4
	},
	/* Soundreg: 7 */
	{
		.addr = (void *) 0x00000000,
		.type = 0
	},
	/* Soundreg: 8 */
	{
		.addr = (void *) 0xbf900198,
		.type = 2
	},
	/* Soundreg: 9 */
	{
		.addr = (void *) 0xbf900598,
		.type = 2
	},
	/* Soundreg: 10 */
	{
		.addr = (void *) 0xbf9001b0,
		.type = 2
	},
	/* Soundreg: 11 */
	{
		.addr = (void *) 0xbf9005b0,
		.type = 2
	},
	/* Soundreg: 12 */
	{
		.addr = (void *) 0xbf900760,
		.type = 2
	},
	/* Soundreg: 13 */
	{
		.addr = (void *) 0xbf900788,
		.type = 2
	},
	/* Soundreg: 14 */
	{
		.addr = (void *) 0xbf900762,
		.type = 2
	},
	/* Soundreg: 15 */
	{
		.addr = (void *) 0xbf90078a,
		.type = 2
	},
	/* Soundreg: 16 */
	{
		.addr = (void *) 0xbf900764,
		.type = 1
	},
	/* Soundreg: 17 */
	{
		.addr = (void *) 0xbf90078c,
		.type = 1
	},
	/* Soundreg: 18 */
	{
		.addr = (void *) 0xbf900766,
		.type = 1
	},
	/* Soundreg: 19 */
	{
		.addr = (void *) 0xbf90078e,
		.type = 1
	},
	/* Soundreg: 20 */
	{
		.addr = (void *) 0xbf900768,
		.type = 1
	},
	/* Soundreg: 21 */
	{
		.addr = (void *) 0xbf900790,
		.type = 1
	},
	/* Soundreg: 22 */
	{
		.addr = (void *) 0xbf90076a,
		.type = 1
	},
	/* Soundreg: 23 */
	{
		.addr = (void *) 0xbf900792,
		.type = 1
	},
	/* Soundreg: 24 */
	{
		.addr = (void *) 0xbf90076c,
		.type = 1
	},
	/* Soundreg: 25 */
	{
		.addr = (void *) 0xbf900794,
		.type = 1
	},
	/* Soundreg: 26 */
	{
		.addr = (void *) 0xbf90076e,
		.type = 1
	},
	/* Soundreg: 27 */
	{
		.addr = (void *) 0xbf900796,
		.type = 1
	},
};

static void *sound_rpc_server(int funcno, void *data, int size)
{
	uint32_t *param = data;

	funcno = funcno;
	size = size;

	if (param[0] & 0x01) {
			/* DMA 0 */
			if(callback1 != NULL) {
				return (void *) callback1(callback1_arg, 0);
			}
	}
	if (param[0] & 0x02) {
			/* DMA 1 */
			if(callback2 != NULL) {
				return (void *) callback2(callback2_arg, 1);
			}
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
				/* Error */
				carg->result = -2;
			}
			break;

		case 1:
			if (rpcResultBuffer[0] != 0) {
				/* Error */
				carg->result = -2;
			} else {
				soundInitCounter++;
				carg->result = -1;
			}
			break;

		case 2:
			if (rpcResultBuffer[0] != 0) {
				/* Error */
				carg->result = -2;
			} else {
				soundInitCounter++;
				soundInitialized = -1;
				carg->result = 0;
			}
			break;

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
			/* Start SDR callback thread. */
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

static void soundEndCallback(void *rarg)
{
	tge_sbcall_rpc_arg_t *carg = (tge_sbcall_rpc_arg_t *) rarg;

	switch (soundInitCounter) {
		case 0:
		case 1:
			/* Error: should not get here. */
			carg->result = -2;
			break;

		case 2:
			carg->result = rpcResultBuffer[0];
			if (rpcResultBuffer[0] == 0) {
				soundInitCounter--;
			}
			break;

		default:
			soundInitCounter--;
			soundInitialized = 0;
			carg->result = -1;
			break;
	}
	carg->endfunc(carg->efarg, carg->result);
}

int sbcall_soundend(tge_sbcall_rpc_arg_t *carg)
{
	switch (soundInitCounter) {
		case 0:
		case 1:
			/* XXX: Unbind? */
			return -1;

		case 2:
			rpcBuffer[0] = (uint32_t) &rpcResultBuffer;
			rpcBuffer[1] = TGE_SB_SOUNDINIT_MODE_HOT;
			if (SifCallRpc(&sdrsif, SOUND_SERVER_INIT, SIF_RPC_M_NOWAIT, rpcBuffer, 64, rpcResultBuffer, 16, soundEndCallback, carg) < 0) {
				return -SIF_RPCE_SENDP;
			}
			break;
		default:
			/* Stop SDR callback thread. */
			if (SifCallRpc(&sdrsif, 0xe630, SIF_RPC_M_NOWAIT, rpcBuffer, 64, rpcResultBuffer, 16, soundEndCallback, carg) < 0) {
				return -SIF_RPCE_SENDP;
			}
			break;
	}
	return 0;
}


int sbcall_soundgetreg(tge_sbcall_soundgetreg_arg_t *arg)
{
	if (!soundInitialized) {
		return -1;
	}
	if (arg->index >= (sizeof(soundreg) / sizeof(soundreg[0]))) {
		return -1;
	}
	switch(soundreg[arg->index].type) {
		case 1:
		case 2:
			{
				volatile uint16_t *addr = (uint16_t *) soundreg[arg->index].addr;

				arg->val = *addr;
			}
			return 0;
		case 3:
		case 4:
			{
				volatile uint32_t *addr = (uint32_t *) soundreg[arg->index].addr;

				arg->val = *addr;
			}
			return 0;
		default:
			return -1;
	}
}

int sbcall_soundsetreg(tge_sbcall_soundsetreg_arg_t *arg)
{
	if (!soundInitialized) {
		return -1;
	}
	if (arg->index >= (sizeof(soundreg) / sizeof(soundreg[0]))) {
		return -1;
	}
	switch(soundreg[arg->index].type) {
		case 1:
		case 2:
			{
				volatile uint16_t *addr = (uint16_t *) soundreg[arg->index].addr;

				*addr = *((uint16_t *) &arg->val);
			}
			return 0;
		case 3:
		case 4:
			{
				volatile uint32_t *addr = (uint32_t *) soundreg[arg->index].addr;

				*addr = *((uint32_t *) &arg->val);
			}
			return 0;
		default:
			return -1;
	}
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
	} /* TBD: Is this complete? */
	carg = carg;
	return -1;
}


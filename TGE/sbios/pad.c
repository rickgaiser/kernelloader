
/*
# _____     ___ ____     ___ ____
#  ____|   |    ____|   |        | |____|
# |     ___|   |____ ___|    ____| |    \    PS2DEV Open Source Project.
#-----------------------------------------------------------------------
# Copyright 2001-2004, ps2dev - http://www.ps2dev.org
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.
#
# (c) 2007 Mega Man
#
# $Id$
# Pad library functions
# Quite easy rev engineered from util demos..
# Find any bugs? Mail me: pukko@home.se
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
#include "mc.h"
#include "sbcalls.h"
#include "ps2lib_err.h"
#include "iopmemdebug.h"
#include "stdio.h"
#include "string.h"
#include "pad.h"

#ifdef OLD_ROM_MODULE_VERSION
#define ROM_PADMAN 1
#else
#define NEW_PADMAN 1
#endif

/*
 * Slightly different behaviour if using "rom0:padman" or something newer
 * (such as "rom0:xpadman" on those machines that have it)
 * User must define which is used
 */
#if defined(ROM_PADMAN) && defined(NEW_PADMAN)
#error Only one of ROM_PADMAN & NEW_PADMAN should be defined!
#endif

#if !defined(ROM_PADMAN) && !defined(NEW_PADMAN)
#error ROM_PADMAN or NEW_PADMAN must be defined!
#endif



/*
 * Defines
 */
#ifdef ROM_PADMAN

#define PAD_BIND_RPC_ID1 0x8000010f
#define PAD_BIND_RPC_ID2 0x8000011f

#define PAD_RPCCMD_OPEN         0x80000100
#define PAD_RPCCMD_INFO_ACT     0x80000102
#define PAD_RPCCMD_INFO_COMB    0x80000103
#define PAD_RPCCMD_INFO_MODE    0x80000104
#define PAD_RPCCMD_SET_MMODE    0x80000105
#define PAD_RPCCMD_SET_ACTDIR   0x80000106
#define PAD_RPCCMD_SET_ACTALIGN 0x80000107
#define PAD_RPCCMD_GET_BTNMASK  0x80000108
#define PAD_RPCCMD_SET_BTNINFO  0x80000109
#define PAD_RPCCMD_SET_VREF     0x8000010a
#define PAD_RPCCMD_GET_PORTMAX  0x8000010b
#define PAD_RPCCMD_GET_SLOTMAX  0x8000010c
#define PAD_RPCCMD_CLOSE        0x8000010d
#define PAD_RPCCMD_END          0x8000010e

#define PAD_RPCCMD_INIT         0x00000000	/* not supported */
#define PAD_RPCCMD_GET_CONNECT  0x00000000	/* not supported */
#define PAD_RPCCMD_GET_MODVER   0x00000000	/* not supported */

#else

#define PAD_BIND_RPC_ID1 0x80000100
#define PAD_BIND_RPC_ID2 0x80000101

#define PAD_RPCCMD_OPEN         0x01
#define PAD_RPCCMD_SET_MMODE    0x06
#define PAD_RPCCMD_SET_ACTDIR   0x07
#define PAD_RPCCMD_SET_ACTALIGN 0x08
#define PAD_RPCCMD_GET_BTNMASK  0x09
#define PAD_RPCCMD_SET_BTNINFO  0x0A
#define PAD_RPCCMD_SET_VREF     0x0B
#define PAD_RPCCMD_GET_PORTMAX  0x0C
#define PAD_RPCCMD_GET_SLOTMAX  0x0D
#define PAD_RPCCMD_CLOSE        0x0E
#define PAD_RPCCMD_END          0x0F
#define PAD_RPCCMD_INIT         0x10
#define PAD_RPCCMD_GET_CONNECT  0x11
#define PAD_RPCCMD_GET_MODVER   0x12

#endif



/*
 * Types
 */

struct pad_state {
	int open;
	unsigned int port;
	unsigned int slot;
	struct pad_data *padData;
	unsigned char *padBuf;
};

#ifdef ROM_PADMAN
// rom0:padman has only 64 byte of pad data
struct pad_data {
	unsigned int frame;
	unsigned char state;
	unsigned char reqState;
	unsigned char ok;
	unsigned char unkn7;
	unsigned char data[32];
	unsigned int length;
	unsigned int unkn44;
	unsigned int unkn48;
	unsigned int unkn52;
	unsigned int unkn54;
	unsigned int unkn60;
};
#else
struct pad_data 
{
    u8 data[32]; 
	u32 actDirData[2];
	u32 actAlignData[2];
    u8  actData[32]; 
    u16 modeTable[4]; 
    u32 frame;
    u32 findPadRetries;
    u32 length;
    u8 modeConfig;
    u8 modeCurId;
    u8 model;
    u8 buttonDataReady;
    u8 nrOfModes;
    u8 modeCurOffs; 
    u8 nrOfActuators; 
	u8 numActComb;
	u8 val_c6;
	u8 mode;
    u8 lock;
	u8 actDirSize;
    u8 state;
    u8 reqState;
    u8 currentTask; 
	u8 runTask;
	u8 stat70bit;
    u8 padding[11];
};


struct open_slot
{
	u32 frame;
	u32 openSlots[2];
	u8  padding[116];
};
#endif


/*
 * Pad variables etc.
 */

static const char padStateString[8][16] = { "DISCONNECT", "FINDPAD",
	"FINDCTP1", "", "", "EXECCMD",
	"STABLE", "ERROR"
};
static const char padReqStateString[3][16] = { "COMPLETE", "FAILED", "BUSY" };

static int padInitialiseCounter = 0;

// pad rpc call
static SifRpcClientData_t padsif[2] __attribute__ ((aligned(64)));
static char buffer[128] __attribute__ ((aligned(128)));

#ifndef ROM_PADMAN
static struct open_slot openSlot[2];
#endif

/* Port state data */
static struct pad_state PadState[2][8];


/*
 * Local functions
 */

/*
 * Common helper
 */
static struct pad_data *padGetDmaStr(int port, int slot)
{
	struct pad_data *pdata;

	pdata = PadState[port][slot].padData;
	SyncDCache(pdata, (u8 *) pdata + 256);

	if (pdata[0].frame < pdata[1].frame) {
		return &pdata[1];
	}
	else {
		return &pdata[0];
	}
}

static void padCallback(void *rarg)
{
	tge_sbcall_rpc_arg_t *carg = (tge_sbcall_rpc_arg_t *) rarg;
	carg->result = *(int *) (&buffer[12]);
	carg->endfunc(carg->efarg, carg->result);
}

static void padInitCallback(void *rarg)
{
	tge_sbcall_rpc_arg_t *carg = (tge_sbcall_rpc_arg_t *) rarg;
	switch (padInitialiseCounter) {
	case 0:
		if (!padsif[0].server) {
			/* Retry bind */
			carg->result = 0;
		} else {
			carg->result = 0;

			/* Go to next stage. */
			padInitialiseCounter++;
		}
		break;
	case 1:
		if (!padsif[1].server) {
			/* Retry bind */
			carg->result = 0;
		} else {
#ifdef ROM_PADMAN
			/* Finished */
			carg->result = 1;
#else
			carg->result = 0;
#endif
			/* Go to next stage. */
			padInitialiseCounter++;
		}
		break;
#ifndef ROM_PADMAN
	case 2:
		/* Return version number. */
		if (*(int *) (&buffer[12]) > 0) {
			carg->result = 0;

			/* Go to next stage. */
			padInitialiseCounter++;
		} else {
			printf("Padman version number invalid.\n");
			carg->result = -6;
		}
		break;
	case 3:
		carg->result = 1;

		/* Go to next stage. */
		padInitialiseCounter++;
		break;
#endif
	default:
		printf("padInitCallback() called with illegal state.\n");
		carg->result = -20;
		break;
	}
	carg->endfunc(carg->efarg, carg->result);
}


/*
 * Global functions
 */

/*
 * Functions not implemented here
 * padGetFrameCount() <- dunno if it's useful for someone..
 * padInfoComb() <- see above
 * padSetVrefParam() <- dunno
 */

/*
 * Initialise padman
 * a = 0 should work..
 */
int sbcall_padinit(tge_sbcall_rpc_arg_t *carg)
{
	//tge_sbcall_padinit_arg_t *arg = carg->sbarg;
	int i;

	switch (padInitialiseCounter) {
	case 0:
		padsif[0].server = NULL;
		padsif[1].server = NULL;

		for (i = 0; i < 8; i++) {
			PadState[0][i].open = 0;
			PadState[0][i].port = 0;
			PadState[0][i].slot = 0;
			PadState[1][i].open = 0;
			PadState[1][i].port = 0;
			PadState[1][i].slot = 0;
		}

		if (SifBindRpc(&padsif[0], PAD_BIND_RPC_ID1, SIF_RPC_M_NOWAIT,
			padInitCallback, carg) < 0) {
			return -SIF_RPCE_SENDP;
		}
		break;

	case 1:
		if (SifBindRpc(&padsif[1], PAD_BIND_RPC_ID2, SIF_RPC_M_NOWAIT,
				padInitCallback, carg) < 0) {
			return -SIF_RPCE_SENDP;
		}
		break;

#ifndef ROM_PADMAN
	case 2:
		/* Returns the padman version NOT SUPPORTED on module rom0:padman */
		*(u32 *) (&buffer[0]) = PAD_RPCCMD_GET_MODVER;

		if (SifCallRpc(&padsif[0], 1, SIF_RPC_M_NOWAIT, buffer, 128, buffer, 128,
			padInitCallback, carg) < 0) {
			printf("Failed padGetModVersion\n");
			return -SIF_RPCE_SENDP;
		}
		break;

	case 3:
		((u32 *)(&buffer[0]))[0] = PAD_RPCCMD_INIT;
		((u32 *)(&buffer[0]))[4]=(u32)openSlot;
		if (SifCallRpc(&padsif[0], 1, SIF_RPC_M_NOWAIT, buffer, 128, buffer, 128,
			padInitCallback, carg) < 0) {
			return -SIF_RPCE_SENDP;
		}
		break;
#endif
	default:
		/* Already initialised. */
		carg->result = 1;
		carg->endfunc(carg->efarg, carg->result);
		break;
	}
	return 0;

}

/* Since padEnd() further below doesn't work right, a pseudo function is needed
 * to allow recovery after IOP reset. This function has nothing to do with the
 * functions of the IOP modules. It merely resets variables for the EE routines.
 */
int padReset()
{
	padInitialiseCounter = 0;
	padsif[0].server = NULL;
	padsif[1].server = NULL;
	return 0;
}

static void padEndCallback(void *rarg)
{
	tge_sbcall_rpc_arg_t *carg = (tge_sbcall_rpc_arg_t *) rarg;
	int ret;
	
	ret = *(int *) (&buffer[12]);
	if (ret == 1) {
		padReset();
	}

	carg->result = ret;
	carg->endfunc(carg->efarg, carg->result);
}

/*
 * End all pad communication (not tested)
 */
int sbcall_padend(tge_sbcall_rpc_arg_t *carg)
{
	*(u32 *) (&buffer[0]) = PAD_RPCCMD_END;

	if (SifCallRpc(&padsif[0], 1, SIF_RPC_M_NOWAIT, buffer, 128, buffer, 128, padEndCallback, carg) < 0) {
		return -SIF_RPCE_SENDP;
	}

	return 0;
}

static void padPortOpenCallback(void *rarg)
{
	tge_sbcall_rpc_arg_t *carg = (tge_sbcall_rpc_arg_t *) rarg;
	tge_sbcall_padportopen_arg_t *arg = carg->sbarg;

	carg->result = *(int *) (&buffer[12]);

	PadState[arg->port][arg->slot].padBuf = *(char **) (&buffer[20]);

	carg->endfunc(carg->efarg, carg->result);
}

/*
 * The user should provide a pointer to a 256 byte (2xsizeof(struct pad_data))
 * 64 byte aligned pad data area for each pad port opened
 *
 * return != 0 => OK
 */
int sbcall_padportopen(tge_sbcall_rpc_arg_t *carg)
{
	tge_sbcall_padportopen_arg_t *arg = carg->sbarg;
	int i;
	int ret;
	struct pad_data *dma_buf = (struct pad_data *) arg->addr;

	// Check 64 byte alignment
	if ((u32) arg->addr & 0x3f) {
		printf("dmabuf misaligned (%x)!!\n", (uint32_t) dma_buf);
		return -1;
	}

	for (i = 0; i < 2; i++) {	// Pad data is double buffered
		memset(dma_buf[i].data, 0xff, 32);	// 'Clear' data area
		dma_buf[i].frame = 0;
		dma_buf[i].length = 0;
		dma_buf[i].state = PAD_STATE_EXECCMD;
		dma_buf[i].reqState = PAD_RSTAT_BUSY;
		dma_buf[i].length = 0;
#ifndef ROM_PADMAN
		dma_buf[i].currentTask = 0;
		dma_buf[i].buttonDataReady = 0; // Should be cleared in newer padman
#else
		dma_buf[i].ok = 0;
#endif
	}


	*(u32 *) (&buffer[0]) = PAD_RPCCMD_OPEN;
	*(u32 *) (&buffer[4]) = arg->port;
	*(u32 *) (&buffer[8]) = arg->slot;
	*(u32 *) (&buffer[16]) = (u32) arg->addr;

	ret = SifCallRpc(&padsif[0], 1, SIF_RPC_M_NOWAIT, buffer, 128, buffer, 128, padPortOpenCallback, carg);
	if (ret < 0) {
		return -SIF_RPCE_SENDP;
	}
			PadState[arg->port][arg->slot].open = 1;
	PadState[arg->port][arg->slot].padData = arg->addr;
	return 0;
}


/*
 * not tested :/
 */
int sbcall_padportclose(tge_sbcall_rpc_arg_t *carg)
{
	tge_sbcall_padportopen_arg_t *arg = carg->sbarg;
	int ret;

	*(u32 *) (&buffer[0]) = PAD_RPCCMD_CLOSE;
	*(u32 *) (&buffer[4]) = arg->port;
	*(u32 *) (&buffer[8]) = arg->slot;
	*(u32 *) (&buffer[16]) = 1;

	ret = SifCallRpc(&padsif[0], 1, SIF_RPC_M_NOWAIT, buffer, 128, buffer, 128, padCallback, carg);

	if (ret < 0) {
		return -SIF_RPCE_SENDP;
	} else {
		PadState[arg->port][arg->slot].open = 0;
		return 0;
	}
}


/*
 * Read pad data
 * Result is stored in 'data' which should point to a 32 byte array
 */
int sbcall_padread(tge_sbcall_padread_arg_t *arg)
{
	struct pad_data *pdata;

	pdata = padGetDmaStr(arg->port, arg->slot);

	memcpy(arg->data, pdata->data, pdata->length);
	return pdata->length;
}


/*
 * Get current pad state
 * Wait until state == 6 (Ready) before trying to access the pad
 */
int sbcall_padgetstate(tge_sbcall_padgetstate_arg_t *arg)
{
	struct pad_data *pdata;
	unsigned char state;


	pdata = padGetDmaStr(arg->port, arg->slot);

	state = pdata->state;

	if (state == PAD_STATE_STABLE) {	// Ok
		if (padGetReqState(arg->port, arg->slot) == PAD_RSTAT_BUSY) {
			return PAD_STATE_EXECCMD;
		}
	}
	return state;
}


/*
 * Get pad request state
 */
unsigned char padGetReqState(int port, int slot)
{

	struct pad_data *pdata;

	pdata = padGetDmaStr(port, slot);
	return pdata->reqState;
}


/*
 * Set pad request state (after a param setting)
 */
int padSetReqState(int port, int slot, int state)
{

	struct pad_data *pdata;

	pdata = padGetDmaStr(port, slot);
	pdata->reqState = state;
	return 1;
}


/*
 * Debug print functions
 * uh.. these are actually not tested :)
 */
void padStateInt2String(int state, char buf[16])
{

	if (state < 8) {
		strcpy(buf, padStateString[state]);
	}
}

void padReqStateInt2String(int state, char buf[16])
{
	if (state < 4)
		strcpy(buf, padReqStateString[state]);
}


#if 0
/*
 * Returns # slots on the PS2 (usally 2)
 */
int padGetPortMax(void)
{

	*(u32 *) (&buffer[0]) = PAD_RPCCMD_GET_PORTMAX;

	if (SifCallRpc(&padsif[0], 1, SIF_RPC_M_NOWAIT, buffer, 128, buffer, 128, 0, 0) < 0)
		return -SIF_RPCE_SENDP;

	return *(int *) (&buffer[12]);
}


/*
 * Returns # slots the port has (usually 1)
 * probably 4 if using a multi tap (not tested)
 */
int padGetSlotMax(int port)
{

	*(u32 *) (&buffer[0]) = PAD_RPCCMD_GET_SLOTMAX;
	*(u32 *) (&buffer[4]) = port;

	if (SifCallRpc(&padsif[0], 1, SIF_RPC_M_NOWAIT, buffer, 128, buffer, 128, 0, 0) < 0)
		return -SIF_RPCE_SENDP;

	return *(int *) (&buffer[12]);
}
#endif

/*
 * Get pad info (digital (4), dualshock (7), etc..)
 * ID: 3 - KONAMI GUN
 *     4 - DIGITAL PAD
 *     5 - JOYSTICK
 *     6 - NAMCO GUN
 *     7 - DUAL SHOCK
 */
int sbcall_padinfomode(tge_sbcall_padinfomode_arg_t *arg)
{
#ifdef ROM_PADMAN
	*(u32 *) (&buffer[0]) = PAD_RPCCMD_INFO_MODE;
	*(u32 *) (&buffer[4]) = arg->port;
	*(u32 *) (&buffer[8]) = arg->slot;
	*(u32 *) (&buffer[12]) = arg->term;
	*(u32 *) (&buffer[16]) = arg->offset;

	/* XXX: Check if this is working without SIF_RPC_M_NOWAIT. */
	if (SifCallRpc(&padsif[0], 1, 0, buffer, 128, buffer, 128, 0, 0) < 0) {
		/** XXX: Which return code to use if this failing? */
		return 0;
	}

	if (*(int *) (&buffer[20]) == 1) {
		padSetReqState(arg->port, arg->slot, PAD_RSTAT_BUSY);
	}

	return *(int *) (&buffer[20]);
#else

	struct pad_data *pdata;

	pdata = padGetDmaStr(arg->port, arg->slot);

	if (pdata->currentTask != 1)
		return 0;
	if (pdata->reqState == PAD_RSTAT_BUSY)
		return 0;

	switch (arg->term) {
	case PAD_MODECURID:
		if (pdata->modeCurId == 0xF3)
			return 0;
		else
			return (pdata->modeCurId >> 4);
		break;

	case PAD_MODECUREXID:
		if (pdata->modeConfig == pdata->currentTask)
			return 0;
		return pdata->modeTable[pdata->modeCurOffs];
		break;

	case PAD_MODECUROFFS:
		if (pdata->modeConfig != 0)
			return pdata->modeCurOffs;
		else
			return 0;
		break;

	case PAD_MODETABLE:
		if (pdata->modeConfig != 0) {
			if (arg->offset == -1) {
				return pdata->nrOfModes;
			}
			else if (arg->offset < pdata->nrOfModes) {
				return pdata->modeTable[arg->offset];
			}
			else {
				return 0;
			}
		}
		else
			return 0;
		break;
	}
	return 0;
#endif
}


static void padSetMainModeCallback(void *rarg)
{
	tge_sbcall_rpc_arg_t *carg = (tge_sbcall_rpc_arg_t *) rarg;
	tge_sbcall_padsetmainmode_arg_t *arg = carg->sbarg;

	if (*(int *) (&buffer[20]) == 1) {
		padSetReqState(arg->port, arg->slot, PAD_RSTAT_BUSY);
	}
	carg->result = *(int *) (&buffer[20]);

	carg->endfunc(carg->efarg, carg->result);
}

/*
 * mode = 1, -> Analog/dual shock enabled; mode = 0 -> Digital  
 * lock = 3 -> Mode not changeable by user
 */
int sbcall_padsetmainmode(tge_sbcall_rpc_arg_t *carg)
{
	tge_sbcall_padsetmainmode_arg_t *arg = carg->sbarg;
	int ret;

	*(u32 *) (&buffer[0]) = PAD_RPCCMD_SET_MMODE;
	*(u32 *) (&buffer[4]) = arg->port;
	*(u32 *) (&buffer[8]) = arg->slot;
	*(u32 *) (&buffer[12]) = arg->offset;
	*(u32 *) (&buffer[16]) = arg->lock;

	ret = SifCallRpc(&padsif[0], 1, SIF_RPC_M_NOWAIT, buffer, 128, buffer, 128, padSetMainModeCallback, carg);
	if (ret < 0) {
		return -SIF_RPCE_SENDP;
	}

	return 0;
}


static void padInfoPressModeCallback(void *rarg)
{
	tge_sbcall_rpc_arg_t *carg = (tge_sbcall_rpc_arg_t *) rarg;
	int mask;

	mask = *(int *) (&buffer[12]);
	if (mask ^ 0x3ffff) {
		carg->result = 0;
	}
	else {
		carg->result = 1;
	}
	carg->endfunc(carg->efarg, carg->result);
}

/*
 * Pressure sensitive mode ON
 */
int sbcall_padenterpressmode(tge_sbcall_rpc_arg_t *carg)
{
	return padSetButtonInfo(carg, 0xFFF);
}


/*
 * Check for newer version
 * Pressure sensitive mode OFF
 */
int sbcall_padexitpressmode(tge_sbcall_rpc_arg_t *carg)
{
	return padSetButtonInfo(carg, 0);
}


/*
 * Check if the pad has pressure sensitive buttons
 */
int sbcall_padinfopressmode(tge_sbcall_rpc_arg_t *carg)
{
	tge_sbcall_padpressmode_arg_t *arg = carg->sbarg;
	int ret;

	*(u32 *) (&buffer[0]) = PAD_RPCCMD_GET_BTNMASK;
	*(u32 *) (&buffer[4]) = arg->port;
	*(u32 *) (&buffer[8]) = arg->slot;

	ret = SifCallRpc(&padsif[0], 1, SIF_RPC_M_NOWAIT, buffer, 128, buffer, 128, padInfoPressModeCallback, carg);
	if (ret < 0) {
		return -SIF_RPCE_SENDP;
	}

	return 0;
}

static void padSetButtonInfoCallback(void *rarg)
{
	tge_sbcall_rpc_arg_t *carg = (tge_sbcall_rpc_arg_t *) rarg;
	tge_sbcall_padpressmode_arg_t *arg = carg->sbarg;
	int val;

	val = *(int *) (&buffer[16]);

	if (val == 1) {
		padSetReqState(arg->port, arg->slot, PAD_RSTAT_BUSY);
	}
	carg->result = *(int *) (&buffer[16]);
	carg->endfunc(carg->efarg, carg->result);
}

/*
 *
 */
int padSetButtonInfo(tge_sbcall_rpc_arg_t *carg, int buttonInfo)
{
	tge_sbcall_padpressmode_arg_t *arg = carg->sbarg;
	int ret;

	*(u32 *) (&buffer[0]) = PAD_RPCCMD_SET_BTNINFO;
	*(u32 *) (&buffer[4]) = arg->port;
	*(u32 *) (&buffer[8]) = arg->slot;
	*(u32 *) (&buffer[12]) = buttonInfo;

	ret = SifCallRpc(&padsif[0], 1, SIF_RPC_M_NOWAIT, buffer, 128, buffer, 128, padSetButtonInfoCallback, carg);
	if (ret < 0) {
		return -SIF_RPCE_SENDP;
	}

	return 0;
}


/*
 * Get actuator status for this controller
 * If padInfoAct(port, slot, -1, 0) != 0, the controller has actuators
 * (i think ;) )
 */
int sbcall_padinfoact(tge_sbcall_padinfoact_arg_t *arg)
{
#ifdef ROM_PADMAN
	*(u32 *) (&buffer[0]) = PAD_RPCCMD_INFO_ACT;
	*(u32 *) (&buffer[4]) = arg->port;
	*(u32 *) (&buffer[8]) = arg->slot;
	*(u32 *) (&buffer[12]) = arg->actno;
	*(u32 *) (&buffer[16]) = arg->term;

	/* XXX: Check if this is working without SIF_RPC_M_NOWAIT. */
	if (SifCallRpc(&padsif[0], 1, 0, buffer, 128, buffer, 128, 0, 0) < 0) {
		/* XXX: Which return code to use if this is failing? */
		return 0;
	}

	if (*(int *) (&buffer[20]) == 1) {
		padSetReqState(arg->port, arg->slot, PAD_RSTAT_BUSY);
	}
	return *(int *) (&buffer[20]);
#else
	struct pad_data *pdata;

	pdata = padGetDmaStr(arg->port, arg->slot);

	if (pdata->currentTask != 1)
		return 0;
	if (pdata->modeConfig < 2)
		return 0;
	if (arg->actno >= pdata->nrOfActuators)
		return 0;

	if (arg->actno == -1)
		return pdata->nrOfActuators;	// # of acutators?

	if (arg->term >= 4)
		return 0;

	return pdata->actData[arg->actno * 4 + arg->term];
#endif
}

static void padSetActAlignCallback(void *rarg)
{
	tge_sbcall_rpc_arg_t *carg = (tge_sbcall_rpc_arg_t *) rarg;
	tge_sbcall_padsetactalign_arg_t *arg = carg->sbarg;

	if (*(int *) (&buffer[20]) == 1) {
		padSetReqState(arg->port, arg->slot, PAD_RSTAT_BUSY);
	}
	carg->result = *(int *) (&buffer[20]);
	carg->endfunc(carg->efarg, carg->result);
}

/*
 * Initalise actuators. On dual shock controller:
 * actAlign[0] = 0 enables 'small' engine
 * actAlign[1] = 1 enables 'big' engine
 * set actAlign[2-5] to 0xff (disable)
 */
int sbcall_padsetactalign(tge_sbcall_rpc_arg_t *carg)
{
	tge_sbcall_padsetactalign_arg_t *arg = carg->sbarg;
	char *actAlign = arg->data;
	int i;
	char *ptr;
	int ret;

	*(u32 *) (&buffer[0]) = PAD_RPCCMD_SET_ACTALIGN;
	*(u32 *) (&buffer[4]) = arg->port;
	*(u32 *) (&buffer[8]) = arg->slot;

	ptr = (char *) (&buffer[12]);
	for (i = 0; i < 6; i++)
		ptr[i] = actAlign[i];

	ret = SifCallRpc(&padsif[0], 1, SIF_RPC_M_NOWAIT, buffer, 128, buffer, 128, padSetActAlignCallback, carg);
	if (ret < 0) {
		return -SIF_RPCE_SENDP;
	}
	return 0;
}

static void padSetActDirectCallback(void *rarg)
{
	tge_sbcall_rpc_arg_t *carg = (tge_sbcall_rpc_arg_t *) rarg;
	carg->result = *(int *) (&buffer[20]);
	carg->endfunc(carg->efarg, carg->result);
}


/*
 * Set actuator status
 * On dual shock controller, 
 * actAlign[0] = 0/1 turns off/on 'small' engine
 * actAlign[1] = 0-255 sets 'big' engine speed
 */
int sbcall_padsetactdirect(tge_sbcall_rpc_arg_t *carg)
{
	tge_sbcall_padsetactdirect_arg_t *arg = carg->sbarg;
	char *actAlign = arg->data;
	int i;
	char *ptr;
	int ret;

	*(u32 *) (&buffer[0]) = PAD_RPCCMD_SET_ACTDIR;
	*(u32 *) (&buffer[4]) = arg->port;
	*(u32 *) (&buffer[8]) = arg->slot;

	ptr = (char *) (&buffer[12]);
	for (i = 0; i < 6; i++)
		ptr[i] = actAlign[i];

	ret = SifCallRpc(&padsif[0], 1, SIF_RPC_M_NOWAIT, buffer, 128, buffer, 128, padSetActDirectCallback, carg);
	if (ret < 0) {
		return -SIF_RPCE_SENDP;
	}

	return 0;
}


#if 0
/*
 * NOT SUPPORTED with module rom0:padman
 */
int padGetConnection(int port, int slot)
{
#ifdef ROM_PADMAN
	port = port;
	slot = slot;
	return 1;
#else
	struct open_slot *oslot;

	if(openSlot[0].frame < openSlot[1].frame)
		oslot = &openSlot[1];
	else
		oslot = &openSlot[0];

	return ((oslot->openSlots[port] >> slot) & 0x1);
#endif
}
#endif

int sbcall_padgetreqstate(tge_sbcall_padgetreqstate_arg_t *arg)
{
	return padGetReqState(arg->port, arg->slot);
}

#if 0
int sbcall_padinfocomb(tge_sbcall_padinfocomb_arg_t *arg)
{
	return padInfoComb(arg->port, arg->slot, arg->listno, arg->offset);
}
#endif


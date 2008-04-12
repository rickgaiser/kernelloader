
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
#include "iopmem.h"
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
struct pad_data {
	unsigned char data[32];		// 0, length = 32 bytes
	unsigned int unkn32;		// not used??
	unsigned int unkn36;		// not used??
	unsigned int unkn40;		// byte 40  not used??
	unsigned int unkn44;		// not used?? 44
	unsigned char actData[32];	// actuator (6x4?) 48
	unsigned short modeTable[4];	// padInfo   80
	unsigned int frame;			// byte 88, u32 22
	unsigned int unkn92;		// not used ??
	unsigned int length;		// 96
	unsigned char modeOk;		// padInfo  100 Dunno about the name though...
	unsigned char modeCurId;	// padInfo    101
	unsigned char unkn102;		// not used??
	unsigned char unknown;		// unknown
	unsigned char nrOfModes;	// padInfo   104
	unsigned char modeCurOffs;	// padInfo   105
	unsigned char nrOfActuators;	// act  106
	unsigned char unkn107[5];	// not used??
	unsigned char state;		// byte 112
	unsigned char reqState;		// byte 113
	unsigned char ok;			// padInfo  114
	unsigned char unkn115[13];	// not used??
};
#endif


typedef struct {
	SifRpcEndFunc_t endfunc;
	void *efarg;
	int *result;
	void *optArg;
} padCallbackData_t;

typedef struct {
	SifRpcEndFunc_t endfunc;
	void *efarg;
	int *result;
	int port;
	int slot;
} padOpenData_t;

/*
 * Pad variables etc.
 */

static const char padStateString[8][16] = { "DISCONNECT", "FINDPAD",
	"FINDCTP1", "", "", "EXECCMD",
	"STABLE", "ERROR"
};
static const char padReqStateString[3][16] = { "COMPLETE", "FAILED", "BUSY" };

static int padInitialised = 0;

// pad rpc call
static SifRpcClientData_t padsif[2] __attribute__ ((aligned(64)));
static char buffer[128] __attribute__ ((aligned(16)));

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

static void padCallback(void *arg)
{
	padCallbackData_t *data = (padCallbackData_t *) arg;

	*data->result = *(int *) (&buffer[12]);
	data->endfunc(data->efarg);
}

static void padCallback20(void *arg)
{
	padCallbackData_t *data = (padCallbackData_t *) arg;

	*data->result = *(int *) (&buffer[20]);
	data->endfunc(data->efarg);
}

static void padInitStage5(void *arg)
{
	padCallbackData_t *data = (padCallbackData_t *) arg;

	padInitialised = 1;
	*data->result = 1;

	data->endfunc(data->efarg);
}

static void padInitStage4(void *arg)
{
	padCallbackData_t *data = (padCallbackData_t *) arg;
	int i;
	// If you require a special version of the padman, check for that here

	for (i = 0; i < 8; i++) {
		PadState[0][i].open = 0;
		PadState[0][i].port = 0;
		PadState[0][i].slot = 0;
		PadState[1][i].open = 0;
		PadState[1][i].port = 0;
		PadState[1][i].slot = 0;
	}

#ifndef ROM_PADMAN
	*(u32 *) (&buffer[0]) = PAD_RPCCMD_INIT;
	if (SifCallRpc(&padsif[0], 1, SIF_RPC_M_NOWAIT, buffer, 128, buffer, 128, padInitStage5, data) < 0) {
		*data->result = -1;
		data->endfunc(data->efarg);
		return;
	}
#else
	padInitStage5(data);
#endif
}

static void padInitStage3(void *arg)
{
	padCallbackData_t *data = (padCallbackData_t *) arg;

	if (!padsif[1].server) {
		*data->result = -5;
		data->endfunc(data->efarg);
	}
	else {
		int ret;

		ret = padGetModVersion(padInitStage4, data, data->result);
		if (ret < 0) {
			*data->result = -6;
			data->endfunc(data->efarg);
			return;
		}
#ifdef ROM_PADMAN
		/* There is no Callback, continue. */
		padInitStage4(data);
#endif
	}
}

static void padInitStage2(void *arg)
{
	padCallbackData_t *data = (padCallbackData_t *) arg;

	if (!padsif[0].server) {
		*data->result = -2;
		data->endfunc(data->efarg);
		return;
	}

	if (SifBindRpc(&padsif[1], PAD_BIND_RPC_ID2, SIF_RPC_M_NOWAIT,
			padInitStage3, data) < 0) {
		*data->result = -3;
		data->endfunc(data->efarg);
		return;
	}
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
int padInit(int a, SifRpcEndFunc_t endfunc, void *efarg, int *result)
{
	static padCallbackData_t data;

	if (padInitialised) {
		endfunc(efarg);
		return 0;
	}

	padsif[0].server = NULL;
	padsif[1].server = NULL;

	data.endfunc = endfunc;
	data.efarg = efarg;
	data.result = result;
	if (SifBindRpc(&padsif[0], PAD_BIND_RPC_ID1, SIF_RPC_M_NOWAIT,
			padInitStage2, &data) < 0) {
		return -1;
	}
	return 0;

}

/* Since padEnd() further below doesn't work right, a pseudo function is needed
 * to allow recovery after IOP reset. This function has nothing to do with the
 * functions of the IOP modules. It merely resets variables for the EE routines.
 */
int padReset()
{
	padInitialised = 0;
	padsif[0].server = NULL;
	padsif[1].server = NULL;
	return 0;
}

static void padEndStage2(void *arg)
{
	padCallbackData_t *data = (padCallbackData_t *) arg;
	int ret;
	
	ret = *(int *) (&buffer[12]);
	if (ret == 1) {
		padInitialised = 0;
	}

	*data->result = ret;
	data->endfunc(data->efarg);
}

/*
 * End all pad communication (not tested)
 */
int padEnd(SifRpcEndFunc_t endfunc, void *efarg, int *result)
{
	static padCallbackData_t data;
	int ret;


	*(u32 *) (&buffer[0]) = PAD_RPCCMD_END;

	data.endfunc = endfunc;
	data.efarg = efarg;
	data.result = result;
	if (SifCallRpc(&padsif[0], 1, SIF_RPC_M_NOWAIT, buffer, 128, buffer, 128, padEndStage2, &data) < 0)
		return -1;

	ret = *(int *) (&buffer[12]);
	if (ret == 1) {
		padInitialised = 0;
	}

	return ret;
}

static void padPortOpenStage2(void *arg)
{
	padOpenData_t *data = (padOpenData_t *) arg;

	*data->result = *(int *) (&buffer[12]);

	PadState[data->port][data->slot].padBuf = *(char **) (&buffer[20]);

	data->endfunc(data->efarg);
}

/*
 * The user should provide a pointer to a 256 byte (2xsizeof(struct pad_data))
 * 64 byte aligned pad data area for each pad port opened
 *
 * return != 0 => OK
 */
int padPortOpen(int port, int slot, void *padArea, SifRpcEndFunc_t endfunc, void *efarg, int *result)
{
	int i;
	int ret;
	struct pad_data *dma_buf = (struct pad_data *) padArea;
	static padOpenData_t data;

	// Check 64 byte alignment
	if ((u32) padArea & 0x3f) {
		//        scr_printf("dmabuf misaligned (%x)!!\n", dma_buf);
		return 0;
	}

	for (i = 0; i < 2; i++) {	// Pad data is double buffered
		memset(dma_buf[i].data, 0xff, 32);	// 'Clear' data area
		dma_buf[i].frame = 0;
		dma_buf[i].length = 0;
		dma_buf[i].state = PAD_STATE_EXECCMD;
		dma_buf[i].reqState = PAD_RSTAT_BUSY;
		dma_buf[i].ok = 0;
		dma_buf[i].length = 0;
#ifndef ROM_PADMAN
		dma_buf[i].unknown = 0;	// Should be cleared in newer padman
#endif
	}


	*(u32 *) (&buffer[0]) = PAD_RPCCMD_OPEN;
	*(u32 *) (&buffer[4]) = port;
	*(u32 *) (&buffer[8]) = slot;
	*(u32 *) (&buffer[16]) = (u32) padArea;

	data.endfunc = endfunc;
	data.efarg = efarg;
	data.result = result;
	data.port = port;
	data.slot = slot;
	ret = SifCallRpc(&padsif[0], 1, SIF_RPC_M_NOWAIT, buffer, 128, buffer, 128, padPortOpenStage2, &data);
	if (ret < 0) {
		return ret;
	}
	PadState[port][slot].open = 1;
	PadState[port][slot].padData = padArea;
	return 0;
}


/*
 * not tested :/
 */
int padPortClose(int port, int slot, SifRpcEndFunc_t endfunc, void *efarg, int *result)
{
	static padCallbackData_t data;
	int ret;

	*(u32 *) (&buffer[0]) = PAD_RPCCMD_CLOSE;
	*(u32 *) (&buffer[4]) = port;
	*(u32 *) (&buffer[8]) = slot;
	*(u32 *) (&buffer[16]) = 1;

	data.endfunc = endfunc;
	data.efarg = efarg;
	data.result = result;
	ret = SifCallRpc(&padsif[0], 1, SIF_RPC_M_NOWAIT, buffer, 128, buffer, 128, padCallback, &data);

	if (ret < 0)
		return ret;
	else {
		PadState[port][slot].open = 0;
		return 0;
	}
}


/*
 * Read pad data
 * Result is stored in 'data' which should point to a 32 byte array
 */
unsigned char padRead(int port, int slot, struct padButtonStatus *data)
{
	struct pad_data *pdata;

	pdata = padGetDmaStr(port, slot);

	memcpy(data, pdata->data, pdata->length);
	return pdata->length;
}


/*
 * Get current pad state
 * Wait until state == 6 (Ready) before trying to access the pad
 */
int padGetState(int port, int slot)
{
	struct pad_data *pdata;
	unsigned char state;


	pdata = padGetDmaStr(port, slot);

	state = pdata->state;

	if (state == PAD_STATE_STABLE) {	// Ok
		if (padGetReqState(port, slot) == PAD_RSTAT_BUSY) {
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
		return -1;

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
		return -1;

	return *(int *) (&buffer[12]);
}
#endif

/*
 * Returns the padman version
 * NOT SUPPORTED on module rom0:padman
 */
int padGetModVersion(SifRpcEndFunc_t endfunc, void *efarg, int *result)
{
	static padCallbackData_t data;

#ifdef ROM_PADMAN
	return 1;					// Well.. return a low version #
#else

	*(u32 *) (&buffer[0]) = PAD_RPCCMD_GET_MODVER;

	data.endfunc = endfunc;
	data.efarg = efarg;
	data.result = result;
	if (SifCallRpc(&padsif[0], 1, SIF_RPC_M_NOWAIT, buffer, 128, buffer, 128, padCallback,
			&data) < 0) {
		iop_prints("Failed padGetModVersion\n");
		return -1;
	}

	return 0;
#endif
}


/*
 * Get pad info (digital (4), dualshock (7), etc..)
 * ID: 3 - KONAMI GUN
 *     4 - DIGITAL PAD
 *     5 - JOYSTICK
 *     6 - NAMCO GUN
 *     7 - DUAL SHOCK
 */
int padInfoMode(int port, int slot, int infoMode, int index)
{

#ifdef ROM_PADMAN
	*(u32 *) (&buffer[0]) = PAD_RPCCMD_INFO_MODE;
	*(u32 *) (&buffer[4]) = port;
	*(u32 *) (&buffer[8]) = slot;
	*(u32 *) (&buffer[12]) = infoMode;
	*(u32 *) (&buffer[16]) = index;

	/* XXX: Check if this is working without SIF_RPC_M_NOWAIT. */
	if (SifCallRpc(&padsif[0], 1, 0, buffer, 128, buffer, 128, 0, 0) < 0)
		return 0;

	if (*(int *) (&buffer[20]) == 1) {
		padSetReqState(port, slot, PAD_RSTAT_BUSY);
	}
	return *(int *) (&buffer[20]);
#else

	struct pad_data *pdata;

	pdata = padGetDmaStr(port, slot);

	if (pdata->ok != 1)
		return 0;
	if (pdata->reqState == PAD_RSTAT_BUSY)
		return 0;

	switch (infoMode) {
	case PAD_MODECURID:
		if (pdata->modeCurId == 0xF3)
			return 0;
		else
			return (pdata->modeCurId >> 4);
		break;

	case PAD_MODECUREXID:
		if (pdata->modeOk == pdata->ok)
			return 0;
		return pdata->modeTable[pdata->modeCurOffs];
		break;

	case PAD_MODECUROFFS:
		if (pdata->modeOk != 0)
			return pdata->modeCurOffs;
		else
			return 0;
		break;

	case PAD_MODETABLE:
		if (pdata->modeOk != 0) {
			if (index == -1) {
				return pdata->nrOfModes;
			}
			else if (index < pdata->nrOfModes) {
				return pdata->modeTable[index];
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


static void padSetMainModeStage2(void *arg)
{
	padOpenData_t *data = (padOpenData_t *) arg;

	if (*(int *) (&buffer[20]) == 1) {
		padSetReqState(data->port, data->slot, PAD_RSTAT_BUSY);
	}
	*data->result = *(int *) (&buffer[20]);

	data->endfunc(data->efarg);
}

/*
 * mode = 1, -> Analog/dual shock enabled; mode = 0 -> Digital  
 * lock = 3 -> Mode not changeable by user
 */
int padSetMainMode(int port, int slot, int mode, int lock, SifRpcEndFunc_t endfunc, void *efarg, int *result)
{
	padOpenData_t data;
	int ret;

	*(u32 *) (&buffer[0]) = PAD_RPCCMD_SET_MMODE;
	*(u32 *) (&buffer[4]) = port;
	*(u32 *) (&buffer[8]) = slot;
	*(u32 *) (&buffer[12]) = mode;
	*(u32 *) (&buffer[16]) = lock;

	data.endfunc = endfunc;
	data.efarg = efarg;
	data.result = result;
	data.port = port;
	data.slot = slot;
	ret = SifCallRpc(&padsif[0], 1, SIF_RPC_M_NOWAIT, buffer, 128, buffer, 128, padSetMainModeStage2, &data);
	if (ret < 0)
		return ret;

	return 0;
}


static void padInfoPressModeStage2(void *arg)
{
	padCallbackData_t *data = (padCallbackData_t *) arg;
	int mask;

	mask = *(int *) (&buffer[12]);
	if (mask ^ 0x3ffff) {
		*data->result = 0;
	}
	else {
		*data->result = 1;
	}
	data->endfunc(data->efarg);
}

/*
 * Check if the pad has pressure sensitive buttons
 */
int padInfoPressMode(int port, int slot, SifRpcEndFunc_t endfunc, void *efarg, int *result)
{
	return padGetButtonMask(port, slot, endfunc, efarg, result);
}


/*
 * Pressure sensitive mode ON
 */
int padEnterPressMode(int port, int slot, SifRpcEndFunc_t endfunc, void *efarg, int *result)
{
	return padSetButtonInfo(port, slot, 0xFFF, endfunc, efarg, result);
}


/*
 * Check for newer version
 * Pressure sensitive mode OFF
 */
int padExitPressMode(int port, int slot, SifRpcEndFunc_t endfunc, void *efarg, int *result)
{
	return padSetButtonInfo(port, slot, 0, endfunc, efarg, result);

}


/*
 *
 */
int padGetButtonMask(int port, int slot, SifRpcEndFunc_t endfunc, void *efarg, int *result)
{
	static padCallbackData_t data;
	int ret;
	*(u32 *) (&buffer[0]) = PAD_RPCCMD_GET_BTNMASK;
	*(u32 *) (&buffer[4]) = port;
	*(u32 *) (&buffer[8]) = slot;

	data.endfunc = endfunc;
	data.efarg = efarg;
	data.result = result;
	ret = SifCallRpc(&padsif[0], 1, SIF_RPC_M_NOWAIT, buffer, 128, buffer, 128, padInfoPressModeStage2, &data);
	if (ret < 0)
		return ret;

	return 0;
}

static void padSetButtonInfoStage2(void *arg)
{
	padOpenData_t *data = (padOpenData_t *) arg;
	int val;

	val = *(int *) (&buffer[16]);

	if (val == 1) {
		padSetReqState(data->port, data->slot, PAD_RSTAT_BUSY);
	}
	*data->result = *(int *) (&buffer[16]);
	data->endfunc(data->efarg);
}

/*
 *
 */
int padSetButtonInfo(int port, int slot, int buttonInfo, SifRpcEndFunc_t endfunc, void *efarg, int *result)
{
	static padOpenData_t data;
	int ret;

	*(u32 *) (&buffer[0]) = PAD_RPCCMD_SET_BTNINFO;
	*(u32 *) (&buffer[4]) = port;
	*(u32 *) (&buffer[8]) = slot;
	*(u32 *) (&buffer[12]) = buttonInfo;

	data.endfunc = endfunc;
	data.efarg = efarg;
	data.result = result;
	data.port = port;
	data.slot = slot;
	ret = SifCallRpc(&padsif[0], 1, SIF_RPC_M_NOWAIT, buffer, 128, buffer, 128, padSetButtonInfoStage2, &data);
	if (ret < 0)
		return ret;

	return 0;
}


/*
 * Get actuator status for this controller
 * If padInfoAct(port, slot, -1, 0) != 0, the controller has actuators
 * (i think ;) )
 */
unsigned char padInfoAct(int port, int slot, int actuator, int cmd)
{

#ifdef ROM_PADMAN
	*(u32 *) (&buffer[0]) = PAD_RPCCMD_INFO_ACT;
	*(u32 *) (&buffer[4]) = port;
	*(u32 *) (&buffer[8]) = slot;
	*(u32 *) (&buffer[12]) = actuator;
	*(u32 *) (&buffer[16]) = cmd;

	/* XXX: Check if this is working without SIF_RPC_M_NOWAIT. */
	if (SifCallRpc(&padsif[0], 1, 0, buffer, 128, buffer, 128, 0, 0) < 0)
		return 0;

	if (*(int *) (&buffer[20]) == 1) {
		padSetReqState(port, slot, PAD_RSTAT_BUSY);
	}
	return *(int *) (&buffer[20]);
#else
	struct pad_data *pdata;

	pdata = padGetDmaStr(port, slot);

	if (pdata->ok != 1)
		return 0;
	if (pdata->modeOk < 2)
		return 0;
	if (actuator >= pdata->nrOfActuators)
		return 0;

	if (actuator == -1)
		return pdata->nrOfActuators;	// # of acutators?

	if (cmd >= 4)
		return 0;

	return pdata->actData[actuator * 4 + cmd];
#endif
}

static void padSetActAlignStage2(void *arg)
{
	padOpenData_t *data = (padOpenData_t *) arg;

	if (*(int *) (&buffer[20]) == 1) {
		padSetReqState(data->port, data->slot, PAD_RSTAT_BUSY);
	}
	*data->result = *(int *) (&buffer[20]);
	data->endfunc(data->efarg);
}

/*
 * Initalise actuators. On dual shock controller:
 * actAlign[0] = 0 enables 'small' engine
 * actAlign[1] = 1 enables 'big' engine
 * set actAlign[2-5] to 0xff (disable)
 */
int padSetActAlign(int port, int slot, char actAlign[6], SifRpcEndFunc_t endfunc, void *efarg, int *result)
{
	static padOpenData_t data;
	int i;
	char *ptr;
	int ret;

	*(u32 *) (&buffer[0]) = PAD_RPCCMD_SET_ACTALIGN;
	*(u32 *) (&buffer[4]) = port;
	*(u32 *) (&buffer[8]) = slot;

	ptr = (char *) (&buffer[12]);
	for (i = 0; i < 6; i++)
		ptr[i] = actAlign[i];

	data.endfunc = endfunc;
	data.efarg = efarg;
	data.result = result;
	data.port = port;
	data.slot = slot;
	ret = SifCallRpc(&padsif[0], 1, SIF_RPC_M_NOWAIT, buffer, 128, buffer, 128, padSetActAlignStage2, &data);
	if (ret < 0)
		return ret;
	return 0;
}


/*
 * Set actuator status
 * On dual shock controller, 
 * actAlign[0] = 0/1 turns off/on 'small' engine
 * actAlign[1] = 0-255 sets 'big' engine speed
 */
int padSetActDirect(int port, int slot, char actAlign[6], SifRpcEndFunc_t endfunc, void *efarg, int *result)
{
	static padCallbackData_t data;
	int i;
	char *ptr;
	int ret;

	*(u32 *) (&buffer[0]) = PAD_RPCCMD_SET_ACTDIR;
	*(u32 *) (&buffer[4]) = port;
	*(u32 *) (&buffer[8]) = slot;

	ptr = (char *) (&buffer[12]);
	for (i = 0; i < 6; i++)
		ptr[i] = actAlign[i];

	data.endfunc = endfunc;
	data.efarg = efarg;
	data.result = result;
	ret = SifCallRpc(&padsif[0], 1, SIF_RPC_M_NOWAIT, buffer, 128, buffer, 128, padCallback20, &data);
	if (ret < 0) {
		return ret;
	}

	return 0;
}


/*
 * Dunno about this one.. always returns 1?
 * I guess it should've returned if the pad was connected..
 *
 * NOT SUPPORTED with module rom0:padman
 */
int padGetConnection(int port, int slot)
{
#ifdef ROM_PADMAN
	return 1;
#else

	*(u32 *) (&buffer[0]) = PAD_RPCCMD_GET_CONNECT;
	*(u32 *) (&buffer[4]) = port;
	*(u32 *) (&buffer[8]) = slot;

	if (SifCallRpc(&padsif[0], 1, SIF_RPC_M_NOWAIT, buffer, 128, buffer, 128, 0, 0) < 0)
		return -1;

	return *(int *) (&buffer[12]);
#endif
}

int sbcall_padinit(tge_sbcall_rpc_arg_t *carg)
{
	tge_sbcall_padinit_arg_t *arg = carg->sbarg;
	return padInit(arg->mode, carg->endfunc, carg->efarg, &carg->result);
}

int sbcall_padend(tge_sbcall_rpc_arg_t *carg)
{
	return padEnd(carg->endfunc, carg->efarg, &carg->result);
}

int sbcall_padportopen(tge_sbcall_rpc_arg_t *carg)
{
	tge_sbcall_padportopen_arg_t *arg = carg->sbarg;
	return padPortOpen(arg->port, arg->slot, arg->addr, carg->endfunc, carg->efarg, &carg->result);
}

int sbcall_padportclose(tge_sbcall_rpc_arg_t *carg)
{
	tge_sbcall_padportopen_arg_t *arg = carg->sbarg;
	return padPortClose(arg->port, arg->slot, carg->endfunc, carg->efarg, &carg->result);
}

int sbcall_padsetmainmode(tge_sbcall_rpc_arg_t *carg)
{
	tge_sbcall_padsetmainmode_arg_t *arg = carg->sbarg;
	return padSetMainMode(arg->port, arg->slot, arg->offset, arg->lock, carg->endfunc, carg->efarg, &carg->result);
}

int sbcall_padsetactdirect(tge_sbcall_rpc_arg_t *carg)
{
	tge_sbcall_padsetactdirect_arg_t *arg = carg->sbarg;
	return padSetActDirect(arg->port, arg->slot, arg->data, carg->endfunc, carg->efarg, &carg->result);
}

int sbcall_padsetactalign(tge_sbcall_rpc_arg_t *carg)
{
	tge_sbcall_padsetactalign_arg_t *arg = carg->sbarg;
	return padSetActAlign(arg->port, arg->slot, arg->data, carg->endfunc, carg->efarg, &carg->result);
}

int sbcall_padinfopressmode(tge_sbcall_rpc_arg_t *carg)
{
	tge_sbcall_padpressmode_arg_t *arg = carg->sbarg;
	return padInfoPressMode(arg->port, arg->slot, carg->endfunc, carg->efarg, &carg->result);
}

int sbcall_padenterpressmode(tge_sbcall_rpc_arg_t *carg)
{
	tge_sbcall_padpressmode_arg_t *arg = carg->sbarg;
	return padEnterPressMode(arg->port, arg->slot, carg->endfunc, carg->efarg, &carg->result);
}
int sbcall_padexitpressmode(tge_sbcall_rpc_arg_t *carg)
{
	tge_sbcall_padpressmode_arg_t *arg = carg->sbarg;
	return padExitPressMode(arg->port, arg->slot, carg->endfunc, carg->efarg, &carg->result);
}

int sbcall_padread(tge_sbcall_padread_arg_t *arg)
{
	return padRead(arg->port, arg->slot, arg->data);
}

int sbcall_padgetstate(tge_sbcall_padgetstate_arg_t *arg)
{
	return padGetState(arg->port, arg->slot);
}

int sbcall_padgetreqstate(tge_sbcall_padgetreqstate_arg_t *arg)
{
	return padGetReqState(arg->port, arg->slot);
}

int sbcall_padinfoact(tge_sbcall_padinfoact_arg_t *arg)
{
	return padInfoAct(arg->port, arg->slot, arg->actno, arg->term);
}

#if 0
int sbcall_padinfocomb(tge_sbcall_padinfocomb_arg_t *arg)
{
	return padInfoComb(arg->port, arg->slot, arg->listno, arg->offset);
}
#endif

int sbcall_padinfomode(tge_sbcall_padinfomode_arg_t *arg)
{
	return padInfoMode(arg->port, arg->slot, arg->term, arg->offset);
}

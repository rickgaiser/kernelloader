/*
# _____     ___ ____     ___ ____
#  ____|   |    ____|   |        | |____|
# |     ___|   |____ ___|    ____| |    \    PS2DEV Open Source Project.
#-----------------------------------------------------------------------
# (C) 2002 Nicholas Van Veen (nickvv@xtra.co.nz)
#     2003 loser (loser@internalreality.com)
# (c) 2004 Marcus R. Brown <mrbrown@0xd6.org>
# (c) 2007 Mega Man
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.
#
# $Id$
#
# Function definitions for libcdvd (EE side calls to the iop module cdvdfsv).
#
# NOTE: These functions will work with the CDVDMAN/CDVDFSV or XCDVDMAN/XCDVDFSV
# modules stored in rom0.
#		
# NOTE: not all functions work with each set of modules!
*/

#include <stdarg.h>
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
#include "cdvd.h"
#include "scmd.h"
#include "ncmd.h"

// rpc bind values
#define CD_SERVER_INIT			0x80000592
#define CD_SERVER_SEARCHFILE		0x80000597
#define CD_SERVER_DISKREADY		0x8000059A
#define CD_SERVER_EXTRA			0x80000596	// XCDVDFSV only

static int cdvdInitCounter = 0;

// allows access to gp reg value from linker script
extern void *_gp;

// prototypes
void cdSemaExit(void);

// searchfile structure
typedef struct {
	u8 padding[32];
	char name[256];
	void *dest;
} SearchFilePkt;

// bind variables
s32 bindInit = -1;
s32 bindDiskReady = -1;
s32 bindSearchFile = -1;
// rpc binded client data
SifRpcClientData_t clientInit __attribute__ ((aligned(64)));	// for cdInit()
SifRpcClientData_t clientDiskReady __attribute__ ((aligned(64)));	// for cdDiskReady() (s-cmd)
SifRpcClientData_t clientSearchFile __attribute__ ((aligned(64)));	// for cdSearchFile() (n-cmd)

// set this to 1 or 2 to print cdvd debug messages
s32 cdDebug = 0;

// semaphore ids
s32 callbackSemaId = -1;	// callback semaphore id
#if 0
vs32 cbSema = 0;	// callback semaphore variable (not a real semaphore)

// callbacks
vs32 cdCallbackNum __attribute__ ((aligned(64)));
#endif
volatile CdCBFunc cdCallbackFunc __attribute__ ((aligned(64)));

// threads
s32 cdThreadId = 0;
//ee_thread_t cdThreadParam;
s32 callbackThreadId = 0;
//ee_thread_t callbackThreadParam;

// current command variables

s32 diskReadyMode __attribute__ ((aligned(64)));
s32 trayReqData __attribute__ ((aligned(64)));
u32 initMode __attribute__ ((aligned(64)));

// searchfile stuff
SearchFilePkt searchFileSendBuff __attribute__ ((aligned(64)));
u32 searchFileRecvBuff __attribute__ ((aligned(64)));

// Prototypes for multimodule
extern s32 bindInit;
extern s32 bindDiskReady;
extern s32 bindSearchFile;
extern SifRpcClientData_t clientInit;
extern SifRpcClientData_t clientDiskReady;
extern SifRpcClientData_t clientSearchFile;
extern s32 cdDebug;
#if 0
extern s32 callbackSemaId;
extern vs32 cbSema;
extern vs32 cdCallbackNum;
extern volatile CdCBFunc cdCallbackFunc;
extern s32 cdThreadId;
extern ee_thread_t cdThreadParam;
extern s32 callbackThreadId;
extern ee_thread_t callbackThreadParam;
extern s32 diskReadyMode;
extern s32 trayReqData;
extern u32 initMode;
extern SearchFilePkt searchFileSendBuff;
extern u32 searchFileRecvBuff;
#endif

// **** Other Functions ****

static void cdvdInitCallback(void *rarg)
{
	tge_sbcall_rpc_arg_t *carg = (tge_sbcall_rpc_arg_t *) rarg;
	switch(cdvdInitCounter) {
	case 0:
		if (clientInit.server != 0) {
			bindInit = 0;
			cdvdInitCounter++;
		} else {
			printf("CDVD: Bind clientInit failed.\n");
		}
		carg->result = -1;
		CDVD_UNLOCKS();
		break;

	case 1:
		if (clientSearchFile.server != 0) {
			bindSearchFile = 0;
			cdvdInitCounter++;
		} else {
			printf("CDVD: Bind clientSearchFile failed.\n");
		}
		carg->result = -1;
		CDVD_UNLOCKS();
		break;

	case 2:
		if (clientDiskReady.server != 0) {
			bindDiskReady = 0;
			cdvdInitCounter++;
		} else {
			printf("CDVD: Bind clientDiskReady failed.\n");
		}
		carg->result = -1;
		CDVD_UNLOCKS();
		break;

	case 3:
		cdvdInitCounter++;
		carg->result = -1;
		CDVD_UNLOCKS();
		break;

	case 4:
		if (cdNCmdInitCallback(carg) == 0) {
			cdvdInitCounter++;
		} else {
			printf("cdNCmdInit failed\n");
		}
		carg->result = -1;
		CDVD_UNLOCKN();
		break;

	case 5:
		if (cdSCmdInitCallback(carg) == 0) {
			cdvdInitCounter++;
			carg->result = 0;
		} else {
			printf("cdNCmdInit failed\n");
			carg->result = -1;
		}
		CDVD_UNLOCKS();
		break;
	}
	carg->endfunc(carg->efarg, carg->result);
}

// init cdvd system
// 
// args:        init mode (CDVD_INIT_???)
// returns:     1 if successful
//                      0 if error
int sbcall_cdvdinit(tge_sbcall_rpc_arg_t *carg)
{
	//printf("sbcall_cdvdinit stage %d\n", cdvdInitCounter);
	switch(cdvdInitCounter) {
	case 0:
		if (CDVD_LOCKS()) {
			/* Temporay not available. */
			return -SIF_RPCE_SENDP;
		}
		bindSearchFile = -1;
		bindNCmd = -1;
		bindSCmd = -1;
		bindDiskReady = -1;
		bindInit = -1;

		if (SifBindRpc(&clientInit, CD_SERVER_INIT, SIF_RPC_M_NOWAIT, cdvdInitCallback, carg) < 0) {
			printf("CDVD: bind err CD_Init\n");
			CDVD_UNLOCKS();
			return -SIF_RPCE_SENDP;
		}
		break;
	case 1:
		if (CDVD_LOCKS()) {
			/* Temporay not available. */
			return -SIF_RPCE_SENDP;
		}
		if (SifBindRpc(&clientSearchFile, CD_SERVER_SEARCHFILE, SIF_RPC_M_NOWAIT, cdvdInitCallback, carg) < 0) {
			printf("CDVD: bind err cdSearchFile\n");
			CDVD_UNLOCKS();
			return -SIF_RPCE_SENDP;
		}
		break;
	case 2:
		if (CDVD_LOCKS()) {
			/* Temporay not available. */
			return -SIF_RPCE_SENDP;
		}
		if (SifBindRpc(&clientDiskReady, CD_SERVER_DISKREADY, SIF_RPC_M_NOWAIT, cdvdInitCallback, carg) < 0) {
			printf("CDVD: bind err CdDiskReady\n");
			CDVD_UNLOCKS();
			return -SIF_RPCE_SENDP;
		}
		break;
	case 3:
		if (CDVD_LOCKS()) {
			/* Temporay not available. */
			return -SIF_RPCE_SENDP;
		}
		initMode = CDVD_INIT_INIT;
		SifWriteBackDCache(&initMode, 4);
		if (SifCallRpc(&clientInit, 0, SIF_RPC_M_NOWAIT, &initMode, 4, 0, 0, cdvdInitCallback, carg) < 0) {
			CDVD_UNLOCKS();
			printf("CDVD: rpc call err CDVD_INIT_INIT\n");
			return -SIF_RPCE_SENDP;
		}
		break;
	case 4:
		if (CDVD_LOCKN()) {
			/* Temporay not available. */
			return -SIF_RPCE_SENDP;
		}
		if (cdNCmdInit(carg, cdvdInitCallback) < 0) {
			CDVD_UNLOCKN();
			printf("CDVD: bind err N CMD\n");
			return -SIF_RPCE_SENDP;
		}
		break;
	case 5:
		if (CDVD_LOCKS()) {
			/* Temporay not available. */
			return -SIF_RPCE_SENDP;
		}
		if (cdSCmdInit(carg, cdvdInitCallback) < 0) {
			CDVD_UNLOCKS();
			printf("CDVD: bind err S CMD\n");
			return -SIF_RPCE_SENDP;
		}
		break;
	default:
		//printf("CDVD: already initialized.\n");
		carg->result = 0;
		carg->endfunc(carg->efarg, carg->result);
		break;
	}

	return 0;
}

static void cdResetCallback(void *rarg)
{
	tge_sbcall_rpc_arg_t *carg = (tge_sbcall_rpc_arg_t *) rarg;
	carg->result = 0;
	carg->endfunc(carg->efarg, carg->result);
}

int sbcall_cdvdreset(tge_sbcall_rpc_arg_t *carg)
{
#if 0
	//cdThreadId = GetThreadId();
	bindSearchFile = -1;
	bindNCmd = -1;
	bindSCmd = -1;
	bindDiskReady = -1;
	bindInit = -1;
#endif

	/* Just call init again. */
	initMode = CDVD_INIT_INIT;
	SifWriteBackDCache(&initMode, 4);
	if (SifCallRpc(&clientInit, 0, SIF_RPC_M_NOWAIT, &initMode, 4, 0, 0, cdResetCallback, carg) < 0) {
		return -SIF_RPCE_SENDP;
	}
	return 0;
}

// convert from sector number to minute:second:frame
#ifdef F_cdIntToPos
CdvdLocation_t *cdIntToPos(s32 i, CdvdLocation_t * p)
{
	p->minute = (((((i + 150) / 75) / 60) / 10) * 16) + ((((i + 150) / 75) / 60) % 10);
	p->second = (((((i + 150) / 75) % 60) / 10) * 16) + ((((i + 150) / 75) % 60) % 10);
	p->sector = ((((i + 150) % 75) / 10) * 16) + (((i + 150) % 75) % 10);
	return p;
}
#endif

// convert from minute:second:frame to sector number
#ifdef F_cdPosToInt
s32 cdPosToInt(CdvdLocation_t * p)
{
	return ((((p->minute / 16) * 10) + (p->minute & 0xF)) * 60 + ((p->second / 16) * 10) + (p->second & 0xF)
	    ) * 75 + (p->sector / 16) * 10 + (p->sector & 0xF) - 150;
}
#endif


// search for a file on disc
// 
// args:        file structure to get file info in
//                      name of file to search for (no wildcard characters)
//                              (should be in the form '\\SYSTEM.CNF;1')
// returns:     1 if successful
//                      0 if error (or no file found)
#ifdef F_cdSearchFile
s32 cdSearchFile(CdvdFileSpec_t * file, const char *name)
{
	s32 i;

	cdSemaInit();
	if (PollSema(nCmdSemaId) != nCmdSemaId)
		return 0;
	nCmdNum = CD_SERVER_SEARCHFILE;
	ReferThreadStatus(cdThreadId, &cdThreadParam);
	if (cdSync(1)) {
		SignalSema(nCmdSemaId);
		return 0;
	}
	if (bindSearchFile < 0) {
		return 0;
	}

	strncpy(searchFileSendBuff.name, name, 255);
	searchFileSendBuff.name[255] = '\0';
	searchFileSendBuff.dest = &searchFileSendBuff;

	if (cdDebug > 0)
		printf("ee call cmd search %s\n", searchFileSendBuff.name);
	SifWriteBackDCache(&searchFileSendBuff, sizeof(SearchFilePkt));
	if (SifCallRpc(&clientSearchFile, 0, 0, &searchFileSendBuff, sizeof(SearchFilePkt), nCmdRecvBuff, 4, 0, 0) < 0) {
		SignalSema(nCmdSemaId);
		return -SIF_RPCE_SENDP;
	}

	memcpy(file, UNCACHED_SEG(&searchFileSendBuff), 32);

	if (cdDebug > 0) {
		printf("search name %s\n", file->name);
		printf("search size %d\n", file->size);
		printf("search loc lnn %d\n", file->lsn);
		printf("search loc date %02X %02X %02X %02X %02X %02X %02X %02X\n",
		       file->date[0], file->date[1], file->date[2], file->date[3],
		       file->date[4], file->date[5], file->date[6], file->date[7]);
		printf("search loc date %02d %02d %02d %02d %02d %02d %02d %02d\n",
		       file->date[0], file->date[1], file->date[2], file->date[3],
		       file->date[4], file->date[5], file->date[6], file->date[7]);
	}

	SignalSema(nCmdSemaId);
//	return 1;
	return *(s32*)UNCACHED_SEG(nCmdRecvBuff);
}
#endif

static void cdDiskReadyCallback(void *rarg)
{
	tge_sbcall_rpc_arg_t *carg = (tge_sbcall_rpc_arg_t *) rarg;
	if (cdDebug > 0) {
		printf("DiskReady ended\n");
	}

	carg->result = *(s32 *) UNCACHED_SEG(sCmdRecvBuff);
	CDVD_UNLOCKS();
	carg->endfunc(carg->efarg, carg->result);
}

// checks if drive is ready
// 
// args:         mode
// returns:     CDVD_READY_READY if ready
//                      CDVD_READY_NOTREADY if busy
int sbcall_cdvdready(tge_sbcall_rpc_arg_t *carg) {
	tge_sbcall_cdvdready_arg_t *arg = carg->sbarg;

	if (cdDebug > 0) {
		printf("DiskReady 0\n");
	}

	//	return CDVD_READY_NOTREADY;
	if (CDVD_LOCKS()) {
		/* Temporay not available. */
		return CDVD_READY_NOTREADY;
	}

	diskReadyMode = arg->mode;
	if (bindDiskReady < 0) {
		CDVD_UNLOCKS();
		return -1;
	}
	SifWriteBackDCache(&diskReadyMode, 4);

	if (SifCallRpc(&clientDiskReady, 0, SIF_RPC_M_NOWAIT, &diskReadyMode, 4, sCmdRecvBuff, 4, cdDiskReadyCallback, carg) < 0) {
		CDVD_UNLOCKS();
		return -SIF_RPCE_SENDP;
	}

	return 0;
}

#ifdef F_cdSemaInit
void cdSemaInit(void)
{
	struct t_ee_sema semaParam;

	// return if both semaphores are already inited
	if (nCmdSemaId != -1 && sCmdSemaId != -1)
		return;

	semaParam.init_count = 1;
	semaParam.max_count = 1;
	semaParam.option = 0;
	nCmdSemaId = CreateSema(&semaParam);
	sCmdSemaId = CreateSema(&semaParam);

	semaParam.init_count = 0;
	callbackSemaId = CreateSema(&semaParam);

	cbSema = 0;
}
#endif

#ifdef F_cdSemaExit
void cdSemaExit(void)
{
	if (callbackThreadId) {
		cdCallbackNum = -1;
		SignalSema(callbackSemaId);
	}
	DeleteSema(nCmdSemaId);
	DeleteSema(sCmdSemaId);
	DeleteSema(callbackSemaId);
}
#endif

// initialise callback thread
// 
// args:        callback thread priority
//                      callback thread stack address (16 byte aligned)
//                      callback thread stack size
// returns:     1 if initialised callback
//                      0 if only priority was changed
#ifdef F_cdInitCallbackThread
static void cdCallbackLoop(void);
s32 cdInitCallbackThread(s32 priority, void *stackAddr, s32 stackSize)
{
	// if callback thread has already been initialised, just change its priority
	if (callbackThreadId != 0) {
		ChangeThreadPriority(callbackThreadId, priority);
		return 0;
	}
	// initialise callback thread
	cdThreadId = GetThreadId();
	ReferThreadStatus(cdThreadId, &cdThreadParam);
	callbackThreadParam.stack_size = stackSize;
	callbackThreadParam.gp_reg = &_gp;
	callbackThreadParam.func = cdCallbackLoop;
	callbackThreadParam.stack = stackAddr;
	callbackThreadParam.initial_priority = priority;
	callbackThreadId = CreateThread(&callbackThreadParam);
	StartThread(callbackThreadId, 0);

	return 1;
}
#endif

// sets the cd callback function
// gets called when the following functions complete:
//    cdSeek, cdStandby, cdStop, cdPause, cdRead
// 
// args:        pointer to new callback function (or null)
// returns:     pointer to old function
#ifdef F_cdSetCallback
CdCBFunc cdSetCallback(CdCBFunc newFunc)
{
	CdCBFunc oldFunc = cdCallbackFunc;

	if (cdSync(1))
		return 0;

	cdCallbackFunc = newFunc;
	return oldFunc;
}
#endif

// **** Util Functions ****


// callback loop thread
// once callbacks have been inited using cdInitCallbackThread()
// this function continually loops until a callback with function
// number '-1' is generated
#ifdef F_cdInitCallbackThread
static void cdCallbackLoop(void)
{
	while (1) {
		WaitSema(callbackSemaId);

		// if callback number if -1, stop callbck thread loop
		if (cdCallbackNum == -1)
			ExitThread();

		if (cdDebug > 0)
			printf("cdCallbackFunc = %08X   cdCallbackNum = %d\n", (u32) cdCallbackFunc, cdCallbackNum);

		// if callback function number and 'custom callback function' pointer are valid, do callback
		if (cdCallbackFunc && cdCallbackNum)
			cdCallbackFunc(cdCallbackNum);

		cbSema = 0;
	}
}
#endif

// generic cd callback function
#ifdef F_cdCallback
void cdCallback(void *funcNum)
{
	// set the currently executing function num
	cdCallbackNum = *(s32*) funcNum;
	iSignalSema(nCmdSemaId);
	
	// check if user callback is registered
	if (callbackThreadId)
	{
		if (cdCallbackFunc)
		{
			iSignalSema(callbackSemaId);
			return;
		}
	}
	
	// clear the currently executing function num
	cbSema = 0;
	cdCallbackNum = 0;
	CDVD_UNLOCKN();

}
#endif

#if 0
int sbcall_cdvdpowerhook(tge_sbcall_rpc_arg_t *carg)
{
	tge_sbcall_cdvdpowerhook_arg_t *arg = carg->sbarg;
	return cdPowerHook(arg->powerfunc, arg->pfarg, carg->endfunc, carg->efarg, &carg->result);
}
#endif


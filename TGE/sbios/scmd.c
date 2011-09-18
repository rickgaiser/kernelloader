/*
# _____     ___ ____     ___ ____
#  ____|   |    ____|   |        | |____|
# |     ___|   |____ ___|    ____| |    \    PS2DEV Open Source Project.
#-----------------------------------------------------------------------
## (C) 2002 Nicholas Van Veen (nickvv@xtra.co.nz)
#     2003 loser (loser@internalreality.com)
# (c) 2004 Marcus R. Brown <mrbrown@0xd6.org> Licenced under Academic Free License version 2.0
# (c) 2007 Mega Man
# Review ps2sdk README & LICENSE files for further details.
#
# $Id$
# Function definitions for libcdvd (EE side calls to the iop module cdvdfsv).
#
# NOTE: These functions will work with the CDVDMAN/CDVDFSV or XCDVDMAN/XCDVDFSV
# modules stored in rom0.
#
# NOTE: not all functions work with each set of modules!
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
#include "cdvd.h"
#include "scmd.h"
#include "mutex.h"

#define CD_SERVER_SCMD			0x80000593	// blocking commands (Synchronous)

#define CD_SCMD_READCLOCK			0x01
#define CD_SCMD_WRITECLOCK			0x02
#define CD_SCMD_GETDISCTYPE			0x03
#define CD_SCMD_GETERROR			0x04
#define CD_SCMD_TRAYREQ				0x05
#define CD_SCMD_READILINKID			0x06
#define CD_SCMD_WRITEILINKID		0x07
#define CD_SCMD_READNVM				0x08
#define CD_SCMD_WRITENVM			0x09
#define CD_SCMD_DECSET1				0x0A
#define CD_SCMD_SCMD				0x0B
#define CD_SCMD_STATUS				0x0C
#define CD_SCMD_SETHDMODE			0x0D
#define CD_SCMD_OPENCFG				0x0E
#define CD_SCMD_CLOSECFG			0x0F
#define CD_SCMD_READCFG				0x10
#define CD_SCMD_WIRTECFG			0x11
#define CD_SCMD_READCONSOLEID		0x12
#define CD_SCMD_WRITECONSOLEID		0x13
#define CD_SCMD_GETMECACONVERSION	0x14
#define CD_SCMD_CTRLAUDIODIGITALOUT	0x15
#define CD_SCMD_BREAK				0x16
#define CD_SCMD_READSUBQ			0x17
#define CD_SCMD_FORBIDDVDP			0x18
#define CD_SCMD_AUTOADJUSTCTRL		0x19
#ifdef NEW_ROM_MODULE_VERSION
#define CD_SCMD_CANCELPOWEROFF	0x1F	// XCDVDFSV only
#define CD_SCMD_BLUELEDCTRL		0x20	// XCDVDFSV only
#define CD_SCMD_POWEROFF		0x21	// XCDVDFSV only
#define CD_SCMD_MMODE			0x22	// XCDVDFSV only
#define CD_SCMD_SETTHREADPRI	0x23	// XCDVDFSV only
#endif

/** Trace macro for function call. */
#define CD_CHECK_SCMD(cmd) cdCheckSCmd(cmd, __FILE__, __LINE__)

/* SCMD is locked. Retry possible if interrupts are enabled otherwise error. */
#define SCMD_EAGAIN (core_in_interrupt() ? -SIF_RPCE_GETP : -SIF_RPCE_SENDP)

s32 bindSCmd = -1;

SifRpcClientData_t clientSCmd __attribute__ ((aligned(64)));	// for s-cmds

/** Receive buffer requires only 48, use 64 because of cache aliasing problems.*/
u8 sCmdRecvBuff[64] __attribute__ ((aligned(64)));
/** Receive buffer requires only 48, use 64 because of cache aliasing problems.*/
u8 sCmdSendBuff[64] __attribute__ ((aligned(64)));

s32 sCmdNum = 0;

/** Mutex to protect RPC calls. */
static sbios_mutex_t cdvdMutexS = SBIOS_MUTEX_INIT;

s32 cdCheckSCmd(s32 cmd, const char *file, int line);


// **** S-Command Functions ****

static void cdvdreadrtcStage2(void *rarg)
{
	tge_sbcall_rpc_arg_t *carg = (tge_sbcall_rpc_arg_t *) rarg;
	tge_sbcall_cdvdwritertc_arg_t *arg = carg->sbarg;

	memcpy(arg, UNCACHED_SEG(sCmdRecvBuff + 4), 8);

	carg->result = *(s32 *) UNCACHED_SEG(sCmdRecvBuff);
	CDVD_UNLOCKS();
	carg->endfunc(carg->efarg, carg->result);
}

// read clock value from ps2s clock
// 
// args:        time/date struct
// returns:     1 if successful
//                      0 if error
int sbcall_cdvdreadrtc(tge_sbcall_rpc_arg_t *carg)
{
	//tge_sbcall_cdvdwritertc_arg_t *arg = carg->sbarg;
	if (CD_CHECK_SCMD(CD_SCMD_READCLOCK) == 0)
		return SCMD_EAGAIN;

	if (SifCallRpc(&clientSCmd, CD_SCMD_READCLOCK, SIF_RPC_M_NOWAIT, 0, 0, sCmdRecvBuff, 16, cdvdreadrtcStage2, carg) < 0) {
		CDVD_UNLOCKS();
		return -SIF_RPCE_SENDP;
	}
	return 0;
}

// write clock value to ps2s clock
// 
// args:        time/date struct to set clocks time with
// returns:     1 if successful
//                      0 if error
int sbcall_cdvdwritertc(tge_sbcall_rpc_arg_t *carg)
{
	tge_sbcall_cdvdwritertc_arg_t *arg = carg->sbarg;
	if (CD_CHECK_SCMD(CD_SCMD_WRITECLOCK) == 0)
		return SCMD_EAGAIN;

	memcpy(sCmdSendBuff, arg, 8);
	SifWriteBackDCache(sCmdSendBuff, 8);

	if (SifCallRpc(&clientSCmd, CD_SCMD_WRITECLOCK, SIF_RPC_M_NOWAIT, sCmdSendBuff, 8, sCmdRecvBuff, 16, cdvdreadrtcStage2, carg) < 0) {
		CDVD_UNLOCKS();
		return -SIF_RPCE_SENDP;
	}
	return 0;
}

static void SCmdStage2(void *rarg)
{
	tge_sbcall_rpc_arg_t *carg = (tge_sbcall_rpc_arg_t *) rarg;
	carg->result = *(s32 *) UNCACHED_SEG(sCmdRecvBuff);
	CDVD_UNLOCKS();
	carg->endfunc(carg->efarg, carg->result);
}

// gets the type of the currently inserted disc
// 
// returns:     disk type (CDVD_TYPE_???)
int sbcall_cdvdgettype(tge_sbcall_rpc_arg_t *carg)
{
	if (CD_CHECK_SCMD(CD_SCMD_GETDISCTYPE) == 0) {
		return SCMD_EAGAIN;
	}

	if (SifCallRpc(&clientSCmd, CD_SCMD_GETDISCTYPE, SIF_RPC_M_NOWAIT, 0, 0, sCmdRecvBuff, 4, SCmdStage2, carg) < 0) {
		CDVD_UNLOCKS();
		return -SIF_RPCE_SENDP;
	}
	return 0;
}

// gets the last error that occurred
// 
// returns:     error type (CDVD_ERR_???)
int sbcall_cdvdgeterror(tge_sbcall_rpc_arg_t *carg)
{
	if (CD_CHECK_SCMD(CD_SCMD_GETERROR) == 0)
		return SCMD_EAGAIN;

	if (SifCallRpc(&clientSCmd, CD_SCMD_GETERROR, SIF_RPC_M_NOWAIT, 0, 0, sCmdRecvBuff, 4, SCmdStage2, carg) < 0) {
		CDVD_UNLOCKS();
		return -SIF_RPCE_SENDP;
	}
	return 0;
}

static void cdvdtrayrequestStage2(void *rarg)
{
	tge_sbcall_rpc_arg_t *carg = (tge_sbcall_rpc_arg_t *) rarg;
	tge_sbcall_cdvdtrayrequest_arg_t *arg = carg->sbarg;

	arg->traycount = *(u32 *) UNCACHED_SEG(sCmdRecvBuff + 4);

	carg->result = ! (*(s32 *) UNCACHED_SEG(sCmdRecvBuff));
	CDVD_UNLOCKS();

	carg->endfunc(carg->efarg, carg->result);
}

// open/close/check disk tray
// 
// args:        param (CDVD_TRAY_???)
//                      address for returning tray state change
// returns:     1 if successful
//                      0 if error
int sbcall_cdvdtrayrequest(tge_sbcall_rpc_arg_t *carg)
{
	tge_sbcall_cdvdtrayrequest_arg_t *arg = carg->sbarg;
	if (CD_CHECK_SCMD(CD_SCMD_TRAYREQ) == 0)
		return SCMD_EAGAIN;

	memcpy(sCmdSendBuff, &arg->request, 4);
	SifWriteBackDCache(sCmdSendBuff, 4);

	if (SifCallRpc(&clientSCmd, CD_SCMD_TRAYREQ, SIF_RPC_M_NOWAIT, sCmdSendBuff, 4, sCmdRecvBuff, 8, cdvdtrayrequestStage2, carg) < 0) {
		CDVD_UNLOCKS();
		return -SIF_RPCE_SENDP;
	}
	return 0;
}

// send an s-command by function number
// 
// args:        command number
//                      input buffer  (can be null)
//                      size of input buffer  (0 - 16 byte)
//                      output buffer (can be null)
//                      size of output buffer (0 - 16 bytes)
// returns:     1 if successful
//                      0 if error
#ifdef F_cdApplySCmd
s32 cdApplySCmd(u8 cmdNum, const void *inBuff, u16 inBuffSize, void *outBuff, u16 outBuffSize)
{
	if (CD_CHECK_SCMD(CD_SCMD_SCMD) == 0)
		return SCMD_EAGAIN;

	*(u16 *) & sCmdSendBuff[0] = cmdNum;
	*(u16 *) & sCmdSendBuff[2] = inBuffSize;
	memset(&sCmdSendBuff[4], 0, 16);
	if (inBuff)
		memcpy(&sCmdSendBuff[4], inBuff, inBuffSize);
	SifWriteBackDCache(sCmdSendBuff, 20);

	if (SifCallRpc(&clientSCmd, CD_SCMD_SCMD, 0, sCmdSendBuff, 20, sCmdRecvBuff, 16, 0, 0) < 0) {
		SignalSema(sCmdSemaId);
		CDVD_UNLOCKS();
		return -SIF_RPCE_SENDP;
	}

	if (outBuff)
		memcpy(outBuff, UNCACHED_SEG(sCmdRecvBuff), outBuffSize);
	SignalSema(sCmdSemaId);
	return 1;
}
#endif

// gets the status of the cd system
// 
// returns:     status (CDVD_STAT_???)
#ifdef F_cdStatus
s32 cdStatus(void)
{
	if (CD_CHECK_SCMD(CD_SCMD_STATUS) == 0)
		return SCMD_EAGAIN;

	if (SifCallRpc(&clientSCmd, CD_SCMD_STATUS, 0, 0, 0, sCmdRecvBuff, 4, 0, 0) < 0) {
		SignalSema(sCmdSemaId);
		CDVD_UNLOCKS();
		return -SIF_RPCE_SENDP;
	}

	if (cdDebug >= 2) {
		printf("status called\n");
	}

	SignalSema(sCmdSemaId);
	return *(s32 *) UNCACHED_SEG(sCmdRecvBuff);
}
#endif

// 'breaks' the currently executing command
// 
// returns:     1 if successful
//                      0 if error
#ifdef F_cdBreak
s32 cdBreak(void)
{
	if (CD_CHECK_SCMD(CD_SCMD_BREAK) == 0)
		return SCMD_EAGAIN;

	if (SifCallRpc(&clientSCmd, CD_SCMD_BREAK, 0, 0, 0, sCmdRecvBuff, 4, 0, 0) < 0) {
		SignalSema(sCmdSemaId);
		CDVD_UNLOCKS();
		return -SIF_RPCE_SENDP;
	}

	SignalSema(sCmdSemaId);
	return *(s32 *) UNCACHED_SEG(sCmdRecvBuff);
}
#endif

// cancel power off
// 
// SUPPORTED IN XCDVDMAN/XCDVDFSV ONLY
// 
// args:        result
// returns:     1 if successful
//                      0 if error
#ifdef F_cdCancelPowerOff
s32 cdCancelPowerOff(u32 * result)
{
	if (CD_CHECK_SCMD(CD_SCMD_CANCELPOWEROFF) == 0)
		return SCMD_EAGAIN;

	if (SifCallRpc(&clientSCmd, CD_SCMD_CANCELPOWEROFF, 0, 0, 0, sCmdRecvBuff, 8, 0, 0) < 0) {
		SignalSema(sCmdSemaId);
		CDVD_UNLOCKS();
		return -SIF_RPCE_SENDP;
	}

	*result = *(u32 *) UNCACHED_SEG(sCmdRecvBuff + 4);
	SignalSema(sCmdSemaId);
	return *(s32 *) UNCACHED_SEG(sCmdRecvBuff);
}
#endif

// blue led control
// 
// SUPPORTED IN XCDVDMAN/XCDVDFSV ONLY
// 
// args:        control value
//                      result
// returns:     1 if successful
//                      0 if error
#ifdef F_cdBlueLedCtrl
s32 cdBlueLedCtrl(u8 control, u32 * result)
{
	if (CD_CHECK_SCMD(CD_SCMD_BLUELEDCTRL) == 0)
		return SCMD_EAGAIN;

	*(u32 *) sCmdSendBuff = control;
	SifWriteBackDCache(sCmdSendBuff, 4);

	if (SifCallRpc(&clientSCmd, CD_SCMD_BLUELEDCTRL, 0, sCmdSendBuff, 4, sCmdRecvBuff, 8, 0, 0) < 0) {
		SignalSema(sCmdSemaId);
		CDVD_UNLOCKS();
		return -SIF_RPCE_SENDP;
	}

	*result = *(u32 *) UNCACHED_SEG(sCmdRecvBuff + 4);
	SignalSema(sCmdSemaId);
	return *(s32 *) UNCACHED_SEG(sCmdRecvBuff);
}
#endif

#ifdef NEW_ROM_MODULE_VERSION
// power off
// 
// SUPPORTED IN XCDVDMAN/XCDVDFSV ONLY
// 
// args:        result
// returns:     1 if successful
//                      0 if error
s32 cdPowerOff(u32 * result)
{
	if (CD_CHECK_SCMD(CD_SCMD_POWEROFF) == 0)
		return SCMD_EAGAIN;

	if (SifCallRpc(&clientSCmd, CD_SCMD_POWEROFF, 0, 0, 0, sCmdRecvBuff, 8, 0, 0) < 0) {
		CDVD_UNLOCKS();
		return -SIF_RPCE_SENDP;
	}

	*result = *(u32 *) UNCACHED_SEG(sCmdRecvBuff + 4);
	CDVD_UNLOCKS();
	return *(s32 *) UNCACHED_SEG(sCmdRecvBuff);
}
#endif

static void MmodeStage2(void *rarg)
{
	tge_sbcall_rpc_arg_t *carg = (tge_sbcall_rpc_arg_t *) rarg;
	CDVD_UNLOCKS();
	carg->result = 1;
	carg->endfunc(carg->efarg, carg->result);
}

// set media mode
// 
// SUPPORTED IN XCDVDMAN/XCDVDFSV ONLY
// 
// args:        media mode
// returns:     1 if successful
//                      0 if error
int sbcall_cdvdmmode(tge_sbcall_rpc_arg_t *carg)
{
#ifdef NEW_ROM_MODULE_VERSION
	tge_sbcall_cdvdmmode_arg_t *arg = carg->sbarg;

	if (CD_CHECK_SCMD(CD_SCMD_MMODE) == 0)
		return SCMD_EAGAIN;

	memcpy(sCmdSendBuff, &arg->media, 4);
	SifWriteBackDCache(sCmdSendBuff, 4);

	if (SifCallRpc(&clientSCmd, CD_SCMD_MMODE, SIF_RPC_M_NOWAIT, sCmdSendBuff, 4, sCmdRecvBuff, 4, MmodeStage2, carg) < 0) {
		CDVD_UNLOCKS();
		return -SIF_RPCE_SENDP;
	}
	return 0;
#else
	/* Function not supported. */
	if (core_in_interrupt()) {
		/* Interrupts are disabled, need to call callback function when interrupts are enabled again. */
		/* On Linux 2.2: Simulate by calling different RPC function. The state machine needs this emulation. */
		if (CD_CHECK_SCMD(CD_SCMD_GETERROR) == 0)
			return SCMD_EAGAIN;

		if (SifCallRpc(&clientSCmd, CD_SCMD_GETERROR, SIF_RPC_M_NOWAIT, 0, 0, sCmdRecvBuff, 4, MmodeStage2, carg) < 0) {
			CDVD_UNLOCKS();
			return -SIF_RPCE_SENDP;
		}
	} else {
		/* Interrupts are enabled. So caller must expect that callback is called before the function returns. */
		/* On Linux 2.4: directly call callback function. */
		carg->result = 1;
		carg->endfunc(carg->efarg, carg->result);
	}
	return 0;
#endif
}

// change cd thread priority
// 
// SUPPORTED IN XCDVDMAN/XCDVDFSV ONLY
// 
// args:        media mode
// returns:     1 if successful
//                      0 if error
#ifdef F_cdChangeThreadPriority
s32 cdChangeThreadPriority(u32 priority)
{
	if (CD_CHECK_SCMD(CD_SCMD_SETTHREADPRI) == 0)
		return CD_SCMD_MMODE;

	memcpy(sCmdSendBuff, &priority, 4);
	SifWriteBackDCache(sCmdSendBuff, 4);

	if (SifCallRpc(&clientSCmd, CD_SCMD_SETTHREADPRI, 0, sCmdSendBuff, 4, sCmdRecvBuff, 4, 0, 0) < 0) {
		SignalSema(sCmdSemaId);
		CDVD_UNLOCKS();
		return -SIF_RPCE_SENDP;
	}

	SignalSema(sCmdSemaId);
	return *(s32 *) UNCACHED_SEG(sCmdRecvBuff);
}
#endif

/**
 * Checks for completion of s-commands and try to lock.
 *
 * @returns
 * @retval 0 if completed and locked.
 * @retval 1 if still executing command or locked by other.
 */
int cdvdLockS(const char *file, int line)
{
	file = file;
	line = line;

	if (sbios_tryLock(&cdvdMutexS)) {
		printf("cdvdLockS %s:%d fail 1\n", file, line);
		return 1;
	}
	// check status and return
	if (SifCheckStatRpc(&clientSCmd)) {
		sbios_unlock(&cdvdMutexS);
		printf("cdvdLockS %s:%d fail 2\n", file, line);
		return 1;
	}
#if 0
	printf("cdvdLockS %s:%d ok\n", file, line);
#endif

	return 0;
}

/**
 * Unlock cdvd scmd mutex.
 * Only working if RPC calls has been finished.
 */
void cdvdUnlockS(const char *file, int line)
{
	file = file;
	line = line;
#if 0
	printf("cdvdUnlockS %s:%d\n", file, line);
#endif
	sbios_unlock(&cdvdMutexS);
}

/**
 * Check whether ready to send an s-command and lock it.
 *
 * @param cmd current command
 * @returns
 * @retval 1 if ready to send, after use cdvdUnlockN is required.
 * @retval 0 if busy/error
 */
s32 cdCheckSCmd(s32 cmd, const char *file, int line)
{
	if (cdvdLockS(file, line)) {
		return 0;
	}

	sCmdNum = cmd;

	if (bindSCmd >= 0) {
		/* RPC is bound. */
		return 1;
	} else {
		/* Error, RPC not yet bound. */
		cdvdUnlockS(file, line);
		return 0;
	}
}

int cdSCmdInitCallback(tge_sbcall_rpc_arg_t *carg)
{
	carg = carg;

	if (clientSCmd.server != 0) {
		bindSCmd = 0;
		return 0;
	} else {
		return -1;
	}
}

int cdSCmdInit(tge_sbcall_rpc_arg_t *carg, SifRpcEndFunc_t endfunc)
{
	// bind rpc for n-commands
	return SifBindRpc(&clientSCmd, CD_SERVER_SCMD, SIF_RPC_M_NOWAIT, endfunc, carg);
}


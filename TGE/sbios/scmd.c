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
#include "iopmem.h"
#include "stdio.h"
#include "string.h"
#include "cdvd.h"
#include "scmd.h"

#define CD_SERVER_SCMD			0x80000593	// blocking commands (Synchronous)

#define CD_SCMD_READCLOCK		0x01
#define CD_SCMD_WRITECLOCK		0x02
#define CD_SCMD_GETDISCTYPE		0x03
#define CD_SCMD_GETERROR		0x04
#define CD_SCMD_TRAYREQ			0x05
#define CD_SCMD_SCMD			0x0B
#define CD_SCMD_STATUS			0x0C
#define CD_SCMD_BREAK			0x16
#ifdef NEW_ROM_MODULE_VERSION
#define CD_SCMD_CANCELPOWEROFF	0x1F	// XCDVDFSV only
#define CD_SCMD_BLUELEDCTRL		0x20	// XCDVDFSV only
#define CD_SCMD_POWEROFF		0x21	// XCDVDFSV only
#define CD_SCMD_MMODE			0x22	// XCDVDFSV only
#define CD_SCMD_SETTHREADPRI	0x23	// XCDVDFSV only
#endif

s32 bindSCmd = -1;

SifRpcClientData_t clientSCmd __attribute__ ((aligned(64)));	// for s-cmds

s32 sCmdSemaId = -1;		// s-cmd semaphore id

u8 sCmdRecvBuff[48] __attribute__ ((aligned(64)));
u8 sCmdSendBuff[48] __attribute__ ((aligned(64)));

s32 sCmdNum = 0;

extern s32 bindSCmd;
extern SifRpcClientData_t clientSCmd;
extern s32 sCmdSemaId;
extern u8 sCmdREcvBuff[48];
extern u8 sCmdSendBuff[48];
extern s32 sCmdNum;

s32 cdCheckSCmd(s32 cmd);


// **** S-Command Functions ****

int sbcall_cdvdreadrtcStage2(tge_sbcall_rpc_arg_t *carg)
{
	tge_sbcall_cdvdwritertc_arg_t *arg = carg->sbarg;

	memcpy(arg, UNCACHED_SEG(sCmdRecvBuff + 4), 8);

	if (cdDebug > 0)
		printf("Libcdvd call Clock read 2\n");

	carg->result = *(s32 *) UNCACHED_SEG(sCmdRecvBuff);
	carg->endfunc(carg->efarg, carg->result);
}

// read clock value from ps2s clock
// 
// args:        time/date struct
// returns:     1 if successful
//                      0 if error
int sbcall_cdvdreadrtc(tge_sbcall_rpc_arg_t *carg)
{
	tge_sbcall_cdvdwritertc_arg_t *arg = carg->sbarg;
	if (cdCheckSCmd(CD_SCMD_READCLOCK) == 0)
		return -1;

	if (cdDebug > 0)
		printf("Libcdvd call Clock read 1\n");

	if (SifCallRpc(&clientSCmd, CD_SCMD_READCLOCK, SIF_RPC_M_NOWAIT, 0, 0, sCmdRecvBuff, 16, sbcall_cdvdreadrtcStage2, carg) < 0) {
		return -2;
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
	if (cdCheckSCmd(CD_SCMD_WRITECLOCK) == 0)
		return -1;

	memcpy(sCmdSendBuff, arg, 8);
	SifWriteBackDCache(sCmdSendBuff, 8);

	if (SifCallRpc(&clientSCmd, CD_SCMD_WRITECLOCK, SIF_RPC_M_NOWAIT, sCmdSendBuff, 8, sCmdRecvBuff, 16, sbcall_cdvdreadrtcStage2, carg) < 0) {
		return -2;
	}
	return 0;
}

int SCmdStage2(tge_sbcall_rpc_arg_t *carg)
{
	carg->result = *(s32 *) UNCACHED_SEG(sCmdRecvBuff);
	carg->endfunc(carg->efarg, carg->result);
}

// gets the type of the currently inserted disc
// 
// returns:     disk type (CDVD_TYPE_???)
int sbcall_cdvdgettype(tge_sbcall_rpc_arg_t *carg)
{
	if (cdCheckSCmd(CD_SCMD_GETDISCTYPE) == 0)
		return -1;

	if (SifCallRpc(&clientSCmd, CD_SCMD_GETDISCTYPE, SIF_RPC_M_NOWAIT, 0, 0, sCmdRecvBuff, 4, SCmdStage2, carg) < 0) {
		return -2;
	}
	return 0;
}

// gets the last error that occurred
// 
// returns:     error type (CDVD_ERR_???)
int sbcall_cdvdgeterror(tge_sbcall_rpc_arg_t *carg)
{
	if (cdCheckSCmd(CD_SCMD_GETERROR) == 0)
		return -1;

	if (SifCallRpc(&clientSCmd, CD_SCMD_GETERROR, SIF_RPC_M_NOWAIT, 0, 0, sCmdRecvBuff, 4, SCmdStage2, carg) < 0) {
		return -2;
	}
	return 0;
}

void sbcall_cdvdtrayrequestStage2(tge_sbcall_rpc_arg_t *carg)
{
	tge_sbcall_cdvdtrayrequest_arg_t *arg = carg->sbarg;

	arg->traycount = *(u32 *) UNCACHED_SEG(sCmdRecvBuff + 4);
	carg->result = ! (*(s32 *) UNCACHED_SEG(sCmdRecvBuff));

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
	if (cdCheckSCmd(CD_SCMD_TRAYREQ) == 0)
		return -1;

	memcpy(sCmdSendBuff, &arg->request, 4);
	SifWriteBackDCache(sCmdSendBuff, 4);

	if (SifCallRpc(&clientSCmd, CD_SCMD_TRAYREQ, SIF_RPC_M_NOWAIT, sCmdSendBuff, 4, sCmdRecvBuff, 8, sbcall_cdvdtrayrequestStage2, carg) < 0) {
		return -2;
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
	if (cdCheckSCmd(CD_SCMD_SCMD) == 0)
		return 0;

	*(u16 *) & sCmdSendBuff[0] = cmdNum;
	*(u16 *) & sCmdSendBuff[2] = inBuffSize;
	memset(&sCmdSendBuff[4], 0, 16);
	if (inBuff)
		memcpy(&sCmdSendBuff[4], inBuff, inBuffSize);
	SifWriteBackDCache(sCmdSendBuff, 20);

	if (SifCallRpc(&clientSCmd, CD_SCMD_SCMD, 0, sCmdSendBuff, 20, sCmdRecvBuff, 16, 0, 0) < 0) {
		SignalSema(sCmdSemaId);
		return 0;
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
	if (cdCheckSCmd(CD_SCMD_STATUS) == 0)
		return -1;

	if (SifCallRpc(&clientSCmd, CD_SCMD_STATUS, 0, 0, 0, sCmdRecvBuff, 4, 0, 0) < 0) {
		SignalSema(sCmdSemaId);
		return -1;
	}

	if (cdDebug >= 2)
		printf("status called\n");

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
	if (cdCheckSCmd(CD_SCMD_BREAK) == 0)
		return 0;

	if (SifCallRpc(&clientSCmd, CD_SCMD_BREAK, 0, 0, 0, sCmdRecvBuff, 4, 0, 0) < 0) {
		SignalSema(sCmdSemaId);
		return 0;
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
	if (cdCheckSCmd(CD_SCMD_CANCELPOWEROFF) == 0)
		return 0;

	if (SifCallRpc(&clientSCmd, CD_SCMD_CANCELPOWEROFF, 0, 0, 0, sCmdRecvBuff, 8, 0, 0) < 0) {
		SignalSema(sCmdSemaId);
		return 0;
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
	if (cdCheckSCmd(CD_SCMD_BLUELEDCTRL) == 0)
		return 0;

	*(u32 *) sCmdSendBuff = control;
	SifWriteBackDCache(sCmdSendBuff, 4);

	if (SifCallRpc(&clientSCmd, CD_SCMD_BLUELEDCTRL, 0, sCmdSendBuff, 4, sCmdRecvBuff, 8, 0, 0) < 0) {
		SignalSema(sCmdSemaId);
		return 0;
	}

	*result = *(u32 *) UNCACHED_SEG(sCmdRecvBuff + 4);
	SignalSema(sCmdSemaId);
	return *(s32 *) UNCACHED_SEG(sCmdRecvBuff);
}
#endif

// power off
// 
// SUPPORTED IN XCDVDMAN/XCDVDFSV ONLY
// 
// args:        result
// returns:     1 if successful
//                      0 if error
#ifdef F_cdPowerOff
s32 cdPowerOff(u32 * result)
{
	if (cdCheckSCmd(CD_SCMD_POWEROFF) == 0)
		return 0;

	if (SifCallRpc(&clientSCmd, CD_SCMD_POWEROFF, 0, 0, 0, sCmdRecvBuff, 8, 0, 0) < 0) {
		SignalSema(sCmdSemaId);
		return 0;
	}

	*result = *(u32 *) UNCACHED_SEG(sCmdRecvBuff + 4);
	SignalSema(sCmdSemaId);
	return *(s32 *) UNCACHED_SEG(sCmdRecvBuff);
}
#endif

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

	if (cdCheckSCmd(CD_SCMD_MMODE) == 0)
		return -1;

	memcpy(sCmdSendBuff, &arg->media, 4);
	SifWriteBackDCache(sCmdSendBuff, 4);

	if (SifCallRpc(&clientSCmd, CD_SCMD_MMODE, SIF_RPC_M_NOWAIT, sCmdSendBuff, 4, sCmdRecvBuff, 4, SCmdStage2, carg) < 0) {
		return -2;
	}
	return 0;
#else
	/* Function not supported, simulate success. */
	carg->result = 1;
	carg->endfunc(carg->efarg, carg->result);
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
	if (cdCheckSCmd(CD_SCMD_SETTHREADPRI) == 0)
		return 0;

	memcpy(sCmdSendBuff, &priority, 4);
	SifWriteBackDCache(sCmdSendBuff, 4);

	if (SifCallRpc(&clientSCmd, CD_SCMD_SETTHREADPRI, 0, sCmdSendBuff, 4, sCmdRecvBuff, 4, 0, 0) < 0) {
		SignalSema(sCmdSemaId);
		return 0;
	}

	SignalSema(sCmdSemaId);
	return *(s32 *) UNCACHED_SEG(sCmdRecvBuff);
}
#endif

// check whether ready to send an s-command
// 
// args:        current command
// returns:     1 if ready to send
//                      0 if busy/error
s32 cdCheckSCmd(s32 cur_cmd)
{
	s32 i;
	sCmdNum = cur_cmd;
	if (cdSyncS(1)) {
		printf("cdSyncS failed\n");
		return 0;
	}

	if (bindSCmd >= 0) {
		return 1;
	}

	return 0;
}

static void cdSCmdInitStage2(tge_sbcall_rpc_arg_t *carg)
{
	if (clientSCmd.server != 0) {
		bindSCmd = 0;
		carg->result = 0;
	} else {
		carg->result = -10;
	}
	carg->endfunc(carg->efarg, carg->result);
}

void cdSCmdInit(tge_sbcall_rpc_arg_t *carg)
{
	// bind rpc for n-commands
	if (SifBindRpc(&clientSCmd, CD_SERVER_SCMD, SIF_RPC_M_NOWAIT, cdSCmdInitStage2, carg) < 0) {
		if (cdDebug > 0)
			printf("Libcdvd bind err S cmd\n");
		carg->result = -9;
		carg->endfunc(carg->efarg, carg->result);
	}
}

// s-command wait
// (shouldnt really need to call this yourself)
// 
// args:        0 = wait for completion of command (blocking)
//                      1 = check current status and return immediately
// returns:     0 = completed
//                      1 = not completed
s32 cdSyncS(s32 mode)
{
	if (mode == 0) {
		if (cdDebug > 0)
			printf("S cmd wait\n");
		while (SifCheckStatRpc(&clientSCmd))
			;
		return 0;
	}
	return SifCheckStatRpc(&clientSCmd);
}

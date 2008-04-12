#ifndef _SCMD_H_
#define _SCMD_H_
/* Copyright (c) 2007 Mega Man */

/** Trace macro for function call. */
#define CDVD_LOCKS() cdvdLockS(__FILE__, __LINE__)
/** Trace macro for function call. */
#define CDVD_UNLOCKS() cdvdUnlockS(__FILE__, __LINE__)

extern s32 bindSCmd;
int cdvdLockS(const char *file, int line);
void cdvdUnlockS(const char *file, int line);
extern u8 sCmdRecvBuff[];
int cdSCmdInitCallback(tge_sbcall_rpc_arg_t *carg);
int cdSCmdInit(tge_sbcall_rpc_arg_t *carg, SifRpcEndFunc_t endfunc);

#endif

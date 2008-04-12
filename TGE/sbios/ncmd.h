#ifndef _NCMD_H_
#define _NCMD_H_
/* Copyright (c) 2007 Mega Man */

/** Trace macro for function call. */
#define CDVD_LOCKN() cdvdLockN(__FILE__, __LINE__)
/** Trace macro for function call. */
#define CDVD_UNLOCKN() cdvdUnlockN(__FILE__, __LINE__)

extern s32 bindNCmd;

int cdNCmdInitCallback(tge_sbcall_rpc_arg_t *carg);
int cdNCmdInit(tge_sbcall_rpc_arg_t *carg, SifRpcEndFunc_t endfunc);
int cdvdLockN(const char *file, int line);
void cdvdUnlockN(const char *file, int line);

#endif

#ifndef _SCMD_H_
#define _SCMD_H_
/* Copyright (c) 2007 Mega Man */

extern s32 bindSCmd;
s32 cdSyncS(s32 mode);
extern u8 sCmdRecvBuff[];
void cdSCmdInit(tge_sbcall_rpc_arg_t *carg);

#endif

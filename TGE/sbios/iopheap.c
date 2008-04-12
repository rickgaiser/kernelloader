/*      
# _____     ___ ____     ___ ____
#  ____|   |    ____|   |        | |____|
# |     ___|   |____ ___|    ____| |    \    PS2DEV Open Source Project.
#-----------------------------------------------------------------------
# (C)2002, Vzzrzzn
# (c) 2003 Marcus R. Brown (mrbrown@0xd6.org)
# (c) 2007 Mega Man
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.
#
# $Id$
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
#include "sbcalls.h"
#include "ps2lib_err.h"
#include "iopmem.h"

#include "iopheap.h"
#include "stdio.h"

#define IH_C_BOUND	0x0001

extern SifRpcClientData_t _ih_cd;
extern int _ih_caps;

SifRpcClientData_t _ih_cd;
int _ih_caps = 0;

int SifInitIopHeap(SifRpcEndFunc_t endfunc, void *efarg)
{
	int res;

	if (_ih_caps)
		return 0;

	SifInitRpc(0);

	res = SifBindRpc(&_ih_cd, 0x80000003, SIF_RPC_M_NOWAIT, endfunc, efarg);
	if (res != 0) {
		return -E_SIF_RPC_BIND;
	}

	_ih_caps |= IH_C_BOUND;

	return 0;
}

#ifdef F_SifExitIopHeap
void SifExitIopHeap()
{
	_ih_caps = 0;
	memset(&_ih_caps, 0, sizeof _ih_caps);
}
#endif

int SifAllocIopHeap(int size, SifRpcEndFunc_t endfunc, void *efarg, u32 *result)
{
	/* result must not be align until smaller than 8 quadwords. */
	*result = size;

	if (SifCallRpc(&_ih_cd, 1, SIF_RPC_M_NOWAIT, result, 4, result, 4, endfunc, efarg) < 0)
		return -1;

	return 0;
}

int SifFreeIopHeap(void *addr, SifRpcEndFunc_t endfunc, void *efarg, int *result)
{
	/* result must not be align until smaller than 8 quadwords. */
	*result = addr;

	if (SifCallRpc(&_ih_cd, 2, SIF_RPC_M_NOWAIT, result, 4, result, 4, endfunc, efarg) < 0)
		return -E_SIF_RPC_CALL;
	return 0;
}

#ifdef F_SifLoadIopHeap

#define LIH_PATH_MAX	252

struct _iop_load_heap_arg {
	union	{
		void	*addr;
		int	result;
	} p;
	char	path[LIH_PATH_MAX];
};

/* TODO: I think this needs a version check...  */
int SifLoadIopHeap(const char *path, void *addr, SifRpcEndFunc_t endfunc, void *efarg)
{
	struct _iop_load_heap_arg arg;

	arg.p.addr = addr;
	strncpy(arg.path, path, LIH_PATH_MAX - 1);
	arg.path[LIH_PATH_MAX - 1] = 0;

	if (SifCallRpc(&_ih_cd, 3, 0, &arg, sizeof arg, &arg, 4, endfunc, efarg) < 0)
		return -E_SIF_RPC_CALL;

	return arg.p.result;
}
#endif


int sbcall_iopheapinit(tge_sbcall_rpc_arg_t *carg)
{
	/* XXX: Assumption that no error will happen, set result to 0. */
	carg->result = 0;
	return SifInitIopHeap(carg->endfunc, carg->efarg);
}

int sbcall_iopaheapalloc(tge_sbcall_rpc_arg_t *carg)
{
	tge_sbcall_iopheapalloc_arg_t *arg = carg->sbarg;
	return SifAllocIopHeap(arg->size, carg->endfunc, carg->efarg, &carg->result);
}

int sbcall_iopheapfree(tge_sbcall_rpc_arg_t *carg)
{
	tge_sbcall_iopheapfree_arg_t *arg = carg->sbarg;
	return SifFreeIopHeap(arg->iopaddr, carg->endfunc, carg->efarg, &carg->result);
}

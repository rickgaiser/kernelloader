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
#include "iopmemdebug.h"

#include "iopheap.h"
#include "stdio.h"
#include "mutex.h"

#define IH_C_BOUND	0x0001

typedef union {
	uint32_t size;
	uint32_t iopaddr;
} iopheap_pkt_t;

extern SifRpcClientData_t _ih_cd;

SifRpcClientData_t _ih_cd __attribute__ ((aligned(64)));
int _ih_caps = 0;
static sbios_mutex_t iopHeapMutex = SBIOS_MUTEX_INIT;

/** Receive buffer requires only 48, use 64 because of cache aliasing problems.*/
static u8 iopheap_buffer[64] __attribute__ ((aligned(64)));
iopheap_pkt_t *iopheap_pkt = NULL;

void iopHeapInitCallback(void *rarg)
{
	tge_sbcall_rpc_arg_t *carg = (tge_sbcall_rpc_arg_t *) rarg;

	if (_ih_cd.server != 0) {
		carg->result = 0;
		iopheap_pkt = (iopheap_pkt_t *) iopheap_buffer;
		_ih_caps |= IH_C_BOUND;
	} else {
		carg->result = -1;
	}
	sbios_unlock(&iopHeapMutex);
	carg->endfunc(carg->efarg, carg->result);
}

int sbcall_iopheapinit(tge_sbcall_rpc_arg_t *carg)
{
	int res;

	if (_ih_caps) {
		carg->result = 0;
		carg->endfunc(carg->efarg, carg->result);
		return 0;
	}

	SifInitRpc();

	if (sbios_tryLock(&iopHeapMutex)) {
		printf("sbcall_iopheapinit: Semaphore lock failed.\n");
		/* Temporary not available. */
		return -2;
	}

	res = SifBindRpc(&_ih_cd, 0x80000003, SIF_RPC_M_NOWAIT, iopHeapInitCallback, carg);
	if (res != 0) {
		sbios_unlock(&iopHeapMutex);
		return -SIF_RPCE_SENDP;
	}

	return 0;
}

#ifdef F_SifExitIopHeap
void SifExitIopHeap()
{
	_ih_caps = 0;
	memset(&_ih_caps, 0, sizeof _ih_caps);
}
#endif

void iopHeapCallback(void *rarg)
{
	tge_sbcall_rpc_arg_t *carg = (tge_sbcall_rpc_arg_t *) rarg;
	carg->result = iopheap_pkt->iopaddr;
	sbios_unlock(&iopHeapMutex);
	carg->endfunc(carg->efarg, carg->result);
}

int sbcall_iopaheapalloc(tge_sbcall_rpc_arg_t *carg)
{
	tge_sbcall_iopheapalloc_arg_t *arg = carg->sbarg;

	if (!(_ih_caps & IH_C_BOUND)) {
		printf("sbcall_iopaheapalloc: not initialized.\n");
		/* not intialized. */
		return -1;
	}

	if (sbios_tryLock(&iopHeapMutex)) {
		printf("sbcall_iopaheapalloc: lock failed\n");
		/* Temporary not available. */
		return -2;
	}

	/* result must not be align until smaller than 8 quadwords. */
	iopheap_pkt->size = arg->size;

	if (SifCallRpc(&_ih_cd, 1, SIF_RPC_M_NOWAIT, iopheap_pkt, 4, iopheap_pkt, 4, iopHeapCallback, carg) < 0) {
		sbios_unlock(&iopHeapMutex);
		return -SIF_RPCE_SENDP;
	}

	return 0;
}

int sbcall_iopheapfree(tge_sbcall_rpc_arg_t *carg)
{
	tge_sbcall_iopheapfree_arg_t *arg = carg->sbarg;

	if (!(_ih_caps & IH_C_BOUND)) {
		printf("sbcall_iopaheapalloc: not initialized.\n");
		/* not intialized. */
		return -1;
	}

	if (sbios_tryLock(&iopHeapMutex)) {
		printf("sbcall_iopheapfree: lock failed\n");
		/* Temporary not available. */
		return -2;
	}

	/* result must not be align until smaller than 8 quadwords. */
	iopheap_pkt->iopaddr = (int) arg->iopaddr;

	if (SifCallRpc(&_ih_cd, 2, SIF_RPC_M_NOWAIT, iopheap_pkt, 4, iopheap_pkt, 4, iopHeapCallback, carg) < 0) {
		sbios_unlock(&iopHeapMutex);
		return -SIF_RPCE_SENDP;
	}
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

	if (SifCallRpc(&_ih_cd, 3, 0, &arg, sizeof arg, &arg, 4, endfunc, efarg) < 0) {
		return -SIF_RPCE_SENDP;
	}

	return arg.p.result;
}
#endif



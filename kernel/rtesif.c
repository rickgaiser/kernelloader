/*
 * rtesif.c - RTE SIF support code.
 *
 * Copyright (c) 2003-2004 Marcus R. Brown <mrbrown@0xd6.org>
 * Copyright (c) 2007 Mega Man
 *
 * Licensed under the AFL v2.0. See the file LICENSE included with this
 * distribution for licensing terms.
 */

#include "memory.h"
#include "rtesif.h"
#include "stdint.h"
#include "sbcall.h"
#include "stdio.h"
#include "iopmem.h"
#include "sifrpc.h"
#include "cache.h"
#include "dmac.h"
#include "kernel.h"

#define SBIOS_BASE	0x80001000
#define SBIOS_MAGIC	0x80001004
#define SBIOS_MAGICVAL	0x62325350
#define D5_CHCR 0x1000C000
#define D5_MADR 0x1000C010
#define D5_QWC 0x1000C020
#define D5_TADR 0x1000C030
#define D6_CHCR 0x1000C400
#define D6_MADR 0x1000C410
#define D6_QWC 0x1000C420
#define D6_TADR 0x1000C430

static int (*_sbios)(int, void *) = NULL;

/*
 * Sony "nop'd" a bunch of undocumented (w.r.t the Linux kernel source) SBIOS
 * calls related to the SIF.  They can be called directly under the Beta kit
 * (version 0x200) but require a direct pointer in the 1.0 kit.  They will
 * not work under any other RTE (e.g. the BBN).
 */
static int sbversion = 0;

static inline int sbios(int func, void *arg)
{
	if (!_sbios) {
		volatile uint32_t *sbios_magic;

		sbios_magic = SBIOS_MAGIC;
		if (*sbios_magic == SBIOS_MAGICVAL)
			_sbios = *(int (**)(int, void *))(SBIOS_BASE);
		else
			return -1;
	}
	return _sbios(func, arg);
}

#ifdef RTE
static int (*_sb_sif_reg_set)(int, int) = NULL;
#endif

int sif_reg_set(int reg, int val)
{
	struct setreg_arg {
		int	reg;
		int	val;
	} arg;

#ifdef RTE
	if (sbversion <= 0x200) {
#endif
		arg.reg = reg;
		arg.val = val;
		/* sceSifSetReg() */
		return (int)_sbios(21, &arg);
#ifdef RTE
	} else if (sbversion <= 0x250) {
		(int (**)(int, int))_sb_sif_reg_set = 0x80002700;
		return _sb_sif_reg_set(reg, val);
	}
#endif

	return 0;
}

#ifdef RTE
static int (*_sb_sif_reg_get)(int) = NULL;
#endif

int sif_reg_get(int reg)
{
#ifdef RTE
	if (sbversion <= 0x200) {
#endif
		/* sceSifGetReg() */
		return (int)_sbios(22, &reg);
#ifdef RTE
	} else if (sbversion <= 0x250) {
		(int (**)(int))_sb_sif_reg_get = 0x800027c0;
		return _sb_sif_reg_get(reg);
	}

	return 0;
#endif
}

int sif_dma_request(void *dmareq, int count)
{
	struct setdma_arg {
		void	*dmareq;
		int	count;
	} arg;

	arg.dmareq = dmareq;
	arg.count  = count;

	/* sceSifSetDma() */
	return _sbios(18, &arg);
}

void dmac5_handler(uint32_t *regs)
{
	volatile uint32_t *d5_chcr = KSEG1ADDR(D5_CHCR);
	volatile uint32_t *d5_qwc = KSEG1ADDR(D5_QWC);
	volatile uint32_t *d5_madr = KSEG1ADDR(D5_MADR);

	DBG("Got DMAC5 interrupt (SIF0: EE <- IOP transfer).\n");
#if 0
	printf("D5_CHCR 0x%x\n", *d5_chcr);
	printf("D5_MADR 0x%x\n", *d5_madr);
	printf("D5_QWC  0x%x\n", *d5_qwc);
#endif
}

void dmac6_handler(uint32_t *regs)
{
	volatile uint32_t *d6_chcr = KSEG1ADDR(D6_CHCR);
	volatile uint32_t *d6_madr = KSEG1ADDR(D6_MADR);
	volatile uint32_t *d6_qwc = KSEG1ADDR(D6_QWC);
	volatile uint32_t *d6_tadr = KSEG1ADDR(D6_TADR);

	DBG("Got DMAC6 interrupt (SIF1: EE -> IOP transfer).\n");
#if 0
	printf("D6_CHCR 0x%x\n", *d6_chcr);
	printf("D6_MADR 0x%x\n", *d6_madr);
	printf("D6_QWC  0x%x\n", *d6_qwc);
	printf("D6_TADR 0x%x\n", *d6_tadr);
#endif
}

void sif_init(void)
{
	sbversion = sbios(0, NULL);

	/* Call the SBIOS SIF init routine */
	_sbios(16, NULL);

	/* sif0 dma handler */
	dmac_register_handler(DMAC_CIS5, dmac5_handler);
	dmac_enable_irq(DMAC_CIS5);

	/* sif1 dma handler */
	dmac_register_handler(DMAC_CIS6, dmac6_handler);
	dmac_enable_irq(DMAC_CIS6);
}

void sif_set_dchain(void)
{
	/* Call the SBIOS SIF init routine */
	_sbios(SB_SIFSETDCHAIN, NULL);
}

static void rpc_wakeup(void *p, int result)
{
	printf("rpc_wakeup() called.\n");
}


void SifInitRpc(int mode)
{
	struct sbr_common_arg carg;

	_sbios(SB_SIFINITRPC, 0);
	carg.arg = NULL;
	carg.func = rpc_wakeup;
	carg.para = NULL;
	_sbios(SBR_IOPH_INIT, &carg);
#if 0
	while (1) {
		err = sbios_rpc(SBR_IOPH_INIT, NULL, &result);
		if (err < 0)
			return;
		if (result == 0)
			break;
		i = 0x100000;
		while (i--)
		;
	}
	return;
#endif
}

static void rpcendNotify(void *arg)
{
	printf("rpcendNotify %d\n", (uint32_t) arg);
}

int SifBindRpc(SifRpcClientData_t *client, int rpc_number, int mode)
{
	struct sb_sifbindrpc_arg arg;

	arg.bd = client;
	arg.command = rpc_number;
	arg.mode = mode;
	arg.func = rpcendNotify;
	arg.para = rpc_number;
	return sbios(SB_SIFBINDRPC, &arg);
}

int SifCallRpc(SifRpcClientData_t *client, int rpc_number, int mode,
		void *send, int ssize, void *receive, int rsize,
		SifRpcEndFunc_t end_function, void *end_param)
{
	struct sb_sifcallrpc_arg arg;

	arg.bd = client;
	arg.fno = rpc_number;
	arg.mode = mode;
	arg.send = send;
	arg.ssize = ssize;
	arg.receive = receive;
	arg.rsize = rsize;
	arg.func = rpcendNotify;
	arg.para = rpc_number;

	return sbios(SB_SIFCALLRPC, &arg);
}

int SifCheckStatRpc(SifRpcClientData_t *cd)
{
	struct sb_sifcheckstatrpc_arg arg;
	arg.cd = cd;
	return sbios(SB_SIFCHECKSTATRPC, &arg);
}

void setdve(int mode)
{
	struct sb_setdve_arg arg;
	arg.mode = mode;
	sbios(SB_SETDVE, &arg);
}

void syscallSifSetDChain(void)
{
#if 0 /* already done in sif_init() */
	volatile uint32_t *d5_chcr = KSEG1ADDR(D5_CHCR);
	volatile uint32_t *d5_qwc = KSEG1ADDR(D5_QWC);

	*d5_chcr = 0;
	*d5_qwc = 0;
	*d5_chcr = 0x184;
	*d5_chcr;
#else
	DBG("syscallSifSetDChain()\n");
	sif_set_dchain();
#endif
}

int syscallSifGetReg(uint32_t register_num)
{
	DBG("syscallSifGetReg(0x%x)\n", register_num);
	return sif_reg_get(register_num);
}

int syscallSifSetReg(uint32_t register_num, uint32_t register_value)
{
	DBG("syscallSifSetReg(0x%x, 0x%x)\n", register_num, register_value);
	return sif_reg_set(register_num, register_value);
}

uint32_t syscallSifSetDma(void *sdd, int32_t len)
{
	flushDCacheAll();
	DBG("syscallSifSetDma(0x%x, %d)\n", sdd, len);
	return sif_dma_request(KSEG0ADDR(sdd), len);
}

/*
 * sifdma.c - Low-level SIF and SIF DMA
 *
 * Copyright (c) 2003 Marcus R. Brown <mrbrown@0xd6.org>
 *
 * See the file LICENSE, located within this directory, for licensing terms.
 */

#include "tge_types.h"
#include "tge_defs.h"

#include "tge_sbios.h"
#include "tge_sifdma.h"
#include "sifdma.h"
#include "iopmemdebug.h"

#include "sbcalls.h"
#include "sif.h"
#include "hwreg.h"
#include "core.h"

static int initialized;

static u32 sif_reg_table[32] __attribute__((aligned(64)));
/** Next/current dma tag that is used for transfer by software. */
static u8 sif1_dmatag_index;
static u16 sif1_dma_count;
/** Ring buffer of DMA tags. Last tag is for wrap around and can't be used for
 * anything else.
 * Must be cache line size aligned to prevent cache aliasing effects.
 */
static ee_dmatag_t sif1_dmatags[32] __attribute__((aligned(64)));

/* Describes the tag accepted by the IOP on SIF1 DMA transfers.  */
typedef struct {
	u32	addr;
	u32	size;
	u64	pad64;
	u128	data[7];	/* 7 qwords of data.  */
} iop_dmatag_t ALIGNED(16);

static iop_dmatag_t iop_dmatags[32] ALIGNED(64);

static void sif_dma_init(void);

/* Initialization. Functions is same as in RTE with one exception. */
int sbcall_sifinit()
{
	int i;

	if (initialized)
		return 0;

	initialized = 1;
#if 0
	/* Unknown code in RTE: */
	for (i = 0; i < 32; i ++) {
		*KSEG1ADDR(&sif_init_unknown_dma_table_8x4words[i]) = 0;
	}
#endif

	for (i = 0; i < 32; i++) {
		sif_reg_table[i] = 0;
	}

	_sw(0xff, EE_SIF_UNKNF260);

	_sw(0, EE_DMAC_SIF0_CHCR);
	_sw(0, EE_DMAC_SIF1_CHCR);

	sbcall_sifsetdchain();
	sif_dma_init();

	return 0;
}

int sbcall_sifexit()
{
	initialized = 0;
	return 0;
}

/** Function is same in RTE. Reenable DMAC channel. */
int sbcall_sifsetdchain()
{
	u32 status;

	core_save_disable(&status);

	/* Stop DMA.  */
	_sw(0, EE_DMAC_SIF0_CHCR);
	_sw(0, EE_DMAC_SIF0_QWC);

	/* Chain mode; enable tag interrupt; start DMA.  */
	/*
	 * dir: 0 -> to _memory
	 * mod: 01 -> chain.
	 * asp: 00 -> no address pushed by call tag
	 * tte: 0 -> does not transfer DMAtag itself
	 * tie: 1 -> enables IRQ bit of DMAtag
	 * str: 1 -> Starts DMA. Maintains 1 while operating.
	 */
	_sw(0x184, EE_DMAC_SIF0_CHCR);
	_lw(EE_DMAC_SIF0_CHCR);

	core_restore(status);
	return 0;
}

int sbcall_sifstopdma()
{
	_sw(0, EE_DMAC_SIF0_CHCR);
	_sw(0, EE_DMAC_SIF0_QWC);
	_lw(EE_DMAC_SIF0_QWC);
	return 0;
}

/* Function is same as RTE. */
static void sif_dma_init()
{
	ee_dmatag_t *tag;

	/* Will be increased by first dma transfer to 0. */
	sif1_dma_count = 0xffff;
	/* First transfer will begin with dma tag index 0. */
	sif1_dmatag_index = 30;

	/* This causes the DMAC to loop to the first tag when it hits this tag;
	   it's never touched again by us.  */
	tag = KSEG1ADDR(&sif1_dmatags[31]);
	tag->id_qwc = EE_DMATAG_ID(EE_DMATAG_ID_NEXT);
	/* Pointer to next (first) DMAtag. */
	tag->addr = PHYSADDR(&sif1_dmatags);
	/* Set number of quad words to copy to 0. */
	_sw(0, EE_DMAC_SIF1_QWC);
	/* Set as start DMAtag. */
	_sw(PHYSADDR(&sif1_dmatags), EE_DMAC_SIF1_TADR);
#if 0
	iop_prints("sif1_dmatag 0x");
	iop_printx(&sif1_dmatags);
	iop_prints("\n");
#endif
}

/** Advance to next dma tag. dma tag 31 cannot be used. Organized as ring
 * buffer.
 */
#define SET_NEXT_DMATAG()			\
	{					\
		sif1_dmatag_index++;		\
		if (sif1_dmatag_index == 31) {	\
			sif1_dmatag_index = 0;	\
			sif1_dma_count++;	\
		}				\
	}

/**
 * Transfer data from EE to IOP by using SIF1 DMA channel 6.
 * @param src Data to be transfered (located in EE memory).
 * @param dest Where to copy the data in IOP memory.
 * @param size Size in bytes to transfer.
 * @param attr Flags:
 *   TGE_SIFDMA_ATTR_INT_I to enable interrupt in last DMAtag.
 *   TGE_SIFDMA_ATTR_DMATAG Transfer IOP DMAtag without any data.
 *   TGE_SIFDMA_ATTR_INT_O ???
 *   TGE_SIFDMA_ATTR_ERT ???
 * @param id DMAtag id used for last DMAtag (should be EE_DMATAG_ID_REF or to
 * stopt after, use EE_DMATAG_ID_REFE)
 */
static void sif_dma_transfer(void *src, void *dest, u32 size, u32 attr, ee_dmatag_id_t id)
{
	ee_dmatag_t *tag;
	iop_dmatag_t *dtag;
	u128 *s128, *d128;
	u32 data_qwc, i, qwc = (size + 15) / 16;

	/* Get address of next dma tag. */
	SET_NEXT_DMATAG();
	tag = KSEG1ADDR(&sif1_dmatags[sif1_dmatag_index]);

	if (attr & TGE_SIFDMA_ATTR_DMATAG) {
		tag->id_qwc = EE_DMATAG_ID_QWC(id, qwc) |
			((attr & TGE_SIFDMA_ATTR_INT_I) ? EE_DMATAG_IRQ : 0);
		dtag = KSEG1ADDR(&iop_dmatags[sif1_dmatag_index]);
		tag->addr = PHYSADDR(dtag);

		qwc--;
	} else {
		if (qwc >= 8) {
			/* Split the data if it's more than 7 qwords.  */
			tag->id_qwc = EE_DMATAG_ID_QWC(EE_DMATAG_ID_REF, 8);
			dtag = KSEG1ADDR(&iop_dmatags[sif1_dmatag_index]);
			tag->addr = PHYSADDR(dtag);
			data_qwc = 7;

			/* Get address of next dma tag. */
			SET_NEXT_DMATAG();
			tag = KSEG1ADDR(&sif1_dmatags[sif1_dmatag_index]);
			tag->id_qwc = EE_DMATAG_ID_QWC(id, qwc - 7) |
				((attr & TGE_SIFDMA_ATTR_INT_I) ? EE_DMATAG_IRQ : 0);
			tag->addr = PHYSADDR((u32)src + 112);
		} else {
			tag->id_qwc = EE_DMATAG_ID_QWC(id, qwc + 1) |
				((attr & TGE_SIFDMA_ATTR_INT_I) ? EE_DMATAG_IRQ : 0);
			dtag = KSEG1ADDR(&iop_dmatags[sif1_dmatag_index]);
			tag->addr = PHYSADDR(dtag);
			data_qwc = qwc;
		}

		/* Copy the source data into the destination tag.  */
		for (i = 0, s128 = KSEG1ADDR(src), d128 = dtag->data; i < data_qwc; i++)
			*d128++ = *s128++;
	}

	dtag->size = qwc * 4;
	dtag->addr = ((u32)dest & 0x00ffffff) |
		((attr & TGE_SIFDMA_ATTR_INT_O) ? 0x40000000 : 0) |
		((attr & TGE_SIFDMA_ATTR_ERT) ? 0x80000000 : 0);
}

static u32 sif_get_number_of_free_dma_tags()
{
	u32 hw_index;

	/* Get index of current dma tag used by hardware. */
	hw_index = (_lw(EE_DMAC_SIF1_TADR) - PHYSADDR(&sif1_dmatags)) / sizeof(ee_dmatag_t);
	/* Calculate index of previous dma tag. We have 32 dma tags. The last dma
	 * tag cannort be used, because it is just a jump command to the beginning
	 * of the ring buffer. So index is 0..30.
	 * Index 30 is the last usable dma tag before 0.
	 */
	hw_index = hw_index > 0 ? hw_index - 1 : 30;

	/* Calculate number of free dma tags. */
	if (hw_index == sif1_dmatag_index)
		return _lw(EE_DMAC_SIF1_QWC) ? 30 : 31;

	if (hw_index < sif1_dmatag_index)
		return hw_index - sif1_dmatag_index + 30;

	return hw_index - sif1_dmatag_index - 1;
}

static void sif_dma_setup_tag(u32 freetags)
{
	ee_dmatag_t *tag;

	if (freetags == 31) {
		/* DMA ring buffer queue is empty. */
		return;
	}
	/* DMA ring buffer is not empty. */

	/* Get last dma tag of previous dma transfer. */
	tag = KSEG1ADDR(&sif1_dmatags[sif1_dmatag_index]);

	if (freetags == 30) {
		/* DMA ring buffer has one entry, which is currently processed by
		 * hardware.
		 */

		/* Rebuild dma tag by dma registers. */
		/* Keep the previous IRQ and PCE bits.  */
		tag->id_qwc &= 0x8c000000;
		/* Don't stop DMA transfer after last DMA tag. */
		tag->id_qwc |= EE_DMATAG_ID(EE_DMATAG_ID_REF);
		tag->id_qwc |= _lw(EE_DMAC_SIF1_QWC);
		tag->addr = _lw(EE_DMAC_SIF1_MADR);

		/* Restart/reload this dma tag. */
		_sw(0, EE_DMAC_SIF1_QWC);
		_sw(PHYSADDR(tag), EE_DMAC_SIF1_TADR);
	} else {
		/* DMA ring buffer has more than one entry. */
		/* Don't stop DMA transfer after last DMA tag. */
		tag->id_qwc |= EE_DMATAG_ID(EE_DMATAG_ID_REF);
	}
}

/**
 * Transfer data from EE to IOP memory.
 * @param transfer Pointer array of sturctures describing the transfers.
 * @param tcount Number of transfers to do.
 * @return ID, describing the DMA transfers.
 * @retval 0 on error, no DMA tag available (-> retry later).
 */
u32 SifSetDma(tge_sifdma_transfer_t *transfer, s32 tcount)
{
	tge_sifdma_transfer_t *t;
	u32 status, freetags, i, ntags, start, count;
	int id = 0;

#if 0
	iop_prints("SifSetDma() called with command 0x");
	iop_printx(((u32 *) transfer[tcount - 1].src)[2]);
	iop_prints("\n");
#endif

	core_save_disable(&status);

	/* Suspend all DMA to stop any active SIF1 transfers.  */
	_sw(_lw(EE_DMAC_ENABLER) | EE_DMAC_CPND, EE_DMAC_ENABLEW);
	/* Stop SIF1 DMA channel. */
	_sw(0, EE_DMAC_SIF1_CHCR);
	_lw(EE_DMAC_SIF1_CHCR);
	_sw(_lw(EE_DMAC_ENABLER) & ~EE_DMAC_CPND, EE_DMAC_ENABLEW);

	freetags = sif_get_number_of_free_dma_tags();

	/* Find out how many tags we'll need for the transfer.  */
	for (i = tcount, t = transfer, ntags = 0; i; i--, t++) {
		if ((t->attr & TGE_SIFDMA_ATTR_DMATAG) || t->size <= 112)
			ntags++;
		else
			ntags += 2;
	}

	/* We can only transfer if we have enough tags free.  */
	if (ntags <= freetags) {
		/* Same as RTE until this line 0x800022d8. */
		/* XXX: (((sif1_dmatag_index + 1) % 31) & 0xff) + 16 * (sif1_dmatag_index + 1); */
		start = ((sif1_dmatag_index + 1) % 31) & 0xff;
		count = start ? sif1_dma_count : sif1_dma_count + 1;
		id = (count << 16) | (start << 8) | (ntags & 0xff);

		sif_dma_setup_tag(freetags);

		for (i = tcount - 1, t = transfer; i; i--, t++)
			sif_dma_transfer(t->src, t->dest, t->size, t->attr, EE_DMATAG_ID_REF);

		/* DMA transfer will stop after last dma transfer. */
		sif_dma_transfer(t->src, t->dest, t->size, t->attr, EE_DMATAG_ID_REFE);
	}

	/* Start the DMA transfer.  */
	_sw(_lw(EE_DMAC_SIF1_CHCR) | 0x184, EE_DMAC_SIF1_CHCR);
	_lw(EE_DMAC_SIF1_CHCR);

	core_restore(status);
	return id;
}

u32 iSifSetDma(tge_sifdma_transfer_t *sdd, s32 len)
{
	return SifSetDma(sdd, len);
}

/* XXX: Figure out the logic of this routine and finish it.  */
int sif_dma_stat(int id)
{
	u32 status, index, ntags, starttag, count, lasttag, dma_count;

	if (!(_lw(EE_DMAC_SIF1_CHCR) & 0x100))
		return -1;

	core_save_disable(&status);

	ntags = id & 0xff;
	starttag = (id >> 8) & 0xff;
	lasttag = (starttag + ntags - 1) % 31;
	count = lasttag ? id >> 16 : (id >> 16) + 1;

	index = (_lw(EE_DMAC_SIF1_TADR) - PHYSADDR(&sif1_dmatags)) / sizeof(ee_dmatag_t);
	index = index > 0 ? index - 1 : 30;

	if (sif1_dmatag_index < index)
		dma_count = sif1_dma_count - 1;
	else
		dma_count = sif1_dma_count;

	if ((dma_count == (((u32) id) >> 16)) || dma_count == count) {
		if (index >= starttag && index <= lasttag)
			return 0;
	}

	core_restore(status);
	return -1;
}

u32 sif_dma_get_hw_index(void)
{
	u32 index;

	index = (_lw(EE_DMAC_SIF1_TADR) - PHYSADDR(&sif1_dmatags)) / sizeof(ee_dmatag_t);
	index = index > 0 ? index - 1 : 30;

	return index;
}

u32 sif_dma_get_sw_index(void)
{
	return sif1_dmatag_index;
}

/* Function is same as RTE. */
int sbcall_sifsetdma(tge_sbcall_sifsetdma_arg_t *arg)
{
	return SifSetDma(arg->transfer, arg->tcount);
}

int sbcall_sifdmastat(tge_sbcall_sifdmastat_arg_t *arg)
{
	return sif_dma_stat(arg->id);
}

/* Register manipulation.  */
static u32 sif_get_msflag()
{
	u32 val;

	do {
		val = _lw(EE_SIF_MSFLAG);

		/* If the IOP has updated this register, wait for it.  */
		asm ("nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\n" \
		     "nop\nnop\nnop\nnop\nnop\nnop\nnop");
	} while (val != _lw(EE_SIF_MSFLAG));

	return val;
}

static u32 sif_set_msflag(u32 val)
{
	_sw(val, EE_SIF_MSFLAG);
	return sif_get_msflag();
}

static u32 sif_get_smflag()
{
	u32 val;

	do {
		val = _lw(EE_SIF_SMFLAG);

		/* If the IOP has updated this register, wait for it.  */
		asm ("nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\n" \
		     "nop\nnop\nnop\nnop\nnop\nnop\nnop");
	} while (val != _lw(EE_SIF_SMFLAG));

	return val;
}

static u32 sif_set_smflag(u32 val)
{
	_sw(val, EE_SIF_SMFLAG);
	return sif_get_smflag();
}

u32 SifSetReg(u32 reg, u32 val)
{
	switch (reg) {
		case 1:
			_sw(val, EE_SIF_MAINADDR);
			return _lw(EE_SIF_MAINADDR);
		case 3:
			return sif_set_msflag(val);
		case 4:
			return sif_set_smflag(val);
	}

	/* Is bit 31 set? */
	if ((int)reg < 0) {
		reg &= 0x7fffffff;
		if (reg < 32)
			sif_reg_table[reg] = val;
	}

	return 0;
}

u32 SifGetReg(u32 reg)
{
	switch (reg) {
		case 1:
			return _lw(EE_SIF_MAINADDR);
		case 2:
			return _lw(EE_SIF_SUBADDR);
		case 3:
			return sif_get_msflag();
		case 4:
			return sif_get_smflag();
	}

	/* Is bit 31 set? */
	if ((int)reg < 0) {
		reg &= 0x7fffffff;
		if (reg < 32)
			return sif_reg_table[reg];
	}

	return 0;
}

u32 sbcall_sifsetreg(tge_sbcall_sifsetreg_arg_t *arg)
{
	return SifSetReg(arg->reg, arg->val);
}

u32 sbcall_sifgetreg(tge_sbcall_sifgetreg_arg_t *arg)
{
	return SifGetReg(arg->reg);
}

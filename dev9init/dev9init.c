/*
# _____     ___ ____     ___ ____
#  ____|   |    ____|   |        | |____|
# |     ___|   |____ ___|    ____| |    \    PS2DEV Open Source Project.
#-----------------------------------------------------------------------
# Copyright (c) 2003 Marcus R. Brown <mrbrown@0xd6.org>
# Copyright (c) 2009 Mega Man
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.
#
# $Id$
# DEV9 Device Initialisation for PS2 Linux.
*/

#include "types.h"
#include "defs.h"
#include "intrman.h"
#include "dmacman.h"
#include "stdio.h"

#include "dev9regs.h"
#include "speedregs.h"
#include "smapregs.h"
#include "dev9init.h"
#include "hdd.h"
#include "thbase.h"
#include "loadcore.h"
#include "sysclib.h"

IRX_ID(MODNAME, 1, 1);

#define DEV9_INTR		13

/* SSBUS registers.  */
#define SSBUS_R_1418		0xbf801418
#define SSBUS_R_141c		0xbf80141c
#define SSBUS_R_1420		0xbf801420

//#define DEV9_R_POWER DEV9_R_146C

static int dev9type = -1;	/* 0 for PCMCIA, 1 for expansion bay */
static int using_aif = 0;	/* 1 if using AIF on a T10K */

static int pcic_cardtype;	/* Translated value of bits 0-1 of 0xbf801462 */
static int pcic_voltage;	/* Translated value of bits 2-3 of 0xbf801462 */

static s16 eeprom_data[5];	/* 2-byte EEPROM status (0/-1 = invalid, 1 = valid),
				   6-byte MAC address,
				   2-byte MAC address checksum.  */

static void smap_set_stat(int stat);
static int read_eeprom_data(void);

static int smap_device_probe(void);
static int smap_device_reset(void);
static int smap_subsys_init(void);
static int smap_device_init(void);

static void pcmcia_set_stat(int stat);
static int pcic_ssbus_mode(int voltage);
static int pcmcia_device_probe(void);
static int pcmcia_device_reset(void);
static int card_find_manfid(u32 manfid);
static int pcmcia_init(void);

static void expbay_set_stat(int stat);
static int expbay_device_probe(void);
static int expbay_device_reset(void);
static int expbay_init(void);

int _start(int argc, char **argv)
{
	USE_DEV9_REGS;
	int res = 1;
	u16 dev9hw;

	(void) argc;
	(void) argv;

	M_PRINTF("dev9init\n");

	dev9hw = DEV9_REG(DEV9_R_REV) & 0xf0;
	M_PRINTF("dev9 hw 0x%02x\n", dev9hw);
	if (dev9hw == 0x20) {		/* CXD9566 (PCMCIA) */
		dev9type = 0;
		res = pcmcia_init();
	} else if (dev9hw == 0x30) {	/* CXD9611 (Expansion Bay) */
		dev9type = 1;
		res = expbay_init();
	}

	if (res)
		return -1;
	
	/* Normal termination.  */
	M_PRINTF("Dev9 initialized.\n");

	/* Activate network */
	DEV9_REG(DEV9_R_1464) = 3;

	M_PRINTF("SMAP activated.\n");

	ata_setup();

	return pcic_cardtype;
}

static void smap_set_stat(int stat)
{
	if (dev9type == 0)
		pcmcia_set_stat(stat);
	else if (dev9type == 1)
		expbay_set_stat(stat);
}

static int smap_device_probe()
{
	if (dev9type == 0)
		return pcmcia_device_probe();
	else if (dev9type == 1)
		return expbay_device_probe();

	return -1;
}

static int smap_device_reset()
{
	if (dev9type == 0)
		return pcmcia_device_reset();
	else if (dev9type == 1)
		return expbay_device_reset();

	return -1;
}

void dev9IntrEnable(int mask)
{
	USE_SPD_REGS;
	int flags;

	CpuSuspendIntr(&flags);
	SPD_REG16(SPD_R_INTR_MASK) = SPD_REG16(SPD_R_INTR_MASK) | mask;
	CpuResumeIntr(flags);
}

void dev9IntrDisable(int mask)
{
	USE_SPD_REGS;
	int flags;

	CpuSuspendIntr(&flags);
	SPD_REG16(SPD_R_INTR_MASK) = SPD_REG16(SPD_R_INTR_MASK) & ~mask;
	CpuResumeIntr(flags);
}

static int read_eeprom_data()
{
	USE_SPD_REGS;
	int i, j, res = -2;
	u8 val;

	if (eeprom_data[0] < 0)
		goto out;

	SPD_REG8(SPD_R_PIO_DIR)  = 0xe1;
	DelayThread(1);
	SPD_REG8(SPD_R_PIO_DATA) = 0x80;
	DelayThread(1);

	for (i = 0; i < 2; i++) {
		SPD_REG8(SPD_R_PIO_DATA) = 0xa0;
		DelayThread(1);
		SPD_REG8(SPD_R_PIO_DATA) = 0xe0;
		DelayThread(1);
	}
	for (i = 0; i < 7; i++) {
		SPD_REG8(SPD_R_PIO_DATA) = 0x80;
		DelayThread(1);
		SPD_REG8(SPD_R_PIO_DATA) = 0xc0;
		DelayThread(1);
	}
	SPD_REG8(SPD_R_PIO_DATA) = 0xc0;
	DelayThread(1);

	val = SPD_REG8(SPD_R_PIO_DATA);
	DelayThread(1);
	if (val & 0x10) {	/* Error.  */
		SPD_REG8(SPD_R_PIO_DATA) = 0;
		DelayThread(1);
		res = -1;
		eeprom_data[0] = 0;
		goto out;
	}

	SPD_REG8(SPD_R_PIO_DATA) = 0x80;
	DelayThread(1);

	/* Read the MAC address and checksum from the EEPROM.  */
	for (i = 0; i < 4; i++) {
		eeprom_data[i+1] = 0;

		for (j = 15; j >= 0; j--) {
			SPD_REG8(SPD_R_PIO_DATA) = 0xc0;
			DelayThread(1);
			val = SPD_REG8(SPD_R_PIO_DATA);
			if (val & 0x10)
				eeprom_data[i+1] |= (1<<j);
			SPD_REG8(SPD_R_PIO_DATA) = 0x80;
			DelayThread(1);
		}
	}

	SPD_REG8(SPD_R_PIO_DATA) = 0;
	DelayThread(1);
	eeprom_data[0] = 1;	/* The EEPROM data is valid.  */
	res = 0;

out:
	SPD_REG8(SPD_R_PIO_DIR) = 1;
	return res;
}

static void dev9LEDCtl(int ctl)
{
	USE_SPD_REGS;
	SPD_REG8(SPD_R_PIO_DATA) = (ctl == 0);
}


static int smap_subsys_init(void)
{
	int stat, flags;

	DisableIntr(IOP_IRQ_DMA_DEV9, &stat);
	CpuSuspendIntr(&flags);

	/* Enable the DEV9 DMAC channel.  */
	dmac_set_dpcr2(dmac_get_dpcr2() | 0x80);
	CpuResumeIntr(flags);

	/* Not quite sure what this enables yet.  */
	smap_set_stat(0x103);

	/* Disable all device interrupts.  */
	dev9IntrDisable(0xffff);

	/* Read in the MAC address.  */
	read_eeprom_data();

	/* Turn the LED off.  */
	dev9LEDCtl(0);

	return 0;
}

void dev9Shutdown(void)
{
	USE_DEV9_REGS;

	if (dev9type == 0) {	/* PCMCIA */
		DEV9_REG(DEV9_R_POWER) = 0;
		DEV9_REG(DEV9_R_1474) = 0;
	} else if (dev9type == 1) {
		DEV9_REG(DEV9_R_1466) = 1;
		DEV9_REG(DEV9_R_1464) = 0;
		DEV9_REG(DEV9_R_1460) = DEV9_REG(DEV9_R_1464);
		DEV9_REG(DEV9_R_POWER) = DEV9_REG(DEV9_R_POWER) & ~4;
		DEV9_REG(DEV9_R_POWER) = DEV9_REG(DEV9_R_POWER) & ~1;
	}
	DelayThread(1000000);
}

static int smap_device_init(void)
{
	USE_SPD_REGS;
	const char *spdnames[] = { "(unknown)", "TS", "ES1", "ES2" };
	int idx, res;
	u16 spdrev;

	eeprom_data[0] = 0;

	if (smap_device_probe() < 0) {
		M_PRINTF("PC card or expansion device isn't connected.\n");
		return -1;
	}

	smap_device_reset();

	/* Locate the SPEED Lite chip and get the bus ready for the
	   PCMCIA device.  */
	if (dev9type == 0) {
		if ((res = card_find_manfid(0xf15300)))
			M_PRINTF("SPEED Lite not found.\n");

		if (!res && (res = pcic_ssbus_mode(5)))
			M_PRINTF("Unable to change SSBUS mode.\n");

		if (res) {
			dev9Shutdown();
			return -1;
		}
	}

	/* Print out the SPEED chip revision.  */
	spdrev = SPD_REG16(SPD_R_REV_1);
	idx    = (spdrev & 0xffff) - 14;
	if (spdrev == 9)
		idx = 1;	/* TS */
	else if (spdrev < 9 || (spdrev < 16 || spdrev > 17))
		idx = 0;	/* Unknown revision */

	M_PRINTF("SPEED chip '%s', revision 0x%0X\n", spdnames[idx], spdrev);
	return 0;
}

static int pcic_get_cardtype()
{
	USE_DEV9_REGS;
	u16 val = DEV9_REG(DEV9_R_1462) & 0x03;

	if (val == 0)
		return 1;	/* 16-bit */
	else
	if (val < 3)
		return 2;	/* CardBus */
	return 0;
}

static int pcic_get_voltage()
{
	USE_DEV9_REGS;
	u16 val = DEV9_REG(DEV9_R_1462) & 0x0c;

	if (val == 0x04)
		return 3;
	if (val == 0 || val == 0x08)
		return 1;
	if (val == 0x0c)
		return 2;
	return 0;
}

static int pcic_power(int voltage, int flag)
{
	USE_DEV9_REGS;
	u16 cstc1, cstc2;
	u16 val = (voltage == 1) << 2;

	DEV9_REG(DEV9_R_POWER) = 0;

	if (voltage == 2)
		val |= 0x08;
	if (flag == 1)
		val |= 0x10;

	DEV9_REG(DEV9_R_POWER) = val;
	DelayThread(22000);

	if (DEV9_REG(DEV9_R_1462) & 0x100)
		return 0;

	DEV9_REG(DEV9_R_POWER) = 0;
	DEV9_REG(DEV9_R_1464) = cstc1 = DEV9_REG(DEV9_R_1464);
	DEV9_REG(DEV9_R_1466) = cstc2 = DEV9_REG(DEV9_R_1466);
	return -1;
}

static void pcmcia_set_stat(int stat)
{
	USE_DEV9_REGS;
	u16 val = stat & 0x01;

	if (stat & 0x10)
		val = 1;
	if (stat & 0x02)
		val |= 0x02;
	if (stat & 0x20)
		val |= 0x02;
	if (stat & 0x04)
		val |= 0x08;
	if (stat & 0x08)
		val |= 0x10;
	if (stat & 0x200)
		val |= 0x20;
	if (stat & 0x100)
		val |= 0x40;
	if (stat & 0x400)
		val |= 0x80;
	if (stat & 0x800)
		val |= 0x04;
	DEV9_REG(DEV9_R_1476) = val & 0xff;
}

static int pcic_ssbus_mode(int voltage)
{
	USE_DEV9_REGS;
	USE_SPD_REGS;
	u16 stat = DEV9_REG(DEV9_R_1474) & 7;

	if (voltage != 3 && voltage != 5)
		return -1;

	DEV9_REG(DEV9_R_1460) = 2;
	if (stat)
		return -1;

	if (voltage == 3) {
		DEV9_REG(DEV9_R_1474) = 1;
		DEV9_REG(DEV9_R_1460) = 1;
		SPD_REG8(0x20) = 1;
		DEV9_REG(DEV9_R_1474) = voltage;
	} else if (voltage == 5) {
		DEV9_REG(DEV9_R_1474) = voltage;
		DEV9_REG(DEV9_R_1460) = 1;
		SPD_REG8(0x20) = 1;
		DEV9_REG(DEV9_R_1474) = 7;
	}
	_sw(0xe01a3043, SSBUS_R_1418);

	DelayThread(5000);
	DEV9_REG(DEV9_R_POWER) = DEV9_REG(DEV9_R_POWER) & ~1;
	return 0;
}

static int pcmcia_device_probe()
{
	const char *pcic_ct_names[] = { "No", "16-bit", "CardBus" };
	int voltage;

	pcic_voltage = pcic_get_voltage();
	pcic_cardtype = pcic_get_cardtype();
	voltage = (pcic_voltage == 2 ? 5 : (pcic_voltage == 1 ? 3 : 0));

	M_PRINTF("%s PCMCIA card detected. Vcc = %dV\n",
			pcic_ct_names[pcic_cardtype], voltage);

	if (pcic_voltage == 3 || pcic_cardtype != 1)
		return -1;

	return 0;
}

static int pcmcia_device_reset(void)
{
	USE_DEV9_REGS;
	u16 cstc1, cstc2;

	/* The card must be 16-bit (type 2?) */
	if (pcic_cardtype != 1)
		return -1;

	DEV9_REG(DEV9_R_147E) = 1;
	if (pcic_power(pcic_voltage, 1) < 0)
		return -1;

	DEV9_REG(DEV9_R_POWER) = DEV9_REG(DEV9_R_POWER) | 0x02;
	DelayThread(500000);

	DEV9_REG(DEV9_R_POWER) = DEV9_REG(DEV9_R_POWER) | 0x01;
	DEV9_REG(DEV9_R_1464) = cstc1 = DEV9_REG(DEV9_R_1464);
	DEV9_REG(DEV9_R_1466) = cstc2 = DEV9_REG(DEV9_R_1466);
	return 0;
}

static int card_find_manfid(u32 manfid)
{
	USE_DEV9_REGS;
	USE_SPD_REGS;
	u32 spdaddr, spdend, next, tuple;
	u8 hdr, ofs;

	DEV9_REG(DEV9_R_1460) = 2;
	_sw(0x1a00bb, SSBUS_R_1418);

	/* Scan the card for the MANFID tuple.  */
	spdaddr = 0;
	spdend =  0x1000;
	/* I hate this code, and it hates me.  */
	while (spdaddr < spdend) {
		hdr = SPD_REG8(spdaddr) & 0xff;
		spdaddr += 2;
		if (!hdr)
			continue;
		if (hdr == 0xff)
			break;
		if (spdaddr >= spdend)
			goto error;

		ofs = SPD_REG8(spdaddr) & 0xff;
		spdaddr += 2;
		if (ofs == 0xff)
			break;

		next = spdaddr + (ofs * 2);
		if (next >= spdend)
			goto error;

		if (hdr == 0x20) {
			if ((spdaddr + 8) >= spdend)
				goto error;

			tuple = (SPD_REG8(spdaddr + 2) << 24)|
				(SPD_REG8(spdaddr) << 16)|
				(SPD_REG8(spdaddr + 6) << 8)|
				 SPD_REG8(spdaddr + 4);
			if (manfid == tuple)
				return 0;
			M_PRINTF("MANFID 0x%08lx doesn't match expected 0x%08lx\n",
					tuple, manfid);
			return -1;
		}
		spdaddr = next;
	}

	M_PRINTF("MANFID 0x%08lx not found.\n", manfid);
	return -1;
error:
	M_PRINTF("Invalid tuples at offset 0x%08lx.\n", spdaddr - SPD_REGBASE);
	return -1;
}

static int pcmcia_init(void)
{
	USE_DEV9_REGS;
	int *mode;
	int flags;
	u16 cstc1, cstc2;

	_sw(0x51011, SSBUS_R_1420);
	_sw(0x1a00bb, SSBUS_R_1418);
	_sw(0xef1a3043, SSBUS_R_141c);

	/* If we are a T10K, then we go through AIF.  */
	if ((mode = QueryBootMode(6)) != NULL) {
		if ((*(u16 *)mode & 0xfe) == 0x60) {
			if (_lh(0xb4000000) == 0xa1) {
				_sh(4, 0xb4000006);
				using_aif = 1;
			} else {
				M_PRINTF("T10k detected, but AIF not detected.\n");
				return 1;
			}
		}
	}

	if (DEV9_REG(DEV9_R_POWER) == 0) {
		DEV9_REG(DEV9_R_POWER) = 0;
		DEV9_REG(DEV9_R_147E) = 1;
		DEV9_REG(DEV9_R_1460) = 0;
		DEV9_REG(DEV9_R_1474) = 0;
		DEV9_REG(DEV9_R_1464) = cstc1 = DEV9_REG(DEV9_R_1464);
		DEV9_REG(DEV9_R_1466) = cstc2 = DEV9_REG(DEV9_R_1466);
		DEV9_REG(DEV9_R_1468) = 0x10;
		DEV9_REG(DEV9_R_146A) = 0x90;
		DEV9_REG(DEV9_R_147C) = 1;
		DEV9_REG(DEV9_R_147A) = DEV9_REG(DEV9_R_147C);

		pcic_voltage = pcic_get_voltage();
		pcic_cardtype = pcic_get_cardtype();

		if (smap_device_init() != 0)
			return 1;
	} else {
		_sw(0xe01a3043, SSBUS_R_1418);
	}

	if (smap_subsys_init() != 0)
		return 1;

	CpuSuspendIntr(&flags);
	EnableIntr(DEV9_INTR);
	CpuResumeIntr(flags);

	DEV9_REG(DEV9_R_147E) = 0;
	M_PRINTF("CXD9566 (PCMCIA type) initialized.\n");
	return 0;
}

static void expbay_set_stat(int stat)
{
	USE_DEV9_REGS;
	DEV9_REG(DEV9_R_1464) = stat & 0x3f;
}

static int expbay_device_probe()
{
	USE_DEV9_REGS;
	return (DEV9_REG(DEV9_R_1462) & 0x01) ? -1 : 0;
}

static int expbay_device_reset(void)
{
	USE_DEV9_REGS;

	if (expbay_device_probe() < 0)
		return -1;

	DEV9_REG(DEV9_R_POWER) = (DEV9_REG(DEV9_R_POWER) & ~1) | 0x04;	// power on
	DelayThread(500000);

	DEV9_REG(DEV9_R_1460) = DEV9_REG(DEV9_R_1460) | 0x01;
	DEV9_REG(DEV9_R_POWER) = DEV9_REG(DEV9_R_POWER) | 0x01;
	DelayThread(500000);
	return 0;
}

static int expbay_init(void)
{
	USE_DEV9_REGS;
	int flags;

	_sw(0x51011, SSBUS_R_1420);
	_sw(0xe01a3043, SSBUS_R_1418);
	_sw(0xef1a3043, SSBUS_R_141c);

	if ((DEV9_REG(DEV9_R_POWER) & 0x04) == 0) { // if not already powered
		DEV9_REG(DEV9_R_1466) = 1;
		DEV9_REG(DEV9_R_1464) = 0;
		DEV9_REG(DEV9_R_1460) = DEV9_REG(DEV9_R_1464);

		if (smap_device_init() != 0)
			return 1;
	}

	if (smap_subsys_init() != 0)
		return 1;

	CpuSuspendIntr(&flags);
	EnableIntr(DEV9_INTR);
	CpuResumeIntr(flags);

	DEV9_REG(DEV9_R_1466) = 0;
	M_PRINTF("CXD9611 (Expansion Bay type) initialized.\n");
	return 0;
}


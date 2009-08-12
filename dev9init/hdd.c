/* Copyright (c) 2008 Mega Man */
/* Copied code from ps2sdk. HDD initialisation on PS2. */
#include "types.h"
#include "defs.h"
#include "intrman.h"
#include "dmacman.h"
#include "stdio.h"
#include "thbase.h"

#include "dev9regs.h"
#include "atahw.h"
#include "hdd.h"
#include "dev9init.h"
#include "irx_imports.h"

/* 0x80 for busy, 0x88 for bus busy.  */
static int ata_wait_busy(int bits)
{
	USE_ATA_REGS;
	int i, didx, delay;

	for (i = 0; i < 80; i++) {
		if (!(ata_hwport->r_control & bits))
			return 0;

		didx = i / 10;
		switch (didx) {
			case 0:
				continue;
			case 1:
				delay = 100;
				break;
			case 2:
				delay = 1000;
				break;
			case 3:
				delay = 10000;
				break;
			case 4:
				delay = 100000;
				break;
			default:
				delay = 1000000;
		}

		DelayThread(delay);
	}

	M_PRINTF("Timeout while waiting on busy (0x%02x).\n", bits);
	return -502;
}

/* Reset the ATA controller/bus.  */
static int ata_bus_reset()
{
	USE_SPD_REGS;
	SPD_REG16(SPD_R_IF_CTRL) = SPD_IF_ATA_RESET;
	DelayThread(100);
	SPD_REG16(SPD_R_IF_CTRL) = 0x48;
	DelayThread(3000);
	return ata_wait_busy(0x80);
}

/* Not sure if it's reset, but it disables ATA interrupts.  */
static int ata_reset_devices()
{
	USE_ATA_REGS;

	if (ata_hwport->r_control & 0x80)
		return -501;

	/* Dunno what this does.  */
	ata_hwport->r_control = 6;
	DelayThread(100);

	/* Disable ATA interrupts.  */
	ata_hwport->r_control = 2;
	DelayThread(3000);
	
	return ata_wait_busy(0x80);
}

static void ata_pio_mode(int mode)
{
	USE_SPD_REGS;
	u16 val = 0x92;

	switch (mode) {
		case 1:
			val = 0x72;
			break;
		case 2:
			val = 0x32;
			break;
		case 3:
			val = 0x24;
			break;
		case 4:
			val = 0x23;
			break;
	}

	SPD_REG16(SPD_R_PIO_MODE) = val;
}

void ata_setup(void)
{
	USE_SPD_REGS;

	if (!(SPD_REG16(SPD_R_REV_3) & SPD_CAPS_ATA) || !(SPD_REG16(SPD_R_REV_8) & 0x02)) {
		M_PRINTF("HDD is not connected, exiting.\n");
		return;
	}

	ata_pio_mode(0);

	if (ata_bus_reset() != 0) {
		M_PRINTF("Failed ata_bus_reset().\n");
		return;
	}

	ata_reset_devices();

	dev9IntrEnable(SPD_INTR_ATA0);
	dev9IntrEnable(SPD_INTR_ATA1);
}

#include "ledflash.h"

/* ledflash.c - Repeatedly flash the internal PS2 LED.
 *
 * Written by James Lee <jbit@jbit.net> and M. R. Brown <mrbrown@0xd6.org>
 * Copyright (c) 2008 Mega Man
 */

/* These registers taken from drivers/block/ps2ide.h in the PS2/Linux kernel
 * source.  Apparently they control the orange LED next to the PS2's
 * peripheral block.  The LED is normally used by Linux to indicate activity
 * during HDD reads and writes. */
#define PS2SPD_PIO_DIR  0xb400002c
#define PS2SPD_PIO_DATA 0xb400002e

void ledflash(void) {
	int i;

	while (1) {
		/* Turn the LED on... */
		*(volatile unsigned char *)PS2SPD_PIO_DIR  = 1;
		*(volatile unsigned char *)PS2SPD_PIO_DATA = 0;
		for (i=0;i<10000000;i++) ;

		/* ...and off */
		*(volatile unsigned char *)PS2SPD_PIO_DIR  = 1;
		*(volatile unsigned char *)PS2SPD_PIO_DATA = 1;
		for (i=0;i<10000000;i++) ;
	}
}

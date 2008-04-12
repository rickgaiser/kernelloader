/* Copyright (c) 2007 Mega Man */
#include "gsKit.h"
#include "dmaKit.h"
#include "malloc.h"
#include "stdio.h"
#include "kernel.h"

#include "config.h"

/** gsGlobal is required for all painting functiions of gsKit. */
static GSGLOBAL *gsGlobal = NULL;
/** Colours used for painting. */
static u64 White, Black, Blue, Red;
/** Text colour. */
static u64 TexCol;
/** Font used for printing text. */
static GSFONT *gsFont;
/** File name that is printed on screen. */
static const char *loadName = NULL;
/** Percentage for loading file shown as progress bar. */
static int loadPercentage = 0;
/** Scale factor for font. */
static float scale = 1.0f;

/** Paint current state on screen. */
void graphic_paint(void)
{
	gsKit_clear(gsGlobal, Blue);

	gsKit_font_print_scaled(gsGlobal, gsFont, 50, 50, 3, scale, TexCol,
		"Kernelloader for linux");
	if (loadName != NULL) {
		gsKit_font_print_scaled(gsGlobal, gsFont, 50, 90, 3, scale, TexCol,
			loadName);
		gsKit_prim_sprite(gsGlobal, 50, 120, 50 + 520, 140, 2, White);
		if (loadPercentage > 0) {
			gsKit_prim_sprite(gsGlobal, 50, 120, 50 + (520 * loadPercentage) / 100, 140, 2, Red);
		}
	}

	gsKit_queue_exec(gsGlobal);
	gsKit_sync_flip(gsGlobal);
}

/**
 * Set load percentage of file.
 * @param percentage Percentage to set (0 - 100).
 * @param name File name printed on screen.
 */
void graphic_setPercentage(int percentage, const char *name)
{
	if (percentage > 100) {
		percentage = 100;
	}
	loadPercentage = percentage;
	loadName = name;
	graphic_paint();
}

/**
 * Initialize graphic screen.
 * @param mode Graphic mode to use.
 */
int graphic_main(graphic_mode_t mode)
{
	int i;

	if (mode == MODE_NTSC) {
		gsGlobal = gsKit_init_global(GS_MODE_NTSC);
	} else {
		gsGlobal = gsKit_init_global(GS_MODE_PAL_I);
	}
//	GSGLOBAL *gsGlobal = gsKit_init_global(GS_MODE_VGA_640_60);


	dmaKit_init(D_CTRL_RELE_OFF,D_CTRL_MFD_OFF, D_CTRL_STS_UNSPEC,
		    D_CTRL_STD_OFF, D_CTRL_RCYC_8, 1 << DMA_CHANNEL_GIF);

	// Initialize the DMAC
	dmaKit_chan_init(DMA_CHANNEL_GIF);
	dmaKit_chan_init(DMA_CHANNEL_FROMSPR);
	dmaKit_chan_init(DMA_CHANNEL_TOSPR);
	
	Black = GS_SETREG_RGBAQ(0x00,0x00,0x00,0x00,0x00);
	White = GS_SETREG_RGBAQ(0xFF,0xFF,0xFF,0x00,0x00);
	Blue = GS_SETREG_RGBAQ(0x10,0x10,0xF0,0x00,0x00);
	Red = GS_SETREG_RGBAQ(0xF0,0x10,0x10,0x00,0x00);
	
        TexCol = GS_SETREG_RGBAQ(0xFF,0xFF,0xFF,0x80,0x00);

        gsGlobal->PrimAlphaEnable = GS_SETTING_ON;

	gsKit_init_screen(gsGlobal);

	gsFont = gsKit_init_font(GSKIT_FTYPE_FONTM, NULL);
	if (gsKit_font_upload(gsGlobal, gsFont) != 0) {
		printf("Can't find any font to use\n");
		SleepThread();
	}

	gsFont->FontM_Spacing = 0.8f;

        gsKit_mode_switch(gsGlobal, GS_ONESHOT);

	for (i = 0; i < 2; i++)
	{
		graphic_paint();
	}
	
	return 0;
}

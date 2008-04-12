
/* Copyright (c) 2007 Mega Man */
#include "gsKit.h"
#include "dmaKit.h"
#include "malloc.h"
#include "stdio.h"
#include "kernel.h"

#include "config.h"
#include "menu.h"

#define MAX_BUFFER 128

static Menu *menu = NULL;

/** gsGlobal is required for all painting functiions of gsKit. */
static GSGLOBAL *gsGlobal = NULL;

/** Colours used for painting. */
static u64 White, Black, Blue, Red;

/** Text colour. */
static u64 TexCol;

/** Red Text colour. */
static u64 TexRed;

/** Font used for printing text. */
static GSFONT *gsFont;

/** File name that is printed on screen. */
static const char *loadName = NULL;

static const char *statusMessage = NULL;

/** Percentage for loading file shown as progress bar. */
static int loadPercentage = 0;

/** Scale factor for font. */
static float scale = 1.0f;

static const char *errorMessage = NULL;

/** Paint current state on screen. */
void graphic_paint(void)
{
	gsKit_clear(gsGlobal, Blue);

	gsKit_font_print_scaled(gsGlobal, gsFont, 50, 50, 3, scale, TexCol,
		"Kernelloader for linux 1.0");
	gsKit_font_print_scaled(gsGlobal, gsFont, 400, 470, 3, 0.5, TexCol,
		"by Mega Man 24.12.2007");
	if (statusMessage != NULL) {
		gsKit_font_print_scaled(gsGlobal, gsFont, 50, 90, 3, scale, TexCol,
			statusMessage);
	} else if (loadName != NULL) {
		gsKit_font_print_scaled(gsGlobal, gsFont, 50, 90, 3, scale, TexCol,
			loadName);
		gsKit_prim_sprite(gsGlobal, 50, 120, 50 + 520, 140, 2, White);
		if (loadPercentage > 0) {
			gsKit_prim_sprite(gsGlobal, 50, 120,
				50 + (520 * loadPercentage) / 100, 140, 2, Red);
		}
	}
	if (errorMessage != NULL) {
		char lineBuffer[30];
		int i;
		int pos;
		int lastSpace = 0;
		int lastSpacePos = 0;
		int y;

		pos = 0;
		y = 170;
		gsKit_font_print_scaled(gsGlobal, gsFont, 50, y, 3, scale, TexRed,
			"Error Message:");
		y += 60;
		do {
			i = 0;
			while (i < 26) {
				lineBuffer[i] = errorMessage[pos];
				if (errorMessage[pos] == 0) {
					lastSpace = i;
					lastSpacePos = pos;
					i++;
					break;
				}
				if (errorMessage[pos] == ' ') {
					lastSpace = i;
					lastSpacePos = pos + 1;
					if (i != 0) {
						i++;
					}
				} else if (errorMessage[pos] == '\r') {
					lineBuffer[i] = 0;
					lastSpace = i;
					lastSpacePos = pos + 1;
				} else if (errorMessage[pos] == '\n') {
					lineBuffer[i] = 0;
					lastSpace = i;
					lastSpacePos = pos + 1;
					pos++;
					break;
				} else {
					i++;
				}
				pos++;
			}
			if (lastSpace != 0) {
				pos = lastSpacePos;
			} else {
				lastSpace = i;
			}
			lineBuffer[lastSpace] = 0;
#if 1
			gsKit_font_print_scaled(gsGlobal, gsFont, 50, y, 3, scale, TexCol,
				lineBuffer);
#else
			printf("Test pos %d i %d %s\n", pos, i, lineBuffer);
#endif
			y += 30;
		} while(errorMessage[pos] != 0);
	}
	else {
		if (menu != NULL) {
			menu->paint();
		}
	}
	gsKit_queue_exec(gsGlobal);
	gsKit_sync_flip(gsGlobal);
}

extern "C" {

	/**
	 * Set load percentage of file.
	 * @param percentage Percentage to set (0 - 100).
	 * @param name File name printed on screen.
	 */
	void graphic_setPercentage(int percentage, const char *name) {
		if (percentage > 100) {
			percentage = 100;
		}
		loadPercentage = percentage;

		loadName = name;
		graphic_paint();
	}

	/**
	 * Set status message.
	 * @param text Text displayed on screen.
	 */
	void graphic_setStatusMessage(const char *text) {
		statusMessage = text;
		graphic_paint();
	}
}

/**
 * Initialize graphic screen.
 * @param mode Graphic mode to use.
 */
Menu *graphic_main(graphic_mode_t mode)
{
	int i;

	if (mode == MODE_NTSC) {
		gsGlobal = gsKit_init_global(GS_MODE_NTSC);
	}
	else {
		gsGlobal = gsKit_init_global(GS_MODE_PAL_I);
	}
	//  GSGLOBAL *gsGlobal = gsKit_init_global(GS_MODE_VGA_640_60);


	dmaKit_init(D_CTRL_RELE_OFF, D_CTRL_MFD_OFF, D_CTRL_STS_UNSPEC,
		D_CTRL_STD_OFF, D_CTRL_RCYC_8, 1 << DMA_CHANNEL_GIF);

	// Initialize the DMAC
	dmaKit_chan_init(DMA_CHANNEL_GIF);
	dmaKit_chan_init(DMA_CHANNEL_FROMSPR);
	dmaKit_chan_init(DMA_CHANNEL_TOSPR);

	Black = GS_SETREG_RGBAQ(0x00, 0x00, 0x00, 0x00, 0x00);
	White = GS_SETREG_RGBAQ(0xFF, 0xFF, 0xFF, 0x00, 0x00);
	Blue = GS_SETREG_RGBAQ(0x10, 0x10, 0xF0, 0x00, 0x00);
	Red = GS_SETREG_RGBAQ(0xF0, 0x10, 0x10, 0x00, 0x00);

	TexCol = GS_SETREG_RGBAQ(0xFF, 0xFF, 0xFF, 0x80, 0x00);
	TexRed = GS_SETREG_RGBAQ(0xF0, 0x10, 0x10, 0x80, 0x00);

	gsGlobal->PrimAlphaEnable = GS_SETTING_ON;

	gsKit_init_screen(gsGlobal);

	gsFont = gsKit_init_font(GSKIT_FTYPE_FONTM, NULL);
	if (gsKit_font_upload(gsGlobal, gsFont) != 0) {
		printf("Can't find any font to use\n");
		SleepThread();
	}

	gsFont->FontM_Spacing = 0.8f;

	menu = new Menu(gsGlobal, gsFont);
	menu->setPosition(50, 120);

	gsKit_mode_switch(gsGlobal, GS_ONESHOT);

	for (i = 0; i < 2; i++) {
		graphic_paint();
	}
	return menu;
}

int setCurrentMenu(void *arg)
{
	Menu *newMenu = (Menu *) arg;

	menu = newMenu;

	return 0;
}

Menu *getCurrentMenu(void)
{
	return menu;
}

extern "C" {
	int setErrorMessage(const char *text) {
		errorMessage = text;
	}

	const char *getErrorMessage(void) {
		return errorMessage;
	}

	int error_printf(const char *format, ...)
	{
		static char buffer[MAX_BUFFER];
		int ret;
		va_list varg;
		va_start(varg, format);
		ret = vsnprintf(buffer, MAX_BUFFER, format, varg);
		setErrorMessage(buffer);

		/* Show it before doing anything else. */
		graphic_paint();

		va_end(varg);
		return ret;
	}
}



/* Copyright (c) 2007 - 2009 Mega Man */
#include "gsKit.h"
#include "dmaKit.h"
#include "malloc.h"
#include "stdio.h"
#include "kernel.h"

#include "config.h"
#include "menu.h"
#include "rom.h"
#include "graphic.h"
#include "loader.h"
#include <screenshot.h>


/** Maximum buffer size for error_printf(). */
#define MAX_BUFFER 128

/** Maximum buffer size of info message buffer. */
#define MAX_INFO_BUFFER 4096

/** Maximum number of error messages. */
#define MAX_MESSAGES 10

/** Maximum texture size in Bytes. */
#define MAX_TEX_SIZE 0x30000

/** Size of small textures (will be uploaded). */
#define SMALL_TEXT_SIZE 0x4000

/** True, if graphic is initialized. */
static bool graphicInitialized = false;

static Menu *menu = NULL;

static bool enableDisc = 0;

/** gsGlobal is required for all painting functiions of gsKit. */
static GSGLOBAL *gsGlobal = NULL;

/** Colours used for painting. */
static u64 White, Black, Blue, Red;

/** Text colour. */
static u64 TexCol;

/** Red text colour. */
static u64 TexRed;

/** Black text colour. */
static u64 TexBlack;

/** Font used for printing text. */
static GSFONTM *gsFont;

/** File name that is printed on screen. */
static const char *loadName = NULL;

static const char *statusMessage = NULL;

/** Percentage for loading file shown as progress bar. */
static int loadPercentage = 0;

/** Scale factor for font. */
static float scale = 1.0f;

/** Ring buffer with error messages. */
static const char *errorMessage[MAX_MESSAGES] = {
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL
};

/** Read pointer into ring buffer of error messages. */
static int readMsgPos = 0;

/** Write pointer into ring buffer of error messages. */
static int writeMsgPos = 0;

static GSTEXTURE *texFolder = NULL;

static GSTEXTURE *texUp = NULL;

static GSTEXTURE *texBack = NULL;

static GSTEXTURE *texSelected = NULL;

static GSTEXTURE *texUnselected = NULL;

static GSTEXTURE *texPenguin = NULL;

static GSTEXTURE *texDisc = NULL;

static GSTEXTURE *texCloud = NULL;

static int reservedEndOfDisplayY = 42;

static bool usePad = false;

int scrollPos = 0;

int inputScrollPos = 0;

u32 globalVram;

static void gsKit_texture_upload_inline(GSGLOBAL *gsGlobal, GSTEXTURE *Texture)
{
	static u32 *lastMem;
	static u32 lastVram;

	/* Check if already uploaded the last time. */
	if ((lastMem != Texture->Mem) || (lastVram != Texture->Vram)) {
		/* Texture was not uploaded, need to upload it. */
		gsKit_setup_tbw(Texture);
	
		if (Texture->PSM == GS_PSM_T8)
		{
			gsKit_texture_send_inline(gsGlobal, Texture->Mem, Texture->Width, Texture->Height, Texture->Vram, Texture->PSM, Texture->TBW, GS_CLUT_TEXTURE);
			gsKit_texture_send_inline(gsGlobal, Texture->Clut, 16, 16, Texture->VramClut, Texture->ClutPSM, 1, GS_CLUT_PALLETE);
	
		}
		else if (Texture->PSM == GS_PSM_T4)
		{
			gsKit_texture_send_inline(gsGlobal, Texture->Mem, Texture->Width, Texture->Height, Texture->Vram, Texture->PSM, Texture->TBW, GS_CLUT_TEXTURE);
			gsKit_texture_send_inline(gsGlobal, Texture->Clut, 8,  2, Texture->VramClut, Texture->ClutPSM, 1, GS_CLUT_PALLETE);
		}
		else
		{
			gsKit_texture_send_inline(gsGlobal, Texture->Mem, Texture->Width, Texture->Height, Texture->Vram, Texture->PSM, Texture->TBW, GS_CLUT_NONE);
		}
	}
}

void paintTexture(GSTEXTURE *tex, int x, int y, int z)
{
	if (tex != NULL) {
		if (tex->Vram != GSKIT_ALLOC_ERROR) {
			u32 size;

			size = gsKit_texture_size_ee(tex->Width, tex->Height, tex->PSM);
			if (size <= SMALL_TEXT_SIZE) {
				/* No uploading required for small textures. */
				gsKit_prim_sprite_texture(gsGlobal, tex,
					x, y, 0, 0, x + tex->Width, y + tex->Height,
					tex->Width, tex->Height, z,
					GS_SETREG_RGBAQ(0x80,0x80,0x80,0x80,0x00));
			} else {
				GSTEXTURE uploadTex;
				u32 slice;
				u32 offset;
				u32 vramSize;

				/* Need to upload texture, because it is too big for cache. */
				slice = tex->Height;
				vramSize = gsKit_texture_size(tex->Width, slice, tex->PSM);
				while (vramSize > MAX_TEX_SIZE) {
					slice = (slice + 1) >> 1;
					do {
						size = gsKit_texture_size_ee(tex->Width, slice, tex->PSM);
						vramSize = gsKit_texture_size(tex->Width, slice, tex->PSM);
						if (slice == 0) {
							printf("minimum vramSize 0x%08x\n", vramSize);
						}
						if (size & 15) {
							/* Get next 16 byte aligned size. */
							/* DMA will only support a start address which is 16 Byte aligned. */
							/* The start address of the next block is calculated by adding slice. */
							slice++;
						}
					} while (size & 15); /* Wait until size is 16 Byte aligned. */
				}
	
				uploadTex = *tex;
				uploadTex.Height = slice;
	
				/* Upload image as slices. */	
				for (offset = 0; offset < tex->Height; offset += slice) {
					u32 remaining;
	
					remaining = tex->Height - offset;
					if (remaining < slice) {
						uploadTex.Height = remaining;
					}
					gsKit_texture_upload_inline(gsGlobal, &uploadTex);
					gsKit_prim_sprite_texture(gsGlobal, &uploadTex,
						x, y + offset, 0, 0, x + uploadTex.Width, y + offset + uploadTex.Height,
						uploadTex.Width, uploadTex.Height, z,
						GS_SETREG_RGBAQ(0x80,0x80,0x80,0x80,0x00));
					uploadTex.Mem = (u32 *) (((u32) uploadTex.Mem) + size);
				}
			}
		}
	}
}

static char infoBuffer[MAX_INFO_BUFFER];
static int infoBufferPos = 0;

static char *inputBuffer = NULL;

int printTextBlock(int x, int y, int z, int maxCharsPerLine, int maxY, const char *msg, int scrollPos)
{
	char lineBuffer[maxCharsPerLine];
	int i;
	int pos;
	int lastSpace;
	int lastSpacePos;
	int lineNo;

	pos = 0;
	lineNo = 0;
	do {
		i = 0;
		lastSpace = 0;
		lastSpacePos = 0;
		while (i < maxCharsPerLine) {
			lineBuffer[i] = msg[pos];
			if (msg[pos] == 0) {
				lastSpace = i;
				lastSpacePos = pos;
				i++;
				break;
			}
			if (msg[pos] == ' ') {
				lastSpace = i;
				lastSpacePos = pos + 1;
				if (i != 0) {
					i++;
				}
			} else if (msg[pos] == '\r') {
				lineBuffer[i] = 0;
				lastSpace = i;
				lastSpacePos = pos + 1;
			} else if (msg[pos] == '\n') {
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

		if (lineNo >= scrollPos) {
#if 0
			printf("Test pos %d i %d lastSpacePos %d %s\n", pos, i, lastSpacePos, lineBuffer);
#else
			gsKit_fontm_print_scaled(gsGlobal, gsFont, x, y, z, scale, TexCol,
				lineBuffer);
#endif
			y += 30;
			if (y > (maxY - 30)) {
				break;
			}
		}
		lineNo++;
	} while(msg[pos] != 0);
	if (lineNo < scrollPos) {
		return lineNo;
	} else {
		return scrollPos;
	}
}

void graphic_common(void)
{
	gsKit_clear(gsGlobal, Blue);

	/* Paint background. */
	paintTexture(texCloud, 0, 0, 0);

	paintTexture(texPenguin, 5, 10, 1);

	gsKit_fontm_print_scaled(gsGlobal, gsFont, 110, 50, 3, scale, TexCol,
		"Loader for Linux " LOADER_VERSION
#ifdef RESET_IOP
		"R"
#endif
#ifdef PS2LINK
		"P"
#endif
#ifdef NAPLINK
		"N"
#endif
#ifdef SCREENSHOT
		"S"
#endif
#ifdef NEW_ROM_MODULES
		"M"
#endif
#ifdef OLD_ROM_MODULES
		"O"
#endif
#ifdef SHARED_MEM_DEBUG
		"S"
#endif
	);
	gsKit_fontm_print_scaled(gsGlobal, gsFont, 490, gsGlobal->Height - reservedEndOfDisplayY, 3, 0.5, TexBlack,
		"by Mega Man");
	gsKit_fontm_print_scaled(gsGlobal, gsFont, 490, gsGlobal->Height - reservedEndOfDisplayY + 15, 3, 0.5, TexBlack,
		"TODAY"
#ifdef RTE
		" RTE"
#endif
	);
}

/** Paint screen when Auto Boot is in process. */
void graphic_auto_boot_paint(int time)
{
	static char msg[80];

	if (!graphicInitialized) {
		return;
	}
	graphic_common();

	snprintf(msg, sizeof(msg), "Auto Boot in %d seconds.", time);
	gsKit_fontm_print_scaled(gsGlobal, gsFont, 50, gsGlobal->Height - reservedEndOfDisplayY, 3, 0.8, TexBlack,
		msg);

	gsKit_queue_exec(gsGlobal);
	gsKit_sync_flip(gsGlobal);
}

/** Paint current state on screen. */
void graphic_paint(void)
{
	const char *msg;

	if (!graphicInitialized) {
		return;
	}
	graphic_common();

	if (enableDisc) {
		paintTexture(texDisc, 100, 300, 40);
	}

	if (statusMessage != NULL) {
		gsKit_fontm_print_scaled(gsGlobal, gsFont, 50, 90, 3, scale, TexCol,
			statusMessage);
	} else if (loadName != NULL) {
		gsKit_fontm_print_scaled(gsGlobal, gsFont, 50, 90, 3, scale, TexCol,
			loadName);
		gsKit_prim_sprite(gsGlobal, 50, 120, 50 + 520, 140, 2, White);
		if (loadPercentage > 0) {
			gsKit_prim_sprite(gsGlobal, 50, 120,
				50 + (520 * loadPercentage) / 100, 140, 2, Red);
		}
	}
	msg = getErrorMessage();
	if (msg != NULL) {
		gsKit_fontm_print_scaled(gsGlobal, gsFont, 50, 170, 3, scale, TexRed,
			"Error Message:");
		printTextBlock(50, 230, 3, 26, gsGlobal->Height - reservedEndOfDisplayY, msg, 0);
	} else {
		if (!isInfoBufferEmpty()) {
			scrollPos = printTextBlock(50, 170, 3, 26, gsGlobal->Height - reservedEndOfDisplayY, infoBuffer, scrollPos);
		} else {
			if (inputBuffer != NULL) {
				inputScrollPos = printTextBlock(50, 170, 3, 26, gsGlobal->Height - reservedEndOfDisplayY, inputBuffer, inputScrollPos);
			} else if (menu != NULL) {
				menu->paint();
			}
		}
	}
	if (enableDisc) {
		gsKit_fontm_print_scaled(gsGlobal, gsFont, 50, gsGlobal->Height - reservedEndOfDisplayY, 3, 0.8, TexBlack,
			"Loading, please wait...");
	} else {
		if (msg != NULL) {
			if (usePad) {
				gsKit_fontm_print_scaled(gsGlobal, gsFont, 50, gsGlobal->Height - reservedEndOfDisplayY, 3, 0.8, TexBlack,
					"Press CROSS to continue.");
			}
		} else {
			if (!isInfoBufferEmpty()) {
				if (usePad) {
					gsKit_fontm_print_scaled(gsGlobal, gsFont, 50, gsGlobal->Height - reservedEndOfDisplayY, 3, 0.8, TexBlack,
						"Press CROSS to continue.");
					gsKit_fontm_print_scaled(gsGlobal, gsFont, 50, gsGlobal->Height - reservedEndOfDisplayY + 18, 3, 0.8, TexBlack,
						"Use UP and DOWN to scroll.");
				}
			} else {
				if (inputBuffer != NULL) {
					gsKit_fontm_print_scaled(gsGlobal, gsFont, 50, gsGlobal->Height - reservedEndOfDisplayY, 3, 0.8, TexBlack,
						"Please use USB keyboard.");
					gsKit_fontm_print_scaled(gsGlobal, gsFont, 50, gsGlobal->Height - reservedEndOfDisplayY + 18, 3, 0.8, TexBlack,
						"Press CROSS to quit.");
				} else if (menu != NULL) {
					if (usePad) {
						gsKit_fontm_print_scaled(gsGlobal, gsFont, 50, gsGlobal->Height - reservedEndOfDisplayY, 3, 0.8, TexBlack,
							"Press CROSS to select menu.");
						gsKit_fontm_print_scaled(gsGlobal, gsFont, 50, gsGlobal->Height - reservedEndOfDisplayY + 18, 3, 0.8, TexBlack,
							"Use UP and DOWN to scroll.");
					}
				}
			}
		}
		if (!usePad) {
			gsKit_fontm_print_scaled(gsGlobal, gsFont, 50, gsGlobal->Height - reservedEndOfDisplayY, 3, 0.8, TexBlack,
				"Please wait...");
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

GSTEXTURE *getTexture(const char *filename)
{
	GSTEXTURE *tex = NULL;
	rom_entry_t *romfile;
	romfile = rom_getFile(filename);
	if (romfile != NULL) {
		tex = (GSTEXTURE *) malloc(sizeof(GSTEXTURE));
		if (tex != NULL) {
			u32 size;

			tex->Width = romfile->width;
			tex->Height = romfile->height;
			if (romfile->depth == 4) {
				tex->PSM = GS_PSM_CT32;
			} else {
				tex->PSM = GS_PSM_CT24;
			}
			tex->Mem = (u32 *) romfile->start;
			tex->Filter = GS_FILTER_LINEAR;

			size = gsKit_texture_size_ee(tex->Width, tex->Height, tex->PSM);
			if (size <= SMALL_TEXT_SIZE) {
				tex->Vram = gsKit_vram_alloc(gsGlobal, gsKit_texture_size(tex->Width, tex->Height, tex->PSM), GSKIT_ALLOC_USERBUFFER);
				if (tex->Vram != GSKIT_ALLOC_ERROR) {
					gsKit_texture_upload(gsGlobal, tex);
				} else {
					printf("Out of VRAM \"%s\".\n", filename);
					free(tex);
					error_printf("Out of VRAM while loading texture (%s).", filename);
					return NULL;
				}
			} else {
				tex->Vram = globalVram;
			}
		} else {
			error_printf("Out of memory while loading texture (%s).", filename);
		}
	} else {
		error_printf("Failed to open texture \"%s\".", filename);
	}
	return tex;
}

bool isNTSCMode(void)
{
	return (gsGlobal->Mode != GS_MODE_PAL);
}

/**
 * Initialize graphic screen.
 */
Menu *graphic_main(void)
{
	int i;
	int numberOfMenuItems;

	gsGlobal = gsKit_init_global();
	if (isNTSCMode()) {
		numberOfMenuItems = 7;
	} else {
		numberOfMenuItems = 8;
	}

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
	TexBlack = GS_SETREG_RGBAQ(0x00, 0x00, 0x00, 0x80, 0x00);

	gsGlobal->PrimAlphaEnable = GS_SETTING_ON;
	gsGlobal->ZBuffering = GS_SETTING_OFF;

	gsKit_init_screen(gsGlobal);

	gsFont = gsKit_init_fontm();
	if (gsKit_fontm_upload(gsGlobal, gsFont) != 0) {
		printf("Can't find any font to use\n");
		SleepThread();
	}

	globalVram = gsKit_vram_alloc(gsGlobal, MAX_TEX_SIZE, GSKIT_ALLOC_USERBUFFER);
	if (globalVram == GSKIT_ALLOC_ERROR) {
		printf("Failed to allocate texture buffer.\n");
		error_printf("Failed to allocate texture buffer.\n");
	}

	gsFont->Spacing = 0.8f;
	texFolder = getTexture("folder.rgb");
	texUp = getTexture("up.rgb");
	texBack = getTexture("back.rgb");
	texSelected = getTexture("selected.rgb");
	texUnselected = getTexture("unselected.rgb");
	texPenguin = getTexture("penguin.rgb");
	texDisc = getTexture("disc.rgb");
	texCloud = getTexture("cloud.rgb");

	menu = new Menu(gsGlobal, gsFont, numberOfMenuItems);
	menu->setPosition(50, 120);

	gsKit_mode_switch(gsGlobal, GS_ONESHOT);

	/* Activate graphic routines. */
	graphicInitialized = true;

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

GSTEXTURE *getTexFolder(void)
{
	return texFolder;
}

GSTEXTURE *getTexUp(void)
{
	return texUp;
}

GSTEXTURE *getTexBack(void)
{
	return texBack;
}

GSTEXTURE *getTexSelected(void)
{
	return texSelected;
}

GSTEXTURE *getTexUnselected(void)
{
	return texUnselected;
}

extern "C" {
	void setErrorMessage(const char *text) {
		if (errorMessage[writeMsgPos] == NULL) {
			errorMessage[writeMsgPos] = text;
			writeMsgPos = (writeMsgPos + 1) % MAX_MESSAGES;
		} else {
			printf("Error message queue is full at error:\n");
			printf("%s\n", text);
		}
	}

	void goToNextErrorMessage(void)
	{
		if (errorMessage[readMsgPos] != NULL) {
			errorMessage[readMsgPos] = NULL;
			readMsgPos = (readMsgPos + 1) % MAX_MESSAGES;
		}
	}

	const char *getErrorMessage(void) {
		return errorMessage[readMsgPos];
	}

	int error_printf(const char *format, ...)
	{
		static char buffer[MAX_MESSAGES][MAX_BUFFER];
		int ret;
		va_list varg;
		va_start(varg, format);

		if (errorMessage[writeMsgPos] == NULL) {
			ret = vsnprintf(buffer[writeMsgPos], MAX_BUFFER, format, varg);

			setErrorMessage(buffer[writeMsgPos]);

			if (graphicInitialized) {
				if (readMsgPos == writeMsgPos) {
					/* Show it before doing anything else. */
					graphic_paint();
				}
			}
		} else {
			printf("error_printf loosing message: %s\n", format);
			ret = -1;
		}

		va_end(varg);
		return ret;
	}

	void info_prints(const char *text)
	{
		int len = strlen(text) + 1;
		int remaining;

		if (len > MAX_INFO_BUFFER) {
			printf("info_prints(): text too long.\n");
			return;
		}

		remaining = MAX_INFO_BUFFER - infoBufferPos;
		if (len > remaining) {
			int required;
			int i;

			/* required space in buffer. */
			required = len - remaining;

			/* Find next new line. */
			for (i = required; i < MAX_INFO_BUFFER; i++) {
				if (infoBuffer[i] == '\n') {
					i++;
					break;
				}
			}

			if (i >= MAX_INFO_BUFFER) {
				/* Delete complete buffer, buffer doesn't have any carriage returns. */
				infoBufferPos = 0;
			} else {
				/* Scroll buffer and delete old stuff. */
				for (i = 0; i < (infoBufferPos - required); i++) {
					infoBuffer[i] = infoBuffer[required + i];
				}
				infoBufferPos = infoBufferPos - required;
			}
			infoBufferPos -= required;
		}
		strcpy(&infoBuffer[infoBufferPos], text);
		infoBufferPos += len - 1;
	}

	int info_printf(const char *format, ...)
	{
		int ret;
		static char buffer[MAX_BUFFER];
		va_list varg;
		va_start(varg, format);

		ret = vsnprintf(buffer, MAX_BUFFER, format, varg);
		info_prints(buffer);

		va_end(varg);
		return ret;
	}

	void setEnableDisc(int v)
	{
		enableDisc = v;

		/* Show it before doing anything else. */
		graphic_paint();
	}

	void scrollUpFast(void)
	{
		int i;

		for (i = 0; i < 8; i++) {
			scrollUp();
		}
	}

	void scrollUp(void)
	{
		if (inputBuffer != NULL) {
			inputScrollPos--;
			if (inputScrollPos < 0) {
				inputScrollPos = 0;
			}
		} else {
			scrollPos--;
			if (scrollPos < 0) {
				scrollPos = 0;
			}
		}
	}

	void scrollDownFast(void)
	{
		int i;

		for (i = 0; i < 8; i++) {
			scrollDown();
		}
	}

	void scrollDown(void)
	{
		if (inputBuffer != NULL) {
			inputScrollPos++;
		} else {
			scrollPos++;
		}
	}

	int getScrollPos(void)
	{
		return scrollPos;
	}

	int isInfoBufferEmpty(void)
	{
		return !(loaderConfig.enableEEDebug && (infoBufferPos > 0));
	}

	void clearInfoBuffer(void)
	{
		infoBuffer[0] = 0;
		scrollPos = 0;
		infoBufferPos = 0;

		if (graphicInitialized) {
			/* Show it before doing anything else. */
			graphic_paint();
		}
	}
	void enablePad(int val)
	{
		usePad = val;
	}

	void setInputBuffer(char *buffer)
	{
		inputBuffer = buffer;
	}

	char *getInputBuffer(void)
	{
		return inputBuffer;
	}

	void graphic_screenshot(void)
	{
		static int screenshotCounter = 0;
		char text[256];

#ifdef RESET_IOP
		snprintf(text, 256, "mass0:kloader%d.tga", screenshotCounter);
#else
		snprintf(text, 256, "host:kloader%d.tga", screenshotCounter);
#endif
		ps2_screenshot_file(text, gsGlobal->ScreenBuffer[gsGlobal->ActiveBuffer & 1],
			gsGlobal->Width, gsGlobal->Height / 2, gsGlobal->PSM);
		screenshotCounter++;

		/* Fix deadlock in gsKit. */
		gsGlobal->FirstFrame = GS_SETTING_ON;
	}

	void moveScreen(int dx, int dy)
	{
		gsGlobal->StartX += dx;
		gsGlobal->StartX &= 0xFFF;
		gsGlobal->StartY += dy;
		gsGlobal->StartY &= 0xFFF;
	
		GS_SET_DISPLAY1(gsGlobal->StartX,		// X position in the display area (in VCK unit
				gsGlobal->StartY,		// Y position in the display area (in Raster u
				gsGlobal->MagH,			// Horizontal Magnification
				gsGlobal->MagV,			// Vertical Magnification
				gsGlobal->DW - 1,	// Display area width
				gsGlobal->DH - 1);		// Display area height
	
		GS_SET_DISPLAY2(gsGlobal->StartX,		// X position in the display area (in VCK units)
				gsGlobal->StartY,		// Y position in the display area (in Raster units)
				gsGlobal->MagH,			// Horizontal Magnification
				gsGlobal->MagV,			// Vertical Magnification
				gsGlobal->DW - 1,	// Display area width
				gsGlobal->DH - 1);		// Display area height
	}

}



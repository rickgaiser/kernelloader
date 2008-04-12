/* Copyright (c) 2007 Mega Man */
#include <gsKit.h>

#include "config.h"
#include "graphic.h"
#include "loader.h"
#include "pad.h"
#include "modules.h"
#include "loadermenu.h"
#include "configuration.h"
#include "smod.h"
#include "libkbd.h"
#include "fileXio_rpc.h"

/**
 * Entry point for loader.
 * @param argc unused.
 * @param argv unsused.
 * @returns Function doesn't return.
 */
int main(int argc, char **argv)
{
	graphic_mode_t mode;
	Menu *menu;
	Menu *lastValidMenu = NULL;
	int new_pad;
	int paddata;
	int old_pad;
	const char *errorMessage = NULL;
	int i;

	if (*((char *) 0x1FC80000 - 0xAE) != 'E') {
		mode = MODE_NTSC;
	} else {
		mode = MODE_PAL;
	}

	/* Enable debug output at startup. */
	loaderConfig.enableEEDebug = 1;

	/* Setup graphic screen. */
	menu = graphic_main(mode);
	lastValidMenu = menu;

	/* Disable menu until start up. */
	setCurrentMenu(NULL);

	/* Show disc symbol while start up. */
	setEnableDisc(true);

	initMenu(menu, mode);

	loadLoaderModules();

	fileXioInit();

	setEnableDisc(false);

	initializeController();

	PS2KbdInit();

	setCurrentMenu(menu);

	enablePad(true);
	old_pad = 0;
	do {
		char key;

		graphic_paint();
		paddata = readPad(0);
		new_pad = paddata & ~old_pad;
		old_pad = paddata;

		if (PS2KbdRead(&key) > 0) {
			switch(key) {
				case 27:
					if (PS2KbdRead(&key) > 0) {
						switch(key) {
							case 41:
								new_pad |= PAD_RIGHT;
								break;
							case 42:
								new_pad |= PAD_LEFT;
								break;
							case 43:
								new_pad |= PAD_DOWN;
								break;
							case 44:
								new_pad |= PAD_UP;
								break;
							default:
								//info_printf("ESC Key %d \"%c\"\n", key, key);
								break;
						}
					}
					key = 0;
					break;
				case 10:
					new_pad |= PAD_CROSS;
					break;
				default:
					//info_printf("Key %d \"%c\"\n", key, key);
					break;
			}
		} else {
			key = 0;
		}
#ifdef SCREENSHOT
		if (new_pad & PAD_R1) {
			graphic_screenshot();
		}
#endif

		menu = getCurrentMenu();
		errorMessage = getErrorMessage();
		if (menu != NULL) {
			lastValidMenu = menu;
		}
		if (errorMessage != NULL) {
			if (new_pad & PAD_CROSS) {
				goToNextErrorMessage();
				errorMessage = getErrorMessage();
				if (errorMessage != NULL) {
					graphic_setPercentage(0, NULL);
					graphic_setStatusMessage(NULL);
				}
			}
		} else {
			if (isInfoBufferEmpty()) {
				char *buffer = getInputBuffer();
				if (buffer != NULL) {
					if (new_pad & PAD_DOWN) {
						scrollDown();
					} else if (new_pad & PAD_UP) {
						scrollUp();
					} else if (new_pad & PAD_LEFT) {
						scrollUpFast();
					} else if (new_pad & PAD_RIGHT) {
						scrollDownFast();
					}
					if (new_pad & PAD_CROSS) {
						setInputBuffer(NULL);
					} else {
						int pos;
						pos = strlen(buffer);
						if (key == 7) {
							pos--;
							if (pos >= 0) {
								buffer[pos] = 0;
							}
						} else if (key != 0) {
							if (pos < (MAX_INPUT_LEN - 1)) {
								buffer[pos] = key;
								buffer[pos + 1] = 0;
							}
						}
					}
				} else if (menu != NULL) {
					if (new_pad & PAD_UP) {
						menu->selectUp();
					} else if (new_pad & PAD_RIGHT) {
						for (i = 0; i < 8; i++) {
							menu->selectDown();
						}
					} else if (new_pad & PAD_LEFT) {
						for (i = 0; i < 8; i++) {
							menu->selectUp();
						}
					} else if (new_pad & PAD_DOWN) {
						menu->selectDown();
					} else if (new_pad & PAD_CROSS) {
						/* Deactivate menu, in case menu will print something. */
						setCurrentMenu(NULL);
						menu->execute();
						if (getCurrentMenu() == NULL) {
							/* Restore menu if not already changed by menu entry. */
							setCurrentMenu(menu);
						}
					}
				} else {
					setCurrentMenu(lastValidMenu);
				}
			} else {
				if (new_pad & PAD_DOWN) {
					scrollDown();
				} else if (new_pad & PAD_UP) {
					scrollUp();
				} else if (new_pad & PAD_LEFT) {
					scrollUpFast();
				} else if (new_pad & PAD_RIGHT) {
					scrollDownFast();
				} else if (new_pad & PAD_CROSS) {
					clearInfoBuffer();
				}
			}
		}
	} while (1);

	/* not reached! */
	return 0;
}

extern "C" {
	void waitForUser(void)
	{
		int new_pad;
		int paddata;
		int old_pad;
		smod_mod_info_t module;
		bool usePad;

		usePad = false;	
		if (smod_get_mod_by_name("padman", &module) != NULL) {
			/* Only old ROM version of padman is working. */
			if ((module.version >> 8) <= 1) {
				usePad = true;
			}
		}
		enablePad(usePad);

		if (usePad) {
			initializeController();
			old_pad = 0;
			do {
				graphic_paint();
				paddata = readPad(0);
				new_pad = paddata & ~old_pad;
				old_pad = paddata;

				if (new_pad & PAD_DOWN) {
					scrollDown();
				} else if (new_pad & PAD_UP) {
					scrollUp();
				} else if (new_pad & PAD_LEFT) {
					scrollUpFast();
				} else if (new_pad & PAD_RIGHT) {
					scrollDownFast();
				}
			} while (!(new_pad & PAD_CROSS));
	
			clearInfoBuffer();
			padEnd();
		} else {
			int i;
			int lastScrollPos;
			int scrollPos;

			i = 0;
			scrollPos = 0;
			lastScrollPos = -1;
			do {
				graphic_paint();
				i++;
				if ((i % 50) == 0) {
					scrollDown();
					lastScrollPos = scrollPos;
					scrollPos = getScrollPos();
				}
			} while (scrollPos != lastScrollPos);
			clearInfoBuffer();
		}
	}
};


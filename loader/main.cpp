/* Copyright (c) 2007 Mega Man */
#include <gsKit.h>
#include <gsFontM.h>

#include "config.h"
#include "graphic.h"
#include "loader.h"
#include "pad.h"
#include "modules.h"
#include "loadermenu.h"
#include "configuration.h"
#include "smod.h"
#include "libkbd.h"
#include "SMS_CDVD.h"
#include "SMS_CDDA.h"
#include "nvram.h"

#define KEY_F1 1
#define KEY_F2 2
#define KEY_F3 3
#define KEY_F4 4
#define KEY_F5 5
#define KEY_F6 6
#define KEY_F7 7
#define KEY_F8 8
#define KEY_F9 9
#define KEY_F10 10
#define KEY_F11 11
#define KEY_F12 12
#define KEY_PRINT_SCREEN 32
#define KEY_PAGE_UP 37
#define KEY_PAGE_DOWN 40
#define KEY_DELETE 38
#define KEY_HOME 36
#define KEY_END 39
#define KEY_BREAK 34
#define KEY_ESCAPE 27
#define KEY_RIGHT 41
#define KEY_LEFT 42
#define KEY_DOWN 43
#define KEY_UP 44

int debug_mode;

/**
 * Entry point for loader.
 * @param argc unused.
 * @param argv unsused.
 * @returns Function doesn't return.
 */
int main(int argc, char **argv)
{
	Menu *menu;
	Menu *lastValidMenu = NULL;
	int new_pad;
	int paddata;
	int old_pad;
	const char *errorMessage = NULL;
	int i;
	int refreshesPerSecond;
	int emulatedKey;

	debug_mode = -1;
	for (i = 0; i < argc; i++) {
		if (strcmp(argv[i], "-d") == 0) {
			/* Enter debug mode, activate ps2link. */
			debug_mode = 1;
		}
	}

	/* Disable debug output at startup. */
	loaderConfig.enableEEDebug = 0;

	/* Setup graphic screen. */
	menu = graphic_main();

	lastValidMenu = menu;

	/* Disable menu until start up. */
	setCurrentMenu(NULL);

	/* Show disc symbol while start up. */
	setEnableDisc(true);

	checkROMVersion();

	initMenu(menu);

	loadLoaderModules(debug_mode);

	setEnableDisc(false);

	initializeController();

	PS2KbdInit();

	setCurrentMenu(menu);

	enablePad(true);
	old_pad = 0;

	refreshesPerSecond = getModeFrequenzy();
	printf("argc %d\n", argc);
	for (i = 0; i < argc; i++) {
		printf("argv[%d] = %s\n", i, argv[i]);
	}

	if (loaderConfig.autoBootTime > 0) {
		int refreshCounter = 0;
		char key;

		do {
			int time;

			time = refreshCounter / refreshesPerSecond;
			time = loaderConfig.autoBootTime - time;
			graphic_auto_boot_paint(time);
			paddata = readPad(0);
			new_pad = paddata & ~old_pad;
			old_pad = paddata;

			if (PS2KbdRead(&key) > 0) {
				if (key == 27) {
					/* Remove escape code from queue. */
					PS2KbdRead(&key);
				}
				/* Stop auto boot. */
				break;
			}
			if (new_pad & 0xFFFF) {
				/* Stop auto boot if any button is pressed. */
				break;
			}
			/* Wait until timeis elapsed. autoBootTime is in seconds. */
			if (refreshCounter > (loaderConfig.autoBootTime * refreshesPerSecond)) {
				/* Deactivate menu, in case menu will print something. */
				setCurrentMenu(NULL);

				/* Execute default menu. */
				menu->execute();

				/* Change back to main menu. */
				/* In case it fails to load, go to menu. */
				setCurrentMenu(menu);
				break;
			}
			refreshCounter++;
		} while (true);
		old_pad = paddata;
	}
	emulatedKey = ' ';
	do {
		char key;
		int written;
		int functionKey;

		graphic_paint();
		paddata = readPad(0);
		new_pad = paddata & ~old_pad;
		old_pad = paddata;
		written = 0;
		functionKey = -1;

		if (PS2KbdRead(&key) > 0) {
			switch(key) {
				case 27:
					if (PS2KbdRead(&key) > 0) {
						functionKey = key;
						switch(functionKey) {
							case KEY_F1:
							case KEY_F2:
							case KEY_F3:
							case KEY_F4:
							case KEY_F5:
							case KEY_F6:
							case KEY_F7:
							case KEY_F8:
								setMode(functionKey - KEY_F1);
								changeMode();
								refreshesPerSecond = getModeFrequenzy();
								break;
							case KEY_RIGHT:
								new_pad |= PAD_RIGHT;
								break;
							case KEY_LEFT:
								new_pad |= PAD_LEFT;
								break;
							case KEY_DOWN:
								new_pad |= PAD_DOWN;
								break;
							case KEY_UP:
								new_pad |= PAD_UP;
								break;
							default:
								//info_printf("ESC Key %d \"%c\"\n", functionKey, functionKey);
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
		if ((new_pad & PAD_R3) || (functionKey == KEY_PRINT_SCREEN)) {
			graphic_screenshot();
		}
#endif
		if (new_pad & PAD_R2) {
			incrementMode();
			changeMode();
			refreshesPerSecond = getModeFrequenzy();
		} else if (new_pad & PAD_L2) {
			decrementMode();
			changeMode();
			refreshesPerSecond = getModeFrequenzy();
		}

		menu = getCurrentMenu();
		errorMessage = getErrorMessage();
		if (menu != NULL) {
			lastValidMenu = menu;
		}
		if (errorMessage != NULL) {
			if (new_pad & PAD_CROSS) {
				goToNextErrorMessage();
				errorMessage = getErrorMessage();
				if (errorMessage == NULL) {
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
						if (isWriteable()) {
							decCursorPos();
						} else {
							scrollUpFast();
						}
					} else if (new_pad & PAD_RIGHT) {
						if (isWriteable()) {
							incCursorPos();
						} else {
							scrollDownFast();
						}
					} else if (functionKey == KEY_PAGE_UP) {
						scrollUpFast();
					} else if (functionKey == KEY_PAGE_DOWN) {
						scrollDownFast();
					}
					if (new_pad & PAD_CROSS) {
						setInputBuffer(NULL, 0);
					} else {
						if (isWriteable()) {
							int pos;
							int len;
							char *c;

							written = 1;
							len = strlen(buffer);
							pos = getCursorPos();
							if (new_pad & PAD_TRIANGLE) {
								key = emulatedKey;
							} else if (new_pad & PAD_R1) {
								emulatedKey++;
								if (emulatedKey > 0x7E) {
									emulatedKey = 0x20;
								}
								setEmulatedKey(emulatedKey);
							} else if (new_pad & PAD_L1) {
								emulatedKey--;
								if (emulatedKey < 0x20) {
									emulatedKey = 0x7E;
								}
								setEmulatedKey(emulatedKey);
							} else if (new_pad & PAD_CIRCLE) {
								emulatedKey += 0x10;
								if (emulatedKey > 0x7E) {
									emulatedKey -= 0x7E - 0x20;
								}
								setEmulatedKey(emulatedKey);
							}
							if ((new_pad & PAD_SQUARE) || (key == 7)) {
								/* Delete character left to cursor. */
								if (pos > 0) {
									for (c = &buffer[pos - 1]; c[0] != 0; c++) {
										c[0] = c[1];
									}
									decCursorPos();
								}
							} else if (functionKey == KEY_DELETE) {
								/* Delete character right to cursor. */
								if (pos >= 0) {
									for (c = &buffer[pos]; c[0] != 0; c++) {
										c[0] = c[1];
									}
								}
							} else if ((functionKey == KEY_HOME) || (new_pad & PAD_START)) {
								homeCursorPos();
							} else if ((functionKey == KEY_END) || (new_pad & PAD_SELECT)) {
								endCursorPos();
							} else if (key != 0) {
								/* Get input from keyboard. */
								if (len < (MAX_INPUT_LEN - 1)) {
									for (c = &buffer[len + 1]; c >= &buffer[pos]; c--) {
										c[1] = c[0];
									}
									buffer[pos] = key;
									incCursorPos();
								}
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
				} else if ((new_pad & PAD_LEFT) || (functionKey == KEY_PAGE_UP)) {
					scrollUpFast();
				} else if ((new_pad & PAD_RIGHT) || (functionKey == KEY_PAGE_DOWN)) {
					scrollDownFast();
				} else if (new_pad & PAD_CROSS) {
					clearInfoBuffer();
				}
			}
		}
		if (!written) {
			if (key == '+') {
				incrementMode();
				changeMode();
				refreshesPerSecond = getModeFrequenzy();
			} else if (key == '-') {
				decrementMode();
				changeMode();
				refreshesPerSecond = getModeFrequenzy();
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
		if (smod_get_mod_by_name("padman", &module) != 0) {
#ifdef OLD_ROM_MODULES
			/* Only old ROM version of padman is working. */
			if ((module.version >> 8) <= 1) {
				usePad = true;
			}
#endif
#ifdef NEW_ROM_MODULES
			/* Only old ROM version of padman is working. */
			if ((module.version >> 8) > 1) {
				usePad = true;
			}
#endif
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
			deinitializeController();
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


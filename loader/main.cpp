
/* Copyright (c) 2007 Mega Man */
#include <gsKit.h>

#include "config.h"
#include "graphic.h"
#include "loader.h"
#include "pad.h"
#include "libmc.h"
#include "modules.h"
#include "fileXio_rpc.h"

int fsDir(void *arg);

loader_config_t loaderConfig;

static char kernelFilename[1024 + 10];
static char initrdFilename[1024 + 10];

static int *sbiosCallEnabled;

static const char *sbiosDescription[] = {
	"0 SB_GETVER",
	"1 SB_HALT",
	"2 SB_SETDVE",
	"3 SB_PUTCHAR",
	"4 SB_GETCHAR",
	"5 SB_SETGSCRT",
	"6 SB_SETRGBYC",
	"7 unknown",
	"8 unknown",
	"9 unknown",
	"10 unknown",
	"11 unknown",
	"12 unknown",
	"13 unknown",
	"14 unknown",
	"15 unknown",
	"16 SB_SIFINIT",
	"17 SB_SIFEXIT",
	"18 SB_SIFSETDMA",
	"19 SB_SIFDMASTAT",
	"20 SB_SIFSETDCHAIN",
	"21 unknown",
	"22 unknown",
	"23 unknown",
	"24 unknown",
	"25 unknown",
	"26 unknown",
	"27 unknown",
	"28 unknown",
	"29 unknown",
	"30 unknown",
	"31 unknown",
	"32 SB_SIFINITCMD",
	"33 SB_SIFEXITCMD",
	"34 SB_SIFSENDCMD",
	"35 SB_SIFCMDINTRHDLR",
	"36 SB_SIFADDCMDHANDLER",
	"37 SB_SIFREMOVECMDHANDLER",
	"38 SB_SIFSETCMDBUFFER",
	"39 unknown",
	"40 unknown",
	"41 unknown",
	"42 unknown",
	"43 unknown",
	"44 unknown",
	"45 unknown",
	"46 unknown",
	"47 unknown",
	"48 SB_SIFINITRPC",
	"49 SB_SIFEXITRPC",
	"50 SB_SIFGETOTHERDATA",
	"51 SB_SIFBINDRPC",
	"52 SB_SIFCALLRPC",
	"53 SB_SIFCHECKSTATRPC",
	"54 SB_SIFSETRPCQUEUE",
	"55 SB_SIFREGISTERRPC",
	"56 SB_SIFREMOVERPC",
	"57 SB_SIFREMOVERPCQUEUE",
	"58 SB_SIFGETNEXTREQUEST",
	"59 SB_SIFEXECREQUEST",
	"60 unknown",
	"61 unknown",
	"62 unknown",
	"63 unknown",
	"64 SBR_IOPH_INIT",
	"65 SBR_IOPH_ALLOC",
	"66 SBR_IOPH_FREE",
	"67 unknown",
	"68 unknown",
	"69 unknown",
	"70 unknown",
	"71 unknown",
	"72 unknown",
	"73 unknown",
	"74 unknown",
	"75 unknown",
	"76 unknown",
	"77 unknown",
	"78 unknown",
	"79 unknown",
	"80 SBR_PAD_INIT",
	"81 SBR_PAD_END",
	"82 SBR_PAD_PORTOPEN",
	"83 SBR_PAD_PORTCLOSE",
	"84 SBR_PAD_SETMAINMODE",
	"85 SBR_PAD_SETACTDIRECT",
	"86 SBR_PAD_SETACTALIGN",
	"87 SBR_PAD_INFOPRESSMODE",
	"88 SBR_PAD_ENTERPRESSMODE",
	"89 SBR_PAD_EXITPRESSMODE",
	"90 SB_PAD_READ",
	"91 SB_PAD_GETSTATE",
	"92 SB_PAD_GETREQSTATE",
	"93 SB_PAD_INFOACT",
	"94 SB_PAD_INFOCOMB",
	"95 SB_PAD_INFOMODE",
	"96 unknown",
	"97 unknown",
	"98 unknown",
	"99 unknown",
	"100 unknown",
	"101 unknown",
	"102 unknown",
	"103 unknown",
	"104 unknown",
	"105 unknown",
	"106 unknown",
	"107 unknown",
	"108 unknown",
	"109 unknown",
	"110 unknown",
	"111 unknown",
	"112 SBR_SOUND_INIT",
	"113 SBR_SOUND_END",
	"114 SB_SOUND_GREG",
	"115 SB_SOUND_SREG",
	"116 SBR_SOUND_GCOREATTR",
	"117 SBR_SOUND_SCOREATTR",
	"118 SBR_SOUND_TRANS",
	"119 SBR_SOUND_TRANSSTAT",
	"120 SBR_SOUND_TRANSCALLBACK",
	"121 unknown",
	"122 unknown",
	"123 SBR_SOUND_REMOTE",
	"124 unknown",
	"125 unknown",
	"126 unknown",
	"127 unknown",
	"128 unknown",
	"129 unknown",
	"130 unknown",
	"131 unknown",
	"132 unknown",
	"133 unknown",
	"134 unknown",
	"135 unknown",
	"136 unknown",
	"137 unknown",
	"138 unknown",
	"139 unknown",
	"140 unknown",
	"141 unknown",
	"142 unknown",
	"143 unknown",
	"144 SBR_MC_INIT",
	"145 SBR_MC_OPEN",
	"146 SBR_MC_MKDIR",
	"147 SBR_MC_CLOSE",
	"148 SBR_MC_SEEK",
	"149 SBR_MC_READ",
	"150 SBR_MC_WRITE",
	"151 SBR_MC_GETINFO",
	"152 SBR_MC_GETDIR",
	"153 SBR_MC_FORMAT",
	"154 SBR_MC_DELETE",
	"155 SBR_MC_FLUSH",
	"156 SBR_MC_SETFILEINFO",
	"157 SBR_MC_RENAME",
	"158 SBR_MC_UNFORMAT",
	"159 SBR_MC_GETENTSPACE",
	"160 SBR_MC_CALL",
	"161 unknown",
	"162 unknown",
	"163 unknown",
	"164 unknown",
	"165 unknown",
	"166 unknown",
	"167 unknown",
	"168 unknown",
	"169 unknown",
	"170 unknown",
	"171 unknown",
	"172 unknown",
	"173 unknown",
	"174 unknown",
	"175 unknown",
	"176 SBR_CDVD_INIT",
	"177 SBR_CDVD_RESET",
	"178 SBR_CDVD_READY",
	"179 SBR_CDVD_READ",
	"180 SBR_CDVD_STOP",
	"181 SBR_CDVD_GETTOC",
	"182 SBR_CDVD_READRTC",
	"183 SBR_CDVD_WRITERTC",
	"184 SBR_CDVD_MMODE",
	"185 SBR_CDVD_GETERROR",
	"186 SBR_CDVD_GETTYPE",
	"187 SBR_CDVD_TRAYREQ",
	"188 SB_CDVD_POWERHOOK",
	"189 SBR_CDVD_DASTREAM",
	"190 SBR_CDVD_READSUBQ",
	"191 SBR_CDVD_OPENCONFIG",
	"192 SBR_CDVD_CLOSECONFIG",
	"193 SBR_CDVD_READCONFIG",
	"194 SBR_CDVD_WRITECONFIG",
	"195 SBR_CDVD_RCBYCTL",
	"196 unknown",
	"197 unknown",
	"198 unknown",
	"199 unknown",
	"200 unknown",
	"201 unknown",
	"202 unknown",
	"203 unknown",
	"204 unknown",
	"205 unknown",
	"206 unknown",
	"207 unknown",
	"208 SBR_REMOCON_INIT",
	"209 SBR_REMOCON_END",
	"210 SBR_REMOCON_PORTOPEN",
	"211 SBR_REMOCON_PORTCLOSE",
	"212 SB_REMOCON_READ",
	"213 SBR_REMOCON2_INIT",
	"214 SBR_REMOCON2_END",
	"215 SBR_REMOCON2_PORTOPEN",
	"216 SBR_REMOCON2_PORTCLOSE",
	"217 SB_REMOCON2_READ",
	"218 SBR_REMOCON2_IRFEATURE"
};

static int numberOfSbiosCalls = sizeof(sbiosDescription) / sizeof(const char *);

void checkTGEandDebug(moduleEntry_t * module)
{
	if (loaderConfig.enableTGE) {
		if (module->rte) {
			module->load = 0;
		}
		if (module->tge) {
			if (loaderConfig.newModules) {
				/* Enable all new ROM modules */
				if (module->newmods) {
					module->load = 1;
				}
				if (module->oldmods) {
					module->load = 0;
				}
			} else {
				/* Enable all old ROM modules */
				if (module->oldmods) {
					module->load = 1;
				}
				if (module->newmods) {
					module->load = 0;
				}
			}
		}
	} else {
		if (module->rte) {
			module->load = 1;
		}
		if (module->tge) {
			module->load = 0;
		}
		if (module->newmods || module->oldmods) {
			module->load = 0;
		}
	}
	if (loaderConfig.enableDebug) {
		if (module->debug) {
			module->load = 1;
		}
	} else {
		if (module->debug) {
			module->load = 0;
		}
	}
}

int submitConfiguration(void *arg)
{
	int i;

	for (i = 0; i < getNumberOfModules(); i++) {
		moduleEntry_t *module;

		module = getModuleEntry(i);

		if (loaderConfig.enablePS2LINK) {
			if (module->ps2link == 1) {
				module->load = 0;
			} else if (module->ps2link == -1) {
				module->load = 1;

				/* Enable module if compatible with TGE/RTE. */
				checkTGEandDebug(module);
			} else {
				checkTGEandDebug(module);
			}
		} else {
			if (module->ps2link == 1) {
				module->load = 1;
			} else if (module->ps2link == -1) {
				module->load = 0;
			}
			/* Enable module if required for TGE/RTE or disable it if it is for
			 * the opposite (RTE/TGE). */
			checkTGEandDebug(module);
		}
	}

	return 0;
}

int enableAllModules(void * arg)
{
	int i;

	for (i = 0; i < getNumberOfModules(); i++) {
		moduleEntry_t *module;

		module = getModuleEntry(i);
		module->load = 1;
	}
	return 0;
}

int disableAllModules(void * arg)
{
	int i;

	for (i = 0; i < getNumberOfModules(); i++) {
		moduleEntry_t *module;

		module = getModuleEntry(i);
		module->load = 0;
	}
	return 0;
}

int enableAllSBIOSCalls(void *arg)
{
	int i;

	for (i = 0; i < numberOfSbiosCalls; i++) {
		sbiosCallEnabled[i] = 1;
	}
	return 0;
}

int disableAllSBIOSCalls(void *arg)
{
	int i;

	for (i = 0; i < numberOfSbiosCalls; i++) {
		sbiosCallEnabled[i] = 0;
	}
	return 0;
}

/** Set enabled and disabled SBIOS calls to default setup. */
int defaultSBIOSCalls(void *arg)
{
	int i;
	for (i = 0; i < numberOfSbiosCalls; i++) {
		if ((i >= 176) && (i <= 195)) {
			/* Disable CDVD, because of some problems. */
			sbiosCallEnabled[i] = 0;
		} else {
			sbiosCallEnabled[i] = 1;
		}
	}
	return 0;
}

#define MAX_ENTRIES 256
#define MAX_FILE_LEN 256
#define MAX_PATH_LEN 1024

typedef struct {
	char *target;
	Menu *mainMenu;
	Menu *menu;
	Menu *fileMenu;
	const char *menuName;
	const char *fsName;
	char fileName[MAX_PATH_LEN];
} fsRootParam_t;

typedef struct {
	fsRootParam_t *rootParam;
	char name[MAX_FILE_LEN];
} fsDirParam_t;

int fsFile(void *arg)
{
	fsDirParam_t *param = (fsDirParam_t *) arg;
	fsRootParam_t *rootParam = param->rootParam;

	strcat(rootParam->fileName, "/");
	strcat(rootParam->fileName, param->name);

	strcpy(rootParam->target, rootParam->fileName);
	printf("Filename is \"%s\"\n", rootParam->target);
	setCurrentMenu(rootParam->mainMenu);

	return 0;
}

static fsDirParam_t fsDirParam[MAX_ENTRIES];

void fsGenerateDirListMenu(fsRootParam_t *rootParam)
{
	int dirFd;
	iox_dirent_t dir;
	int rv;
	int i;

	dirFd = fileXioDopen(rootParam->fileName);
	if(dirFd >= 0) {
		i = 0;
		rootParam->fileMenu->deleteAll();
		rootParam->fileMenu->setTitle(rootParam->fileName);
		rootParam->fileMenu->addItem(rootParam->menu->getTitle(), setCurrentMenu, rootParam->menu);
		do {
			rv = fileXioDread(dirFd, &dir);
			if (rv > 0) {
				//printf("dir: %s 0x%08x\n", dir.name, dir.stat.mode);

				if (strcmp(dir.name, ".") != 0) {
					fsDirParam[i].rootParam = rootParam;
					strncpy(fsDirParam[i].name, dir.name, MAX_FILE_LEN);
					fsDirParam[i].name[MAX_FILE_LEN - 1] = 0;
					if (dir.stat.mode & 0x1000) {
						rootParam->fileMenu->addItem(fsDirParam[i].name, fsDir, &fsDirParam[i]);
					} else {
						rootParam->fileMenu->addItem(fsDirParam[i].name, fsFile, &fsDirParam[i]);
					}
					i++;
					if (i >= MAX_ENTRIES) {
						break;
					}
				}
			}
		} while(rv > 0);
		setCurrentMenu(rootParam->fileMenu);
	} else {
		error_printf("Error while reading directory \"%s\". Is medium inserted?", rootParam->fileName);
	}
}

int fsDir(void *arg)
{
	fsDirParam_t *param = (fsDirParam_t *) arg;
	fsRootParam_t *rootParam = param->rootParam;

	if (strcmp(param->name, "..") != 0) {
		strcat(rootParam->fileName, "/");
		strcat(rootParam->fileName, param->name);
	} else {
		char *p;

		p = strrchr(rootParam->fileName, '/');
		if (p != NULL) {
			 *p = 0;
		} else {
			/* Something strange happened, begin at root again. */
			strcpy(rootParam->fileName, rootParam->fsName);
		}
	}
	fsGenerateDirListMenu(rootParam);

	return 0;
}

int fsroot(void *arg)
{
	fsRootParam_t *rootParam = (fsRootParam_t *) arg;

	if (rootParam->fileMenu == NULL) {
		rootParam->fileMenu = rootParam->menu->getSubMenu(rootParam->menuName);
	}
	strcpy(rootParam->fileName, rootParam->fsName);
	fsGenerateDirListMenu(rootParam);

	return 0;
}
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
	int old_pad;
	int paddata;
	const char *errorMessage = NULL;
	int i;

	if (*((char *) 0x1FC80000 - 0xAE) != 'E') {
		mode = MODE_NTSC;
	} else {
		mode = MODE_PAL;
	}
	/* Setup graphic screen. */
	menu = graphic_main(mode);
	lastValidMenu = menu;

	loadLoaderModules();

	fileXioInit();

	menu->setTitle("Boot Menu");
	menu->addItem("Boot Current Config", loader, (void *) mode);

	Menu *linuxMenu = menu->addSubMenu("Select Kernel");
	linuxMenu->addItem(menu->getTitle(), setCurrentMenu, menu);

	kernelFilename[0] = 0;

	static fsRootParam_t kusbParam = {
		kernelFilename,
		menu,
		linuxMenu,
		NULL,
		"USB Memory Stick",
		"mass0:",
		""
	};
	linuxMenu->addItem(kusbParam.menuName, fsroot, (void *) &kusbParam);

	static fsRootParam_t kmc0Param = {
		kernelFilename,
		menu,
		linuxMenu,
		NULL,
		"Memory Card 1",
		"mc0:",
		""
	};
	linuxMenu->addItem(kmc0Param.menuName, fsroot, (void *) &kmc0Param);

	static fsRootParam_t kmc1Param = {
		kernelFilename,
		menu,
		linuxMenu,
		NULL,
		"Memory Card 2",
		"mc1:",
		""
	};
	linuxMenu->addItem(kmc1Param.menuName, fsroot, (void *) &kmc1Param);

	initrdFilename[0] = 0;
	Menu *initrdMenu = menu->addSubMenu("Select Initrd");
	initrdMenu->addItem(menu->getTitle(), setCurrentMenu, menu);

	kernelFilename[0] = 0;

	static fsRootParam_t usbParam = {
		initrdFilename,
		menu,
		initrdMenu,
		NULL,
		"USB Memory Stick",
		"mass0:",
		""
	};
	initrdMenu->addItem(usbParam.menuName, fsroot, (void *) &usbParam);

	static fsRootParam_t mc0Param = {
		initrdFilename,
		menu,
		initrdMenu,
		NULL,
		"Memory Card 1",
		"mc0:",
		""
	};
	initrdMenu->addItem(mc0Param.menuName, fsroot, (void *) &mc0Param);

	static fsRootParam_t mc1Param = {
		initrdFilename,
		menu,
		initrdMenu,
		NULL,
		"Memory Card 2",
		"mc1:",
		""
	};
	initrdMenu->addItem(mc1Param.menuName, fsroot, (void *) &mc1Param);

	/* Config menu */
	Menu *configMenu = menu->addSubMenu("Configuration Menu");

	configMenu->addItem(menu->getTitle(), setCurrentMenu, menu);
	loaderConfig.newModules = 0;
	configMenu->addCheckItem("New Modules", &loaderConfig.newModules);
	loaderConfig.enableTGE = 1;
#ifdef RTE
	configMenu->addCheckItem("Enable TGE (disable RTE)", &loaderConfig.enableTGE);
#endif
#ifdef PS2LINK
	loaderConfig.enablePS2LINK = 1;
#else
	loaderConfig.enablePS2LINK = 0;
#endif
	configMenu->addCheckItem("Enable PS2LINK (debug)",
		&loaderConfig.enablePS2LINK);
	loaderConfig.enableDebug = 1;
	configMenu->addCheckItem("Enable debug modules",
		&loaderConfig.enableDebug);
	configMenu->addItem("Submit above config", submitConfiguration, NULL);
	loaderConfig.enableSBIOSTGE = 1;
#ifdef RTE
	configMenu->addCheckItem("Use SBIOS from TGE (ow RTE)",
		&loaderConfig.enableSBIOSTGE);
#endif
	loaderConfig.enableDebug = 1;
	configMenu->addCheckItem("Enable debug modules",
		&loaderConfig.enableDebug);
	loaderConfig.enableDev9 = 0;
	configMenu->addCheckItem("Enable hard disc and network",
		&loaderConfig.enableDev9);

	/* Module Menu */
	Menu *moduleMenu;

	moduleMenu = configMenu->addSubMenu("Module List");
	moduleMenu->addItem(configMenu->getTitle(), setCurrentMenu, configMenu);
	moduleMenu->addItem("Enable all modules", enableAllModules, NULL);
	moduleMenu->addItem("Disable all modules", disableAllModules, NULL);

	/* Add list with all modules. */
	for (i = 0; i < getNumberOfModules(); i++) {
		moduleEntry_t *module;

		module = getModuleEntry(i);

		moduleMenu->addCheckItem(module->path, &module->load);
	}

	/* SBIOS Calls Menu */
	sbiosCallEnabled = (int *) malloc(numberOfSbiosCalls * sizeof(int));
	if (sbiosCallEnabled != NULL) {
		Menu *sbiosCallsMenu;
		sbiosCallsMenu = configMenu->addSubMenu("Enabled SBIOS Calls");
		sbiosCallsMenu->addItem(configMenu->getTitle(), setCurrentMenu, configMenu);
		sbiosCallsMenu->addItem("Enable all Calls", enableAllSBIOSCalls, NULL);
		sbiosCallsMenu->addItem("Disable all Calls", disableAllSBIOSCalls, NULL);
		sbiosCallsMenu->addItem("Set default", defaultSBIOSCalls, NULL);
		defaultSBIOSCalls(NULL);
		for (i = 0; i < numberOfSbiosCalls; i++) {
			sbiosCallsMenu->addCheckItem(sbiosDescription[i], &sbiosCallEnabled[i]);
		}
	} else {
		error_printf("Not enough memory.");
	}


	initializeController();

	old_pad = 0;
	do {
		graphic_paint();
		paddata = readPad(0);
		new_pad = paddata & ~old_pad;
		old_pad = paddata;

		menu = getCurrentMenu();
		errorMessage = getErrorMessage();
		if (menu != NULL) {
			lastValidMenu = menu;
		}
		if (errorMessage != NULL) {
			if (new_pad & PAD_CROSS) {
				setErrorMessage(NULL);
				graphic_setPercentage(0, NULL);
				graphic_setStatusMessage(NULL);
			}
		} else {
			if (menu != NULL) {
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
		}
	} while (1);				/* XXX: Detect menu selelction? */

	/* not reached! */
	return 0;
}

extern "C" {
	void disableSBIOSCalls(uint32_t *SBIOSCallTable)
	{
		int i;

		for (i = 0; i < numberOfSbiosCalls; i++) {
			/* Set all function entry pointers to 0 if SBIOS call is disabled. */
			if (!sbiosCallEnabled[i]) {
				SBIOSCallTable[i] = 0;
			}
		}
	}

	const char *getSBIOSFilename(void)
	{
		if (loaderConfig.enableSBIOSTGE) {
			return "host:TGE/sbios.elf";
		} else {
			return "host:RTE/sbios.elf";
		}
	}

	const char *getKernelFilename(void)
	{
		if (kernelFilename[0] != 0) {
			return kernelFilename;
		} else {
			return "host:kernel.elf";
		}
	}
}

const char *getInitRdFilename(void) {
	if (initrdFilename[0] == 0) {
		return NULL;
	} else {
		return initrdFilename;
	}
}

#if 0

/* Functions reuqired for libstdc++. */
char *setlocale(int category, const char *locale)
{
	return "C";
}

int atoi(const char *nptr)
{
	return strtol(nptr, NULL, 10);
}
#endif

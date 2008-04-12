/* Copyright (c) 2007 Mega Man */
#include <gsKit.h>

#include "config.h"
#include "graphic.h"
#include "loader.h"
#include "modules.h"
#include "fileXio_rpc.h"
#include "configuration.h"

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


static fsDirParam_t fsDirParam[MAX_ENTRIES];


loader_config_t loaderConfig;
static char kernelFilename[MAX_PATH_LEN];
static char initrdFilename[MAX_PATH_LEN];
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
int numberOfSbiosCalls = sizeof(sbiosDescription) / sizeof(const char *);

/** Default Linux parameter for PAL mode. */
const char commandline_pal[] = "crtmode=pal ramdisk_size=16384";
/** Default Linux parameter for NTSC mode. */
const char commandline_ntsc[] = "crtmode=ntsc ramdisk_size=16384";

char kernelParameter[MAX_INPUT_LEN];

static char ps2linkParams[3 * MAX_INPUT_LEN];
static char myIP[MAX_INPUT_LEN];
static char netmask[MAX_INPUT_LEN];
static char gatewayIP[MAX_INPUT_LEN];

/** Current graphic mode. */
graphic_mode_t graphicMode;

static unsigned int getStatusReg() {
	register unsigned int rv;
	asm volatile (
		"mfc0 %0, $12\n"
		"nop\n" : "=r"
	(rv) : );
	return rv;
}

static void setStatusReg(unsigned int v) {
	asm volatile (
		"mtc0 %0, $12\n"
		"nop\n"
	: : "r" (v) );
}


static void setKernelMode() {
	setStatusReg(getStatusReg() & (~0x18));
}

static void setUserMode() {
	setStatusReg((getStatusReg() & (~0x18)) | 0x10);
}

int poweroff(void *arg)
{
	printf("Try to power off.\n");
	graphic_paint();

	// Turn off PS2
	setKernelMode();
	
	*((volatile unsigned char *)0xBF402017) = 0;
	*((volatile unsigned char *)0xBF402016) = 0xF;
	
	setUserMode();
	while(1) {
		graphic_paint();
	}
}

int fsFile(void *arg)
{
	fsDirParam_t *param = (fsDirParam_t *) arg;
	fsRootParam_t *rootParam = param->rootParam;

	int len = strlen(rootParam->fileName) + strlen(param->name) + 2;
	if (len < MAX_PATH_LEN) {
		strcat(rootParam->fileName, "/");
		strcat(rootParam->fileName, param->name);

		strcpy(rootParam->target, rootParam->fileName);
		printf("Filename is \"%s\"\n", rootParam->target);
		setCurrentMenu(rootParam->mainMenu);
		return 0;
	} else  {
		error_printf("Filename is too long.\n");
		return -1;
	}
}

int fsDir(void *arg);

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

int enableAllModules(void *arg)
{
	int i;

	for (i = 0; i < getNumberOfModules(); i++) {
		moduleEntry_t *module;

		module = getModuleEntry(i);
		module->load = 1;
	}
	return 0;
}

int disableAllModules(void *arg)
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


void fsGenerateDirListMenu(fsRootParam_t * rootParam, bool isRoot)
{
	int dirFd;
	iox_dirent_t dir;
	int rv;
	int i;

	setEnableDisc(true);
	dirFd = fileXioDopen(rootParam->fileName);
	if (dirFd >= 0) {
		i = 0;
		rootParam->fileMenu->deleteAll();
		rootParam->fileMenu->setTitle(rootParam->fileName);
		rootParam->fileMenu->addItem(rootParam->menu->getTitle(),
			setCurrentMenu, rootParam->menu, getTexBack());
		do {
			rv = fileXioDread(dirFd, &dir);
			if (rv > 0) {
				//printf("dir: %s 0x%08x\n", dir.name, dir.stat.mode);

				if (strcmp(dir.name, ".") != 0) {
					fsDirParam[i].rootParam = rootParam;
					strncpy(fsDirParam[i].name, dir.name, MAX_FILE_LEN);
					fsDirParam[i].name[MAX_FILE_LEN - 1] = 0;
					if (dir.stat.mode & 0x1000) {
						GSTEXTURE *tex = NULL;

						if (strcmp(fsDirParam[i].name, "..") == 0) {
							tex = getTexUp();
							if (isRoot) {
								/* It is no good idea to add ".." to root
								 * directory.
								 */
								continue;
							}
						} else {
							tex = getTexFolder();
						}
						rootParam->fileMenu->addItem(fsDirParam[i].name, fsDir,
							&fsDirParam[i], tex);
					} else {
						rootParam->fileMenu->addItem(fsDirParam[i].name, fsFile,
							&fsDirParam[i]);
					}
					i++;
					if (i >= MAX_ENTRIES) {
						break;
					}
				}
			}
		} while (rv > 0);
		setCurrentMenu(rootParam->fileMenu);
		fileXioDclose(dirFd);
	} else {
		error_printf("Error while reading directory \"%s\". Is medium inserted?",
			rootParam->fileName);
	}
	setEnableDisc(false);
}

int fsDir(void *arg)
{
	fsDirParam_t *param = (fsDirParam_t *) arg;
	fsRootParam_t *rootParam = param->rootParam;

	if (strcmp(param->name, "..") != 0) {
		int len = strlen(rootParam->fileName) + strlen(param->name) + 2;
		if (len < MAX_PATH_LEN) {
			strcat(rootParam->fileName, "/");
			strcat(rootParam->fileName, param->name);
		} else {
			error_printf("Path is too long.\n");
			return -1;
		}
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
	fsGenerateDirListMenu(rootParam, false);

	return 0;
}

int fsroot(void *arg)
{
	fsRootParam_t *rootParam = (fsRootParam_t *) arg;

	if (rootParam->fileMenu == NULL) {
		rootParam->fileMenu = rootParam->menu->getSubMenu(rootParam->menuName);
	}
	strcpy(rootParam->fileName, rootParam->fsName);
	fsGenerateDirListMenu(rootParam, true);

	return 0;
}

int mcSaveConfig(void *arg)
{
	setEnableDisc(true);
	saveConfiguration();
	setEnableDisc(false);
	return 0;
}

int mcLoadConfig(void *arg)
{
	Menu *menu;

	menu = getCurrentMenu();
	setCurrentMenu(NULL);
	setEnableDisc(true);
	if (loadConfiguration() < 0) {
		error_printf("Failed to load configuration from MC0.");
	}
	setCurrentMenu(menu);
	setEnableDisc(false);
	return 0;
}

int unsetFilename(void *arg)
{
	char *fileName = (char *) arg;

	fileName[0] = 0;
	return 0;
}

int showFilename(void *arg)
{
	char *fileName = (char *) arg;

	info_prints(fileName);
	return 0;
}

int editString(void *arg)
{
	char *text = (char *) arg;

	setInputBuffer(text);
	return 0;
}

void setDefaultKernelParameter(char *text)
{
	/* Set commandline for correct video mode. */
	if(graphicMode == MODE_NTSC) {
		strcpy(text, commandline_ntsc);
	} else {
		strcpy(text, commandline_pal);
	}
}

int setDefaultKernelParameterMenu(void *arg)
{
	char *text = (char *) arg;

	setDefaultKernelParameter(text);
	editString(text);
}

void initMenu(Menu *menu, graphic_mode_t mode)
{
	int i;

	graphicMode = mode;
	setDefaultKernelParameter(kernelParameter);
	strcpy(myIP, "192.168.0.10");
	strcpy(netmask, "255.255.255.0");
	strcpy(gatewayIP, "192.168.0.1");

	addConfigTextItem("KernelParameter", kernelParameter, MAX_INPUT_LEN);
	addConfigTextItem("ps2linkMyIP", myIP, MAX_INPUT_LEN);
	addConfigTextItem("ps2linkNetmask", netmask, MAX_INPUT_LEN);
	addConfigTextItem("ps2linkGatewayIP", gatewayIP, MAX_INPUT_LEN);

	menu->setTitle("Boot Menu");
	menu->addItem("Boot Current Config", loader, (void *) mode);
	menu->addItem("Save Current Config", mcSaveConfig, NULL);
	menu->addItem("Load Config", mcLoadConfig, NULL);

	Menu *linuxMenu = menu->addSubMenu("Select Kernel");

	linuxMenu->addItem(menu->getTitle(), setCurrentMenu, menu, getTexBack());

	kernelFilename[0] = 0;
	addConfigTextItem("KernelFileName", kernelFilename, MAX_PATH_LEN);
	linuxMenu->addItem("Show Filename", showFilename, (void *) &kernelFilename);
	linuxMenu->addItem("Edit Filename", editString, (void *) &kernelFilename);
	linuxMenu->addItem("Example Kernel", unsetFilename, (void *) &kernelFilename);

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
#if !defined(RESET_IOP) || defined(PS2LINK)
	static fsRootParam_t khostParam = {
		kernelFilename,
		menu,
		linuxMenu,
		NULL,
		"Host",
		"host:",
		""
	};
	linuxMenu->addItem(khostParam.menuName, fsroot, (void *) &khostParam);
#endif

	Menu *initrdMenu = menu->addSubMenu("Select Initrd");

	initrdMenu->addItem(menu->getTitle(), setCurrentMenu, menu, getTexBack());
	initrdFilename[0] = 0;
	addConfigTextItem("InitrdFileName", initrdFilename, MAX_PATH_LEN);
	initrdMenu->addItem("Show Filename", showFilename, (void *) &initrdFilename);
	initrdMenu->addItem("Edit Filename", editString, (void *) &initrdFilename);
	initrdMenu->addItem("Disable Initrd", unsetFilename, (void *) &initrdFilename);

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
#if !defined(RESET_IOP) || defined(PS2LINK)
	static fsRootParam_t hostParam = {
		initrdFilename,
		menu,
		initrdMenu,
		NULL,
		"Host",
		"host:",
		""
	};
	initrdMenu->addItem(hostParam.menuName, fsroot, (void *) &hostParam);
#endif

	/* Config menu */
	Menu *configMenu = menu->addSubMenu("Configuration Menu");

	configMenu->addItem(menu->getTitle(), setCurrentMenu, menu, getTexBack());
	configMenu->addItem("Edit Kernel Parameter", editString, kernelParameter);
	configMenu->addItem("Default Kernel Parameter", setDefaultKernelParameterMenu, kernelParameter);
	loaderConfig.newModules = 0;
	configMenu->addCheckItem("New Modules", &loaderConfig.newModules);
	loaderConfig.enableTGE = 1;
#ifdef RTE
	configMenu->addCheckItem("Enable TGE (disable RTE)",
		&loaderConfig.enableTGE);
#endif
#ifdef PS2LINK
	loaderConfig.enablePS2LINK = 1;
#else
	loaderConfig.enablePS2LINK = 0;
#endif
	configMenu->addCheckItem("Enable PS2LINK (debug)",
		&loaderConfig.enablePS2LINK);
	loaderConfig.enableDebug = 1;
	configMenu->addCheckItem("Enable debug modules", &loaderConfig.enableDebug);
	configMenu->addItem("Submit above config", submitConfiguration, NULL);
	loaderConfig.enableSBIOSTGE = 1;
#ifdef RTE
	configMenu->addCheckItem("Use SBIOS from TGE (ow RTE)",
		&loaderConfig.enableSBIOSTGE);
#endif
	loaderConfig.newModulesInTGE = 0;
	configMenu->addCheckItem("TGE SBIOS for New Modules", &loaderConfig.newModulesInTGE);
	loaderConfig.enableDev9 = 0;
	configMenu->addCheckItem("Enable hard disc and network",
		&loaderConfig.enableDev9);
	configMenu->addCheckItem("Enable IOP debug output", &loaderConfig.enableEEDebug);

	/* Module Menu */
	Menu *moduleMenu;

	moduleMenu = configMenu->addSubMenu("Module List");
	moduleMenu->addItem(configMenu->getTitle(), setCurrentMenu, configMenu,
		getTexBack());
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
		sbiosCallsMenu->addItem(configMenu->getTitle(), setCurrentMenu,
			configMenu, getTexBack());
		sbiosCallsMenu->addItem("Enable all Calls", enableAllSBIOSCalls, NULL);
		sbiosCallsMenu->addItem("Disable all Calls", disableAllSBIOSCalls,
			NULL);
		sbiosCallsMenu->addItem("Set default", defaultSBIOSCalls, NULL);
		defaultSBIOSCalls(NULL);
		for (i = 0; i < numberOfSbiosCalls; i++) {
			sbiosCallsMenu->addCheckItem(sbiosDescription[i],
				&sbiosCallEnabled[i]);
		}
	} else {
		error_printf("Not enough memory.");
	}
	menu->addItem("Power off", poweroff, NULL);

	/* PS2LINK debug entries. */
	Menu *ps2linkMenu = configMenu->addSubMenu("PS2LINK Options");
	ps2linkMenu->addItem(configMenu->getTitle(), setCurrentMenu, configMenu,
		getTexBack());
	ps2linkMenu->addItem("Set IP address", editString, myIP);
	ps2linkMenu->addItem("Set Netmask", editString, netmask);
	ps2linkMenu->addItem("Set Gateway IP address", editString, gatewayIP);
}

extern "C" {
	void disableSBIOSCalls(uint32_t * SBIOSCallTable) {
		int i;

		for (i = 0; i < numberOfSbiosCalls; i++) {
			/* Set all function entry pointers to 0 if SBIOS call is disabled. */
			if (!sbiosCallEnabled[i]) {
				SBIOSCallTable[i] = 0;
			}
		}
	}

	const char *getSBIOSFilename(void) {
		if (loaderConfig.enableSBIOSTGE) {
			if (loaderConfig.newModulesInTGE) {
				return "host:TGE/sbios_new.elf";
			} else {
				return "host:TGE/sbios_old.elf";
			}
		} else {
			return "host:RTE/sbios.elf";
		}
	}

	const char *getKernelFilename(void) {
		if (kernelFilename[0] != 0) {
			return kernelFilename;
		} else {
			return "host:kernel.elf";
		}
	}

	const char *getInitRdFilename(void) {
		if (initrdFilename[0] == 0) {
			return NULL;
		} else {
			return initrdFilename;
		}
	}

	char *getKernelParameter(void)
	{
		return kernelParameter;
	}

	const char *getPS2MAPParameter(int *len) {

		*len = 0;
		strcpy(&ps2linkParams[*len], myIP);
		*len += strlen(&ps2linkParams[*len]) + 1;
		strcpy(&ps2linkParams[*len], netmask);
		*len += strlen(&ps2linkParams[*len]) + 1;
		strcpy(&ps2linkParams[*len], gatewayIP);
		*len += strlen(&ps2linkParams[*len]) + 1;

		return ps2linkParams;
	}
}

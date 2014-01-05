/* Copyright (c) 2007-2010 Mega Man */
#include <gsKit.h>
#include <loadfile.h>
#include <kernel.h>
#include <sifrpc.h>
#include <iopcontrol.h>
#include <iopheap.h>
#include <iopcontrol.h>
#include <unistd.h>

#include "config.h"
#include "graphic.h"
#include "loader.h"
#include "modules.h"
#include "fileXio_rpc.h"
#include "configuration.h"
#include "pad.h"
#include "SMS_CDVD.h"
#include "SMS_CDDA.h"
#include "getrte.h"
#include "getsbios.h"
#include "getelf.h"
#include "libkbd.h"
#include "nvram.h"
#include "kprint.h"

#define MAX_ENTRIES 256
#define MAX_FILE_LEN 256
#define MAX_PATH_LEN MAX_INPUT_LEN
#define EXAMPLE_KERNEL "host:kernel.elf"

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

typedef struct {
	const char *commandline;
	char *text;
	int size;
	const char *param;
} kernel_param_t;

static fsDirParam_t fsDirParam[MAX_ENTRIES];


loader_config_t loaderConfig;
static char configfile[MAX_PATH_LEN];
static char kernelFilename[MAX_PATH_LEN];
static char initrdFilename[MAX_PATH_LEN];
static int *sbiosCallEnabled = NULL;
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
	"15 SB_REGISTER_DEBUG_CALLBACK", /* Added by TGE for debug puspose, not used in RTE. */
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
	"196 SBR_CDVD_READ_DVD", /* Added by TGE. */
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
const char commandline_pal[] = "crtmode=pal1 xmode=PAL video=ps2fb:pal,640x480-32";
/** Default Linux parameter for NTSC mode. */
const char commandline_ntsc[] = "crtmode=ntsc1 xmode=NTSC video=ps2fb:ntsc,640x448-32";
/** Default Linux parameter for VGA mode. */
const char commandline_vga60[] = "crtmode=vesa0,60 xmode=VESA,1024x768x24 video=ps2fb:vesa,1024x768-16@60";
const char commandline_vga72[] = "crtmode=vesa0 xmode=VESA,1024x768x24 video=ps2fb:vesa,1024x768-16@72";
const char commandline_vga75[] = "crtmode=vesa0,75 xmode=VESA,1024x768x24 video=ps2fb:vesa,1024x768-16@75";
const char commandline_vga85[] = "crtmode=vesa0,75 xmode=VESA,1024x768x24 video=ps2fb:vesa,1024x768-16@85";
/** Default Linux parameter for HDTV mode. */
const char commandline_dtv[] = "crtmode=dtv2 xmode=dtv,720p video=ps2fb:dtv,720x480-32";
/** Default kernel parameter for ramdisk. */
const char commandline_ramdisk[] = "ramdisk_size=16384";

char kernelParameter[MAX_INPUT_LEN];
char videoParameter[MAX_INPUT_LEN];
char fullKernelParameter[sizeof(kernelParameter) + sizeof(videoParameter)];

char pcicType[MAX_INPUT_LEN];

static char ps2linkParams[3 * MAX_INPUT_LEN];
static char myIP[MAX_INPUT_LEN];
static char netmask[MAX_INPUT_LEN];
static char gatewayIP[MAX_INPUT_LEN];
static char dnsIP[MAX_INPUT_LEN];

static kernel_param_t hda1Config = {
	"root=/dev/hda1",
	kernelParameter,
	sizeof(kernelParameter),
	NULL,
};

static kernel_param_t hda2Config = {
	"root=/dev/hda2",
	kernelParameter,
	sizeof(kernelParameter),
	NULL,
};

static kernel_param_t hda3Config = {
	"root=/dev/hda3",
	kernelParameter,
	sizeof(kernelParameter),
	NULL,
};

static kernel_param_t nfsConfig = {
	"ip=dhcp root=/dev/nfs nfsroot=%s:/ps2root,tcp",
	kernelParameter,
	sizeof(kernelParameter),
	gatewayIP,
};

/** Parameter for IOP reset. */
static char s_pUDNL   [] __attribute__(   (  section( ".data" ), aligned( 1 )  )   ) = "rom0:UDNL rom0:EELOADCNF";

/** Text for auto boot menu. */
const char *autoBootText[] = {
	"Auto Boot: off",
	"Auto Boot Time: 1 Second",
	"Auto Boot Time: 2 Seconds",
	"Auto Boot Time: 3 Seconds",
	"Auto Boot Time: 4 Seconds",
	"Auto Boot Time: 5 Seconds",
	"Auto Boot Time: 6 Seconds",
	"Auto Boot Time: 7 Seconds",
	"Auto Boot Time: 8 Seconds",
	"Auto Boot Time: 9 Seconds",
	"Auto Boot Time: 10 Seconds",
	NULL };

static char libsd_version[16] = "none";


static unsigned int getStatusReg() {
	register unsigned int rv;
	asm volatile (
		"sync.p\n"
		"mfc0 %0, $12\n"
		"nop\n" : "=r"
	(rv) : );
	return rv;
}

static void setStatusReg(unsigned int v) {
	asm volatile (
		"mtc0 %0, $12\n"
		"sync.p\n"
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
	arg = arg;

	kprintf("Try to power off.\n");
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

int reboot(void *arg)
{
	arg = arg;

	kprintf("Try to reboot.\n");
	graphic_paint();

	deinitializeController();

	if (isDVDVSupported()) {
		graphic_setStatusMessage("Stopping DVD");

		/* Stop CD/DVD. */
		CDVD_Stop();
		CDVD_FlushCache();

		CDDA_Exit();
	}

	graphic_setStatusMessage("Exit IOP Heap");
	SifExitIopHeap();
	graphic_setStatusMessage("Exit LoadFile");
	SifLoadFileExit();
	graphic_setStatusMessage("Exit FIO");
	fioExit();
	graphic_setStatusMessage("Exit RPC");
	SifExitRpc();
	graphic_setStatusMessage("Stop DMA");
	SifStopDma();
	graphic_setStatusMessage("PreReset Init RPC");
	SifInitRpc(0);

	graphic_setStatusMessage("Reseting IOP");
	while(!SifIopReset(s_pUDNL, 0));

	graphic_setStatusMessage("IOP Sync");
	while(!SifIopSync());

	graphic_setStatusMessage("Initialize RPC");
	SifInitRpc(0);
	SifLoadModule("rom0:SIO2MAN", 0, NULL);
	graphic_setStatusMessage("Exit RPC 2");
	SifExitRpc();
	graphic_setStatusMessage("Stop DMA 2");
	SifStopDma();

	graphic_setStatusMessage("Reboot");
	/* Return to the PS2 browser. */
	LoadExecPS2("", 0, NULL);

	/* not reached. */
	return 0;
}

int fsFile(void *arg)
{
	fsDirParam_t *param = (fsDirParam_t *) arg;
	fsRootParam_t *rootParam = param->rootParam;

	int len = strlen(rootParam->fileName) + strlen(param->name) + 2;
	if (len < MAX_PATH_LEN) {
		int i;
#if 0
		/* XXX: strcat() seems to have a bug. */
		strcat(rootParam->fileName, "/");
		strcat(rootParam->fileName, param->name);
#else
		i = strlen(rootParam->fileName);
		rootParam->fileName[i] = '/';
		i++;
		strcpy(&rootParam->fileName[i], param->name);
		i += strlen(param->name);
		rootParam->fileName[i] = 0;
#endif

		strcpy(rootParam->target, rootParam->fileName);
		kprintf("Filename is \"%s\"\n", rootParam->fileName);
		if (strcmp(rootParam->fsName, "cdfs:") == 0) {
			/* Stop CD when finished. */
			CDVD_Stop();
			CDVD_FlushCache();
		}
		setCurrentMenu(rootParam->mainMenu);
		return 0;
	} else  {
		error_printf("Filename is too long.\n");
		return -1;
	}
}

static int removefile(void *arg)
{
	const char *filename = (const char *) arg;
	int rv;

	rv = fioRemove(filename);
	if (rv == 0) {
		/* Bug in ROM FILEIO/XFILEIO drivers.
		 * break; statement is missing at the end of the case for fioRemove
		 * so it continues on to the next case, which happens to be mkdir.
		 * So we need to delete the create directory.
		 */
		fioRmdir(filename);
		return 0;
	} else {
		error_printf("Failed to delete file %s (rv = %d)\n",
			filename, rv);
		return -1;
	}

}

int fsDir(void *arg);

int enableAllSBIOSCalls(void *arg)
{
	int i;

	arg = arg;

	for (i = 0; i < numberOfSbiosCalls; i++) {
		sbiosCallEnabled[i] = 1;
	}
	return 0;
}

int disableAllSBIOSCalls(void *arg)
{
	int i;

	arg = arg;

	for (i = 0; i < numberOfSbiosCalls; i++) {
		sbiosCallEnabled[i] = 0;
	}
	return 0;
}

/** Set enabled and disabled SBIOS calls to default setup for RTE. */
extern "C" {
	int defaultSBIOSCalls(void *arg)
	{
		int i;
	
		arg = arg;
	
		for (i = 0; i < numberOfSbiosCalls; i++) {
			if ((i >= 176) && (i <= 196)) {
				/* Disable CDVD, because of some problems. */
				sbiosCallEnabled[i] = 0;
			} else {
				sbiosCallEnabled[i] = 1;
			}
		}
		return 0;
	}
}

int setCurrentMenuAndStopCDVD(void *arg)
{
	if (isDVDVSupported()) {
		CDVD_Stop();
		CDVD_FlushCache();
	}
	setCurrentMenu(arg);

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
		if (strcmp(rootParam->fsName, "cdfs:") == 0) {
			/* CD must be stopped after accessing it. */
			rootParam->fileMenu->addItem(rootParam->menu->getTitle(),
				setCurrentMenuAndStopCDVD, rootParam->menu, getTexBack());
		} else {
			rootParam->fileMenu->addItem(rootParam->menu->getTitle(),
				setCurrentMenu, rootParam->menu, getTexBack());
		}
		do {
			rv = fileXioDread(dirFd, &dir);
			if (rv > 0) {
				//kprintf("dir: %s 0x%08x\n", dir.name, dir.stat.mode);

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
	if (isDVDVSupported()) {
		if (strcmp(rootParam->fsName, "cdfs:") == 0) {
			DiskType type;
	
			type = CDDA_DiskType();
	
			if (type == DiskType_DVDV) {
				CDVD_SetDVDV(1);
			} else {
				CDVD_SetDVDV(0);
			}
		}
	}
	fsGenerateDirListMenu(rootParam, true);

	return 0;
}

int mcSaveConfig(void *arg)
{
	const char *configfile = (char *) arg;

	setEnableDisc(true);
	saveConfiguration(configfile);
	setEnableDisc(false);
	return 0;
}

int mcLoadConfig(void *arg)
{
	Menu *menu;
	const char *configfile = (char *) arg;

	menu = getCurrentMenu();
	setCurrentMenu(NULL);
	setEnableDisc(true);

	if (isDVDVSupported()) {
		if (strncmp(configfile, "cdfs:", 5) == 0) {
			DiskType type;
	
			type = CDDA_DiskType();
	
			if (type == DiskType_DVDV) {
				CDVD_SetDVDV(1);
			} else {
				CDVD_SetDVDV(0);
			}
		}
	}

	if (loadConfiguration(configfile) < 0) {
		error_printf("Failed to load configuration \"%s\".", configfile);
	}
	if (isDVDVSupported()) {
		if (strncmp(configfile, "cdfs:", 5) == 0) {
			/* Stop CD when finished. */
			CDVD_Stop();
			CDVD_FlushCache();
		}
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

int setExampleKernel(void *arg)
{
	char *fileName = (char *) arg;

	strcpy(fileName, EXAMPLE_KERNEL);
	return 0;
}

int showText(void *arg)
{
	char *text = (char *) arg;

#if 0
	info_prints(text);
#else
	setInputBuffer(text, 0);
#endif
	return 0;
}

int editString(void *arg)
{
	char *text = (char *) arg;

	setInputBuffer(text, -1);
	return 0;
}

void setDefaultVideoParameter(char *text)
{
	switch(getCurrentMode()) {
	case GS_MODE_VGA_640_60:
		strcpy(text, commandline_vga60);
		break;

	case GS_MODE_VGA_640_72:
		strcpy(text, commandline_vga72);
		break;

	case GS_MODE_VGA_640_75:
		strcpy(text, commandline_vga75);
		break;

	case GS_MODE_VGA_640_85:
		strcpy(text, commandline_vga85);
		break;

	case GS_MODE_DTV_480P:
		strcpy(text, commandline_dtv);
		break;

	case GS_MODE_NTSC:
		strcpy(text, commandline_ntsc);
		break;

	case GS_MODE_PAL:
		strcpy(text, commandline_pal);
		break;

	default:
		if (ps2_rom_version[4] != 'E') {
			strcpy(text, commandline_ntsc);
		} else {
			strcpy(text, commandline_pal);
		}
		break;
	}
}

extern "C" {
	void configureVideoParameter(void)
	{
		setDefaultVideoParameter(videoParameter);
	}
}

void setDefaultKernelParameter(char *text)
{
	strcpy(text, commandline_ramdisk);
}

int setDefaultKernelParameterMenu(void *arg)
{
	char *text = (char *) arg;

	setDefaultKernelParameter(text);
	editString(text);

	return 0;
}

int setParameterMenu(void *arg)
{
	kernel_param_t *param = (kernel_param_t *) arg;

	snprintf(param->text, param->size, param->commandline, param->param);
	editString(param->text);

	return 0;
}

int setDefaultVideoParameterMenu(void *arg)
{
	char *text = (char *) arg;

	setDefaultVideoParameter(text);
	editString(text);

	return 0;
}

#ifdef __cplusplus
extern "C" {
#endif
int setDefaultConfiguration(void *arg)
{
	int moduletype;
	int slim;
	int i;
	int version;

	(void) arg;

	setDefaultVideoParameter(videoParameter);
	strcpy(myIP, "192.168.0.10");
	strcpy(netmask, "255.255.255.0");
	strcpy(gatewayIP, "192.168.0.1");
	strcpy(dnsIP, "192.168.0.1");
	strcpy(pcicType, "");
	strcpy(configfile, CONFIG_FILE);

	loaderConfig.enableSBIOSTGE = 1;
	if (debug_mode == -1) {
		loaderConfig.enableDev9 = 1;
	} else {
		/* Hard disc and network not working with ps2link. */
		loaderConfig.enableDev9 = 0;
	}
	loaderConfig.enableEEDebug = 0;
	loaderConfig.autoBootTime = 0;
	loaderConfig.patchLibsd = 0;

	if (sbiosCallEnabled == NULL) {
		sbiosCallEnabled = (int *) malloc(numberOfSbiosCalls * sizeof(int));
	}
	if (sbiosCallEnabled != NULL) {
		if (do_default_sbios_calls) {
			defaultSBIOSCalls(NULL);
		} else {
			enableAllSBIOSCalls(NULL);
		}
	} else {
		error_printf("Not enough memory.");
	}

	if (isSlimPSTwo() && (debug_mode == -1)) {
		/* Value for slim PSTwo. */
		moduletype = 1;
		/* New modules seems to be more stable on slim on heavy USB use. */
		loaderConfig.newModulesInTGE = 1;
		/* Load newer CDVDMAN module on IOP reset. */
		strcpy(iop_reset_param, "rom0:UDNL rom0:OSDCNF");
	} else {
		/* Value for fat PS2. */
		moduletype = -1;
		loaderConfig.newModulesInTGE = 0;
		/* Load old CDVDMAN module on IOP reset. */
		strcpy(iop_reset_param, "rom0:UDNL");
	}
	if (isSlimPSTwo()) {
		slim = 1;
	} else {
		slim = -1;
	}

	for (i = 0; i < getNumberOfModules(); i++) {
		moduleEntry_t *module;


		module = getModuleEntry(i);

		if (module->debug_mode) {
			if (module->debug_mode != debug_mode) {
				module->load = 0;

				/* Don't load modules which are not compatible with ps2link. */
				continue;
			}
		}
		switch (module->defaultmod)
		{
		case 1: /* Default module depending whether new or old modules used. */
			if ((module->slim == 0) || (module->slim == moduletype)) {
				module->load = 1;
			} else {
				module->load = 0;
			}
			break;

		case 2: /* Default module depending whether slim or fat PS2. */
			if ((module->slim == 0) || (module->slim == slim)) {
				module->load = 1;
			} else {
				module->load = 0;
			}
			break;

		default:
			module->load = 0;
			break;
		}
		if (module->sound) {
			if (get_libsd_version() <= 0x104) {
				module->load = 1;
			} else {
				module->load = 0;
			}
		}
	}

	/* This not a default configuration, but the version is known at this point,
	 * so it can be updated here
	 */
	version = get_libsd_version();
	if (version < 0x10000) {
		snprintf(libsd_version, sizeof(libsd_version),
			"%u.%u%u", 
			(version >> 8) & 0xf,
			(version >> 4) & 0xf,
			(version >> 0) & 0xf);
	} else {
		strcpy(libsd_version, "none");
	}

	return 0;
}
#ifdef __cplusplus
}
#endif

int reloadModules(void *arg)
{
	(void) arg;

	if (isDVDVSupported()) {
		/* Always stop CD/DVD when an error happened. */
		CDVD_Stop();
		CDVD_FlushCache();

		CDDA_Exit();
	}

	PS2KbdClose();
	deinitializeController();

	/* Show disc symbol while start up. */
	setEnableDisc(true);

	loadLoaderModules(debug_mode, disable_cdrom);

	setEnableDisc(false);

	initializeController();

	PS2KbdInit();
	return 0;
}

void initMenu(Menu *menu)
{
	int i;

	setDefaultConfiguration(NULL);
	setDefaultKernelParameter(kernelParameter);

	addConfigTextItem("KernelParameter", kernelParameter, MAX_INPUT_LEN);
	addConfigTextItem("VideoParameter", videoParameter, MAX_INPUT_LEN);
	addConfigTextItem("ps2linkMyIP", myIP, MAX_INPUT_LEN);
	addConfigTextItem("ps2linkNetmask", netmask, MAX_INPUT_LEN);
	addConfigTextItem("ps2linkGatewayIP", gatewayIP, MAX_INPUT_LEN);
	addConfigTextItem("ps2DNSIP", dnsIP, MAX_INPUT_LEN);
	addConfigTextItem("IOPResetParameter", iop_reset_param, MAX_INPUT_LEN);

	menu->setTitle("Boot Menu");
	menu->addItem("Boot Current Config", loader, NULL);
	Menu *fileMenu = menu->addSubMenu("File Menu");
	fileMenu->addItem(menu->getTitle(), setCurrentMenu, menu, getTexBack());
	fileMenu->addItem("Restore defaults", setDefaultConfiguration, NULL);
	fileMenu->addItem("Load Config from MC0", mcLoadConfig, (void *) CONFIG_FILE);
	fileMenu->addItem("Load Config from DVD", mcLoadConfig, (void *) DVD_CONFIG_FILE);
	fileMenu->addItem("Load Config from USB", mcLoadConfig, (void *) USB_CONFIG_FILE);
	fileMenu->addItem("Load NetSurf Config from USB", mcLoadConfig, (void *) PS2NS_CONFIG_FILE);
	fileMenu->addItem("Load Selected Config", mcLoadConfig, configfile);
	fileMenu->addItem("Save Config on MC0", mcSaveConfig, (void *) CONFIG_FILE);
	fileMenu->addItem("Save Selected Config", mcSaveConfig, configfile);
	fileMenu->addItem("Delete Config on MC0", removefile, (void *) CONFIG_FILE);

	Menu *configFileMenu = fileMenu->addSubMenu("Select Config File");
	configFileMenu->addItem(fileMenu->getTitle(), setCurrentMenu, fileMenu, getTexBack());
	configFileMenu->addItem("Edit Filename", editString, (void *) &configfile);

	static fsRootParam_t cusbParam = {
		configfile,
		fileMenu,
		configFileMenu,
		NULL,
		"USB Memory Stick",
		"mass0:",
		""
	};
	configFileMenu->addItem(cusbParam.menuName, fsroot, (void *) &cusbParam);

	static fsRootParam_t cmc0Param = {
		configfile,
		fileMenu,
		configFileMenu,
		NULL,
		"Memory Card 1",
		"mc0:",
		""
	};
	configFileMenu->addItem(cmc0Param.menuName, fsroot, (void *) &cmc0Param);

	static fsRootParam_t cmc1Param = {
		configfile,
		fileMenu,
		configFileMenu,
		NULL,
		"Memory Card 2",
		"mc1:",
		""
	};
	configFileMenu->addItem(cmc1Param.menuName, fsroot, (void *) &cmc1Param);
#if !defined(RESET_IOP)
	static fsRootParam_t chostParam = {
		configfile,
		fileMenu,
		configFileMenu,
		NULL,
		"Host",
		"host:",
		""
	};
	configFileMenu->addItem(chostParam.menuName, fsroot, (void *) &chostParam);
#endif
	static fsRootParam_t ccdfsParam = {
		configfile,
		fileMenu,
		configFileMenu,
		NULL,
		"CD/DVD",
		"cdfs:",
		""
	};
	configFileMenu->addItem(ccdfsParam.menuName, fsroot, (void *) &ccdfsParam);

	Menu *linuxMenu = menu->addSubMenu("Select Kernel");
	linuxMenu->addItem(menu->getTitle(), setCurrentMenu, menu, getTexBack());

	strcpy(kernelFilename, EXAMPLE_KERNEL);
	addConfigTextItem("KernelFileName", kernelFilename, MAX_PATH_LEN);
	//linuxMenu->addItem("Show Filename", showText, (void *) &kernelFilename);
	linuxMenu->addItem("Edit Filename", editString, (void *) &kernelFilename);
	linuxMenu->addItem("Example Kernel", setExampleKernel, (void *) &kernelFilename);

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
#if !defined(RESET_IOP)
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
	static fsRootParam_t kcdfsParam = {
		kernelFilename,
		menu,
		linuxMenu,
		NULL,
		"CD/DVD",
		"cdfs:",
		""
	};
	linuxMenu->addItem(kcdfsParam.menuName, fsroot, (void *) &kcdfsParam);

	Menu *initrdMenu = menu->addSubMenu("Select RAM disc");

	initrdMenu->addItem(menu->getTitle(), setCurrentMenu, menu, getTexBack());
	initrdFilename[0] = 0;
	addConfigTextItem("InitrdFileName", initrdFilename, MAX_PATH_LEN);
	//initrdMenu->addItem("Show Filename", showText, (void *) &initrdFilename);
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
#if !defined(RESET_IOP)
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

	static fsRootParam_t cdfsParam = {
		initrdFilename,
		menu,
		initrdMenu,
		NULL,
		"CD/DVD",
		"cdfs:",
		""
	};
	initrdMenu->addItem(cdfsParam.menuName, fsroot, (void *) &cdfsParam);

	/* Config menu */
	Menu *configMenu = menu->addSubMenu("Configuration Menu");
	configMenu->addItem(menu->getTitle(), setCurrentMenu, menu, getTexBack());
	configMenu->addItem("Edit Kernel Parameter", editString, kernelParameter);
	configMenu->addItem(" Set to initrd", setDefaultKernelParameterMenu, kernelParameter);
	configMenu->addItem(" Set to hda1", setParameterMenu, &hda1Config);
	configMenu->addItem(" Set to hda2", setParameterMenu, &hda2Config);
	configMenu->addItem(" Set to hda3", setParameterMenu, &hda3Config);
	configMenu->addItem(" Set to nfs", setParameterMenu, &nfsConfig);
	configMenu->addItem("Edit Video Parameter", editString, videoParameter);
	configMenu->addItem(" Set to current mode", setDefaultVideoParameterMenu, videoParameter);
	configMenu->addItem("Edit PCIC Type", editString, pcicType);

	/* Module Menu */
	Menu *moduleMenu;

	moduleMenu = configMenu->addSubMenu("Module List");
	moduleMenu->addItem(configMenu->getTitle(), setCurrentMenu, configMenu,
		getTexBack());

	/* Add list with all modules. */
	for (i = 0; i < getNumberOfModules(); i++) {
		moduleEntry_t *module;

		module = getModuleEntry(i);

		moduleMenu->addCheckItem(module->path, &module->load);
	}

	configMenu->addCheckItem("Use SBIOS from TGE (ow RTE)",
		&loaderConfig.enableSBIOSTGE);
	configMenu->addCheckItem("TGE SBIOS for New Modules", &loaderConfig.newModulesInTGE);
	configMenu->addCheckItem("Enable hard disc and network",
		&loaderConfig.enableDev9);
	configMenu->addCheckItem("Patch libsd (enable USB)",
		&loaderConfig.patchLibsd);
	configMenu->addCheckItem("Enable IOP debug output", &loaderConfig.enableEEDebug);
	configMenu->addItem("IOP Reset Param", editString, iop_reset_param);

	/* SBIOS Calls Menu */
	Menu *sbiosCallsMenu;

	sbiosCallsMenu = configMenu->addSubMenu("Enabled SBIOS Calls");
	sbiosCallsMenu->addItem(configMenu->getTitle(), setCurrentMenu,
		configMenu, getTexBack());
	sbiosCallsMenu->addItem("Enable all Calls", enableAllSBIOSCalls, NULL);
	sbiosCallsMenu->addItem("Disable all Calls", disableAllSBIOSCalls,
		NULL);
	sbiosCallsMenu->addItem("Set default for RTE", defaultSBIOSCalls, NULL);
	for (i = 0; i < numberOfSbiosCalls; i++) {
		sbiosCallsMenu->addCheckItem(sbiosDescription[i],
			&sbiosCallEnabled[i]);
	}
	menu->addMultiSelectionItem("Auto Boot", autoBootText, &loaderConfig.autoBootTime, NULL);
	menu->addItem("Power off", poweroff, NULL);
	menu->addItem("Reboot", reboot, NULL);

	/* PS2LINK debug entries. */
	Menu *netMenu = configMenu->addSubMenu("Net Options");
	netMenu->addItem(configMenu->getTitle(), setCurrentMenu, configMenu,
		getTexBack());
	netMenu->addItem("Set IP address", editString, myIP);
	netMenu->addItem("Set Netmask", editString, netmask);
	netMenu->addItem("Set Gateway IP address", editString, gatewayIP);
	netMenu->addItem("Set DNS IP address", editString, dnsIP);
	netMenu->addItem("Reload modules", reloadModules, NULL);

	Menu *rteMenu = configMenu->addSubMenu("RTE Copy Menu");
	rteMenu->addItem(configMenu->getTitle(), setCurrentMenu, configMenu,
		getTexBack());
	rteMenu->addItem("Edit RTE module path", editString, rtePath);
	rteMenu->addItem("Copy RTE modules", copyRTEModules, NULL);
	rteMenu->addItem("Edit RTE elf", editString, rteElf);
	rteMenu->addItem("Edit RTE elf Offset", editString, rteElfOffset);
	rteMenu->addItem("Copy RTE SBIOS", copyRTESBIOS, NULL);
	static copyRTEELF_param_t ExtractCDVDMANParam = {
		"3",
		CONFIG_DIR "/cdvdman.irx"
	};
	static copyRTEELF_param_t ExtractCDVDFSVParam = {
		"4",
		CONFIG_DIR "/cdvdfsv.irx"
	};

	rteMenu->addItem("Edit RTE CDVD ELF", editString, rteELF);
	rteMenu->addItem("Edit RTE CDVDMAN nr", editString, ExtractCDVDMANParam.elfNumber);
	rteMenu->addItem("Edit RTE CDVDFSV nr", editString, ExtractCDVDFSVParam.elfNumber);
	rteMenu->addItem("Extract RTE CDVDMAN", copyRTEELF, &ExtractCDVDMANParam);
	rteMenu->addItem("Extract RTE CDVDFSV", copyRTEELF, &ExtractCDVDFSVParam);

	/* Module Menu */
	Menu *versionMenu;
	static char kloader_version[] = LOADER_VERSION
#ifdef RESET_IOP
		" RESET_IOP"
#endif
#ifdef SCREENSHOT
		" SCREENSHOT"
#endif
#ifdef NEW_ROM_MODULES
		" NEW_ROM_MODULES"
#endif
#ifdef OLD_ROM_MODULES
		" OLD_ROM_MODULES"
#endif
#ifdef SHARED_MEM_DEBUG
		" SHARED_MEM_DEBUG"
#endif
#ifdef FILEIO_DEBUG
		" FILEIO_DEBUG"
#endif
#ifdef CALLBACK_DEBUG
		" CALLBACK_DEBUG"
#endif
#ifdef SBIOS_DEBUG
		" SBIOS_DEBUG"
#endif
	;

	versionMenu = menu->addSubMenu("Versions");
	versionMenu->addItem(menu->getTitle(), setCurrentMenu, menu, getTexBack());
	versionMenu->addItem("Kernelloader Version", showText, (void *) kloader_version);
	versionMenu->addItem("PS2 Console type", showText, (void *) ps2_console_type);
	versionMenu->addItem("PS2 Region type", showText, (void *) ps2_region_type);
	versionMenu->addItem("PS2 ROM Version", showText, (void *) ps2_rom_version);
	versionMenu->addItem("LIBSD Version", showText, (void *) libsd_version);
	versionMenu->addItem("Hardware Information", showText, (void *) hardware_information);
	versionMenu->addItem("EROMDRV Path", editString, (void *) get_eromdrvpath());
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
#ifdef RTE
			return "host:RTE/sbios.elf";
#else
			return "mc0:kloader/sbios.bin";
#endif
		}
	}

	const char *getKernelFilename(void) {
		return kernelFilename;
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
		int n;
		strcpy(fullKernelParameter, kernelParameter);
		n = strlen(fullKernelParameter);
		fullKernelParameter[n] = ' ';
		n++;
		strcpy(&fullKernelParameter[n], videoParameter);
		return fullKernelParameter;
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

	const char *getPS2DNS(int *len) {
		*len = strlen(dnsIP) + 1;
		return dnsIP;
	}

	const char *getPcicType(void)
	{
		return pcicType;
	}
}

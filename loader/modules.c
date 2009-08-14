/* Copyright (c) 2007 Mega Man */
#include <stdio.h>
#include <string.h>
#include <kernel.h>
#include <iopheap.h>
#include <loadfile.h>
#include <sbv_patches.h>
#include <sifrpc.h>
#include <iopcontrol.h>

#include "modules.h"
#include "graphic.h"
#include "loader.h"
#include "rom.h"
#include "eedebug.h"
#include "configuration.h"
#include "fileXio_rpc.h"
#include "SMS_CDVD.h"
#include "SMS_CDDA.h"

#ifdef NEW_ROM_MODULES
#define MODPREFIX "X"
#endif

#ifdef OLD_ROM_MODULES
#define MODPREFIX ""
#endif

/** Structure describing module that should be loaded. */
typedef struct
{
	/** Path to module file. */
	const char *path;
	/** Parameter length. */
	unsigned int argLen;
	/** Module parameters. */
	const char *args;
	/** True, if ps2smap. */
	int ps2smap;
	/** True, if configuration should be loaded. */
	int loadCfg;
	/** True, if module can be loaded from "mc0:/kloader/". */
	int checkMc;
} moduleLoaderEntry_t;


static moduleLoaderEntry_t moduleList[] = {
#if defined(IOP_RESET) && !defined(PS2LINK)
	{
		.path = "eedebug.irx",
		.argLen = 0,
		.args = NULL
	},
#endif
	{
		/* Stop sound. */
		.path = "rom0:CLEARSPU",
		.argLen = 0,
		.args = NULL
	},
	{
		/* Module is required to access rom1: */
		.path = "rom0:ADDDRV",
		.argLen = 0,
		.args = NULL
	},
	{
		/* Module is required to access video DVDs */
		.path = "eromdrvloader.irx",
		.argLen = 0,
		.args = NULL
	},
#if defined(RESET_IOP)
	{
		.path = "rom0:" MODPREFIX "SIO2MAN",
		.argLen = 0,
		.args = NULL
	},
	{
		.path = "rom0:" MODPREFIX "MCMAN",
		.argLen = 0,
		.args = NULL
	},
	{
		.path = "rom0:" MODPREFIX "MCSERV",
		.argLen = 0,
		.args = NULL
	},
#endif
	{
		.path = "rom0:" MODPREFIX "PADMAN",
		.argLen = 0,
		.args = NULL,
		.loadCfg = -1 /* MC modules are loaded before this entry. */
	},
#if defined(RESET_IOP)
	{
		.path = "rom0:" MODPREFIX "CDVDMAN",
		.argLen = 0,
		.args = NULL
	},
#endif
	{
		.path = "SMSUTILS.irx",
		.argLen = 0,
		.args = NULL,
		.checkMc = -1
	},
	{
		.path = "SMSCDVD.irx",
		.argLen = 0,
		.args = NULL,
		.checkMc = -1
	},
#if defined(RESET_IOP)
	{
		.path = "ioptrap.irx",
		.argLen = 0,
		.args = NULL,
		.checkMc = -1
	},
	{
		.path = "iomanX.irx",
		.argLen = 0,
		.args = NULL,
		.checkMc = -1
	},
	{
		.path = "poweroff.irx",
		.argLen = 0,
		.args = NULL,
		.checkMc = -1
	},
	{
		.path = "ps2dev9.irx",
		.argLen = 0,
		.args = NULL,
		.checkMc = -1
	},
	{
		.path = "ps2ip.irx",
		.argLen = 0,
		.args = NULL,
		.checkMc = -1
	},
	{
		.path = "ps2smap.irx",
		.argLen = 0,
		.args = NULL,
		.ps2smap = 1
	},
#ifdef PS2LINK
	{
		.path = "ps2link.irx",
		.argLen = 0,
		.args = NULL,
		.checkMc = -1
	},
#endif
#endif
#ifdef NAPLINK
	{
		.path = "npm-usbd.irx",
		.argLen = 0,
		.args = NULL,
		.checkMc = -1
	},
	{
		.path = "npm-2301.irx",
		.argLen = 0,
		.args = NULL,
		.checkMc = -1
	},
#else
	{
		.path = "usbd.irx",
		.argLen = 0,
		.args = NULL,
		.checkMc = -1
	},
#endif
	{
		.path = "usb_mass.irx",
		.argLen = 0,
		.args = NULL,
		.checkMc = -1
	},
	{
		.path = "fileXio.irx",
		.argLen = 0,
		.args = NULL,
		.checkMc = -1
	},
	{
		.path = "ps2kbd.irx",
		.argLen = 0,
		.args = NULL,
		.checkMc = -1
	},
};

static int moduleLoaderNumberOfModules = sizeof(moduleList) / sizeof(moduleLoaderEntry_t);

/** Parameter for IOP reset. */
static char s_pUDNL   [] __attribute__(   (  section( ".data" ), aligned( 1 )  )   ) = "rom0:UDNL rom0:EELOADCNF";

static char version[256];

static int romver;

int isSlimPSTwo(void)
{
	if (romver > 0x0190) {
		return -1;
	} else {
		return 0;
	}
}

int loadLoaderModules(void)
{
	int i;
	int rv;
	int lrv = -1;
	int ret;
	int fd;

#ifdef RESET_IOP
	graphic_setStatusMessage("Reseting IOP");
	FlushCache(0);

	SifExitIopHeap();
	SifLoadFileExit();
	SifExitRpc();
	SifStopDma();

	SifIopReset(s_pUDNL, 0);

	while (SifIopSync());

	graphic_setStatusMessage("Initialize RPC");
	SifInitRpc(0);

	graphic_setStatusMessage("Patching enable LMB");
	sbv_patch_enable_lmb();
	graphic_setStatusMessage("Patching disable prefix check");
	sbv_patch_disable_prefix_check();
#else
	SifInitRpc(0);
#endif

	graphic_setStatusMessage("Add eedebug handler");
	addEEDebugHandler();

	graphic_setStatusMessage("Checking ROM Version");
	fd = open("rom0:ROMVER", O_RDONLY);
	if (fd >= 0) {
		ret = read(fd, version, sizeof(version));
		close(fd);
		version[4] = 0;
		romver = strtoul(version, NULL, 16);
	}

	graphic_setStatusMessage("Loading modules");
	for (i = 0; i < moduleLoaderNumberOfModules; i++) {
		rom_entry_t *romfile;

		/* Load configuration when necessary modules are loaded. */
		if (moduleList[i].loadCfg) {
			lrv = loadConfiguration(CONFIG_FILE);
		}
		graphic_setStatusMessage(moduleList[i].path);
		printf("Loading module %s)\n", moduleList[i].path);

		if (moduleList[i].ps2smap) {
			moduleList[i].args = getPS2MAPParameter(&moduleList[i].argLen);
		}
		if (moduleList[i].checkMc) {
			static char file[256];

			/* Try to load module from MC if available. */
			snprintf(file, sizeof(file), CONFIG_DIR "/%s", moduleList[i].path);
			rv = SifLoadModule(file, moduleList[i].argLen, moduleList[i].args);
		} else {
			rv = -1;
		}
		if (rv < 0) {
			romfile = rom_getFile(moduleList[i].path);
			if (romfile != NULL) {
				int ret;

				ret = SifExecModuleBuffer(romfile->start, romfile->size, moduleList[i].argLen, moduleList[i].args, &rv);
				if (ret < 0) {
					rv = ret;
				}
			} else {
				rv = SifLoadModule(moduleList[i].path, moduleList[i].argLen, moduleList[i].args);
			}
			if (rv < 0) {
				printf("Failed to load module \"%s\".", moduleList[i].path);
				error_printf("Failed to load module \"%s\".", moduleList[i].path);
			}
		}
	}
	graphic_setStatusMessage(NULL);
	printAllModules();

	fileXioInit();

	CDDA_Init();
	CDVD_Init();

	if (lrv != NULL) {
		DiskType type;

		type = CDDA_DiskType();

		if (type == DiskType_DVDV) {
			CDVD_SetDVDV(1);
		} else {
			CDVD_SetDVDV(0);
		}

		lrv = loadConfiguration(DVD_CONFIG_FILE);
#if 0
		if (lrv != 0) {
			error_printf("Failed to load config from \"%s\", using default configuration.", DVD_CONFIG_FILE);
		}
#endif
		/* Stop CD when finished. */
		CDVD_Stop();
		CDVD_FlushCache();
	}

	return 0;
}

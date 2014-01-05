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
#include "graphic.h"
#include "loadermenu.h"
#include "nvram.h"
#include "kprint.h"

#ifdef NEW_ROM_MODULES
#define MODPREFIX "X"
#endif

#ifdef OLD_ROM_MODULES
#define MODPREFIX ""
#endif

#define IRX_MAGIC_EXPORT 0x41c00000

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
	/** True, if module is responsible eromdrv. */
	int eromdrv;
	/** True, if it is SMS module which controls DVDV. */
	int sms_mod;
	/** True, if module handles DNS. */
	int dns;
	/** True, if module needs network. */
	int network;
	/** 1, if debug mode. 0, load always. -1, no debug mode */
	int debug_mode;
} moduleLoaderEntry_t;


static char eromdrvpath[MAX_INPUT_LEN] = "rom1:EROMDRVE";

static moduleLoaderEntry_t moduleList[] = {
#if defined(IOP_RESET)
	{
		.path = "eedebug.irx",
		.argLen = 0,
		.args = NULL,
		.debug_mode = -1,
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
		.args = NULL,
		.eromdrv = -1
	},
#if defined(RESET_IOP)
	{
		.path = "rom0:" MODPREFIX "SIO2MAN",
		.argLen = 0,
		.args = NULL,
		.debug_mode = -1,
	},
	{
		.path = "rom0:" MODPREFIX "MCMAN",
		.argLen = 0,
		.args = NULL,
		.debug_mode = -1,
	},
	{
		.path = "rom0:" MODPREFIX "MCSERV",
		.argLen = 0,
		.args = NULL,
		.debug_mode = -1,
	},
#endif
	{
		.path = "rom0:" MODPREFIX "PADMAN",
		.argLen = 0,
		.args = NULL,
		.loadCfg = -1 /* MC modules are loaded before this entry. */
	},
	{
		.path = "SMSUTILS.irx",
		.argLen = 0,
		.args = NULL,
		.checkMc = -1,
		.sms_mod = -1
	},
	{
		.path = "SMSCDVD.irx",
		.argLen = 0,
		.args = NULL,
		.checkMc = -1,
		.sms_mod = -1
	},
#if defined(RESET_IOP)
	{
		.path = "ioptrap.irx",
		.argLen = 0,
		.args = NULL,
		.checkMc = -1,
		.debug_mode = -1,
	},
	{
		.path = "iomanX.irx",
		.argLen = 0,
		.args = NULL,
		.checkMc = -1,
		.debug_mode = -1,
	},
	{
		.path = "poweroff.irx",
		.argLen = 0,
		.args = NULL,
		.checkMc = -1,
		.debug_mode = -1,
	},
	{
		.path = "ps2dev9.irx",
		.argLen = 0,
		.args = NULL,
		.checkMc = -1,
		.debug_mode = -1,
	},
	{
		.path = "ps2ip.irx",
		.argLen = 0,
		.args = NULL,
		.checkMc = -1,
		.debug_mode = -1,
	},
	{
		.path = "ps2smap.irx",
		.argLen = 0,
		.args = NULL,
		.ps2smap = 1,
		.network = -1,
		.debug_mode = -1,
	},
	{
		.path = "ps2link.irx",
		.argLen = 0,
		.args = NULL,
		.checkMc = -1,
		.network = -1,
		.debug_mode = -1,
	},
#endif
	{
		.path = "dns.irx",
		.argLen = 0,
		.args = NULL,
		.checkMc = -1,
		.dns = -1,
		.network = -1,
	},
	{
		.path = "ps2http.irx",
		.argLen = 0,
		.args = NULL,
		.checkMc = -1,
		.network = -1,
	},
	{
		.path = "usbd.irx",
		.argLen = 0,
		.args = NULL,
		.checkMc = -1
	},
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
		.checkMc = -1,
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
#ifdef NEW_ROM_MODULES
/* XXX: This will load the newer CDVDMAN module which doesn't support reading of NVRAM. */
static char s_pUDNL   [] __attribute__(   (  section( ".data" ), aligned( 1 )  )   ) = "rom0:UDNL rom0:EELOADCNF";
#endif
#ifdef OLD_ROM_MODULES
/* This will load the old CDVDMAN module which supports reading of NVRAM. */
static char s_pUDNL   [] __attribute__(   (  section( ".data" ), aligned( 1 )  )   ) = "rom0:UDNL";
#endif

char ps2_rom_version[256] = "unknown";
static char version[256];

static int romver = 0;

static int eromdrvSupport;

static int libsd_version = 0x7FFFFFFF;

static int network_support = -1;

char hardware_information[128] = "unknown";

int isSlimPSTwo(void)
{
	if (romver > 0x0190) {
		return -1;
	} else {
		return 0;
	}
}

int hasNetworkSupport(void)
{
	return network_support;
}

int isDVDVSupported(void)
{
	return eromdrvSupport;
}

void checkROMVersion(void)
{
	int fd;
	int ret;

	fd = open("rom0:ROMVER", O_RDONLY);
	if (fd >= 0) {
		ret = read(fd, version, sizeof(version));
		close(fd);
		if (ret > 0) {
			memcpy(ps2_rom_version, version, ret);
			ps2_rom_version[ret - 1] = 0;
		}
		version[4] = 0;
		romver = strtoul(version, NULL, 16);
	}
}

int getBiosVersion(void)
{
	return romver;
}

void checkLibsdExport(FILE *fin)
{
	int dummy;
	char modulename[9];

	// unused:
	if (fread(&dummy, sizeof(int), 1, fin) != 1)
		return;

	// Read version number
	if (fread(&libsd_version, sizeof(int), 1, fin) != 1)
		return;

	// Read version number
	if (fread(modulename, 8, 1, fin) != 1)
		return;

	modulename[8] = 0;

	if (strcmp(modulename, "libsd") != 0) {
		/* Found module description. */
		libsd_version = 0x7FFFFFFF;
	}
}

int get_libsd_version(void)
{
	return libsd_version;
}

void checkForMusicSupport(void)
{
	FILE *fin;

	fin = fopen("rom1:LIBSD", "rb");
	if (fin != NULL) {
		int magic;

		while(fread(&magic, sizeof(int), 1, fin) == 1) {
			if (magic == IRX_MAGIC_EXPORT)
				checkLibsdExport(fin);
		}
		fclose(fin);
	} else {
		kprintf("Failed to open rom1:LIBSD.\n");
	}
}

int loadLoaderModules(int debug_mode, int disable_cdrom)
{
	static int load_dvd_config = -1;
	static int load_netsurf_config = -1;
	static int load_usb_config = -1;
	int i;
	int rv;
	int lrv = -1;

	if (debug_mode == 1) {
		/* Network is used by ps2link and can't be used by Linux. */
		network_support = 0;
	}

#ifdef RESET_IOP
	if (debug_mode == -1) {
		graphic_setStatusMessage("Flushing cache");
		FlushCache(0);
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
		while (!SifIopSync());

		graphic_setStatusMessage("Initialize RPC");
		SifInitRpc(0);
	}
#endif

	graphic_setStatusMessage("Patching enable LMB");
	sbv_patch_enable_lmb();
	graphic_setStatusMessage("Patching disable prefix check");
	sbv_patch_disable_prefix_check();

	/* CDVDMAN is loaded by IopReset and NVRAM can be read. */
	graphic_setStatusMessage("Read NVRAM from CDVD");

	if (!disable_cdrom) {
		nvram_init();
	}

	eromdrvSupport = 0;

	graphic_setStatusMessage("Add eedebug handler");
	addEEDebugHandler();

	graphic_setStatusMessage("Loading modules");
	for (i = 0; i < moduleLoaderNumberOfModules; i++) {
		const rom_entry_t *romfile;

		if (moduleList[i].debug_mode != 0) {
			if (moduleList[i].debug_mode != debug_mode) {
				continue;
			}
		}

		/* Load configuration when necessary modules are loaded. */
		if (moduleList[i].loadCfg) {
			checkForMusicSupport();

			setDefaultConfiguration(NULL);

			lrv = loadConfiguration(CONFIG_FILE);

			changeMode();

			/* Load configuration on startup and not on IOP reset. */
			moduleList[i].loadCfg = 0;
		}
		graphic_setStatusMessage(moduleList[i].path);
		kprintf("Loading module (%s)\n", moduleList[i].path);

		if (!network_support) {
			if (moduleList[i].network) {
				continue;
			}
		}

		if (moduleList[i].ps2smap) {
			moduleList[i].args = getPS2MAPParameter(&moduleList[i].argLen);
		}
		if (moduleList[i].dns) {
			moduleList[i].args = getPS2DNS(&moduleList[i].argLen);
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
			if ((moduleList[i].sms_mod == 0) || (isDVDVSupported())) {
				if (moduleList[i].eromdrv < 0) {
					/* Try to detect EROM driver only the first time. */
					moduleList[i].eromdrv = 1;
					if (disable_cdrom) {
						continue;
					}

					rv = open("rom1:EROMDRV", O_RDONLY);
					if (rv >=0 ) {
						eromdrvpath[12] = 0;

						/* This is an old fat PS2 (working with SCPH-50004 and SCPH-39004). */
						close(rv);
					} else {
						const u8 *nvm;

						nvm = get_nvram();
						if (nvm_errors == 0) {
							/* NVM layout seems to be correct. */
							eromdrvpath[12] = nvm[NVM_REAL_REGION];
							rv = open(eromdrvpath, O_RDONLY);
							if (rv >=0 ) {
								/* Region code seems to be correct. */
								close(rv);
							} else {
								error_printf("The region code stored in the NVRAM S%02x T%02x F%02x R%02x "
									"can't be detected by version string %s (%s).",
									nvm[0x180],
									nvm[0x181],
									nvm[NVM_FAKE_REGION],
									nvm[NVM_REAL_REGION],
									ps2_rom_version);
								continue;
							}
						} else {
							error_printf("%d errors when reading NVRAM. Please set path "
								"to EROMDRV and reload modules.", nvm_errors);
							continue;
						}
					}
				}
				if (moduleList[i].eromdrv != 0) {
					moduleList[i].args = get_eromdrvpath();
					moduleList[i].argLen = strlen(moduleList[i].args) + 1;
				}
				romfile = rom_getFile(moduleList[i].path);
				if (romfile != NULL) {
					int ret;
	
					ret = SifExecModuleBuffer((void *) romfile->start, romfile->size, moduleList[i].argLen, moduleList[i].args, &rv);
					if (ret < 0) {
						rv = ret;
					}
				} else {
					rv = SifLoadModule(moduleList[i].path, moduleList[i].argLen, moduleList[i].args);
				}
				if (rv < 0) {
					if (moduleList[i].eromdrv != 0) {
						kprintf("Failed to load module \"%s\".\n", get_eromdrvpath());
					} else {
						kprintf("Failed to load module \"%s\".\n", moduleList[i].path);
					}
					if (moduleList[i].ps2smap && !isSlimPSTwo()) {
						network_support = 0;
					} else {
						if (moduleList[i].eromdrv != 0) {
							error_printf("Failed to load module \"%s\".", get_eromdrvpath());
						} else {
							error_printf("Failed to load module \"%s\".", moduleList[i].path);
						}
					}
				} else {
					if (moduleList[i].eromdrv != 0) {
						eromdrvSupport = -1;
					}
				}
			}
		}
	}
	graphic_setStatusMessage(NULL);
	printAllModules();

	fileXioInit();

	if (load_netsurf_config) {
		load_netsurf_config = 0;

		if (lrv != 0) {
			graphic_setStatusMessage("Check for NetSurf config");

			lrv = loadConfiguration(PS2NS_CONFIG_FILE);

			graphic_setStatusMessage(NULL);
		}
	}

	if (load_usb_config) {
		load_usb_config = 0;

		if (lrv != 0) {
			graphic_setStatusMessage("Check for USB config");

			lrv = loadConfiguration(USB_CONFIG_FILE);

			graphic_setStatusMessage(NULL);
		}
	}

	if (load_dvd_config && isDVDVSupported()) {
		load_dvd_config = 0;

		graphic_setStatusMessage("Init DVD driver");

		CDDA_Init();
		CDVD_Init();

		if (lrv != 0) {
			DiskType type;
	
			graphic_setStatusMessage("Load config from DVD");

			type = CDDA_DiskType();
	
			if (type == DiskType_DVDV) {
				CDVD_SetDVDV(1);
			} else {
				CDVD_SetDVDV(0);
			}

			kprintf("kloader disc type %u\n", type);
			switch (type) {
			case DiskType_CD:
			case DiskType_DVD:
			case DiskType_DVDV:
				/* Load configuration from disc. */
				lrv = loadConfiguration(DVD_CONFIG_FILE);

				changeMode();
#if 0
				if (lrv != 0) {
					error_printf("Failed to load config from \"%s\", using default configuration.", DVD_CONFIG_FILE);
				}
#endif
				break;
			default:
				kprintf("kloader unsupported disc type %u\n", type);
				break;
			}

			/* Stop CD when finished. */
			CDVD_Stop();
			CDVD_FlushCache();
		}
		graphic_setStatusMessage(NULL);
	}

	snprintf(hardware_information, sizeof(hardware_information),
		"%s with DVD-R %s, %s sound support and %s network adapter",
		isSlimPSTwo() ? "slim PSTwo" : "fat PS2",
		disable_cdrom ? "disabled" : (isDVDVSupported() ? "support" : "problem"),
		(libsd_version <= 0x104) ? "direct" : "indirect",
		network_support ? "with" : "without");

	return 0;
}

const char *get_eromdrvpath(void)
{
	return eromdrvpath;
}


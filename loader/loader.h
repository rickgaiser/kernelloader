/* Copyright (c) 2007 - 2014 Mega Man */
#ifndef _LOADER_H_
#define _LOADER_H_

#include "stdint.h"

#define LOADER_VERSION "3.0"

#ifdef __cplusplus
extern "C" {
#endif

	/** Structure describing module that should be loaded. */
	typedef struct
	{
		/** Path to module file. */
		const char *path;
		/** Buffer used for loading the module. */
		const unsigned char *buffer;
		/** Module size. */
		unsigned int size;
		/** True, if module must be buffered and can't be loaded after IOP reset. */
		int buffered;
		/** Parameter length. */
		unsigned int argLen;
		/** Module parameters. */
		const char *args;
		/** Whether module should be loaded. */
		int load;
		/** True, if allocated memory is used. */
		int allocated;
		/** True, if ps2smap module. */
		int ps2smap;
		/** -1, if for fat PS2, 1 if for slim PSTwo, 0 for both. */
		int slim;
		/** -1, if dev9init.irx, 0 otherwise. */
		int dev9init;
		/** 1, if default module, 0 otherwise. */
		int defaultmod;
		/** -1, if sdlib, 0 otherwise. */
		int libsd;
		/** 1, if ROM1 sound module, 0 otherwise. */
		int sound;
		/** -1, if module needs network, 0 otherwise. */
		int network;
		/** True, if module is responsible eromdrv. */
		int eromdrv;
		/** 1, if debug mode. 0, load always. -1, no debug mode */
		int debug_mode;
	} moduleEntry_t;

	typedef struct {
		int enableSBIOSTGE;
		int newModulesInTGE;
		int enableDev9;
		int enableEEDebug;
		int autoBootTime;
		int patchLibsd;
	} loader_config_t;

	extern loader_config_t loaderConfig;

	int loader(void *arg);
	int getNumberOfModules(void);
	moduleEntry_t *getModuleEntry(int idx);
	void disableSBIOSCalls(uint32_t *SBIOSCallTable);
	const char *getSBIOSFilename(void);
	const char *getKernelFilename(void);
	const char *getInitRdFilename();
	char *getKernelParameter(void);
	void waitForUser(void);
	const char *getPS2MAPParameter(int *len);
	const char *getPS2DNS(int *len);
	void printAllModules(void);
	void DelayThread(int delay);
	const char *getPcicType(void);

	extern char iop_reset_param[];
	extern int debug_mode;
	extern int do_default_sbios_calls;
	extern int disable_cdrom;


#ifdef __cplusplus
}
#endif

#endif

/* Copyright (c) 2007 Mega Man */
#ifndef _LOADER_H_
#define _LOADER_H_

#include "stdint.h"

#define LOADER_VERSION "2.0"

#ifdef __cplusplus
extern "C" {
#endif

	/** Structure describing module that should be loaded. */
	typedef struct
	{
		/** Path to module file. */
		const char *path;
		/** Buffer used for loading the module. */
		unsigned char *buffer;
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
	} moduleEntry_t;

	typedef struct {
		int enableSBIOSTGE;
		int newModulesInTGE;
		int enableDev9;
		int enableEEDebug;
		int autoBootTime;
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
	const char *getGraphicMode(void);
	void printAllModules(void);
	void DelayThread(int delay);
	const char *getPcicType(void);
#ifdef __cplusplus
}
#endif

#endif

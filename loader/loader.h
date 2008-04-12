/* Copyright (c) 2007 Mega Man */
#ifndef _LOADER_H_
#define _LOADER_H_

#include "stdint.h"

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
		/** 0 = no dependency on ps2link. 1 = don't use with ps2link, -1 = use
		 * with ps2link. */
		int ps2link;
		/** Should be used with rte, but not with tge. */
		int rte;
		/** Should be used with tge, but not with rte. */
		int tge;
		/** New rom modules. */
		int newmods;
		/** Old rom modules. */
		int oldmods;
		/** Module only for debug reasons. */
		int debug;
		/** True, if allocated memory is used. */
		int allocated;
	} moduleEntry_t;

	typedef struct {
		int newModules;
		int enableTGE;
		int enablePS2LINK;
		int enableDebug;
		int enableSBIOSTGE;
		int enableDev9;
	} loader_config_t;

	extern loader_config_t loaderConfig;
	/* IP + Netmask + Gateway. */
	static char ifcfg[] = "192.168.0.23\000255.255.255.0\000192.168.0.1";

	int loader(void *arg);
	int getNumberOfModules(void);
	moduleEntry_t *getModuleEntry(int idx);
	void disableSBIOSCalls(uint32_t *SBIOSCallTable);
	const char *getSBIOSFilename(void);
	const char *getKernelFilename(void);
	const char *getInitRdFilename();
#ifdef __cplusplus
}
#endif

#endif

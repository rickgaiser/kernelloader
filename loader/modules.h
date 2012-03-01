/* Copyright (c) 2007 Mega Man */
#ifndef _MODULES_H_
#define _MODULES_H_

#ifdef __cplusplus
extern "C" {
#endif
int loadLoaderModules(int debug_mode, int disable_cdrom);
int isSlimPSTwo(void);
int isDVDVSupported(void);
void checkROMVersion(void);
int get_libsd_version(void);
extern char ps2_rom_version[];
int getBiosVersion(void);
int hasNetworkSupport(void);
const char *get_eromdrvpath(void);

extern char hardware_information[];
#ifdef __cplusplus
}
#endif

#endif

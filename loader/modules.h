/* Copyright (c) 2007 Mega Man */
#ifndef _MODULES_H_
#define _MODULES_H_

#ifdef __cplusplus
extern "C" {
#endif
int loadLoaderModules(void);
int isSlimPSTwo(void);
int isDVDVSupported(void);
void checkROMVersion(void);
#ifdef __cplusplus
}
#endif

#endif

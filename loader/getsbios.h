#ifndef _GET_SBIOS_H_
#define _GET_SBIOS_H_

#ifdef __cplusplus
extern "C" {
#endif

extern char rteElf[1024];
extern char rteElfOffset[1024];

int copyRTESBIOS(void *arg);

#ifdef __cplusplus
}
#endif

#endif

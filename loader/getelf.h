#ifndef _GET_ELF_H_
#define _GET_ELF_H_

typedef struct {
	char elfNumber[1024];
	char outputFilename[1024];
} copyRTEELF_param_t;

#ifdef __cplusplus
extern "C" {
#endif

	extern char rteELF[1024];

	int copyRTEELF(void *arg);

#ifdef __cplusplus
}
#endif

#endif

#ifndef _GET_ELF_H_
#define _GET_ELF_H_

typedef struct {
	char elfNumber[MAX_INPUT_LEN];
	char outputFilename[MAX_INPUT_LEN];
} copyRTEELF_param_t;

#ifdef __cplusplus
extern "C" {
#endif

	extern char rteELF[MAX_INPUT_LEN];

	int copyRTEELF(void *arg);

#ifdef __cplusplus
}
#endif

#endif

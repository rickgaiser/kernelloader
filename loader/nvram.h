#ifndef _NVRAM_H_
#define _NVRAM_H_

#define NVM_FAKE_REGION 0x185
#define NVM_REAL_REGION 0x186

#ifdef __cplusplus
extern "C" {
#endif

extern char ps2_console_type[];
extern char ps2_region_type[];

void nvram_init(void);
u8 *get_nvram(void);
extern int nvm_errors;

#ifdef __cplusplus
}
#endif

#endif

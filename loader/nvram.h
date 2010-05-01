#ifndef _NVRAM_H_
#define _NVRAM_H_

#ifdef __cplusplus
extern "C" {
#endif

extern char ps2_console_type[];
extern char ps2_region_type[];

void nvram_init(void);

#ifdef __cplusplus
}
#endif

#endif

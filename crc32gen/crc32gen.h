#ifndef _CRC32GEN_H_
#define _CRC32GEN_H_

#ifdef __cplusplus                                                              
extern "C" {                                                                    
#endif 

typedef struct {
	uint32_t crc;
	uint32_t fileoffset;
	uint32_t start;
	uint32_t end;
	char section[32];
} __attribute__((packed)) crc32_section_t;

#ifdef __cplusplus                                                              
}                                                                               
#endif

#endif

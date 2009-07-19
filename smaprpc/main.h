#ifndef _MAIN_H_
#define _MAIN_H_

#include "smap_rpc.h"
#include "pbuf.h"

#define DMA_TRANSFER_SIZE 64

err_t SMapLowLevelOutput(PBuf* pOutput);

extern u32 ee_buffer;
extern u32 ee_buffer_pos;
extern volatile u32 ee_buffer_size;

#endif

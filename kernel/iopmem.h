/* Copyright (c) 2007 Mega Man */
#ifndef _IOPMEM_H_
#define _IOPMEM_H_

#include "stdint.h"

#ifdef SHARED_MEM_DEBUG
/** Print string using iop memory (for debugging). */
void iop_prints(const char *text);

/** Print 32-bit value as hexadecimal. */
void iop_printx(uint32_t val);
#else
/** Do nothing .*/
#define iop_prints(text)
/** Do nothing .*/
#define iop_printx(val)
#endif

#endif /* _IOPMEM_H_ */

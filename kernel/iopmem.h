/* Copyright (c) 2007 Mega Man */
#ifndef _IOPMEM_H_
#define _IOPMEM_H_

#include "stdint.h"

/** Print string using iop memory (for debugging). */
void iop_prints(const char *text);

/** Print 32-bit value as hexadecimal. */
void iop_printx(uint32_t val);

#endif /* _IOPMEM_H_ */

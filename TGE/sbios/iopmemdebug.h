#ifndef _IOPMEMDEBUG_H_
#define _IOPMEMDEBUG_H_

#include "stdint.h"

#ifdef SHARED_MEM_DEBUG
/** Print string using iop memory (for debugging). */
int iop_prints(const char *text);

/** Print 32-bit value as hexadecimal. */
void iop_printx(uint32_t val);

void iop_putc(unsigned char c);
#else
/** Do nothing .*/
#define iop_prints(text) do {} while(0)
/** Do nothing .*/
#define iop_printx(val) do {} while(0)
/** Do nothing .*/
#define iop_putc(c) do {} while(0)
#endif

#endif /* _IOPMEMDEBUG_H_ */

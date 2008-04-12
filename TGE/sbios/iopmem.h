#ifndef _IOPMEM_H_
#define _IOPMEM_H_

typedef unsigned int uint32_t;

/** Print string using iop memory (for debugging). */
void iop_prints(const char *text);

/** Print 32-bit value as hexadecimal. */
void iop_printx(uint32_t val);

#endif /* _IOPMEM_H_ */

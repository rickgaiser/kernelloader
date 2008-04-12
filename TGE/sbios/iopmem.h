#ifndef _IOPMEM_H_
#define _IOPMEM_H_

typedef unsigned int uint32_t;

#ifdef SHARED_MEM_DEBUG
/** Print string using iop memory (for debugging). */
void iop_prints(const char *text);

/** Print 32-bit value as hexadecimal. */
void iop_printx(uint32_t val);

/** Print string using iop memory (for debugging). */
int puts(const char *s);
#else
/** Do nothing .*/
#define iop_prints(text)
/** Do nothing .*/
#define iop_printx(val)
/** Do nothing .*/
#define puts(text)
#endif

#endif /* _IOPMEM_H_ */

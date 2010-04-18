/* Copyright (c) 2007 Mega Man */
#ifndef _IOPMEM_H_
#define _IOPMEM_H_

#include "kernel.h"
#include "stdint.h"

#ifdef SHARED_MEM_DEBUG
#ifdef USER_SPACE_SUPPORT
/** iopmem module need to be informed when in kernel mode, to handle it correctly. */
void iop_kmode_enter(void);
#endif
/** Initialize shared memory debug. */
void iop_init_shared(void);
/** Read iop memory. */
u32 iop_read(void *addr, void *buf, u32 size);
/** Write to iop memory. */
u32 iop_write(void *addr, void *buf, u32 size);
/** Print one character using sharedmem.irx (without rpc for testing). */
void iop_putc(unsigned char c);
/** Print one string using sharedmem.irx (without rpc for testing). */
void iop_prints(const char *text);
/** Printf function using sharedmem.irx (without rpc for testing). */
int iop_printf(const char *format, ...);
/** Print 32-bit value as hexadecimal. */
void iop_printx(uint32_t val);
#else
/** Do nothing. */
#define iop_init_shared()
/** Do nothing. */
#define iop_putc(c)
/** Do nothing. */
#define iop_prints(text)
/** Do nothing. */
#define iop_printf(args...)
/** Do nothing. */
#define iop_printx(val)
#endif

#endif /* _IOPMEM_H_ */

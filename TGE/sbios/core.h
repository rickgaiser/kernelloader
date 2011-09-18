/*
 * core.h - TGE EE Core utilities header file
 *
 * Copyright (c) 2003 Marcus R. Brown <mrbrown@0xd6.org>
 * Copyright (c) 2008 Mega Man
 *
 * See the file LICENSE, located within this directory, for licensing terms.
 */

#ifndef TGE_CORE_H
#define TGE_CORE_H

#include "tge_types.h"
#include "regs.h"

/* Get the physical address.  */
#define PHYSADDR(addr)		(((u32)(addr)) & 0x1fffffff)

/* Convert a pointer to an address in KSEG0 or KSEG1.  */
#define KSEG0ADDR(addr)		((void *)(PHYSADDR(addr) | K0BASE))
#define KSEG1ADDR(addr)		((void *)(PHYSADDR(addr) | K1BASE))
#define UNCACHED_SEG(addr) KSEG1ADDR(addr)

#define CORE_IE (1 << 0)
#define CORE_EXL (1 << 1)
#define CORE_ERL (1 << 2)
#define CORE_EIE (1 << 16)

/* Return true if interrupts are disabled. */
static inline int core_in_interrupt(void)
{
	u32 status;

	 __asm__  __volatile__(
		"sync.p\n"
		"mfc0 %0, $12\n"
		:"=r" (status));

	if ((status & CORE_EIE) == 0) {
		/* interrupts disabled, IE bit not used. */
		return -1;
	}
	if ((status & CORE_IE) == 0) {
		/* interrupts disabled. */
		return -1;
	}
	if ((status & (CORE_EXL | CORE_ERL)) != 0) {
		/* exception handler. */
		return -1;
	}
	return 0;
}

/* Disable interrupts and save the previous contents of COP0 Status.  */
static inline void core_save_disable(u32 *status)
{
	__asm__ volatile (
	".set	push\n\t"	\
	".set	noreorder\n\t"	\
	".set	noat\n\t"	\
	"sync.p\n\t"		\
	"mfc0	%0,$12\n\t"	\
	"ori	$1,%0,1\n\t"	\
	"xori	$1,1\n\t"	\
	"mtc0	$1,$12\n\t"	\
	"sync.p\n\t"		\
	".set	pop\n\t"
	: "=r" (*status) : : "$1", "memory");
}

/* Restore the previous contents of COP0 Status.  */
static inline void core_restore(u32 status)
{
	__asm__ volatile (
	".set	push\n\t"	\
	".set	mips3\n\t"	\
	".set	noreorder\n\t"	\
	"sync.p\n\t"		\
	"mfc0	$8,$12\n\t"	\
	"li	$9,0xff00\n\t"	\
	"and	$8,$9\n\t"	\
	"nor	$9,$0,$9\n\t"	\
	"and	%0,$9\n\t"	\
	"or	%0,$8\n\t"	\
	"mtc0	%0,$12\n\t"	\
	"sync.p\n\t"		\
	".set	pop\n\t"
	: : "r" (status) : "$8", "$9", "memory");
}

void SifWriteBackDCache(void *ptr, int size);
void FlushCache(int operation);
void SyncDCache(void *start, void *end);

#endif /* TGE_CORE_H */

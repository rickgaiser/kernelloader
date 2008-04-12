/* Copyright (c) 2007 Mega Man */
#ifndef _MEMORY_H_
#define _MEMORY_H_

#define NULL ((void *)0)

/** Mask to get kseg0 address from physical address. */
#define KSEG0_MASK 0x80000000
/** Address of KSEG0. */
#define KSEG0 0x80000000
/** Size of a memory page (TLB). */
#define PAGE_SIZE 0x1000
/** Address of KSEG1 (uncached memory). */
#define KSEG1		0xa0000000
/** Convert to uncached address. */
#define KSEG1ADDR(a)	((__typeof__(a))(((uint32_t)(a) & 0x1fffffff) | KSEG1))
/** Convert to cached address. */
#define KSEG0ADDR(a)	((__typeof__(a))(((uint32_t)(a) & 0x1fffffff) | KSEG0))

/**
 * Convert userspace address to kernelspace address.
 * Macro is required to use constant strings in kernel mode.
 * @param x Userspace address.
 * @returns Kernelspace address.
 */
#define U2K(x)	((__typeof__(*x) *) (((uint32_t) x) | KSEG0_MASK))

#ifndef ASSEMBLER

/** End address of this loader. */
void _end(void);

#endif

#endif /* _MEMORY_H_ */

/* Copyright (c) 2007 Mega Man */
#ifndef _MEMORY_H_
#define _MEMORY_H_

/** Mask to get kseg0 address from physical address. */
#define KSEG0_MASK 0x80000000
/** Mask to get kseg0 address from physical address. */
#define KSEG1_MASK 0xA0000000
/** Address of KSEG0. */
#define KSEG0 0x80000000
/** Address of KSEG0. */
#define KSEG1 0xA0000000
/** Size of a memory page (TLB). */
#define PAGE_SIZE 0x1000
/** Size of data cache. */
#define dcache_size 0x2000
/** SIze of instruction cache. */
#define icache_size 0x4000
/** Convert unknown address to physical address. */
#define PHYSADDR(x) (((uint32_t) x) & 0x0FFFFFFF)

/** Base address for SBIOS. */
#define SBIOS_START_ADDRESS 0x80001000
#define MAX_SBIOS_SIZE 0xf000
/** Normal usable memory starts here.*/
#define NORMAL_MEMORY_START 0x80000

/**
 * Convert userspace address to kernelspace address.
 * Macro is required to use constant strings in kernel mode.
 * @param x Userspace address.
 * @returns Kernelspace address.
 */
#define U2K(x)	((__typeof__(*x) *) (((uint32_t) x) | KSEG0_MASK))

#endif /* _MEMORY_H_ */

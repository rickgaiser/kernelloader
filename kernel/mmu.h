/* Copyright (c) 2007 Mega Man */
#ifndef _MMU_H_
#define _MMU_H_

/** Mapped address used for some tricks in exception handler,
 * mapped at the end of the memory to get accesses to addresses
 * calculated by (register zero - offset) working.
 */
#define ZERO_REG_ADDRESS 0x78000

#ifndef ASSEMBLER

void mmu_init_module(void);

#endif

#endif

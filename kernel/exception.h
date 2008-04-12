/* Copyright (c) 2007 Mega Man */
#ifndef _EXCEPTION_H_
#define _EXCEPTION_H_

#define V_TLB_REFILL 0
#define V_COUNTER 1
#define V_DEBUG 2
#define V_COMMON 3
#define V_INTERRUPT 4

/** Install an exception handler. */
void install_exception_handler(int type, void *addr);

/** Dump EE registers from CPU context. */
void dumpRegisters(uint32_t *regs);

#endif

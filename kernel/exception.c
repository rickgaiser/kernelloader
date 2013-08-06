/* Copyright (c) 2007 Mega Man */
#include "stdio.h"
#include "stdint.h"
#include "exception.h"
#include "iopmem.h"
#include "panic.h"
#include "memory.h"
#include "cache.h"
#include "cp0register.h"

void install_exception_handler(int type, void *addr)
{
	volatile uint32_t *dest;

	iop_prints("install_exception_handler\n");

	if (type > V_INTERRUPT) {
		panic("Exception type %d is invalid.\n", type);
	}
	dest = (void *) (KSEG0 + type * 0x80);
	dest[0] = 0x00000000; /* nop, required to fix CPU bug. */
	dest[1] = 0x00000000; /* nop, required to fix CPU bug. */
	dest[2] = 0x08000000 /* jump */
		| (0x03ffffff & (((uint32_t) addr) >> 2)); 
	dest[3] = 0x00000000; /* nop, required because of branch delay execution. */
	flushDCacheAll();
	invalidateICacheAll();
}

/**
 * Dump EE processor registers.
 * @param regs Pointer to processor registers.
 */
void dumpRegisters(uint32_t *regs)
{
	uint32_t epc;
	uint32_t error_epc;
	uint32_t badVAddr;
	uint32_t cause;
	int i;

	epc = 0;
	error_epc = 0xFFFFFFFF;


	CP0_GET_EPC(epc);
	printf("epc 0x%x\n", epc);

	CP0_GET_CAUSE(cause);
	printf("cause 0x%x\n", cause);

	CP0_GET_ERROR_EPC(error_epc);
	printf("error epc 0x%x\n", error_epc);

	CP0_GET_BAD_VADDR(badVAddr);
	printf("badVAddr 0x%x\n", badVAddr);


	for (i = 0; i < 32; i++) {
		if ((i & 3) == 0) {
			iop_printx(i);
			iop_prints(":");
		}
		iop_prints(" ");
		iop_printx(regs[4 * i]);
		if ((i & 3) == 3) {
			iop_prints("\n");
		}
	}
}

void errorHandler(uint32_t *regs)
{
	printf("Exception occured\n");

	iop_prints("Register:\n");
	dumpRegisters(regs);
	while(1);
}

void tlbRefillError(uint32_t *regs)
{
	printf("TLB Refill Exception occured\n");

	iop_prints("Register:\n");
	dumpRegisters(regs);
	while(1);
}

void counterError(uint32_t *regs)
{
	printf("Counter Exception occured\n");

	iop_prints("Register:\n");
	dumpRegisters(regs);
	while(1);
}
void debugError(uint32_t *regs)
{
	printf("Debug Exception occured\n");

	iop_prints("Register:\n");
	dumpRegisters(regs);
	while(1);
}

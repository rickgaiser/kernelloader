/* Copyright (c) 2007-2014 Mega Man */
#include <string.h>

#include "iopmem.h"
#include "stdint.h"
#include "memory.h"
#include "kprint.h"
#include "entry.h"

/**
 * Install an exception handler
 * @param number The number of the exception handler to install.
 */
void installExceptionHandler(int number)
{
	void *dstAddr;
	void *srcAddr;
	volatile uint32_t *patch;
	uint32_t size;
	uint32_t handler;

	if (number > 4) {
		kputs("Exception number is too big.\n");
		panic();
	}
	/* Stage 1 exception handler. */
	dstAddr = (void *) (KSEG0_MASK + 0x80 * number);
	// kprintf("Install exception handler at 0x%08x\n", dstAddr);
	srcAddr = (void *) (((uint32_t) exceptionHandlerStart) | KSEG0_MASK);
	size = ((uint32_t) exceptionHandlerEnd) - ((uint32_t) exceptionHandlerStart);

	memcpy(dstAddr, srcAddr, size);

	/* Address of exception handler stage 2. */
	handler = (uint32_t) U2K(exceptionHandleStage2);

	patch = (uint32_t *) (((uint32_t) exceptionHandlerPatch1) - ((uint32_t) exceptionHandlerStart) + ((uint32_t) dstAddr));
	// kprintf("Patch exception handler at 0x%08x\n", patch);
	patch[0] = (patch[0] & 0xFFFF0000) | (handler >> 16);
	// kprintf("exceptionHandlerPatch1 [0] = 0x%08x\n", patch[0]);
	patch[1] = (patch[1] & 0xFFFF0000) | (handler & 0xFFFF);
	// kprintf("exceptionHandlerPatch1 [1] = 0x%08x\n", patch[1]);

	/* Patch the number of the exception handler into the code. */
	patch = (uint32_t *) (((uint32_t) exceptionHandlerPatch2) - ((uint32_t) exceptionHandlerStart) + ((uint32_t) dstAddr));
	// kprintf("Patch exception handler at 0x%08x\n", patch);
	*patch = ((*patch) & 0xFFFF0000) | number;
	// kprintf("exceptionHandlerPatch2 = 0x%08x\n", *patch);
}

/**
 * Dump EE processor registers.
 * @param regs Pointer to processor registers.
 */
void dumpRegisters(uint32_t *regs)
{
	int i;

	(void) regs;

	for (i = 0; i < 32; i++) {
		if ((i & 3) == 0) {
			kputx(i);
			kputc(':');
		}
		kputc(' ');
		kputx(regs[i]);
		if ((i & 3) == 3) {
			kputc('\n');
		}
	}
}

/**
 * Convert exception number into exception name.
 * Called by exceptionHandleStage2 in entry.S.
 * @param nr Exception number.
 * @returns Name of the exception.
 */
const char *getExceptionName(int nr)
{
	/* Switch can't be used here, because function pointer is used (no KSEG0). */
	if (nr == 0) {
		return "V_TLB_REFILL";
	} else if (nr == 1) {
		return "V_COUNTER";
	} else if (nr == 2) {
		return "V_DEBUG";
	} else if (nr == 3) {
		return "V_COMMON";
	} else if (nr == 4) {
		return "V_INTERRUPT";
	} else {
		return "unknown";
	}
}

/**
 * High level exception entry point.
 * @pparam nr Exception number.
 * @param regs Pointer to registers.
 */
void exception(int nr, uint32_t *regs)
{
#if 0
	register int sp asm("sp");
	register int ra asm("ra");
#endif
	uint32_t epc;
	uint32_t error_epc;
	uint32_t badVAddr;
	uint32_t badPAddr;
	uint32_t cause;

	(void) nr;

	epc = 0;
	error_epc = 0xFFFFFFFF;

	kputs("Exception occured\n");
	__asm__ __volatile__(
		"sync.p\n"
		"mfc0 %0,$14\n":"=r" (epc):);
	kputs("epc 0x");
	kputx(epc);
	kputs("\n");

	__asm__ __volatile__(
		"sync.p\n"
		"mfc0 %0,$13\n":"=r" (cause):);
	kputs("cause 0x");
	kputx(cause);
	kputs("\n");

	dumpRegisters(regs);

	kputs("Exception Nr: 0x");
	kputx(nr);
	kputs("\n");
	kputs("Exception \"");
	kputs(getExceptionName(nr));
	kputs("\"\n");
#if 0 /* May not work when no TLBs are mapped. */
	kprintf("Stack 0x%08x\n", sp);
	kprintf("ra 0x%08x\n", ra);
	kprintf("regs 0x%08x\n", regs);
	kprintf("epc 0x%08x\n", epc);
#endif
	__asm__ __volatile__(
		"sync.p\n"
		"mfc0 %0,$30\n":"=r" (error_epc):);
	kputs("error epc 0x");
	kputx(error_epc);
	kputc('\n');
	__asm__ __volatile__(
		"sync.p\n"
		"mfc0 %0,$8\n":"=r" (badVAddr):);
	kputs("badVAddr 0x");
	kputx(badVAddr);
	kputs("\n");
	__asm__ __volatile__(
		"sync.p\n"
		"mfc0 %0,$23\n":"=r" (badPAddr):);
	kputs("badPAddr 0x");
	kputx(badPAddr);
	kputc('\n');
	while(1);
}

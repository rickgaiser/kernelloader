/* Copyright (c) 2007 Mega Man */
#include "iopmem.h"
#include "stdint.h"
#include "memory.h"

/**
 * Dump EE processor registers.
 * @param regs Pointer to processor registers.
 */
void dumpRegisters(uint32_t *regs)
{
	int i;

	for (i = 0; i < 32; i++) {
		if ((i & 3) == 0) {
			iop_printx(i);
			iop_prints(U2K(":"));
		}
		iop_prints(U2K(" "));
		iop_printx(regs[i]);
		if ((i & 3) == 3) {
			iop_prints(U2K("\n"));
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
		return U2K("V_TLB_REFILL");
	} else if (nr == 1) {
		return U2K("V_COUNTER");
	} else if (nr == 2) {
		return U2K("V_DEBUG");
	} else if (nr == 3) {
		return U2K("V_COMMON");
	} else if (nr == 4) {
		return U2K("V_INTERRUPT");
	} else {
		return U2K("unknown");
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

	epc = 0;
	error_epc = 0xFFFFFFFF;

	iop_prints(U2K("Exception occured\n"));
	__asm__ __volatile__(
		"sync.p\n"
		"mfc0 %0,$14\n":"=r" (epc):);
	iop_prints(U2K("epc 0x"));
	iop_printx(epc);
	iop_prints(U2K("\n"));

	__asm__ __volatile__(
		"sync.p\n"
		"mfc0 %0,$13\n":"=r" (cause):);
	iop_prints(U2K("cause 0x"));
	iop_printx(cause);
	iop_prints(U2K("\n"));

	dumpRegisters(regs);

	iop_prints(U2K("Exception Nr: 0x"));
	iop_printx(nr);
	iop_prints(U2K("\n"));
	iop_prints(U2K("Exception \""));
	iop_prints(getExceptionName(nr));
	iop_prints(U2K("\"\n"));
#if 0
	iop_printf(U2K("Stack 0x%08x\n"), sp);
	iop_printf(U2K("ra 0x%08x\n"), ra);
	iop_printf(U2K("regs 0x%08x\n"), regs);
	iop_printf(U2K("epc 0x%08x\n"), epc);
#endif
	__asm__ __volatile__(
		"sync.p\n"
		"mfc0 %0,$30\n":"=r" (error_epc):);
	iop_prints(U2K("error epc 0x"));
	iop_printx(error_epc);
	iop_prints(U2K("\n"));
	__asm__ __volatile__(
		"sync.p\n"
		"mfc0 %0,$8\n":"=r" (badVAddr):);
	iop_prints(U2K("badVAddr 0x"));
	iop_printx(badVAddr);
	iop_prints(U2K("\n"));
	__asm__ __volatile__(
		"sync.p\n"
		"mfc0 %0,$23\n":"=r" (badPAddr):);
	iop_prints(U2K("badPAddr 0x"));
	iop_printx(badPAddr);
	iop_prints(U2K("\n"));
	while(1);
}

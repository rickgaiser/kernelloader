/* Copyright (c) 2007 Mega Man */
#ifndef _CP0_REGISTER_H_
#define _CP0_REGISTER_H_

#define CP0_GET_CAUSE(cause) \
	__asm__ __volatile__("mfc0 %0,$13":"=r" (cause):)

#define CP0_GET_EPC(epc) \
	__asm__ __volatile__("mfc0 %0,$14":"=r" (epc):)

#define CP0_GET_ERROR_EPC(error_epc) \
	__asm__ __volatile__("mfc0 %0,$30":"=r" (error_epc):)

#define CP0_GET_BAD_VADDR(badVAddr) \
	__asm__ __volatile__("mfc0 %0,$8":"=r" (badVAddr):)

#define CP0_SET_INDEX(x) \
	__asm__ __volatile__( \
		"mtc0 %0, $0\n" \
		"sync.p\n" \
		: : "r"(x): "memory")

#define CP0_SET_ENTRYLO0(x) \
	__asm__ __volatile__( \
		"mtc0 %0, $2\n" \
		"sync.p\n" \
		: : "r"(x): "memory")

#define CP0_SET_ENTRYLO1(x) \
	__asm__ __volatile__( \
		"mtc0 %0, $3\n" \
		"sync.p\n" \
		: : "r"(x): "memory")

#define CP0_SET_PAGEMASK(x) \
	__asm__ __volatile__( \
		"mtc0 %0, $5\n" \
		"sync.p\n" \
		: : "r"(x): "memory")

#define CP0_SET_WIRED(x) \
	__asm__ __volatile__( \
		"mtc0 %0, $6\n" \
		"sync.p\n" \
		: : "r"(x): "memory")

#define CP0_SET_ENTRYHI(x) \
	__asm__ __volatile__( \
		"mtc0 %0, $10\n" \
		"sync.p\n" \
		: : "r"(x): "memory")

#define CP0_SET_STATUS(x) \
	__asm__  __volatile__("mtc0 %0, $12\n" \
		"sync.p\n"::"r" (x))

#define CP0_GET_STATUS(x) \
	__asm__  __volatile__("mfc0 %0, $12\n" \
		"sync.p\n":"=r" (x))

#define CP0_SET_EPC(epc) \
	__asm__  __volatile__("mtc0 %0, $14\n" \
		"sync.p\n"::"r" (epc))

#define CP0_SET_CONFIG(x) \
	__asm__ __volatile__("mtc0 %0, $16\n" \
		"sync.p\n"::"r" (x))

#endif

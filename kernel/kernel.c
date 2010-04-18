/* Copyright (c) 2007 Mega Man */
#include "stdint.h"
#include "iopmem.h"
#include "bootinfo.h"
#include "memory.h"
#include "rtesif.h"
#include "stdio.h"
#include "sbcall.h"
#include "irq.h"
#include "timers.h"
#include "mmu.h"
#include "loader.h"
#include "entry.h"
#include "exception.h"
#include "thread.h"
#include "interrupts.h"
#include "cp0register.h"
#include "intc.h"
#include "dmac.h"
#include "graphic.h"
#include "ledflash.h"

#define LINEMARK() \
	do { \
		iop_printx(__LINE__); \
		iop_prints("\n"); \
	} while(0)

#define SBIOS_BASE	0x80001000

#if 0
/** PS2 bootinfo structure. */
struct ps2_bootinfo *bootinfo;
#endif

/**
 * SBIOS function entry.
 * @param sbcall Number of the function that should be called.
 * @param arg Paramaeter for function called.
 * @return Return value of function called.
 */
typedef int (sbios_t) (int sbcall, void *arg);

/** Function pointer to SBIOS. */
sbios_t *sbios;

/*
 * A vastly simplified IOP reset routine.
 */
static void reset_iop()
{
	uint8_t packet[104] __attribute__ ((aligned(16)));
	struct dma_request {
		void *src;
		void *dest;
		uint32_t size;
		uint32_t mode;
	} dmareq;
	uint8_t *upkt = KSEG1ADDR((uint8_t *) & packet);
	volatile uint32_t *p = (uint32_t *) upkt;
	unsigned int i;

	sif_init();

	for (i = 0; i < sizeof(packet) / sizeof(uint32_t); i++) {
		p[i] = 0;
	}

	/* Set the size and command ID */
	p[0] = sizeof(packet);
	p[2] = 0x80000003;

	dmareq.src = upkt;
	dmareq.dest = (void *) sif_reg_get(2);
	dmareq.size = sizeof(packet);
	dmareq.mode = 0x44;

	if (!sif_dma_request(&dmareq, 1)) {
		LINEMARK();
		while (1);
	}

	sif_reg_set(4, 0x10000);
	sif_reg_set(4, 0x20000);

	/* Wait for the IOP to come back */
	while (!(sif_reg_get(4) & 0x40000));
	sif_reg_set(4, 0x40000);
}

/**
 * Entry point of kernel.
 * @param argc unused.
 * @param argv unused.
 * @param envp Pointer to bootinfo structure includeing pointer SBIOS.
 * @param prom_vec unused.
 */
int start_kernel(int argc, char **argv, char **envp, int *prom_vec)
{
#if 0
	volatile uint32_t *crash1 = (uint32_t *) 0x00000008;
	volatile uint32_t *crash2 = (uint32_t *) 0x00001008;
	volatile uint32_t *crash3 = (uint32_t *) 0x04002008;
#endif
	register unsigned long sp asm("sp");
	volatile uint32_t *sbios_addr = (uint32_t *) 0x80001008;
	int version;

	iop_prints("Kernel started\n");

	if (sp >= (KSEG0 + ZERO_REG_ADDRESS)) {
		iop_prints("Stack pointer to big.\n");
	}
	if (sp <= (uint32_t) & _end) {
		iop_prints("Kernel is too big.\n");
	}

	/* Set config register. */
	CP0_SET_STATUS((0 << 30)	/* Deactivate COP2: VPU0 -> not used by loader */
		|(1 << 29)				/* Activate COP1: FPU -> used by compiler and functions like printf. */
		|(1 << 28)				/* Activate COP0: EE Core system control processor. */
		|(1 << 16) /* eie */ );
	/* Set status register. */
	CP0_SET_CONFIG((3 << 0)		/* Kseg0 cache */
		|(1 << 12)				/* Branch prediction */
		|(1 << 13)				/* Non blocking loads */
		|(1 << 16)				/* Data cache enable */
		|(1 << 17)				/* Instruction cache enable */
		|(1 << 18) /* Enable pipeline */ );

	irq_init_module();
	graphic_init_module();
	intc_init_module();
	dmac_init_module();
	// Working with RTE until here:

#if 0
	/* Not working with RTE disc, but working with kernelloader. */
	bootinfo = (struct ps2_bootinfo *) envp;

	sbios = *((sbios_t **) bootinfo->sbios_base);
#else
	/* Working with RTE disc and kernelloader. */
	sbios = *((sbios_t **) SBIOS_BASE);
#endif

	iop_prints("Calling sbios\n");
	version = sbios(0, NULL);
	//ledflash();
	printf("Version %d\n", version);

	mmu_init_module();
	printf("Initialized MMU.\n");
	printf("sbios_addr 0x%x\n", *sbios_addr);

	init_thread_module();
#if 0
	/* XXX: Test if exception handler is working. */
	printf("crash1 0x%x\n", *crash1);
	printf("crash2 0x%x\n", *crash2);
#endif

	install_exception_handler(V_COMMON, commonExceptionHandler);
#if 0
	printf("crash3 0x%x\n", *crash3);
	*crash3 = 0x12345678;
	printf("crash3 0x%x\n", *crash3);
#endif

	/* Install timer. */
	timer_init_module();

	/* Initialize SIF. */
	iop_prints("sif_init\n");
	sif_init();

#if 1
	/* Simulate a program using RPC was already started,
	 * because RPC is already initialized on IOP side
	 * and we will not receive Sreg 0. Otherwise SifInitRpc
	 * will hang.
	 */ 
	sif_reg_set(0x80000002, 1);
#else
	reset_iop();
#endif

	/* Enable interrupts */
	enableInterrupts();

	/* Load user space application and start it. */
	loader();

	/* Should never be reached. */
	iop_prints("Kernel exit\n");
	return (42);
}

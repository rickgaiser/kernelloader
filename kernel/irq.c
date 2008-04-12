/* Copyright (c) 2007 Mega Man */
#include "stdint.h"
#include "irq.h"
#include "memory.h"
#include "cache.h"
#include "iopmem.h"
#include "panic.h"
#include "interrupts.h"
#include "exception.h"
#include "entry.h"
#include "stdio.h"
#include "cp0register.h"

#define SBUS_SMFLG 0xb000f230

#define IRQ_NUMBER_OF_HANDLERS 3

static volatile uint64_t *sbus_smflg = (unsigned long *) SBUS_SMFLG;

static irq_handler_t *irq_handler[IRQ_NUMBER_OF_HANDLERS];

void irq_init_module(void)
{
	int i;

	iop_prints("Setup irq\n");

	/* SBUS stuff */
	*sbus_smflg = (1 << 8) | (1 << 10);

	/* Install exception handler. */
	install_exception_handler(V_INTERRUPT, interruptExceptionHandler);

	for (i = 0; i < IRQ_NUMBER_OF_HANDLERS; i++) {
		irq_handler[i] = NULL;
	}

	/* Enable interrupt source IRQ0 and IRQ1. */
	/* IRQ0: INTC */
	/* IRQ1: DMAC */
	enableIRQ();
}

void irq_interrupt(uint32_t *regs)
{
	uint32_t cause;

	//iop_prints("Interrupt happend.\n");

	CP0_GET_CAUSE(cause);

	if (cause & (1 << 10)) {
		/* Interupt source 2. */
		//printf("Interrupt source 2.\n");

		if (irq_handler[IRQ_IM2] != 0) {
			irq_handler[IRQ_IM2](regs);
		}
	}
	if (cause & (1 << 11)) {
		/* Interupt source 3. */
		//printf("Interrupt source 3.\n");

		if (irq_handler[IRQ_IM3] != 0) {
			irq_handler[IRQ_IM3](regs);
		}
	}
	if (cause & (1 << 15)) {
		/* Interupt source 7. */
		//printf("Interrupt source 7.\n");

		if (irq_handler[IRQ_IM7] != 0) {
			irq_handler[IRQ_IM7](regs);
		}
	}
}

irq_handler_t *irq_register_handler(unsigned int nr, irq_handler_t *handler)
{
	if (nr < IRQ_NUMBER_OF_HANDLERS) {
		irq_handler_t *old;

		old = irq_handler[nr];
		irq_handler[nr] = handler;

		return old;		
	} else {
		panic("irq_register_handler(); number is too big.\n");
		return NULL;
	}
}

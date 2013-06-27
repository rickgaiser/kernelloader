/* Copyright (c) 2007 Mega Man */
#include "intc.h"
#include "memory.h"
#include "panic.h"
#include "kernel.h"
#include "stdio.h"

#define INTC_MASK 0xb000f010
#define INTC_STAT 0xb000f000

#define INTC_NUMBER_OF_HANDLERS (INTC_TIMER1 + 1)

static uint64_t saved_intc_mask;

static volatile uint64_t *intc_mask = (unsigned long *) INTC_MASK;
static volatile uint64_t *intc_stat = (unsigned long *) INTC_STAT;

static irq_handler_t *intc_handler[INTC_NUMBER_OF_HANDLERS];

void intc_enable_irq(unsigned int irq_nr)
{
	DBG("intc_enable_irq(%d)\n", irq_nr);
	if (!(saved_intc_mask & (1 << irq_nr))) {
		saved_intc_mask |= 1 << irq_nr;
		*intc_mask |= 1 << irq_nr;
	}
}

void intc_disable_irq(unsigned int irq_nr)
{
	DBG("intc_disable_irq(%d)\n", irq_nr);
	if (saved_intc_mask & (1 << irq_nr)) {
		saved_intc_mask &= ~(1 << irq_nr);
		*intc_mask |= 1 << irq_nr;
	}
}

void intc_interrupt(uint32_t *regs)
{
	uint64_t mask;
	int i;

	mask = *intc_mask;
	for (i = 0; i < INTC_NUMBER_OF_HANDLERS; i++) {
		/* Check if interrupt is enabled and pending. */
		if ((mask & saved_intc_mask) & (1 << i)) {
			/* Acknowledge interrupt. */
			*intc_stat = 1 << i;

			/* Check if interrupt service routine was registered. */
			if (intc_handler[i] != NULL) {
				/* dispatch interrupt */
				intc_handler[i](regs);
			}
		}
	}
}

void intc_init_module(void)
{
	int i;

	saved_intc_mask = 0;
	*intc_mask = *intc_mask;
	*intc_stat = *intc_stat;

	for (i = 0; i < INTC_NUMBER_OF_HANDLERS; i++) {
		intc_handler[i] = NULL;
	}

	/* Register an interrupt handler. */
	irq_register_handler(IRQ_IM2, intc_interrupt);
}

irq_handler_t *intc_register_handler(unsigned int nr, irq_handler_t *handler)
{
	if (nr < INTC_NUMBER_OF_HANDLERS) {
		irq_handler_t *old;

		old = intc_handler[nr];
		intc_handler[nr] = handler;

		return old;		
	} else {
		panic("irq_register_handler(); number is too big.\n");
		return NULL;
	}
}

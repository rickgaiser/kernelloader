/* Copyright (c) 2007 Mega Man */
#include "dmac.h"
#include "kernel.h"
#include "memory.h"
#include "panic.h"
#include "cp0register.h"
#include "stdio.h"

#define DMAC_STAT 0xb000e010
#define DMAC_MASK 0xb000e010
#define DMAC_NUMBER_OF_HANDLERS (DMAC_MEIM + 1)

static uint64_t saved_dmac_mask;

static volatile uint64_t *dmac_mask = (unsigned long *) DMAC_MASK;
static volatile uint64_t *dmac_stat = (unsigned long *) DMAC_STAT;

/** Kernel DMAC interrupt handler. */
static irq_handler_t *dmac_handler[DMAC_NUMBER_OF_HANDLERS];
/** User space DMAC interrupt handler. */
static dmac_useg_handler_t *dmac_useg_handler[DMAC_NUMBER_OF_HANDLERS];

void dmac_enable_irq(unsigned int irq_nr)
{
	if (!(saved_dmac_mask & (1 << irq_nr))) {
		saved_dmac_mask |= 1 << irq_nr;
		*dmac_mask |= 1 << irq_nr;
	}
}

void dmac_disable_irq(unsigned int irq_nr)
{
	if (saved_dmac_mask & (1 << irq_nr)) {
		saved_dmac_mask &= ~(1 << irq_nr);
		*dmac_mask |= 1 << irq_nr;
	}
}

void dmac_interrupt(uint32_t * regs)
{
	uint64_t mask;
	int i;

	mask = *dmac_mask;
	for (i = 0; i < DMAC_NUMBER_OF_HANDLERS; i++) {
		/* Check if interrupt is enabled and pending. */
		if ((mask & saved_dmac_mask) & (1 << i)) {
			//printf("DMAC interrupt %d pending.\n", i);
			/* Acknowledge interrupt. */
			*dmac_stat = 1 << i;

			/* Check if interrupt service routine was registered. */
			if ((dmac_handler[i] != NULL) || (dmac_useg_handler[i] != NULL)) {
				if (dmac_handler[i] != NULL) {
					/* dispatch interrupt */
					dmac_handler[i] (regs);
				}
				if (dmac_useg_handler[i] != NULL) {
					printf("Calling DMAC user space handler at 0x%08x\n", dmac_handler[i]);
					/* dispatch interrupt */
					/* XXX; Don't know if this is a good idea, direct call from
					 * kernel in exception handler.
					 */
					dmac_handler[i](i - DMAC_CIM0);
				}
			}
		}
	}
}

void dmac_init_module(void)
{
	int i;

	saved_dmac_mask = 0;
	*dmac_mask = *dmac_mask;
	*dmac_stat = *dmac_stat;

	for (i = 0; i < DMAC_NUMBER_OF_HANDLERS; i++) {
		dmac_handler[i] = NULL;
		dmac_useg_handler[i] = NULL;
	}

	/* Register an interrupt handler. */
	irq_register_handler(IRQ_IM3, dmac_interrupt);
}

irq_handler_t *dmac_register_handler(unsigned int nr, irq_handler_t * handler)
{
	if (nr < DMAC_NUMBER_OF_HANDLERS) {
		irq_handler_t *old;

		old = dmac_handler[nr];
		dmac_handler[nr] = handler;

		return old;
	}
	else {
		panic("irq_register_handler(); number is too big.\n");
		return NULL;
	}
}

int32_t syscallAddDmacHandler(uint32_t channel, dmac_useg_handler_t *handler, int32_t next)
{
	printf("syscallAddDmacHandler(%d, 0x%x, %d)\n", channel, handler, next);

	if (channel > (DMAC_CIM9 - DMAC_CIM0)) {
		return -1;
	}
	dmac_useg_handler[channel + DMAC_CIM0] = handler;

	return channel;
}

int32_t syscallEnableDmac(uint32_t channel)
{
	int irq_nr;

	printf("syscallEnableDmac(%d)\n", channel);

	irq_nr = channel + DMAC_CIM0;
	if (!(saved_dmac_mask & (1 << irq_nr))) {
		return 0;
	}
	dmac_enable_irq(irq_nr);
	return 1;
}

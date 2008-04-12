/* Copyright (c) 2007 Mega Man */
#ifndef _IRQ_H_
#define _IRQ_H_

#include "stdint.h"

/** IRQ nr of external interrupt 2. */
#define IRQ_IM2 0
/** IRQ nr of external interrupt 3. */
#define IRQ_IM3 1
/** IRQ nr of external interrupt 7. */
#define IRQ_IM7 2

/** Interrupt handler function definition. */
typedef void irq_handler_t(uint32_t *regs);

void irq_init_module(void);

/** Register an interrupt handler. */
irq_handler_t *irq_register_handler(unsigned int nr, irq_handler_t *handler);

#endif

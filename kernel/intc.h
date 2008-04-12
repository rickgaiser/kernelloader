/* Copyright (c) 2007 Mega Man */
#ifndef _INTC_H_
#define _INTC_H_

#include "stdint.h"
#include "irq.h"

enum 
{ 
   INTC_GS = 0, 
   INTC_SBUS, 
   INTC_VBLANK_START, 
   INTC_VBLANK_END, 
   INTC_VIF0, 
   INTC_VIF1, 
   INTC_VU0, 
   INTC_VU1, 
   INTC_IPU, 
   INTC_TIMER0, 
   INTC_TIMER1 
}; 

void intc_enable_irq(unsigned int irq_nr);
void intc_disable_irq(unsigned int irq_nr);
void intc_init_module(void);
irq_handler_t *intc_register_handler(unsigned int nr, irq_handler_t *handler);
void intc_interrupt(uint32_t *regs);

#endif

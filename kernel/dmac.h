/* Copyright (c) 2007 Mega Man */
#ifndef _DMAC_H_
#define _DMAC_H_

#include "irq.h"
#include "stdint.h"

enum {
	DMAC_CIS0 = 0,
	DMAC_CIS1,
	DMAC_CIS2,
	DMAC_CIS3,
	DMAC_CIS4,
	DMAC_CIS5,
	DMAC_CIS6,
	DMAC_CIS7,
	DMAC_CIS8,
	DMAC_CIS9,
	DMAC_SIS = 13,
	DMAC_MEIS,
	DMAC_BEIS,
	DMAC_CIM0,
	DMAC_CIM1,
	DMAC_CIM2,
	DMAC_CIM3,
	DMAC_CIM4,
	DMAC_CIM5,
	DMAC_CIM6,
	DMAC_CIM7,
	DMAC_CIM8,
	DMAC_CIM9,
	DMAC_SIM = 29,
	DMAC_MEIM
};

void dmac_enable_irq(unsigned int irq_nr);
void dmac_disable_irq(unsigned int irq_nr);
void dmac_init_module(void);
irq_handler_t *dmac_register_handler(unsigned int nr, irq_handler_t *handler);
void dmac_interrupt(uint32_t *regs);

#endif

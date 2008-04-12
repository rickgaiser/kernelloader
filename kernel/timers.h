/* Copyright (c) 2007 Mega Man */
#ifndef _TIMERS_H_
#define _TIMERS_H_

#include "stdint.h"

void timer_init_module(void);
uint64_t timer_time(void);
void timer_exit_module(void);
void timer_interrupt(uint32_t *regs);

#endif

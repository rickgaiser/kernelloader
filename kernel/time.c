/* Copyright (c) 2007 Mega Man */
#include "time.h"

/** EE frequency is 294.912 MHz. */
#define TICKS_PER_MICROSECOND 294

#define GET_COUNT(x) \
	__asm__ __volatile__("mfc0 %0,$9":"=r" (x):);

#define SET_COUNT(x) \
	__asm__ __volatile__("mtc0 %0,$9"::"r" (x):);

void udelay(uint32_t time)
{
	uint32_t now;
	uint32_t end;

	end = TICKS_PER_MICROSECOND * time;

	do {
		GET_COUNT(now);
	} while(now < end);
}

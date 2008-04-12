/* Copyright (c) 2007 Mega Man */
#include "panic.h"
#include "stdio.h"
#include "iopmem.h"
#include "interrupts.h"

#define MAX_BUFFER 256

void panic(const char *format, ...)
{
	char buffer[MAX_BUFFER];
	int ret;
	va_list varg;

	disableInterrupts();
	iop_prints("panic\n");
	va_start(varg, format);
	ret = vsnprintf(buffer, MAX_BUFFER, format, varg);
	iop_prints(buffer);
	va_end(varg);
	

	while(1) {
	}
}

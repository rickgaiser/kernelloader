/* Copyright (c) 2007 Mega Man */
#include <stdarg.h>
#include "stdio.h"
#include "iopmem.h"

#define MAX_BUFFER 256

int printf(const char *format, ...)
{
	char buffer[MAX_BUFFER];
	int ret;
	va_list varg;
	va_start(varg, format);
	ret = vsnprintf(buffer, MAX_BUFFER, format, varg);
	iop_prints(buffer);
	va_end(varg);
	return ret;
}


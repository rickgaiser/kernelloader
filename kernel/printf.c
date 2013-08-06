/* Copyright (c) 2007 Mega Man */
#include "stdint.h"
#include <stdarg.h>
#include "stdio.h"
#include "iopmem.h"
#include "sbcall.h"
#include "rtesif.h"

#define MAX_BUFFER 256

static void sio_putc(int c)
{
	struct sb_putchar_arg arg;

	arg.c = c;
	sbios(SB_PUTCHAR, &arg);
}

static void sio_prints(const char *text)
{
	while(*text != 0) {
		if (*text == '\n') {
			sio_putc('\r');
		}
		sio_putc(*text);
		text++;
	}
}

int printf(const char *format, ...)
{
	char buffer[MAX_BUFFER];
	int ret;
	va_list varg;
	va_start(varg, format);
	ret = vsnprintf(buffer, MAX_BUFFER, format, varg);
	sio_prints(buffer);
	iop_prints(buffer);
	va_end(varg);
	return ret;
}


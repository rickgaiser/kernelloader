/* Copyright (c) 2007 - 2012 Mega Man */
#include <stdarg.h>
#include "stdio.h"
#include "iopmemdebug.h"
#include "fileio.h"
#include "string.h"

#define MAX_BUFFER 256

int printf(const char *format, ...)
{
	char buffer[MAX_BUFFER];
	int ret;
	va_list varg;
	va_start(varg, format);
	ret = vsnprintf(buffer, MAX_BUFFER, format, varg);
#ifdef SIO_DEBUG
	sio_puts(buffer);
#endif
#ifdef SHARED_MEM_DEBUG
	if (iop_prints(buffer)) {
		/* Module is not loaded, use different method for printing string. */
#endif
#ifdef CALLBACK_DEBUG
		callback_prints(buffer);
#endif
#if defined(FILEIO_DEBUG) && !defined(SBIOS_DEBUG)
		fioWrite(1, buffer, strlen(buffer));
#endif
#ifdef SHARED_MEM_DEBUG
	}
#endif
	va_end(varg);
	return ret;
}

int puts(const char *buffer)
{
#ifdef SIO_DEBUG
	sio_puts(buffer);
#endif
#ifdef SIO_DEBUG
	sio_putc('\n');
#endif
#ifdef SHARED_MEM_DEBUG
	if (iop_prints(buffer)) {
		/* Module is not loaded, use different method for printing string. */
#endif
#ifdef CALLBACK_DEBUG
		callback_prints(buffer);
		callback_prints("\n");
#endif
#if defined(FILEIO_DEBUG) && !defined(SBIOS_DEBUG)
		fioWrite(1, buffer, strlen(buffer));
		fioWrite(1, "\n", 1);
#endif
#ifdef SHARED_MEM_DEBUG
	} else {
		iop_putc('\n');
	}
#endif
	return 0;
}

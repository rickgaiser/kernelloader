/* Copyright (c) 2007 - 2012 Mega Man */
#include <stdarg.h>
#include "stdio.h"
#include "iopmemdebug.h"
#include "fileio.h"
#include "string.h"

#define MAX_BUFFER 256

#if defined(SHARED_MEM_DEBUG) || defined(CALLBACK_DEBUG) || defined(FILEIO_DEBUG)
int printf(const char *format, ...)
{
	char buffer[MAX_BUFFER];
	int ret;
	va_list varg;
	va_start(varg, format);
	ret = vsnprintf(buffer, MAX_BUFFER, format, varg);
#ifdef SHARED_MEM_DEBUG
	if (iop_prints(buffer)) {
		/* Module is not loaded, use different method for printing string. */
#endif
#ifdef CALLBACK_DEBUG
		callback_prints(buffer);
#endif
#ifdef FILEIO_DEBUG
		fioWrite(1, buffer, strlen(buffer));
#endif
#ifdef SHARED_MEM_DEBUG
	}
#endif
	va_end(varg);
	return ret;
}
#endif

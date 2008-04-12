/* Copyright (c) 2007 Mega Man */
#include <stdarg.h>
#include "stdio.h"
#include "iopmemdebug.h"
#include "fileio.h"
#include "string.h"

#define MAX_BUFFER 256

#ifdef SHARED_MEM_DEBUG
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
#endif

#ifdef FILEIO_DEBUG
int rpc_printf(const char *format, ...)
{
	char buffer[MAX_BUFFER];
	int ret;
	va_list varg;
	va_start(varg, format);
	ret = vsnprintf(buffer, MAX_BUFFER, format, varg);
	fioWrite(1, buffer, strlen(buffer));
	va_end(varg);
	return ret;
}
#endif

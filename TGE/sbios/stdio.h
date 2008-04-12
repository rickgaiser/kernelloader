/* Copyright (c) 2007 Mega Man */
#ifndef _STDIO_H_
#define _STDIO_H_

#include <stdarg.h>

#ifdef SHARED_MEM_DEBUG
int printf(const char *format, ...);
#else
/** Do nothing. */
#define printf(args...)
#endif
int snprintf(char *str, int len, const char *fmt, ...);
int vsnprintf(char *b, int len, const char *fmt, va_list pvar);

#endif

/* Copyright (c) 2007 - 2012 Mega Man */
#ifndef _STDIO_H_
#define _STDIO_H_

#include <stdarg.h>

#if defined(SHARED_MEM_DEBUG) || defined(CALLBACK_DEBUG) || defined(FILEIO_DEBUG)
int printf(const char *format, ...);
#else
/** Do nothing. */
/** Don't print something. */
#define printf(args...) do {} while(0)
#endif
int snprintf(char *str, int len, const char *fmt, ...);
int vsnprintf(char *b, int len, const char *fmt, va_list pvar);

#ifdef CALLBACK_DEBUG
typedef void callback_prints_t(const char *text);
extern callback_prints_t *callback_prints;
#endif


#endif

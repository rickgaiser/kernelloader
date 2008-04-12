/* Copyright (c) 2007 Mega Man */
#ifndef _STDIO_H_
#define _STDIO_H_

#include <stdarg.h>

#ifdef FILEIO_DEBUG
int rpc_printf(const char *format, ...);
#else
/** Do nothing. */
#define rpc_printf(args...) do {} while(0)
#endif
#ifdef SHARED_MEM_DEBUG
int printf(const char *format, ...);
#else
#ifdef FILEIO_DEBUG
#define printf rpc_printf
#else
/** Do nothing. */
/** Don't print something. */
#define printf(args...) do {} while(0)
#endif
#endif
int snprintf(char *str, int len, const char *fmt, ...);
int vsnprintf(char *b, int len, const char *fmt, va_list pvar);

#endif

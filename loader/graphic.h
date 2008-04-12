/* Copyright (c) 2007 Mega Man */
#ifndef _GRAPHIC_H_
#define _GRAPHIC_H_

#ifdef __cplusplus
#include "menu.h"
#include "config.h"

Menu *graphic_main(graphic_mode_t mode);
void graphic_paint(void);
int setCurrentMenu(void *arg);
Menu *getCurrentMenu(void);
#endif

#ifdef __cplusplus
extern "C" {
#endif
	void graphic_setPercentage(int percentage, const char *name);
	int setErrorMessage(const char *text);
	const char *getErrorMessage(void);
	int error_printf(const char *format, ...);
	void graphic_setStatusMessage(const char *text);
#ifdef __cplusplus
}
#endif

#endif

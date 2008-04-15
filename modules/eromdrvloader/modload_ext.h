/* Copyright 2008 Mega Man. */
#ifndef _MODLOAD_EXT_H_
#define _MODLOAD_EXT_H_

#include <modload.h>

int LoadStartModuleExt(const char *name, int arglen, const char *args, int *result);
#define I_LoadStartModuleExt DECLARE_IMPORT(11, LoadStartModuleExt)

#endif

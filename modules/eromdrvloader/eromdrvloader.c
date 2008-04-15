/* Copyright 2008 Mega Man. */
#include <tamtypes.h>
#include <thbase.h>
#include <thsemap.h>

#include "irx_imports.h"

int _start(int argc, char *argv[])
{
	int rv;
	int ret;

	/* Load eromdrv to activate video DVD support, special function
	 * for loading is required.
	 */
	ret = LoadStartModuleExt("rom1:EROMDRV", 0, NULL, &rv);
	if (ret >= 0) {
		ret = rv;
	}

	if (ret < 0) {
		return ret;
	} else {
		return 0;
	}
}

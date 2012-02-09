/* Copyright 2008 - 2012 Mega Man. */
#include <tamtypes.h>
#include <thbase.h>
#include <thsemap.h>
#include <ioman_mod.h>

#include "irx_imports.h"

int _start(int argc, char *argv[])
{
	int rv;
	int ret;
	int fd;
	const char *name;

	if (argc > 1) {
		name = argv[1];

		/* Load eromdrv to activate video DVD support, special function
		 * for loading is required.
		 */
		printf("eromdrvloader: Loading module \"%s\".\n", name);
		ret = LoadStartModuleExt(name, 0, NULL, &rv);
		if (ret >= 0) {
			ret = rv;
		}

		if (ret < 0) {
			return ret;
		} else {
			return 0;
		}
	} else {
		printf("eromdrvloader: Parameter is missing.\n");
		return -22;
	}
}

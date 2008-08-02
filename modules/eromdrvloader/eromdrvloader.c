/* Copyright 2008 Mega Man. */
#include <tamtypes.h>
#include <thbase.h>
#include <thsemap.h>
#include <ioman_mod.h>

#include "irx_imports.h"

/** Name for driver to load, E = Europe is default. */
static char name[] = "rom1:EROMDRVE";
static char version[256];

int _start(int argc, char *argv[])
{
	int rv;
	int ret;
	int fd;

	/* Detect file to load. */
	ret = open("rom1:EROMDRV", O_RDONLY);
	if (ret >=0 ) {
		/* This is an old fat PS2. */
		close(ret);
		name[12] = 0;
	} else {
		/* This is a new slim PSTwo. */
		fd = open("rom0:ROMVER", O_RDONLY);
		if (fd >= 0) {
			ret = read(fd, version, sizeof(version));
			close(fd);
			if (ret > 5) {
				printf("ROM Version: %s\n", version);
				/* Get region letter from version number. */
				name[12] = version[4];
				ret = open(name, O_RDONLY);
				if (ret >=0 ) {
					close(ret);
				}
			}
		}
	}

	/* Load eromdrv to activate video DVD support, special function
	 * for loading is required.
	 */
	printf("Loading module \"%s\".\n", name);
	ret = LoadStartModuleExt(name, 0, NULL, &rv);
	if (ret >= 0) {
		ret = rv;
	}

	if (ret < 0) {
		return ret;
	} else {
		return 0;
	}
}

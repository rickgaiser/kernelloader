/* Copyright 2008 Mega Man. */
#include <tamtypes.h>
#include <thbase.h>
#include <thsemap.h>
#include <ioman_mod.h>

#include "irx_imports.h"

/* NVM offsets. */
#define NVM_FAKE_REGION 0x185
#define NVM_REAL_REGION 0x186
#define NVM_CONSOLE_TYPE 0x1b0

/** Name for driver to load, E = Europe is default. */
static char name[] = "rom1:EROMDRVE";
static char version[256];
/** Store copy of DVD internal NVRAM. */
static u8 nvm[0x400];

int _start(int argc, char *argv[])
{
	int rv;
	int ret;
	int fd;

	/* Detect file to load. */
	ret = open("rom1:EROMDRV", O_RDONLY);
	if (ret >=0 ) {
		/* This is an old fat PS2 (working with SCPH-50004 and SCPH-39004). */
		close(ret);
		name[12] = 0;
	} else {
		/* This is a new slim PSTwo. */
		fd = open("rom0:ROMVER", O_RDONLY);
		if (fd >= 0) {
			ret = read(fd, version, sizeof(version));
			close(fd);
			if (ret > 5) {
				u16 data;
				u8 stat;
				u32 addr;

				printf("ROM Version: %s\n", version);

				/* Get default region letter from version number. */
				name[12] = version[4];

				CdInit(1);

				memset(nvm, 0, sizeof(nvm));
				for (addr = 0; addr < sizeof(nvm)/2; addr++) {
					ret = sceCdReadNVM(addr, &data, &stat);
					if (ret != 1) {
						printf("sceCdReadNVM Error: ret = %d, addr = 0x%04x data = 0x%04x, stat = 0x%02x\n", ret, 2 * addr, data, stat);
					} else {
						nvm[addr * 2] = data & 0xFF;
						nvm[addr * 2 + 1] = (data >> 8) & 0xFF;
					}
				}
				printf("Console: %s\n", nvm + NVM_CONSOLE_TYPE);
				printf("NVM region code 0x%02x 0x%02x\n", nvm[NVM_FAKE_REGION], nvm[NVM_REAL_REGION]);
				if (nvm[NVM_FAKE_REGION] == version[4]) {
					/* NVM layout seems to be correct. */
					name[12] = nvm[NVM_REAL_REGION];

					printf("Checking file \"%s\".\n", name);
					ret = open(name, O_RDONLY);
					if (ret >=0 ) {
						/* Region code seems to be correct. */
						close(ret);
					} else {
						/* Region code is wrong. */

						/* This will work at least for region 'E'. */
						name[12] = version[4];
						printf("Failed to get region from NVM (ret = %d).\n", ret);
					}
				} else {
					printf("NVM layout seems to be wrong.\n");
				}
		
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

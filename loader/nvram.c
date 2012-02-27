#include <stdio.h>
#include <string.h>
#include <libcdvd.h>

#include "nvram.h"
#include "modules.h"

typedef struct {
	int version;
	u32 fake_region;
	u32 real_region;
	u32 console_type;
} nvm_offsets_t;

/* NVM offsets. */
nvm_offsets_t nvmOff[] = {
	{
		.version = 0x000,
		.fake_region = 0x185,
		.real_region = 0x186,
		.console_type = 0x1a0,
	},
	{
		.version = 0x170,
		.fake_region = 0x185,
		.real_region = 0x186,
		.console_type = 0x1b0,
	},
};

/** Store copy of DVD internal NVRAM. */
static u8 nvm[0x400];

char ps2_console_type[32] = "CDVD error";
char ps2_region_type[32] = "CDVD error";
int nvm_errors;

void nvram_init(void)
{
	int rv;
	u32 addr;
	u16 data;
	u8 stat;
	int version;
	unsigned int type;
	nvm_offsets_t *off = &nvmOff[0];

	version = getBiosVersion();
	for (type = 0; type < (sizeof(nvmOff)/sizeof(nvmOff[0])); type++) {
		if (nvmOff[type].version <= version) {
			off = &nvmOff[type];
		}
	}

	rv = cdInit(CDVD_INIT_INIT);
	if (rv != 1) {
		printf("Error: cdInit(CDVD_INIT_INIT) failed\n");
		return;
	}

	nvm_errors = 0;
	memset(nvm, 0, sizeof(nvm));
	for (addr = 0; addr < sizeof(nvm)/2; addr++) {
		rv = cdReadNVM(addr, &data, &stat);
		if (rv != 1) {
			printf("sceCdReadNVM Error: rv = %d, addr = 0x%04x data = 0x%04x, stat = 0x%02x\n", rv, 2 * addr, data, stat);
			nvm_errors++;
		} else {
			nvm[addr * 2] = data & 0xFF;
			nvm[addr * 2 + 1] = (data >> 8) & 0xFF;
		}
	}
	rv = cdInit(CDVD_INIT_EXIT);
	if (rv != 1) {
		printf("Error: cdInit(CDVD_INIT_EXIT) failed\n");
	}

	memcpy(ps2_console_type, &nvm[off->console_type], sizeof(ps2_console_type));
	ps2_console_type[sizeof(ps2_console_type) - 1] = 0;
	printf("PS2 Console type: %s\n", ps2_console_type);
	snprintf(ps2_region_type, sizeof(ps2_region_type), "0x%02x 0x%02x (%d NVM errors)", nvm[off->fake_region], nvm[off->real_region], nvm_errors);
}

u8 *get_nvram(void)
{
	return nvm;
}

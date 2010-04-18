#include <tamtypes.h>
#include <thbase.h>
#include <thsemap.h>
#include <sifrpc.h>
#include <ioman.h>

#include <cdvdman.h>
#include <sysclib.h>

#include "sharedmem.h"

#define MODNAME SHAREDMEM_MODULE_NAME
IRX_ID(MODNAME, 1, 1);

static void ioThread(void *param);

/** The EE side searches for this structure in the data and BSS segement. */
volatile sharedmem_dbg_t dbg __attribute__((aligned(16))) =
{
	.magic = SHAREDMEM_MAGIC,
	.shared = {
		0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0,
	}
};

/** IOP Module entry point, install i/o thread listening on shared memory address. */
int _start(int argc, char **argv)
{
	iop_thread_t param;
	int th;

	printf("Started module " MODNAME ", magic at 0x%08x\n", &dbg);
	printf("sharedMem[0] = 0x%02x\n", dbg.shared[0]);
	dbg.shared[0] = 0;

	param.attr = TH_C;
	param.thread = ioThread;
	param.priority = 40;
	param.stacksize = 0x800;
	param.option = 0;
	th = CreateThread(&param);
	if (th > 0) {
		StartThread(th,0);
		return 0;
	} else {
		printf("%s: Failed to start io thread.\n");
		return 1;
	}
}

static void ioThread(void *param)
{
	printf("Started io thread\n");
	while(1) {
		/* Wait until something has been received. */
		while(dbg.shared[0] == 0) {
			DelayThread(1000);
		}
		/* Print received character. */
		printf("%c", dbg.shared[1]);
		dbg.shared[0] = 0;
	}
}

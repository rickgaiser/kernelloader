#include <tamtypes.h>
#include <thbase.h>
#include <thsemap.h>
#include <sifrpc.h>
#include <ioman.h>

#include <cdvdman.h>
#include <sysclib.h>

#define MODNAME "sharedmem"
IRX_ID(MODNAME, 1, 1);

static void ioThread(void *param);

/** IOP memory address used as shared memory. */
volatile unsigned char *sharedMem = (unsigned char *) 0x001ff000;

/** IOP Module entry point, install i/o thread listening on shared memory address. */
int _start(int argc, char **argv)
{
	iop_thread_t param;
	int th;

	printf("sharedMem = 0x%02x\n", *sharedMem);
	sharedMem[0] = 0;

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
#if 0
	int c;

	c = 0;
#endif
	printf("Started io thread\n");
	while(1) {
		/* Wait until something has been received. */
		while(sharedMem[0] == 0) {
#if 0
			c++;
			if (c == 500) {
				c = 0;
				printf("sharedmem SifReg 31 %d\n", sceSifGetSreg(31));
			}
#endif
			DelayThread(1000);
		}
		/* Print received character. */
		printf("%c", sharedMem[1]);
		sharedMem[0] = 0;
	}
}

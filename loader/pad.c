/* Copyright (c) 2007 Mega Man */
#include <stdio.h>

#include <kernel.h>
#include <sifrpc.h>
#include <libpad.h>
#include <loadfile.h>

#include "pad.h"

#define PADCOUNT 2

// pad_dma_buf is provided by the user, one buf for each pad
// contains the pad's current state
static char padBuf[PADCOUNT][256] __attribute__((aligned(64)));
static char actAlign[PADCOUNT][6];
static int actuators[PADCOUNT];
static int padInitialized[PADCOUNT];

int initializePad(int port, int slot);

void initializeController(void)
{
	int ret;

	int port; // 0 -> Connector 1, 1 -> Connector 2
	int slot = 0; // Always zero if not using multitap

	if (padInit(0) != 0) {
		printf("padInit() failed.\n");
		return;
	}

	for (port = 0; port < PADCOUNT; port++)
	{
		if((ret = padPortOpen(port, slot, padBuf[port])) == 0) {
			printf("padOpenPort failed: %d\n", ret);
			return;
		}
	
		if(!initializePad(port, slot)) {
			printf("pad %d initalization failed!\n", port);
		}
	}
}

void deinitializeController(void)
{
	int ret;

	int port; // 0 -> Connector 1, 1 -> Connector 2
	int slot = 0; // Always zero if not using multitap

	for (port = 0; port < PADCOUNT; port++)
	{
		ret = padPortClose(port, slot);
		if (ret != 1) {
			printf("padClosePort failed: %d\n", ret);
		}
	}
	padEnd();
	padReset();
}

/*
 * isPadReady()
 */
int isPadReady(int port, int slot)
{
    int state;
    int laststate = -1;
    char stateString[16];

    state = padGetState(port, slot);
    while((state != PAD_STATE_STABLE) && (state != PAD_STATE_FINDCTP1))
    {
	    if ((state == PAD_STATE_DISCONN)
		    || (state == PAD_STATE_ERROR))
		    return 0;
	    if (state != laststate)
	    {
	            padStateInt2String(state, stateString);
        	    printf("Please wait, pad(%d,%d) is in state %s\n", 
	                       port, slot, stateString);
		    laststate = state;
	    }
            state = padGetState(port, slot);
    }
    return -1;
}

/*
 * initializePad()
 */
int
initializePad(int port, int slot)
{

    int ret;
    int modes;
    int i;

    padInitialized[port] = 0;

    if (!isPadReady(port, slot))
	    return 0;

    // How many different modes can this device operate in?
    // i.e. get # entrys in the modetable
    modes = padInfoMode(port, slot, PAD_MODETABLE, -1);
    printf("The device has %d modes\n", modes);

    if (modes > 0) {
        printf("( ");
        for (i = 0; i < modes; i++) {
            printf("%d ", padInfoMode(port, slot, PAD_MODETABLE, i));
        }
        printf(")");
    }

    printf("It is currently using mode %d\n", 
               padInfoMode(port, slot, PAD_MODECURID, 0));

    // If modes == 0, this is not a Dual shock controller 
    // (it has no actuator engines)
    if (modes == 0) {
		printf("This is a digital controller?\n");
		padInitialized[port] = -1;
        return 1;
    }

    // Verify that the controller has a DUAL SHOCK mode
    i = 0;
    do {
        if (padInfoMode(port, slot, PAD_MODETABLE, i) == PAD_TYPE_DUALSHOCK)
            break;
        i++;
    } while (i < modes);
    if (i >= modes) {
        printf("This is no Dual Shock controller\n");
		padInitialized[port] = -1;
        return 1;
    }

    // If ExId != 0x0 => This controller has actuator engines
    // This check should always pass if the Dual Shock test above passed
    ret = padInfoMode(port, slot, PAD_MODECUREXID, 0);
    if (ret == 0) {
        printf("This is no Dual Shock controller??\n");
		padInitialized[port] = -1;
        return 1;
    }

    printf("Enabling dual shock functions of pad %d\n", port);

    // When using MMODE_LOCK, user cant change mode with Select button
    padSetMainMode(port, slot, PAD_MMODE_DUALSHOCK, PAD_MMODE_LOCK);

    if (!isPadReady(port, slot))
    {
	    printf("Failed pad %d\n", port);
	    return 0;
    }
    printf("infoPressMode: %d\n", padInfoPressMode(port, slot));

    if (!isPadReady(port, slot))
	    return 0;
    printf("enterPressMode: %d\n", padEnterPressMode(port, slot));

    if (!isPadReady(port, slot))
	    return 0;
    actuators[port] = padInfoAct(port, slot, -1, 0);
    printf("# of actuators: %d\n",actuators[port]);

    if (actuators[port] != 0) {
        actAlign[port][0] = 0;   // Enable small engine
        actAlign[port][1] = 1;   // Enable big engine
        actAlign[port][2] = 0xff;
        actAlign[port][3] = 0xff;
        actAlign[port][4] = 0xff;
        actAlign[port][5] = 0xff;

    	if (!isPadReady(port, slot))
	    return 0;
        printf("padSetActAlign: %d\n", 
                   padSetActAlign(port, slot, actAlign[port]));
    }
    else {
        printf("Did not find any actuators.\n");
    }

    if (!isPadReady(port, slot))
	    return 0;

    padInitialized[port] = -1;
    return 1;
}

int readPad(int port)
{
	struct padButtonStatus buttons;
	u32 paddata;
	int ret;
	int slot = 0;

	if (!padInitialized[port])
		initializePad(port, slot);
	if (!padInitialized[port])
		return 0;

	ret=padGetState(port, slot);
	while((ret != PAD_STATE_STABLE) && (ret != PAD_STATE_FINDCTP1)) {
		if(ret==PAD_STATE_DISCONN) {
			//printf("Pad(%d, %d) is disconnected\n", port, slot);
			return 0;
		}
		ret=padGetState(port, slot);
	}

	ret = padRead(port, slot, &buttons); // port, slot, buttons

	if (ret != 0) {
            	paddata = 0xffff ^ buttons.btns;
		/*paddata = 0xffff ^ ((buttons.btns[0] << 8) | 
			buttons.btns[1]);*/

		return paddata;
	}
	else
		return 0;
}
 

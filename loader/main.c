/* Copyright (c) 2007 Mega Man */
#include <gsKit.h>

#include "config.h"
#include "graphic.h"
#include "loader.h"

/**
 * Entry point for loader.
 * @param argc unused.
 * @param argv unsused.
 * @returns Function doesn't return.
 */
int main(int argc, char **argv)
{
	graphic_mode_t mode;

	if(*((char *)0x1FC80000 - 0xAE) != 'E') {
		mode = MODE_NTSC;
	} else {
		mode = MODE_PAL;
	}
	/* Setup graphic screen. */
	graphic_main(mode);
	/* Load kernel. */
	loader(mode);

	/* not reached! */
	return 0;
}

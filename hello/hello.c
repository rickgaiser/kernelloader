/*
# _____     ___ ____     ___ ____
#  ____|   |    ____|   |        | |____|
# |     ___|   |____ ___|    ____| |    \    PS2DEV Open Source Project.
#-----------------------------------------------------------------------
# Copyright 2001-2004, ps2dev - http://www.ps2dev.org
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.
#
# $Id$
# Hello sample
*/

#include <stdio.h>
#include <debug.h>

int main()
{   
	init_scr();
	scr_printf("Hello world!\n");
	printf("Hello world!\n");
	printf("This a simple text output in user space using SIF RPC.\n");
    
	return 0;
}

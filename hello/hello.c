/* $Id$ */

#include <stdio.h>
#include <debug.h>

int main()
{   
	init_scr();
	scr_printf("\n");
	scr_printf("    The PS2 test kernel is working.\n");
	scr_printf("    Now you can select a Linux kernel from the kernelloader menu.\n");

	printf("The PS2 test kernel is working.\n");
	printf("Now you can select a Linux kernel from the kernelloader menu.\n");
    
	return 0;
}

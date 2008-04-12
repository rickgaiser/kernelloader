/* Copyright (c) 2007 Mega Man */
#include "mipsregister.h"

.text
.global jump2kernelspace

/* Set stackpointer to kernel KSEG area.
 * Jump back to caller, but into KSEG area.
 * @param a0 Address of KSEG0.
 */
.type jump2kernelspace,@function;
.ent jump2kernelspace
jump2kernelspace:
	or ra, ra, a0
	or sp, sp, a0
	jr ra
	nop
.end jump2kernelspace

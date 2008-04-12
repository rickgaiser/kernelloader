/* Copyright (c) 2007 Mega Man */
#include "mipsregister.h"
#include "mmu.h"
#include "memory.h"
#include "entry.h"

.data
initKernelStack:
.long KSEG0 + ZERO_REG_ADDRESS - 0x10000 - STACK_OVERHEAD

.text
.globl	_start
.type	_start,@function;
.ent	_start
_start:
	lw	sp, initKernelStack
	j	start_kernel
	nop
.end	_start

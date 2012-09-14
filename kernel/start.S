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
#ifndef PS2_EE
	lui	gp, %hi(_gp)
	addiu	gp, %lo(_gp)
#endif
	lw	sp, initKernelStack
#ifdef PS2_EE
	j	start_kernel
#else
	la	t9, start_kernel
	jal	t9
#endif
	nop
.end	_start

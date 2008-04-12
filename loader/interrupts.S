/* Copyright (c) 2007 Mega Man */
#include "mipsregister.h"

.global disable_interrupts
.global enable_interrupts

.type	disable_interrupts,@function;
.ent	disable_interrupts
disable_interrupts:
	.set push
	.set noreorder
	.set noat

	mfc0 at,$12
	ori at,0x1f
	xori at,0x1e
	mtc0 at,$12
	sync.p
	jr ra

	.set pop
.end disable_interrupts

.type	enable_interrupts,@function;
.ent	enable_interrupts
enable_interrupts:
	.set push
	.set noreorder
	.set noat

	mfc0 at,$12
	ori at,1
	xori at,1
	mtc0 at,$12
	sync.p
	nop
	nop
	nop
	jr ra

	.set pop
.end	enable_interrupts


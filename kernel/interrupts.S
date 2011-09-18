/* Copyright (c) 2007 Mega Man */
#include "mipsregister.h"

/** CPU status register. */
#define CP0_STATUS $12

#define STATUS_IE 0x01

/** Interrupt enable IRQ source 0. */
#define STATUS_IM2 (1<<10)
/** Interrupt enable IRQ source 1. */
#define STATUS_IM3 (1<<11)
/** Interrupt enable IRQ source 1. */
#define STATUS_IM7 (1<<15)


.text

.globl enableInterrupts
.type enableInterrupts,@function;
.ent enableInterrupts
enableInterrupts:
	sync.p
	mfc0	v0, CP0_STATUS
	ori	a0, v0, STATUS_IE
	mtc0	a0, CP0_STATUS
	sync.p
	jr	ra
	nop
.end enableInterrupts

.globl disableInterrupts
.type disableInterrupts,@function;
.ent disableInterrupts
disableInterrupts:
	sync.p
	mfc0	v0, CP0_STATUS
	ori	a0, v0, STATUS_IE
	xori	a0, a0, STATUS_IE
	mtc0	a0, CP0_STATUS
	sync.p
	jr	ra
	nop
.end disableInterrupts

.globl restoreInterrupts
.type restoreInterrupts,@function;
.ent restoreInterrupts
restoreInterrupts:
	sync.p
	mfc0	v0, CP0_STATUS
	andi	a0, a0, STATUS_IE
	or	a0, v0, a0
	mtc0	a0, CP0_STATUS
	sync.p
	jr	ra
	nop
.end restoreInterrupts

.globl enableIRQ
.type enableIRQ,@function;
.ent enableIRQ
enableIRQ:
	sync.p
	mfc0	v0, CP0_STATUS
	ori	a0, v0, 0xff00
	xori	a0, a0, 0xff00
	ori	a0, STATUS_IM2 | STATUS_IM3
	mtc0	a0, CP0_STATUS
	sync.p
	jr	ra
	nop
.end enableIRQ

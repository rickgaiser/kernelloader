/* Copyright (c) 2007 Mega Man */
/**
 * Exception handler is devided into 3 stages:
 * First stage exception handler (exceptionHandlerStart copied to 0x80000XXX)
 * Second stage exception handler
 * Thirs stage: High level exception handler function exception in exception.c)
 */
#include "mipsregister.h"
#include "memory.h"

#define CONTEXT_SIZE (32 * 4)
#define SAVE_REG(r) sw $r, 4 * r(sp)

.text
/* Register "at" need to be used, because we want to save it in the register context. */
.set noat
.global exceptionHandlerStart
.global exceptionHandlerEnd
.global exceptionHandlerPatch1
.global exceptionHandlerPatch2
.global exceptionHandleStage2

/** First stage exception handler. */
.type exceptionHandlerStart,@function;
.ent exceptionHandlerStart
exceptionHandlerStart:
	/* When a bus error happens before the exception handler is here,
	 * then the next 2 instructions are treated as nop.
	 * (tx79architecture.pdf: FLX05).
	 */
	nop
	nop /* Linux will not start if you delete the second nop!!! Required at 0x80000204 in V_INTERRUPT. */
	addiu sp,sp,-(16 + CONTEXT_SIZE)
	SAVE_REG(4)
	SAVE_REG(31)
exceptionHandlerPatch1: /* Patched by loader, because %hi is not working. */
	lui ra, %hi(exceptionHandleStage2) /* TBD: %hi is not working. */
	ori ra, %lo(exceptionHandleStage2)

	/* Parameter 1: Number of the exception handler. */
	lui a0, 0
exceptionHandlerPatch2: /* Patched by loader, set to number of exception handler. */
	addi a0, 0
	jr ra
	nop
exceptionHandlerEnd:
.end exceptionHandlerStart

/** Exception handler stage 2 called by first stage exception handler. */
.type exceptionHandleStage2,@function;
.ent exceptionHandleStage2
exceptionHandleStage2:
	SAVE_REG(0)
	SAVE_REG(1)
	SAVE_REG(2)
	SAVE_REG(3)
	SAVE_REG(5)
	SAVE_REG(6)
	SAVE_REG(7)
	SAVE_REG(8)
	SAVE_REG(9)
	SAVE_REG(10)
	SAVE_REG(11)
	SAVE_REG(12)
	SAVE_REG(13)
	SAVE_REG(14)
	SAVE_REG(15)
	SAVE_REG(16)
	SAVE_REG(17)
	SAVE_REG(18)
	SAVE_REG(19)
	SAVE_REG(20)
	SAVE_REG(21)
	SAVE_REG(22)
	SAVE_REG(23)
	SAVE_REG(24)
	SAVE_REG(25)
	SAVE_REG(26)
	SAVE_REG(27)
	SAVE_REG(28)
	SAVE_REG(29)
	SAVE_REG(30)

	/* Parameter 2: Register context. */
	move a1, sp
	j exception
	nop
.end exceptionHandleStage2


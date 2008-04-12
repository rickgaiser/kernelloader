/* Copyright (c) 2007 Mega Man */
#ifndef _ENTRY_H_
#define _ENTRY_H_

/** Stack overhead, functions will touch into the area without moving stack pointer. */
#define STACK_OVERHEAD 16

/** Align stack addresses at this value. */
#define STACK_ALIGN 16
/** Register context size saved by interrupt. */
#define CONTEXT_SIZE (((32 * 16 + 8 * 32 + 9 * 4) + STACK_ALIGN - 1) & ~(STACK_ALIGN - 1))

/** Save one register on stack. */
#define SAVE_REG(r) sq $r, 16 * r(sp)
/** Restore one register from stack. */
#define RESTORE_REG(r) lq $r, 16 * r(sp)

/** Offset into context where fpu registers begin. */
#define FPU_OFFSET (32 * 16)
/** Save FPU register. */
#define SAVE_FPU(r) swc1 $r, FPU_OFFSET + 8 * r(sp)
/** Restore FPU register. */
#define RESTORE_FPU(r) lwc1 $r, FPU_OFFSET + 8 * r(sp)

/** Offset into CPU context where status registers begins. */
#define STATUS_OFFSET (FPU_OFFSET + 32 * 8)

#ifndef ASSEMBLER
/** Entry point for V_COMMON exception handler. */
void commonExceptionHandler(void);
/** Entry point for V_INTERRUPT exception handler. */
void interruptExceptionHandler(void);
#endif

#endif

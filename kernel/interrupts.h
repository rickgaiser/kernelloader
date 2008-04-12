/* Copyright (c) 2007 Mega Man */
#ifndef _INTERRUPTS_H_
#define _INTERRUPTS_H_

/**
 * Disable all interrupts.
 * @returns Previous interrupt state.
 */
uint32_t enableInterrupts(void);
/**
 * Enable all interrupts.
 * @returns Previous interrupt state.
 */
uint32_t disableInterrupts(void);
/**
 * Change back to previous state (interrupts enabled or disabled.
 */
void restoreInterrupts(uint32_t old);

/**
 * Enable IRQ 0 and IRQ 1.
 * @returns Previous state.
 */
uint32_t enableIRQ(void);

#endif

/* Copyright (c) 2007 Mega Man */
#include "iopmem.h"
#include "stdio.h"
#include "irq.h"
#include "timers.h"
#include "intc.h"
#include "entry.h"
#include "exception.h"

// ================================================ 
// Defines and Enumerations for timer related tasks 
// ================================================ 
#define T0_COUNT   ((volatile unsigned long*)0xb0000000)
#define T0_MODE      ((volatile unsigned long*)0xb0000010)
#define T0_COMP      ((volatile unsigned long*)0xb0000020)
#define T0_HOLD      ((volatile unsigned long*)0xb0000030)

#define T1_COUNT   ((volatile unsigned long*)0xb0000800)
#define T1_MODE      ((volatile unsigned long*)0xb0000810)
#define T1_COMP      ((volatile unsigned long*)0xb0000820)
#define T1_HOLD      ((volatile unsigned long*)0xb0000830)

// Note! T2 and T3 don't have a Tn_HOLD register! 
// ---------------------------------------------- 
#define T2_COUNT   ((volatile unsigned long*)0xb0001000)
#define T2_MODE      ((volatile unsigned long*)0xb0001010)
#define T2_COMP      ((volatile unsigned long*)0xb0001020)

#define T3_COUNT   ((volatile unsigned long*)0xb0001800)
#define T3_MODE      ((volatile unsigned long*)0xb0001810)
#define T3_COMP      ((volatile unsigned long*)0xb0001820)

#define Tn_MODE(CLKS,GATE,GATS,GATM,ZRET,CUE,CMPE,OVFE,EQUF,OVFF) \
    (uint32_t)((uint32_t)(CLKS) | ((uint32_t)(GATE) << 2) | \
   ((uint32_t)(GATS) << 3) | ((uint32_t)(GATM) << 4) | \
   ((uint32_t)(ZRET) << 6) | ((uint32_t)(CUE) << 7) | \
   ((uint32_t)(CMPE) << 8) | ((uint32_t)(OVFE) << 9) | \
   ((uint32_t)(EQUF) << 10) | ((uint32_t)(OVFF) << 11))

#define kBUSCLK            (147456000)
#define kBUSCLKBY16         (kBUSCLK / 16)
#define kBUSCLKBY256      (kBUSCLK / 256)
#define kHBLNK_NTSC         (15734)
#define kHBLNK_PAL         (15625)
#define kHBLNK_DTV480p      (31469)
#define kHBLNK_DTV1080i      (33750)

// ===================== 
// Static Timer Variable 
// ===================== 
static uint64_t timer_interruptCount = 0;

// ======================= 
// Time Interrupt Callback 
// ======================= 
void timer_interrupt(uint32_t *regs)
{
	(void) regs;

	timer_interruptCount++;

	// A write to the overflow flag will clear the overflow flag 
	// --------------------------------------------------------- 
	*T0_MODE |= (1 << 11);

#if 0
	if ((timer_interruptCount & 255) == 1) {
		dumpRegisters(regs);
	}
#endif
}

// ============== 
// Time functions 
// ============== 
void timer_init_module(void)
{
	// ============================================================ 
	// I am using 1/256 of the BUSCLK below in the Tn_MODE register 
	// which means that the timer will count at a rate of: 
	//   147,456,000 / 256 = 576,000 Hz 
	// This implies that the accuracy of this timer is: 
	//   1 / 576,000 = 0.0000017361 seconds (~1.74 usec!) 
	// The Tn_COUNT registers are 16 bit and overflow in: 
	//   1 << 16 = 65536 seconds 
	// This implies that our timer will overflow in: 
	//   65536 / 576,000 = 0.1138 seconds 
	// I use an interrupt to recognize this overflow and increment 
	// the <timer_interruptCount> variable so I can easily compute 
	// the total time. This results in a very accurate timer that 
	// is also very efficient. It is possible to have an even more 
	// accurate timer by modifying the Tn_MODE, but at the expense 
	// of having to call the overflow interrupt more frequently. 
	// For example, if you wanted to use 1/16 of the BUSCLK, the 
	// timer would count at a rate of: 
	//   147,456,000 / 16 = 9,216,000 Hz 
	// which implies an accuracy of: 
	//   1 / 9,216,000 = 0.0000001085 seconds (0.11 usec!) 
	// However, the timer will overflow in: 
	//   65536 / 9,216,000 = 0.0071 seconds (7.1 msec) 
	// meaning, the interrupt would be called more then 140 times a 
	// second. For my purposes the accuracy of ~1.74 usec is fine! 
	// ============================================================ 

	// Disable T0_MODE 
	// --------------- 
	*T0_MODE = 0x0000;

	/* Register an interrupt handler. */
	intc_register_handler(INTC_TIMER0, timer_interrupt);

	// Initialize the overflow interrupt handler. 
	// ----------------------------------------- 
	intc_enable_irq(INTC_TIMER0);

	// Initialize the timer registers 
	// CLKS: 0x02 - 1/256 of the BUSCLK (0x01 is 1/16th) 
	//  CUE: 0x01 - Start/Restart the counting 
	// OVFE: 0x01 - An interrupt is generated when an overflow occurs 
	// -------------------------------------------------------------- 
	*T0_COUNT = 0;
	//*T0_MODE = Tn_MODE(0x02, 0, 0, 0, 0, 0x01, 0, 0x01, 0, 0); 
	*T0_MODE = Tn_MODE(0x01, 0, 0, 0, 0, 0x01, 0, 0x01, 0, 0);

	timer_interruptCount = 0;
}

uint64_t timer_time(void)
{
	uint64_t t;

	// Tn_COUNT is 16 bit precision. Therefore, each 
	// <timer_interruptCount> is 65536 ticks 
	// --------------------------------------------- 
	t = *T0_COUNT + (timer_interruptCount << 16);

	t = t * 1000000 / kBUSCLKBY256;

	return t;
}

void timer_exit_module(void)
{
	// Stop the timer 
	// -------------- 
	*T0_MODE = 0x0000;

	intc_disable_irq(INTC_TIMER0);
}

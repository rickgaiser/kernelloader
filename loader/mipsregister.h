/* Copyright (c) 2007 Mega Man */
#ifndef _MIPS_REGISTER_H_
#define _MIPS_REGISTER_H_

/* MIPS CPU Registers */
#define zero	$0	// Always 0
#define at	$1	// Assembler temporary
#define v0	$2	// Function return
#define v1	$3	//
#define a0	$4	// Function argument 0
#define a1	$5	// Function argument 1
#define a2	$6	// Function argument 2
#define a3	$7	// Function argument 3
#define t0	$8	// Temporaries. No need
#define t1	$9	// to preserve in your
#define t2	$10	// functions.
#define t3	$11
#define t4	$12
#define t5	$13
#define t6	$14
#define t7	$15
#define s0	$16	// Saved Temporaries.
#define s1	$17	// Make sure to restore
#define s2	$18	// to original value
#define s3	$19	// if your function
#define s4	$20	// changes their value.
#define s5	$21
#define s6	$22
#define s7	$23
#define t8	$24	// More Temporaries.
#define t9	$25
#define k0	$26	// Reserved for Kernel
#define k1	$27
#define gp	$28	// Global Pointer
#define sp	$29	// Stack Pointer
#define fp	$30	// Frame Pointer
#define ra	$31	// Function Return Address

#endif

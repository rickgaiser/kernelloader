/* Copyright (c) 2007 Mega Man */
#include "regs.h"

/** Data Cache: Index Writeback Invalidate. */
#define DXWBIN 0x14

/** Instruction Cache: Index Invalidate. */
#define IXIN 0x07

/** Size of a cache line. */
#define CACHE_LINE_SIZE 64

/** Number of cache ways. */
#define NUMBER_OF_CACHE_WAYS 2

/** Number of data cache lines per cache way. */
#define NUMBER_OF_DCACHE_LINES 64

/** Number of instruction cache lines per cache way. */
#define NUMBER_OF_ICACHE_LINES 128

.globl flushDCacheAll
.type flushDCacheAll,@function;
.ent flushDCacheAll
.set noreorder
/* Cache instructions as last instuction of a page could lead to errors. */
.align 6
flushDCacheAll:
	li	t0, 0x00000000
	li	t1, NUMBER_OF_DCACHE_LINES
flushDCacheAll_loop:
	sync.l
	/* Flush cache line in Way 0. */
	cache DXWBIN, 0(t0)
	sync.l
	/* Flush cache line in Way 1. */
	cache DXWBIN, 1(t0)
	sync.l
	addi	t1, t1, -1
	addi	t0, t0, CACHE_LINE_SIZE
	bgtz	t1, flushDCacheAll_loop
	nop
	jr	ra
	nop
.end flushDCacheAll

.globl invalidateICacheAll
.type invalidateICacheAll,@function;
.ent invalidateICacheAll
.set noreorder
/* Align at cache line size to be sure, that only one cache line is loaded by code itself. */
.align 6
invalidateICacheAll:
	li	t0, 0x00000000
	li	t1, NUMBER_OF_ICACHE_LINES
invalidateICacheAll_loop:
	sync.p
	/* Flush cache line in Way 0. */
	cache IXIN, 0(t0)
	sync.p
	/* Flush cache line in Way 1. */
	cache IXIN, 1(t0)
	sync.p
	addi	t1, t1, -1
	addi	t0, t0, CACHE_LINE_SIZE
	bgtz	t1, invalidateICacheAll_loop
	nop
	jr	ra
	nop
.end invalidateICacheAll


#define opDCache(name, op)	\
	.set	push;		\
	.set	noreorder;	\
	.set	nomacro;	\
	.text;			\
	.p2align 3;		\
	.globl	name;		\
	.type	name,@function;	\
	.ent	name, 0;	\
name:	lui	$7, 0xffff;	\
	daddu	$6, $0, $0;	\
	ori	$7, 0xf000;	\
	nop;			\
1:	sync;			\
	cache	0x10, 0($6);	\
	sync;			\
	mfc0	$2, $28;	\
	and	$2, $7;		\
	addu	$2, $6;		\
	sltu	$3, $5, $2;	\
	sltu	$2, $4;		\
	bnez	$2, 2f;		\
	nop;			\
	bnez	$3, 2f;		\
	nop;			\
	sync;			\
	cache	op, 0($6);	\
	sync;			\
2:	sync;			\
	cache	0x10, 1($6);	\
	sync;			\
	mfc0	$2, $28;	\
	and	$2, $7;		\
	addu	$2, $6;		\
	sltu	$3, $5, $2;	\
	sltu	$2, $4;		\
	bnez	$2, 3f;		\
	nop;			\
	bnez	$3, 3f;		\
	nop;			\
	sync;			\
	cache	op, 1($6);	\
	sync;			\
3:	sync;			\
	addiu	$6, 64;		\
	slti	$2, $6, 4096;	\
	bnez	$2, 1b;		\
	nop;			\
	jr	$31;		\
	nop;			\
	.end	name;		\
	.size	name,.-name;	\
	.set	pop;

opDCache(_SyncDCache, DXWBIN)

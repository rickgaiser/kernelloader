/* Copyright (c) 2007 Mega Man */
#ifndef _CACHE_H_
#define _CACHE_H_

/**
 * Flush complete data cache.
 * Interrupts need to be disabled before calling this function.
 * Exceptioins are not allowed, memory need to be mapped.
 */
void flushDCacheAll(void);

/**
 * Invalidate complete instuction cache.
 * Interrupts need to be disabled before calling this function.
 */
void invalidateICacheAll(void);

#endif

/* Copyright (c) 2007 Mega Man */
#include "kernel.h"
#include "memory.h"
#include "string.h"
#include "cache.h"

#define MBYTE(x) ((x) * 1024 * 1024)

uint32_t syscallRFU60(uint32_t gp, uint32_t stack, uint32_t stack_size, args_t *args)
{
	args->argc=1;
	args->argv[0] = args->commandline;

	memcpy(args->commandline, "demo.elf", 9);

	/* Return stack pointer for thread. */
	return MBYTE(30);
}

uint32_t syscallRFU61(void)
{
	/* Return heap address. */
	return MBYTE(5);
}

uint32_t syscallRFU100(void)
{
	/* Flush data cache. */
	flushDCacheAll();
	return 0;
}


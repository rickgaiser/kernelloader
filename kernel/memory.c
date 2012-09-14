/* Copyright (c) 2007 Mega Man */
#include "kernel.h"
#include "memory.h"
#include "string.h"
#include "cache.h"
#include "stdio.h"
#include "cp0register.h"

#define MBYTE(x) ((x) * 1024 * 1024)

uint32_t syscallRFU60(uint32_t gp, uint32_t stack, uint32_t stack_size, args_t *args)
{
	DBG("syscallRFU60 gp 0x%x args 0x%x\n", gp, args);
	args->argc=1;
	args->argv[0] = args->commandline;

	memcpy(args->commandline, "demo.elf", 9);

	/* Return stack pointer for thread. */
	return MBYTE(30);
}

uint32_t syscallRFU61(void)
{
	DBG("syscallRFU61\n");
	/* Return heap address. */
	return MBYTE(5);
}

uint32_t syscallRFU100(void)
{
	DBG("syscallRFU100\n");
	/* Flush data cache. */
	flushDCacheAll();
	return 0;
}


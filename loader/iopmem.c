/* Copyright (c) 2007 Mega Man */
#include "kernel.h"
#include "string.h"
#include "stdint.h"
#include "stdio.h"

#include "iopmem.h"
#include "memory.h"

#include "smod.h"
#include "smem.h"
#include "sharedmem.h"

/** IOP RAM is mysteriously mapped into EE HW space at this address. */
#define SUB_VIRT_MEM    0xbc000000
#define BUFFER_SIZE 256

/** Global variable need to be accessed over KSEG0!!!. */
#ifdef USER_SPACE_SUPPORT
static int kernelMode = 0;
#endif
static int initializedU = 0;
static volatile unsigned char *sharedMemU = NULL;

#ifdef USER_SPACE_SUPPORT

/** Access sharedMemU global variable from kernel space. */
#define sharedMem getSharedMem()

/** Access initializedU from kernel space. */
#define initialized getInitialized()

#else

/** Access sharedMemU global variable from kernel space. */
#define sharedMem (*((volatile unsigned char **)(((u32) &sharedMemU) | KSEG0_MASK)))

/** Access initializedU from kernel space. */
#define initialized (*((int *) (((u32) &initializedU) | KSEG0_MASK)))

#endif

int isKernelMode(void)
{
	unsigned int status;

	__asm__ volatile (
		".set push\n"
		".set noreorder\n"
		"mfc0 %0, $12\n"
		"sync.p\n"
		".set pop\n" : "=r"(status));

	return ((status & 0x18) == 0) || ((status & 0x6) != 0);
}

volatile unsigned char *getSharedMem(void)
{
	volatile unsigned char *rv;

	if (isKernelMode()) {
		rv = *(volatile unsigned char **)(((u32) &sharedMemU) | KSEG0_MASK);
	} else {
		rv = sharedMemU;
	}
	return rv;
}

int getInitialized(void)
{
	int rv;

	if (isKernelMode()) {
		rv = *((int *) (((u32) &initializedU) | KSEG0_MASK));
	} else {
		rv = initializedU;
	}
	return rv;
}

/** Search for module by name. */
int find_module(smod_mod_info_t *module, const char *searchtext)
{
	smod_mod_info_t *current;
	char name[32];
	int i;

	current = NULL;
	i = 0;
	while (smod_get_next_mod(current, module) != 0)
	{
		smem_read(module->name, name, sizeof(name));
		name[sizeof(name) - 1] = 0;

		if (strncmp(name, searchtext, sizeof(name)) == 0) {
			return 0;
		}
		current = module;
		i++;
	}
	return -1;
}


/** Initialize shared memory pointer. */
void iop_init_shared(void)
{
	smod_mod_info_t module;
	char magic[32];

	if (find_module(&module, SHAREDMEM_MODULE_NAME) == 0) {
		u32 addr;
		u32 end;

		printf("Found module " SHAREDMEM_MODULE_NAME "\n");
		addr = module.text_start + module.text_size;
		end = addr + module.data_size + module.bss_size;

		addr = (addr + 16 - 1) & ~(16 - 1);

		while(addr < end) {
			smem_read((void *) addr, magic, sizeof(magic));
			if (strncmp(magic, SHAREDMEM_MAGIC, sizeof(magic)) == 0) {
				sharedmem_dbg_t *dbg = (void *) addr;
				sharedMemU = &dbg->shared[0];
				printf("sharedMem at 0x%08x\n", sharedMemU);
				initializedU = -1;
				break;
			}
			addr += 16;
		}
	}
}


#ifdef USER_SPACE_SUPPORT
/** Setup IOP print functions to work in kernel mode. */
void iop_kmode_enter(void)
{
	kernelMode = -1;
}
#endif

/**
 * Read from IOP memory address.
 * @param addr IOP memory address to read.
 * @param buf Pointer to destination buffer.
 * @param size Size of memory to read.
 */
u32 iop_read(void *addr, void *buf, u32 size)
{
#ifdef USER_SPACE_SUPPORT
	if (!isKernelMode()) {
		DI();
		ee_kmode_enter();
	}
#endif

	memcpy(buf, addr + SUB_VIRT_MEM, size);

#ifdef USER_SPACE_SUPPORT
	if (!isKernelMode()) {
		ee_kmode_exit();
		EI();
	}
#endif

	return size;
}

/**
 * Write to IOP memory address.
 * @param addr IOP memory address.
 * @param buf Pointer to data written.
 * @param size Size to read.
 */
u32 iop_write(void *addr, void *buf, u32 size)
{
#ifdef USER_SPACE_SUPPORT
	if (!isKernelMode()) {
		DI();
		ee_kmode_enter();
	}
#endif

	memcpy(addr + SUB_VIRT_MEM, buf, size);

#ifdef USER_SPACE_SUPPORT
	if (!isKernelMode()) {
		ee_kmode_exit();
		EI();
	}
#endif

	return size;
}

/**
 * Print one character using IOP memory interface defined by sharedmem.irx.
 * @param c Charachter to print.
 */
void iop_putc(unsigned char c)
{
	char buf[2];

	if (!initialized) {
		return;
	}

	do {
		iop_read((void *) sharedMem, buf, 1);
	} while(buf[0] != 0);
	buf[0] = 0xFF;
	buf[1] = c;
	iop_write(&sharedMem[1], &buf[1], 1);
	iop_write(&sharedMem[0], &buf[0], 1);
}

/**
 * Print one hexadecimal number using IOP memory interface defined by sharedmem.irx.
 * @param val Value printed.
 */
void iop_printx(uint32_t val)
{
	int i;

	if (!initialized) {
		return;
	}

	for (i = 0; i < 8; i++) {
		char v;

		v = (val >> (4 * (7 - i))) & 0xF;
		if (v < 10) {
			iop_putc('0' + v);
		} else {
			iop_putc('A' + v - 10);
		}
	}
}

/**
 * Print text using IOP memory interface defined by sharedmem.irx.
 * @param text Pointer to printed text.
 */
void iop_prints(const char *text)
{
	if (!initialized) {
		return;
	}

	while(*text != 0) {
		iop_putc(*text);
		text++;
	}
}

/** Printf function for printing text using IOP memory interface defined by sharedmem.irx. */
int iop_printf(const char *format, ...)
{
	va_list args;
	int ret;
	static char buffer[BUFFER_SIZE];
	char *b = buffer;

#ifdef USER_SPACE_SUPPORT
	if (isKernelMode()) {
#endif
		b = (char *) (((uint32_t) b) | KSEG0_MASK);
#ifdef USER_SPACE_SUPPORT
	}
#endif

	va_start(args, format);
	ret = vsnprintf(b, BUFFER_SIZE, format, args);
	iop_prints(b);
	va_end(args);

	return 0;
}


/* Copyright (c) 2007 Mega Man */
#include "kernel.h"
#include "string.h"
#include "stdint.h"
#include "stdio.h"

#include "iopmem.h"
#include "memory.h"

/** IOP RAM is mysteriously mapped into EE HW space at this address. */
#define SUB_VIRT_MEM    0xbc000000
#define BUFFER_SIZE 256

/** Global variable need to be accessed over KSEG0!!!. */
#ifdef USER_SPACE_SUPPORT
static int kernelMode = 0;
#endif
#if 0
static unsigned char *sharedMem = (unsigned char *) 0x001ff000;
#else
/** Global variables cannot be used, so we doing it this way. */
#define sharedMem ((unsigned char *) 0x001ff000)
#endif

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
	if (!kernelMode) {
		DI();
		ee_kmode_enter();
	}
#endif

	memcpy(buf, addr + SUB_VIRT_MEM, size);

#ifdef USER_SPACE_SUPPORT
	if (!kernelMode) {
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
	if (!kernelMode) {
		DI();
		ee_kmode_enter();
	}
#endif

	memcpy(addr + SUB_VIRT_MEM, buf, size);

#ifdef USER_SPACE_SUPPORT
	if (!kernelMode) {
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

	do {
		iop_read(sharedMem, buf, 1);
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
	if (kernelMode) {
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


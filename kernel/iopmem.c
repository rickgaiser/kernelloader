/* Copyright (c) 2007 Mega Man */
#include "iopmem.h"

/** IOP RAM is mysteriously mapped into EE HW space at this address. */
#define SUB_VIRT_MEM    0xbc000000

static unsigned char *sharedMem = (unsigned char *) 0x001ff000;

/** Memory copy function like in libc. */
static void memcpy(unsigned char *dst, unsigned char *src, unsigned int size)
{
	unsigned int n;

	for (n = 0; n < size; n++) {
		dst[n] = src[n];
	}
}

/** Read iop memory address. */
static unsigned int iop_read(void *addr, void *buf, unsigned int size)
{
	memcpy(buf, addr + SUB_VIRT_MEM, size);

	return size;
}

/** Write to iop memory address. */
static unsigned int iop_write(void *addr, void *buf, unsigned int size)
{
	memcpy(addr + SUB_VIRT_MEM, buf, size);

	return size;
}

/** Print one character. */
static void iop_putc(unsigned char c)
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

/** Print one string. */
void iop_prints(const char *text)
{
	while(*text != 0) {
		iop_putc(*text);
		text++;
	}
}


#include "iopmemdebug.h"
#include "iopmem.h"

#define SBIOS_DEBUG 1

static unsigned char *sharedMem = (unsigned char *) 0x001ff000;

/** Print one character. */
static void iop_putc(unsigned char c)
{
#ifdef SBIOS_DEBUG
	char buf[2];
	u32 status;

	core_save_disable(&status);
	do {
		iop_read(sharedMem, buf, 1);
		if (buf[0] != 0) {
			core_restore(status);
			core_save_disable(&status);
		}
	} while(buf[0] != 0);
	buf[0] = 0xFF;
	buf[1] = c;
	iop_write(&sharedMem[1], &buf[1], 1);
	iop_write(&sharedMem[0], &buf[0], 1);
	core_restore(status);
#endif
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

int puts(const char *s)
{
	while(*s != 0) {
		iop_putc(*s);
		s++;
	}
}

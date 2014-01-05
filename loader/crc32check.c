#include <sys/types.h>
#include <sys/config.h>
#include <stdio.h>
#include <graphic.h>
#include <sio.h>

#include "stdint.h"
#include "crc32gen.h"
#include "crc32check.h"

#define CRC32MASK 0x04C11DB7 /* CRC-32 Bitmaske */

void _ftext(void);
void _etext(void);
void _frodata(void);
void _erodata(void);

const crc32_section_t crc[] __attribute__ ((__section__ (".crc32"))) = {
	{ .section = ".text", .start = (uint32_t) &_ftext, .end = (uint32_t) &_etext },
	{ .section = ".rodata", .start = (uint32_t) &_frodata, .end = (uint32_t) &_erodata },
};

uint32_t calc_crc(const uint8_t *data, long size)
{
	uint32_t crc32 = 0; /* Schieberegister */
	int i;
	int n;

	for (i = 0; i < size; i++) {
		for (n = 0; n < 8; n++) {
			uint8_t bit;
			uint8_t databit;

			bit = crc32 >> 31;
			databit = (data[i] >> n) & 1U;
			if (bit != databit) {
				crc32 = (crc32 << 1) ^ CRC32MASK;
			} else {
				crc32 <<= 1;
			}
		}
	}
	return crc32;
}

char getPrintableChar(char c)
{
	if ((c >= 0x20) && (c <= 0x7e)) {
		return c;
	} else {
		return '.';
	}
}

void sio_hexdump(const uint8_t *data, int size, uint32_t offset)
{
	int i;

	for (i = 0; i < size; i+=16) {
		int n;

		sio_printf("%08x  ", offset + i);

		for (n = 0; n < 16; n++) {
			if ((i + n) < size) {
				sio_printf("%02x ", data[i + n]);
			} else {
				sio_printf("  ");
			}
			if (n == 7) {
				sio_putc(' ');
			}
		}
		sio_putc(' ');
		sio_putc('|');
		for (n = 0; n < 16; n++) {
			if ((i + n) < size) {
				sio_putc(getPrintableChar(data[i + n]));
			} else {
				sio_putc(' ');
			}
		}
		sio_putc('|');
		sio_putc('\n');
	}
}

int crc32check(const char *msg)
{
	unsigned int i;

	sio_printf("Checking for \"%s\".\n", msg);

	for (i = 0; i < sizeof(crc)/sizeof(crc[0]); i++) {
		uint32_t crcvalue;
		const uint8_t *start = (void *) crc[i].start;
		const uint8_t *end = (void *) crc[i].end;

		crcvalue = calc_crc(start, end - start);

		if (crc[i].crc != crcvalue) {
			sio_printf("section %s CRC32 expected 0x%08x, but is 0x%08x, addr 0x%08x, end 0x%08x, size 0x%08x\n",
				crc[i].section, crc[i].crc, crcvalue, start, end, end - start);
			error_printf("kloader ELF integrity check failed in section %s. %s\n", crc[i].section, msg);
			//sio_hexdump(start, end - start, crc[i].fileoffset);
			sio_hexdump(start, 0x100, crc[i].fileoffset);
			return -1;
		} else {
			sio_printf("kloader section %s CRC32 OK\n", crc[i].section);
		}
	}
	return 0;
}

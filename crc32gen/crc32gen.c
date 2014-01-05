#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <fcntl.h>
#include <unistd.h>
#include <elf.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include "crc32gen.h"

#define CRC32MASK 0x04C11DB7 /* CRC-32 Bitmaske */
 
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

int find_segment(uint8_t *data, const char *segment, uint8_t **addr)
{
	Elf32_Ehdr *ehdr = (void *) data;
	Elf32_Shdr *shdr = (void *) (data + ehdr->e_shoff);
	Elf32_Shdr *sh_strtab = &shdr[ehdr->e_shstrndx];
	const char *const sh_strtab_p = (void *) (data + sh_strtab->sh_offset);
	int i;

	if ((ehdr->e_ident[EI_MAG0] != ELFMAG0) ||
		(ehdr->e_ident[EI_MAG1] != ELFMAG1) ||
		(ehdr->e_ident[EI_MAG2] != ELFMAG2) ||
		(ehdr->e_ident[EI_MAG3] != ELFMAG3)) {
		fprintf(stderr, "ELF magic wrong\n");
		return -1;
	}
	for (i = 0; i < ehdr->e_shnum; i++) {
		if (strcmp(sh_strtab_p + shdr[i].sh_name, segment) == 0) {
			printf("%02d: 0x%08x size 0x%08x offset 0x%08x %s\n", i,
				shdr[i].sh_addr,
				shdr[i].sh_size,
				shdr[i].sh_offset,
				sh_strtab_p + shdr[i].sh_name);
			*addr = data + shdr[i].sh_offset;
			return shdr[i].sh_size;
		}
	}
	return -1;
}

void fix_segments(uint8_t *data)
{
	int size;
	uint8_t *patchaddr;

	size = find_segment(data, ".crc32", &patchaddr);
	if (size >= (signed int) sizeof(crc32_section_t)) {
		crc32_section_t *crc = (void *) patchaddr;
		uint8_t *addr = NULL;
		int sections = size / sizeof(crc32_section_t);
		int i;

		for (i = 0; i < sections; i++) {
			size = find_segment(data, crc[i].section, &addr);
			if (size > 0) {
				uint32_t crcvalue;

				crcvalue = calc_crc(addr, size);

				/* Patch CRC value into file. */
				printf("Replace old CRC 0x%08x by CRC 0x%08x\n",
					crc[i].crc, crcvalue);
				crc[i].crc = crcvalue;
				crc[i].fileoffset = addr - data;
			}
		}
		if (msync((void *)(((size_t) patchaddr) & (~(getpagesize() - 1))), getpagesize(), MS_SYNC) != 0) {
			perror("msync failed");
		}
	} else {
		fprintf(stderr, "File doesn't contain a .crc32 section\n");
	}
}
 
int main(int argc, char *argv[])
{
	int fd;

	if (argc == 2) {
		fd = open(argv[1], O_RDWR);
		if (fd != -1) {
			long size;
			uint8_t *data;

			size = lseek(fd, 0, SEEK_END);
			printf("File size %ld Bytes.\n", size);

			data = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
			close(fd);
			if (data != MAP_FAILED) {
				fix_segments(data);
				return EXIT_SUCCESS;
			} else {
				fprintf(stderr, "Failed to map file \"%s\".\n", argv[1]);
				perror("Failed to map");
				return EXIT_FAILURE;
			}
		} else {
			fprintf(stderr, "Failed to open file \"%s\".\n", argv[1]);
			return EXIT_FAILURE;
		}
	} else {
		fprintf(stderr, "Parameter wrong: One file name needed.\n");
		return EXIT_FAILURE;
	}
}

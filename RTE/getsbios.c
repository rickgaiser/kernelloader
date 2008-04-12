/* Copyright (c) 2007 Mega Man */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>

#define FILENAME "/pbpx_955.09"

int main(int argc, char *argv[])
{
	FILE *fin;
	char *filename;
	char magic_string[] = "PS2b";
	uint32_t *magic = (uint32_t *) magic_string;
	uint32_t offset = 0;
	uint32_t sbiosSize = 0xf000;

	if ((argc != 2) && (argc != 3)) {
		fprintf(stderr, "Usage:\n");
		fprintf(stderr, "%s [path to ps2 linux DVD 1] [virtuall memory offset]\n\n", argv[0]);
		fprintf(stderr, "Get sbios.bin from ps2 linux DVD 1.\n");
		return -1;
	}
	if (argc == 3) {
		offset = atoi(argv[2]);
		printf("Offset 0x%08x.\n", offset);
	}
	filename = malloc(strlen(argv[1]) + sizeof(FILENAME) + 1);
	sprintf(filename, "%s" FILENAME, argv[1]);
	printf("Search for sbios in \"%s\".\n", filename);

	fin = fopen(filename, "rb");
	if (fin != NULL)
	{
		char *buffer;
		uint32_t size;

		free(filename);
		filename = NULL;
		fseek(fin, 0, SEEK_END);
		size = ftell(fin);
		fseek(fin, 0, SEEK_SET);
		buffer = malloc(size);
		if (buffer != NULL)
		{
			FILE *fout;
			char *addr;
			char *endaddr;

			if (fread(buffer, size, 1, fin) != 1)
			{
				fclose(fin);
				free(buffer);
				fprintf(stderr, "Can't read file.\n");
				return -3;
			}
			fclose(fin);

			endaddr = (char *) (((uint32_t) buffer) + size);
			for (addr = buffer; addr < endaddr; addr++)
			{
				if (*((uint32_t *)addr) == *magic)
				{
					break;
				}
			}
			if (*((uint32_t *)addr) == *magic)
			{
				uint32_t regs[32];
				void *code;
				addr -= 4;

				printf("Found sbios.bin at file offset 0x%08x.\n",
					((uint32_t) addr) - ((uint32_t) buffer));

				if (offset != 0) {
					uint32_t search;

					search = addr - buffer;
					search += offset;
					printf("Search 0x%08x\n", search);
					for (code = buffer; code < endaddr; code++) {
						uint32_t value;

						value = *((uint32_t *)code);
						if ((value >> 26) == 9) {
							/* addiu instruction. */
							int rs;
							int rt;
							int immidiate;

							rs = (value >> 21) & 0x1f;
							rt = (value >> 16) & 0x1f;
							immidiate = ((int16_t) ((uint16_t) (value & 0xFFFF)));

							regs[rt] = regs[rs] + immidiate;
							//printf("addiu %d, %d, 0x%04x\n", rt, rs, immidiate);

							if (regs[rt] == search) {
								uint32_t pos;
								pos = (uint32_t) code - (uint32_t) buffer;
								printf("Loadded at 0x%08x (file position 0x%08x).\n", pos + offset, pos);
							}
						}
						if ((value >> 26) == 15) {
							int rs;
							int rt;
							int immidiate;

							/* lui instruction */
							rs = (value >> 21) & 0x1f;
							rt = (value >> 16) & 0x1f;
							immidiate = ((int16_t) ((uint16_t) (value & 0xFFFF)));

							if (rs == 0) {
								regs[rt] = immidiate << 16;
								//printf("lui %d, 0x%08x\n", rt, regs[rt]);
							}
						}
						if ((value >> 26) == 0) {
							/* Special */
							if ((value & 0x3f) == 35) {
								/* subu instruction */
								int rs;
								int rt;
								int rd;

								rs = (value >> 21) & 0x1f;
								rt = (value >> 16) & 0x1f;
								rd = (value >> 11) & 0x1f;

								if (regs[rt] == search) {
									printf("0x%08x = 0x%08x - 0x%08x\n", regs[rs] - regs[rt], regs[rs], regs[rt]);
								}
								regs[rd] = regs[rs] - regs[rt];
							}
						}
						if ((value >> 26) == 3) {
							/* jal instruction. */
							if (regs[5] == search) {
								/* memcopy gets SBIOS in register a1 and size in register a2. */
								sbiosSize = regs[6];
								printf("SBIOS size is 0x%08x.\n", sbiosSize);
								break;
							}
						}
					}
				}

				fout = fopen("sbios.bin", "wb");
				if (fout != NULL)
				{
					if (fwrite(addr, sbiosSize, 1, fout) != 1)
					{
						fclose(fout);
						free(buffer);
						fprintf(stderr, "Failed to write sbios.bin\n");
						return -5;
					}
					fclose(fout);
					printf("sbios.bin written.\n");
				}
				else
				{
					fprintf(stderr, "Failed to open sbios.bin\n");
					free(buffer);
					return -4;
				}
			}
			else
			{
				fprintf(stderr, "Can't find sbios.\n");
				free(buffer);
				return -6;
			}
			free(buffer);
			buffer = NULL;
		}
		else
		{
			fprintf(stderr, "out of memory\n");
			return -2;
		}
	}
	else
	{
		fprintf(stderr, "Failed to open file \"%s\"\n", filename);
		free(filename);
	}
	
	return 0;
}

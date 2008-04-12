/* Copyright (c) 2007 Mega Man */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <elf.h>

#if 0
#define FILENAME "/pbpx_955.09"
#else
#define FILENAME "/modules/modrt.img"
#endif

int main(int argc, char *argv[])
{
	FILE *fin;
	char *filename;
	char magic_string[] = ELFMAG;
	uint32_t *magic = (uint32_t *) magic_string;
	uint32_t elfSize = 0;
	const char *outputfilename = "a.out";
	uint32_t elfNumber = 1;

	if ((argc != 2) && (argc != 3) && (argc != 4)) {
		fprintf(stderr, "Usage:\n");
		fprintf(stderr, "%s [path to ps2 linux DVD 1] [output file name] [ELF file number]\n\n", argv[0]);
		fprintf(stderr, "Get ELF from ps2 linux DVD 1.\n");
		return -1;
	}
	if (argc >= 3) {
		outputfilename = argv[2];
	}
	if (argc >= 4) {
		elfNumber = atoi(argv[3]);
		printf("elfNumber 0x%08x.\n", elfNumber);
	}
	filename = malloc(strlen(argv[1]) + sizeof(FILENAME) + 1);
	sprintf(filename, "%s" FILENAME, argv[1]);
	printf("Search for elf in \"%s\".\n", filename);

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
					elfNumber--;
					if (elfNumber == 0) {
						break;
					}
				}
			}
			if (*((uint32_t *)addr) == *magic)
			{
				void *code;

				printf("Found elf at file offset 0x%08x.\n",
					((uint32_t) addr) - ((uint32_t) buffer));
				for (code = addr + 4; code < endaddr; code++) {
					uint32_t value;

					value = *((uint32_t *)code);
					if (value == *magic) {
						/* memcopy gets SBIOS in register a1 and size in register a2. */
						elfSize = ((uint32_t) code) - ((uint32_t) addr);
						printf("ELF size is 0x%08x (1).\n", elfSize);
						break;
					}
				}
				if (elfSize == 0) {
					elfSize = ((uint32_t) endaddr) - ((uint32_t) addr);
					printf("ELF size is 0x%08x (2).\n", elfSize);
				}

				fout = fopen(outputfilename, "wb");
				if (fout != NULL)
				{
					if (fwrite(addr, elfSize, 1, fout) != 1)
					{
						fclose(fout);
						free(buffer);
						fprintf(stderr, "Failed to write \"%s\"\n", outputfilename);
						return -5;
					}
					fclose(fout);
					printf("\"%s\" written.\n", outputfilename);
				}
				else
				{
					fprintf(stderr, "Failed to open \"%s\"\n", outputfilename);
					free(buffer);
					return -4;
				}
			}
			else
			{
				fprintf(stderr, "Can't find ELF.\n");
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

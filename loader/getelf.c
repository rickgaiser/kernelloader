/* Copyright (c) 2007 Mega Man */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <elf.h>
#include <unistd.h>
#include "fileXio_rpc.h"

#include "SMS_CDVD.h"
#include "SMS_CDDA.h"
#include "graphic.h"
#include "getelf.h"
#include "configuration.h"
#include "modules.h"
#include "kprint.h"

#define	ELFMAG		"\177ELF"

char rteELF[1024] = "cdfs:/modules/modrt.img";
static char magic_string[] __attribute__((aligned(4))) = ELFMAG;

int real_copyRTEELF(void *arg)
{
	FILE *fin;
	const char *filename = rteELF;
	uint32_t *magic = (uint32_t *) magic_string;
	uint32_t elfSize = 0;
	const char *outputfilename;
	uint32_t elfNumber = 1;
	copyRTEELF_param_t *param = (copyRTEELF_param_t *) arg;
	

	outputfilename = param->outputFilename;
	elfNumber = atoi(param->elfNumber);
	kprintf("elfNumber 0x%08x.\n", elfNumber);
	kprintf("Search for elf in \"%s\".\n", filename);

	graphic_setPercentage(0, filename);

	fin = fopen(filename, "rb");
	if (fin != NULL)
	{
		char *buffer;
		uint32_t size;

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
				error_printf("Can't read file.");
				return -3;
			}
			fclose(fin);

			endaddr = (char *) (((uint32_t) buffer) + size);
			for (addr = buffer; addr < endaddr; addr += 4)
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

				kprintf("Found elf at file offset 0x%08x.\n",
					((uint32_t) addr) - ((uint32_t) buffer));
				for (code = addr + 4; code < ((void *) endaddr); code += 4) {
					uint32_t value;

					value = *((uint32_t *)code);
					if (value == *magic) {
						/* memcopy gets SBIOS in register a1 and size in register a2. */
						elfSize = ((uint32_t) code) - ((uint32_t) addr);
						kprintf("ELF size is 0x%08x (1).\n", elfSize);
						break;
					}
				}
				if (elfSize == 0) {
					elfSize = ((uint32_t) endaddr) - ((uint32_t) addr);
					kprintf("ELF size is 0x%08x (2).\n", elfSize);
				}

				fout = fopen(outputfilename, "wb");
				if (fout == NULL) {
					fileXioMkdir(CONFIG_DIR, 0777);
					fout = fopen(outputfilename, "wb");
				}
				if (fout != NULL)
				{
					if (fwrite(addr, elfSize, 1, fout) != 1)
					{
						fclose(fout);
						fioRemove(outputfilename);
						fioRmdir(outputfilename); /* Needed because of bug in fioRemove. */
						free(buffer);
						error_printf("Failed to write \"%s\"", outputfilename);
						return -5;
					}
					fclose(fout);
					kprintf("\"%s\" written.\n", outputfilename);
				}
				else
				{
					error_printf("Failed to open \"%s\"", outputfilename);
					free(buffer);
					return -4;
				}
			}
			else
			{
				error_printf("Can't find ELF.");
				free(buffer);
				return -6;
			}
			free(buffer);
			buffer = NULL;
		}
		else
		{
			error_printf("out of memory");
			return -2;
		}
	}
	else
	{
		error_printf("Failed to open file \"%s\"", filename);
	}
	
	return 0;
}

int copyRTEELF(void *arg)
{
	DiskType type;
	int rv;

	setEnableDisc(-1);

	if (isDVDVSupported()) {
		type = CDDA_DiskType();
	
		/* Detect disk type, so loading will work. */
		if (type == DiskType_DVDV) {
			CDVD_SetDVDV(1);
		} else {
			CDVD_SetDVDV(0);
		}
	}

	rv = real_copyRTEELF(arg);

	if (isDVDVSupported()) {
		/* Always stop CD/DVD when an error happened. */
		CDVD_Stop();
		CDVD_FlushCache();
	}

	setEnableDisc(0);

	if (getErrorMessage() == NULL) {
		graphic_setPercentage(0, NULL);
	}

	return 0;
}

/* Copyright (c) 2007 Mega Man */
#include "stdint.h"
#include "stdio.h"
#include "elf.h"
#include "memory.h"
#include "cache.h"
#include "panic.h"
#include "string.h"
#include "cp0register.h"

/** Definition of kernel entry function. */
typedef int (entry_t)(int argc, char **argv, char **envp, int *prom_vec);

/**
 * Copy program sections of ELF file to memory.
 * @param buffer Pointer to ELF file.
 */
entry_t *copy_sections(char *buffer)
{
	Elf32_Ehdr_t *file_header;
	int pos = 0;
	int i;
	entry_t *entry = NULL;

	printf("Using pointer 0x%x\n", buffer);

	file_header = (Elf32_Ehdr_t *) &buffer[pos];
	pos += sizeof(Elf32_Ehdr_t);
	if (file_header->magic != ELFMAGIC) {
		printf("Magic 0x%x is wrong.\n", file_header->magic);
		return NULL;
	}
	entry = (entry_t *) file_header->entry;
	printf("entry is 0x%x\n", (int) entry);
	for (i = 0; i < file_header->phnum; i++)
	{
		Elf32_Phdr_t *program_header;
		program_header = (Elf32_Phdr_t *) &buffer[pos];
		pos += sizeof(Elf32_Phdr_t);
		if ( (program_header->type == PT_LOAD)
			&& (program_header->memsz != 0) )
		{
			unsigned char *dest;

			// Copy to physical address which can be accessed by loader.
			dest = (unsigned char *) (program_header->paddr);

			if (program_header->filesz != 0)
			{
				printf("VAddr: 0x%x PAddr: 0x%x Offset 0x%x Size 0x%x\n",
					program_header->vaddr,
					program_header->paddr,
					program_header->offset,
					program_header->filesz);
				memcpy(dest, &buffer[program_header->offset], program_header->filesz);
				printf("First bytes 0x%x 0x%x\n", (int) dest[0], (int) dest[1]);
			}
			int size = program_header->memsz - program_header->filesz;
			if (size > 0)
				memset(&dest[program_header->filesz], 0, size);
		}
	}
	return entry;
}

#if 0
void verify_sections(char *buffer)
{
	Elf32_Ehdr_t *file_header;
	int pos = 0;
	int i;
	entry_t *entry = NULL;

	file_header = (Elf32_Ehdr_t *) &buffer[pos];
	pos += sizeof(Elf32_Ehdr_t);
	if (file_header->magic != ELFMAGIC) {
		printf("Magic 0x%x is wrong.\n", file_header->magic);
		panic();
	}
	entry = (entry_t *) file_header->entry;
	printf("entry is 0x%x\n", (int) entry);
	for (i = 0; i < file_header->phnum; i++)
	{
		Elf32_Phdr_t *program_header;
		program_header = (Elf32_Phdr_t *) &buffer[pos];
		pos += sizeof(Elf32_Phdr_t);
		if ( (program_header->type == PT_LOAD)
			&& (program_header->memsz != 0) )
		{
			unsigned char *dest;

			// Copy to physical address which can be accessed by loader.
			dest = (unsigned char *) (program_header->paddr);

			if (program_header->filesz != 0)
			{
				printf("VAddr: 0x%x PAddr: 0x%x Offset 0x%x Size 0x%x\n",
					program_header->vaddr,
					program_header->paddr,
					program_header->offset,
					program_header->filesz);
				if (memcmp(dest, &buffer[program_header->offset], program_header->filesz) != 0) {
					printf("Verify failed");
					panic();
				};
				printf("First bytes 0x%x 0x%x\n", (int) dest[0], (int) dest[1]);
			}
			int size = program_header->memsz - program_header->filesz;
			if (size > 0) {
				unsigned int i;

				for (i = 0; i < size; i++) {
					if (dest[program_header->filesz] != 0) {
						printf("Verify failed in memset");
						panic();
					}
				}
			}
		}
	}
	return;
}
#endif

/**
 * Load elf file into user memory and start it.
 */
int loader(void)
{
	entry_t *entry;
	extern char demo[];

	/* Install linux kernel. */
	entry = copy_sections(&demo);

#if 0
	/* Verify if everything is correct. */
	verify_sections(demo);
#endif

	if (entry != NULL) {
		uint32_t status;

		/* Activate code. */
		flushDCacheAll();
		invalidateICacheAll();

		printf("Jumping to user mode.\n");
		/* Jump to user mode: */
		CP0_GET_STATUS(status);

		/* Set KSU, ERL and EXL to zero. */	
		status &= ~((3 << 3) | (1 << 2) | (1 << 1));

		/* User mode setting + setting for return from interrupt. */
		status |= (2<<3) /* User Mode */
			| (1<<1) /* EXL */
			| (1<<0) /* ie */
			| (1<<17) /* EDI, enable DI and EI instructions in user mode. */
			;
		CP0_SET_STATUS(status);
		CP0_SET_EPC(entry);
		/* Set stack pointer, to get syscall working. */
		/* Set a0 for function return value. */
		__asm__ __volatile__("li $29, 0x01e00000\n"
			"li $4, 0x00000000\n"
			"eret\n"
			"nop\n":::"memory");
	} else {
		printf("ELF not loaded.\n");
	}
	panic("End reached?\n");

	return(0);
}


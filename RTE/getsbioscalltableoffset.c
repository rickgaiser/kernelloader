/* Copyright (c) 2007 Mega Man */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>

#define NUMBER_OF_INSTRUCTIONS_CHECKED 128
#define SBIOS_START_ADDRESS 0x80001000

#if 0
#define dprintf printf
#else
#define dprintf(args...)
#endif


uint32_t *getSBIOSCallTable(char *addr)
{
	void *code;
	char magic_string[] = "PS2b";
	uint32_t *magic_check = (uint32_t *) magic_string;
	uint32_t *entrypoint;
	uint32_t *magic;
	uint32_t regs[32];
	uint32_t load[32];
	uint32_t jumpBase;

	memset(regs, 0, sizeof(regs));
	memset(load, 0, sizeof(load));
	jumpBase = 0;

	entrypoint = (uint32_t *) addr;
	magic = (uint32_t *) (addr + 4);

	printf("Entrypoint 0x%08x magic 0x%08x\n", *entrypoint, *magic);

	if (*magic != *magic_check) {
		printf("SBIOS file is incorrect (magic is wrong).\n");
		return NULL;
	}

	entrypoint = (uint32_t *) ((((uint32_t) *entrypoint) - SBIOS_START_ADDRESS) + ((uint32_t) addr));

	for (code = entrypoint; code < (void *)(entrypoint + NUMBER_OF_INSTRUCTIONS_CHECKED); code += 4) {
		uint32_t value;

		value = *((uint32_t *)code);
		//printf("Check code at 0x%08x 0x%08x op %d\n", code, value, value >> 26);
		if ((value >> 26) == 9) {
			/* addiu instruction. */
			int rs;
			int rt;
			int immidiate;

			rs = (value >> 21) & 0x1f;
			rt = (value >> 16) & 0x1f;
			immidiate = ((int16_t) ((uint16_t) (value & 0xFFFF)));

			regs[rt] = regs[rs] + immidiate;
			dprintf("addiu r%d, r%d, 0x%04x\n", rt, rs, immidiate);
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
				dprintf("lui r%d, 0x%08x\n", rt, regs[rt]);
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

				dprintf("0x%08x = 0x%08x - 0x%08x\n", regs[rs] - regs[rt], regs[rs], regs[rt]);
				dprintf("subu r%d, r%d, r%d\n", rd, rs, rt);
				regs[rd] = regs[rs] - regs[rt];
			}
			if ((value & 0x3f) == 33) {
				/* subu instruction */
				int rs;
				int rt;
				int rd;

				rs = (value >> 21) & 0x1f;
				rt = (value >> 16) & 0x1f;
				rd = (value >> 11) & 0x1f;

				dprintf("addu r%d, r%d, r%d\n", rd, rs, rt);
				regs[rd] = regs[rs] + regs[rt];
			}
			if ((value & 0x3f) == 9) {
				/* jalr instruction */
				int rs;
				int rt;
				int rd;

				rs = (value >> 21) & 0x1f;
				rt = (value >> 16) & 0x1f;
				rd = (value >> 11) & 0x1f;

				if (rt == 0) {
					dprintf("jalr r%d, r%d\n", rd, rs);
					if (load[rs] != 0) {
						jumpBase = load[rs];
					}
					break;
				}
			}
		}
		if ((value >> 26) == 35) {
			int rt;
			int rs;
			int immidiate;

			rt = (value >> 16) & 0x1f;
			rs = (value >> 21) & 0x1f;
			immidiate = ((int16_t) ((uint16_t) (value & 0xFFFF)));

			dprintf("lw r%d, 0x%02x(r%d)\n", rt, immidiate, rs);
			load[rt] = regs[rs] + immidiate;
			dprintf("Load addr 0x%08x\n", load[rt]);
		}
		if ((value >> 26) == 3) {
			uint32_t target;
			uint32_t pc;

			target = (value & 0x03FFFFFF) << 2;
			pc = ((uint32_t) code) - ((uint32_t) addr) + SBIOS_START_ADDRESS;
			target |= pc & 0xF0000000;
			/* jal instruction. */
			dprintf("jal 0x%x\n", target);
#if 0
			if (regs[5] == search) {
				/* memcopy gets SBIOS in register a1 and size in register a2. */
				sbiosSize = regs[6];
				printf("SBIOS size is 0x%08x.\n", sbiosSize);
				break;
			}
#endif
		}
	}
	printf("SBIOS Call table offset is at 0x%08x.\n", jumpBase);
	if (jumpBase != 0) {
		jumpBase = jumpBase - SBIOS_START_ADDRESS + ((uint32_t) addr);
	}
	return jumpBase;
}

int main(int argc, char *argv[])
{
	FILE *fin;
	const char *filename;
	char magic_string[] = "PS2b";
	uint32_t *magic = (uint32_t *) magic_string;

	if (argc != 2) {
		fprintf(stderr, "Usage:\n");
		fprintf(stderr, "%s [path to sbios.bin]\n\n", argv[0]);
		fprintf(stderr, "Get sbios call table offset from sbios.bin\n");
		fprintf(stderr, "and print function entry pointers.\n");
		return -1;
	}
	filename = argv[1];
	printf("Search for sbios call table in \"%s\".\n", filename);

	fin = fopen(filename, "rb");
	if (fin != NULL)
	{
		char *buffer;
		uint32_t size;

		fseek(fin, 0, SEEK_END);
		size = ftell(fin);
		fseek(fin, 0, SEEK_SET);
		buffer = malloc(size);
		if (buffer != NULL)
		{
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
				uint32_t *SBIOSCallTable;
				int k;
				addr -= 4;

				printf("Found sbios.bin at file offset 0x%08x.\n",
					((uint32_t) addr) - ((uint32_t) buffer));
				SBIOSCallTable = getSBIOSCallTable(addr);
				printf("Found call table at address 0x%08x\n", (uint32_t) SBIOSCallTable);
				if (SBIOSCallTable != NULL) {
					for (k =0 ; k < 200; k++) {
						printf("Call %d 0x%08x\n", k, SBIOSCallTable[k]);
					}
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
	}
	
	return 0;
}

/* Copyright (c) 2007 - 2010 Mega Man */
#include "iopmem.h"
#include "memory.h"
#include "smod.h"
#include "string.h"
#include "sharedmem.h"

/** IOP RAM is mysteriously mapped into EE HW space at this address. */
#define SUB_VIRT_MEM    0xbc000000

static unsigned char *sharedMem = (unsigned char *) 0x001ff000;

static int initialized = 0;

/** Read iop memory address. */
unsigned int iop_read(void *addr, void *buf, unsigned int size)
{
	memcpy(buf, addr + SUB_VIRT_MEM, size);

	return size;
}

/** Write to iop memory address. */
unsigned int iop_write(void *addr, void *buf, unsigned int size)
{
	memcpy(addr + SUB_VIRT_MEM, buf, size);

	return size;
}

/** Search for module by name. */
int find_module(smod_mod_info_t *module, const char *searchtext)
{
	smod_mod_info_t *current;
	char name[32];
	int i;
	int len;

	len = strlen(searchtext);
	current = NULL;
	i = 0;
	while (smod_get_next_mod(current, module) != 0)
	{
		iop_read(module->name, name, sizeof(name));
		name[sizeof(name) - 1] = 0;

		if (memcmp(name, searchtext, len) == 0) {
			return 0;
		}
		current = module;
		i++;
	}
	return -1;
}


/** Initialize shared memory pointer. */
void iop_init_shared(void)
{
	smod_mod_info_t module;
	char magic[32];

	if (find_module(&module, SHAREDMEM_MODULE_NAME) == 0) {
		uint32_t addr;
		uint32_t end;

		addr = module.text_start + module.text_size;
		end = addr + module.data_size + module.bss_size;

		addr = (addr + 16 - 1) & ~(16 - 1);

		while(addr < end) {
			iop_read((void *) addr, magic, sizeof(magic));
			if (memcmp(magic, SHAREDMEM_MAGIC, sizeof(SHAREDMEM_MODULE_NAME)) == 0) {
				sharedmem_dbg_t *dbg = (void *) addr;
				sharedMem = &dbg->shared[0];
				initialized = 2;
				return;
			}
			addr += 16;
		}
	}
	initialized = 1;
}


/** Print one character. */
static void iop_putc(unsigned char c)
{
	char buf[2];

	if (initialized == 0) {
		iop_init_shared();
	}
	if (initialized == 1) {
		return;
	}

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

	if (initialized == 0) {
		iop_init_shared();
	}
	if (initialized == 1) {
		return;
	}

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
	if (initialized == 0) {
		iop_init_shared();
	}
	if (initialized == 1) {
		return;
	}

	while(*text != 0) {
		iop_putc(*text);
		text++;
	}
}


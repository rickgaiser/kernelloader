#include "iopmemdebug.h"
#include "iopmem.h"
#include "core.h"
#include "tge_defs.h"
#include "sharedmem.h"
#include "smod.h"
#include "string.h"

#define SBIOS_DEBUG 1

static unsigned char *sharedMem = (unsigned char *) NULL;

static int initialized = 0;

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
		u32 addr;
		u32 end;

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
void iop_putc(unsigned char c)
{
#ifdef SBIOS_DEBUG
	char buf[2];
	uint32_t status;

	if (initialized == 0) {
		iop_init_shared();
	}
	if (initialized == 1) {
		return;
	}

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
int iop_prints(const char *text)
{
	if (initialized == 0) {
		iop_init_shared();
	}
	if (initialized == 1) {
		return -1;
	}
	while(*text != 0) {
		iop_putc(*text);
		text++;
	}
	return 0;
}

int puts(const char *s)
{
	if (initialized == 0) {
		iop_init_shared();
	}
	if (initialized == 1) {
		return 0;
	}
	while(*s != 0) {
		iop_putc(*s);
		s++;
	}
	iop_putc('\n');
	return 0;
}

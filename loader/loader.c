/* Copyright (c) 2007 - 2009 Mega Man */
#include <stdio.h>
#include <string.h>
#include <kernel.h>
#include <iopheap.h>
#include <loadfile.h>
#include <sbv_patches.h>
#include <sifrpc.h>
#include <iopcontrol.h>
#include <debug.h>
#include <osd_config.h>
#include "libkbd.h"
#include "SMS_CDVD.h"
#include "SMS_CDDA.h"

#include "cache.h"
#include "elf.h"
#include "bootinfo.h"
#include "iopmem.h"
#include "jump.h"
#include "exception.h"
#include "memory.h"
#include "interrupts.h"
#include "config.h"
#include "loader.h"
#include "graphic.h"
#include "smem.h"
#include "smod.h"
#include "ps2dev9.h"
#include "hdd.h"
#include "debug.h"
#include "pad.h"
#include "rom.h"
#include "eedebug.h"
#include "zlib.h"
#include "configuration.h"
#include "ps2dev9.h"
#include "hdd.h"
#include "modules.h"
#include "nvram.h"
#include "kprint.h"
#include "crc32check.h"

#define SET_PCCR(val) \
	__asm__ __volatile__( \
		"mtc0 %0, $25\n" \
		"sync.p\n"::"r" (val))

#define I_STAT 0xB000F000
#define I_MASK 0xB000F010
#define D_CTRL 0xB000E000
#define D_ENABLEW 0xB000F590
#define D0_CHCR  0xB0008000
#define D1_CHCR  0xB0009000
#define D2_CHCR  0xB000A000
#define D3_CHCR  0xB000B000
#define D4_CHCR  0xB000B400
#define D5_CHCR  0xB000C000
#define D6_CHCR  0xB000C400
#define D7_CHCR  0xB000C800
#define D8_CHCR  0xB000D000
#define D9_CHCR  0xB000D400
#define BARRIER() \
	/* Barrier. */ \
	__asm__ __volatile__("sync.p"::);

#if 0
/** Debug print. */
#define dprintf kprintf
#else
/** Debug print. */
#define dprintf(args...) do { } while(0)
#endif

/** Number of instructions checked to find SBIOS call table. */
#define NUMBER_OF_INSTRUCTIONS_CHECKED 128

/** Definition of kernel entry function. */
typedef int (entry_t)(int argc, char **argv, char **envp, int *prom_vec);

/** Parameter for IOP reset. */
char iop_reset_param[MAX_INPUT_LEN] = "rom0:UDNL rom0:OSDCNF";

/** Modules that should be loaded. */
moduleEntry_t modules[] = {
	{
		.path = "host:eedebug.irx",
		.buffered = -1,
		.argLen = 0,
		.args = NULL,
		.debug_mode = -1,
	},
	{
		.path = CONFIG_DIR "/init.irx",
		.buffered = -1,
		.argLen = 0,
		.args = NULL,
	},
	{
		.path = "rom0:ADDDRV",
		.buffered = 0,
		.argLen = 0,
		.args = NULL,
		.defaultmod = 1,
		.debug_mode = -1,
	},
	{
		.path = "host:eromdrvloader.irx",
		.buffered = -1,
		.argLen = 0,
		.args = NULL,
		.defaultmod = 1,
		.eromdrv = 1,
		.debug_mode = -1,
	},
	{
#ifdef RTE
		.path = "host:RTE/sio2man.irx",
#else
		.path = CONFIG_DIR "/sio2man.irx",
#endif
		.buffered = -1,
		.argLen = 0,
		.args = NULL,
		.debug_mode = -1,
	},
	{
		.path = "rom0:XSIO2MAN",
		.buffered = 0,
		.argLen = 0,
		.args = NULL,
		.defaultmod = 1,
		.slim = 1, /* XXX: New modules seems to be more stable on heavy USB usage. */
		.debug_mode = -1,
	},
#if 0
	{
		.path = "host:freesio2.irx",
		.buffered = -1,
		.argLen = 0,
		.args = NULL,
		.debug_mode = -1,
	},
#endif
	{
		.path = "rom0:SIO2MAN",
		.buffered = 0,
		.argLen = 0,
		.args = NULL,
		.defaultmod = 1,
		.slim = -1, /* XXX: SBIOS causes system hang on heavy USB usage. */
		.debug_mode = -1,
	},
	{
		.path = "rom1:SIO2MAN",
		.buffered = 0,
		.argLen = 0,
		.args = NULL,
		.debug_mode = -1,
	},
	{
#ifdef RTE
		.path = "host:RTE/mcman.irx",
#else
		.path = CONFIG_DIR "/mcman.irx",
#endif
		.buffered = -1,
		.argLen = 0,
		.args = NULL,
		.debug_mode = -1,
	},
	{
		.path = "rom0:XMCMAN",
		.buffered = 0,
		.argLen = 0,
		.args = NULL,
		.defaultmod = 1,
		.slim = 1, /* XXX: New modules seems to be more stable on heavy USB usage. */
		.debug_mode = -1,
	},
	{
		.path = "rom0:MCMAN",
		.buffered = 0,
		.argLen = 0,
		.args = NULL,
		.defaultmod = 1,
		.slim = -1, /* XXX: SBIOS causes system hang on heavy USB usage. */
		.debug_mode = -1,
	},
	{
#ifdef RTE
		.path = "host:RTE/mcserv.irx",
#else
		.path = CONFIG_DIR "/mcserv.irx",
#endif
		.buffered = -1,
		.argLen = 0,
		.args = NULL,
		.debug_mode = -1,
	},
	{
		.path = "rom0:XMCSERV",
		.buffered = 0,
		.argLen = 0,
		.args = NULL,
		.defaultmod = 1,
		.slim = 1, /* XXX: New modules seems to be more stable on heavy USB usage. */
		.debug_mode = -1,
	},
	{
		.path = "rom0:MCSERV",
		.buffered = 0,
		.argLen = 0,
		.args = NULL,
		.defaultmod = 1,
		.slim = -1, /* XXX: SBIOS causes system hang on heavy USB usage. */
		.debug_mode = -1,
	},
	{
#ifdef RTE
		.path = "host:RTE/padman.irx",
#else
		.path = CONFIG_DIR "/padman.irx",
#endif
		.buffered = -1,
		.argLen = 0,
		.args = NULL,
		.debug_mode = -1,
	},
	{
		.path = "rom0:XPADMAN",
		.buffered = 0,
		.argLen = 0,
		.args = NULL,
		.defaultmod = 1,
		.slim = 1, /* XXX: New modules seems to be more stable on heavy USB usage. */
		.debug_mode = -1,
	},
#if 0
	{
		.path = "host:freepad.irx",
		.buffered = -1,
		.argLen = 0,
		.args = NULL,
		.debug_mode = -1,
	},
#endif
	{
		.path = "rom0:PADMAN",
		.buffered = 0,
		.argLen = 0,
		.args = NULL,
		.defaultmod = 1,
		.slim = -1, /* XXX: SBIOS causes system hang on heavy USB usage. */
		.debug_mode = -1,
	},
	{
		.path = "rom1:PADMAN",
		.buffered = 0,
		.argLen = 0,
		.args = NULL,
		.debug_mode = -1,
	},
	{
#ifdef RTE
		.path = "host:RTE/libsd.irx",
#else
		.path = CONFIG_DIR "/libsd.irx",
#endif
		.buffered = -1,
		.argLen = 0,
		.args = NULL,
		.libsd = -1,
	},
	{
		.path = CONFIG_DIR "/LIBSD.IRX",
		.buffered = -1,
		.argLen = 0,
		.args = NULL,
		.libsd = -1,
	},
	{
		.path = "rom0:LIBSD",
		.buffered = 0,
		.argLen = 0,
		.args = NULL,
		.libsd = -1,
	},
	{
		.path = "rom1:LIBSD",
		.buffered = 0,
		.argLen = 0,
		.args = NULL,
		.libsd = -1,
		.sound = 1,
	},
#if 0
	{
		.path = "host:freesd.irx",
		.buffered = -1,
		.argLen = 0,
		.args = NULL,
		.libsd = -1,
	},
#endif
	{
#ifdef RTE
		.path = "host:RTE/sdrdrv.irx",
#else
		.path = CONFIG_DIR "/sdrdrv.irx",
#endif
		.buffered = -1,
		.argLen = 0,
		.args = NULL,
	},
	{
		.path = CONFIG_DIR "/SDRDRV.IRX",
		.buffered = -1,
		.argLen = 0,
		.args = NULL,
	},
	{
		.path = "rom1:SDRDRV",
		.buffered = 0,
		.argLen = 0,
		.args = NULL,
		.sound = 1,
	},
	{
		.path = "host:audsrv.irx",
		.buffered = -1,
		.argLen = 0,
		.args = NULL,
	},
	{
		.path = "host:ioptrap.irx",
		.buffered = -1,
		.argLen = 0,
		.args = NULL,
		.debug_mode = -1,
	},
	{
		.path = "host:iomanX.irx",
		.buffered = -1,
		.argLen = 0,
		.args = NULL,
		.defaultmod = 1,
		.slim = 1,
		.debug_mode = -1,
	},
	{
		.path = "host:poweroff.irx",
		.buffered = -1,
		.argLen = 0,
		.args = NULL,
		.defaultmod = 1,
		.debug_mode = -1,
	},
	{
		.path = "host:ps2dev9.irx",
		.buffered = -1,
		.argLen = 0,
		.args = NULL,
		.defaultmod = 1,
		.slim = 1,
		.debug_mode = -1,
	},
	{
		.path = "host:ps2ip.irx",
		.buffered = -1,
		.argLen = 0,
		.args = NULL,
		.network = -1,
		.debug_mode = -1,
	},
	{
		.path = "host:ps2smap.irx",
		.buffered = -1,
		.argLen = 0,
		.args = NULL,
		.ps2smap = 1,
		.network = -1,
		.debug_mode = -1,
	},
 	{
		.path = "host:smaprpc.irx",
		.buffered = -1,
		.argLen = 0,
		.args = NULL,
		.defaultmod = 1,
		.slim = 1,
		.network = -1,
		.debug_mode = -1,
	},
	{
		.path = "host:ps2link.irx",
		.buffered = -1,
		.argLen = 0,
		.args = NULL,
		.network = -1,
		.debug_mode = -1,
	},
	{
		.path = "host:sharedmem.irx",
		.buffered = -1,
		.argLen = 0,
		.args = NULL,
#ifdef SHARED_MEM_DEBUG
		.load = 1,
#else
		.load = 0,
#endif
	},
	{
#ifdef RTE
		.path = "host:RTE/iopintr.irx",
#else
		.path = CONFIG_DIR "/iopintr.irx",
#endif
		.buffered = -1,
		.argLen = 0,
		.args = NULL,
		.debug_mode = -1,
	},
	{
		/* Interrupt relay when DEV9 is not loaded. */
		.path = "host:TGE/intrelay-direct.irx",
		.buffered = -1,
		.argLen = 0,
		.args = NULL,
		.defaultmod = 2,
		.slim = -1,
		.debug_mode = -1,
	},
	{
		/* Interrupt relay when DEV9 is not loaded and slim PSTwo. */
		.path = "host:TGE/intrelay-direct-rpc.irx",
		.buffered = -1,
		.argLen = 0,
		.args = NULL,
		.debug_mode = -1,
	},
	{
		/* Interrupt relay when DEV9 is loaded. */
		.path = "host:TGE/intrelay-dev9.irx",
		.buffered = -1,
		.argLen = 0,
		.args = NULL,
		.defaultmod = 2,
		.slim = -1,
		/* Only hard disc and USB is working. */
	},
	{
		/* Interrupt relay when DEV9 is loaded and slim PSTwo. */
		.path = "host:TGE/intrelay-dev9-rpc.irx",
		.buffered = -1,
		.argLen = 0,
		.args = NULL,
		.defaultmod = 2,
		.slim = 1,
		/* Only hard disc and USB is working. Network not working from EE side (use smaprpc.irx). */
	},
	{
#ifdef RTE
		.path = "host:RTE/dmarelay.irx",
#else
		.path = CONFIG_DIR "/dmarelay.irx",
#endif
		.buffered = -1,
		.argLen = 0,
		.args = NULL,
	},
	{
		.path = "host:TGE/dmarelay.irx",
		.buffered = -1,
		.argLen = 0,
		.args = NULL,
		/* XXX: Module is broken. */
	},
#if 0
	{
		.path = "host:dev9init.irx",
		.buffered = -1,
		.argLen = 0,
		.args = NULL,
#if 0 /* Module seems not to be required. */
		.defaultmod = 1,
		.slim = -1,
#endif
		.dev9init = -1,
		.debug_mode = -1,
	},
#endif
	{
#ifdef RTE
		.path = "host:RTE/cdvdman.irx",
#else
		.path = CONFIG_DIR "/cdvdman.irx",
#endif
		.buffered = -1,
		.argLen = 0,
		.args = NULL,
		.debug_mode = -1,
	},
	{
#ifdef RTE
		.path = "host:RTE/cdvdfsv.irx",
#else
		.path = CONFIG_DIR "/cdvdfsv.irx",
#endif
		.buffered = -1,
		.argLen = 0,
		.args = NULL,
		.debug_mode = -1,
	},
	{
		.path = "rom1:RMMAN",
		.buffered = 0,
		.argLen = 0,
		.args = NULL,
	},
	{
		.path = "rom1:RMMAN2",
		.buffered = 0,
		.argLen = 0,
		.args = NULL,
	},
	{
		.path = CONFIG_DIR "/module1.irx",
		.buffered = -1,
		.argLen = 0,
		.args = NULL,
	},
	{
		.path = CONFIG_DIR "/module2.irx",
		.buffered = -1,
		.argLen = 0,
		.args = NULL,
	},
	{
		.path = CONFIG_DIR "/module3.irx",
		.buffered = -1,
		.argLen = 0,
		.args = NULL,
	},
	{
		.path = CONFIG_DIR "/module4.irx",
		.buffered = -1,
		.argLen = 0,
		.args = NULL,
	},
	{
		.path = CONFIG_DIR "/module5.irx",
		.buffered = -1,
		.argLen = 0,
		.args = NULL,
	},
};

/** Original code to patch in libsd. */
uint8_t libsdOrig[] = {
	0x80, 0xbf, 0x09, 0x3c, 0x04, 0x14, 0x29, 0x35, 0x80, 0xbf, 0x08, 0x3c, 0x0c, 0x14, 0x08, 0x35,
	0x90, 0xbf, 0x03, 0x3c, 0x00, 0x08, 0x63, 0x34, 0x80, 0xbf, 0x04, 0x3c, 0xf0, 0x10, 0x84, 0x34,
	0x80, 0xbf, 0x06, 0x3c, 0x70, 0x15, 0xc6, 0x34, 0x80, 0xbf, 0x0a, 0x3c, 0x14, 0x10, 0x4a, 0x35,
	0x0b, 0x20, 0x05, 0x3c, 0xe1, 0x31, 0xa5, 0x34, 0x80, 0xbf, 0x07, 0x3c, 0x90, 0xbf, 0x02, 0x3c,
	0x00, 0x00, 0x22, 0xad, 0x00, 0x00, 0x03, 0xad, 0x00, 0x00, 0x82, 0x8c, 0x08, 0x00, 0x03, 0x3c,
	0x25, 0x10, 0x43, 0x00, 0x00, 0x00, 0x82, 0xac, 0x00, 0x00, 0xc2, 0x8c, 0x14, 0x14, 0xe7, 0x34,
	0x08, 0x00, 0x42, 0x34, 0x00, 0x00, 0xc2, 0xac, 0x00, 0x00, 0x45, 0xad, 0x08, 0x00, 0xe0, 0x03,
	0x00, 0x00, 0xe5, 0xac, 0x90, 0xbf, 0x02, 0x3c, 0xc6, 0x07, 0x42, 0x34, 0x90, 0xbf, 0x05, 0x3c,
};

/** New code for libsd, which enables USB and sound. USB was not working on newer slim PSTwo (v15). */
uint8_t libsdPatched[] = {
	0x80, 0xbf, 0x0a, 0x3c, 0x04, 0x14, 0x49, 0x35, 0x00, 0x08, 0x0b, 0x3c, 0x0c, 0x14, 0x48, 0x35,
	0x90, 0xbf, 0x03, 0x3c, 0x00, 0x08, 0x63, 0x34, 0x08, 0x00, 0x6b, 0x35, 0xf0, 0x10, 0x44, 0x35,
	0x80, 0xbf, 0x06, 0x3c, 0x70, 0x15, 0xc6, 0x34, 0x80, 0xbf, 0x0a, 0x3c, 0x14, 0x10, 0x4a, 0x35,
	0x0b, 0x20, 0x05, 0x3c, 0xe1, 0x31, 0xa5, 0x34, 0x80, 0xbf, 0x07, 0x3c, 0x90, 0xbf, 0x02, 0x3c,
	0x00, 0x00, 0x22, 0xad, 0x00, 0x00, 0x03, 0xad, 0x00, 0x00, 0x82, 0x8c, 0x08, 0x00, 0x03, 0x3c,
	0x25, 0x10, 0x43, 0x00, 0x00, 0x00, 0x82, 0xac, 0x00, 0x00, 0xc2, 0x8c, 0x14, 0x14, 0xe7, 0x34,
	0x25, 0x10, 0x4b, 0x00, 0x00, 0x00, 0xc2, 0xac, 0x00, 0x00, 0x45, 0xad, 0x08, 0x00, 0xe0, 0x03,
	0x00, 0x00, 0xe5, 0xac, 0x90, 0xbf, 0x02, 0x3c, 0xc6, 0x07, 0x42, 0x34, 0x90, 0xbf, 0x05, 0x3c,
};

/** Start address of linker section ".text" of this loader. */
void _ftext(void);
/** Start address of linker section ".rom" of this loader. */
void _from(void);
/** End address of this loader. */
void _end(void);

void *kmemcpy(void *dest, const void *src, int size)
{
	int i;
	uint8_t *d = dest;
	const uint8_t *s = src;

	for (i = 0; i < size; i++) {
		d[i] = s[i];
	}
	return dest;
}

/** Patch libsd, so that sound and USB is working at the same time. */
void patchLibsd(uint8_t *buffer, uint32_t size)
{
	uint32_t i;

	for (i = 0; i < (size - sizeof(libsdOrig)); i++) {
		if (memcmp(buffer + i, libsdOrig, sizeof(libsdOrig)) == 0) {
			kmemcpy(buffer + i, libsdPatched, sizeof(libsdPatched));
		}
	}
}

/**
 * Check program sections of ELF file against memory range.
 * @param Name of file
 * @param buffer Pointer to ELF file.
 * @param start Physical start address (included).
 * @param end Physical end address (excluded).
 */
int check_sections32(const char *name, const char *buffer, uint32_t filesize, uint32_t start, uint32_t end, uint32_t *highest, uint8_t *destaddr)
{
	const Elf32_Ehdr_t *file_header;
	int pos = 0;
	int i;
	uint32_t entry = 0;
	uint32_t area;
	int copied_entry = 0;

	if (highest != NULL) {
		*highest = 0;
	}

	file_header = (const Elf32_Ehdr_t *) &buffer[pos];
	pos += sizeof(Elf32_Ehdr_t);
	if (file_header->magic != ELFMAGIC) {
		kprintf("Magic 0x%08x is wrong at 0x%08x.\n", file_header->magic, &file_header->magic);
		error_printf("Magic 0x%08x is wrong.\n", file_header->magic);
		return -1;
	}
	entry = file_header->entry;
	kprintf("entry is 0x%08x\n", entry);
	area = entry >> 28;
	switch(area) {
		case 0x8:
		case 0xa:
			/* Everything is fine. */
			break;
		default:
			error_printf("Bad entry address 0x%08x of %s is not accessible by loader.",
				entry, name);
			return -20;
	}
	entry = PHYSADDR(entry);
	if (entry < start) {
		error_printf("Entry point 0x%08x of %s is before 0x%08x.",
			entry, name, start);
		return -21;
	}
	if (entry >= end) {
		error_printf("Entry point 0x%08x of %s is after  0x%08x.",
			entry, name, end);
		return -21;
	}
	for (i = 0; i < file_header->phnum; i++)
	{
		Elf32_Phdr_t *program_header;
		program_header = (Elf32_Phdr_t *) &buffer[pos];
		pos += sizeof(Elf32_Phdr_t);
		if ( (program_header->type == PT_LOAD)
			&& (program_header->memsz != 0) )
		{
			uint32_t dest;
			uint32_t size;

			kprintf("VAddr: 0x%08x PAddr: 0x%08x Offset 0x%08x Size 0x%08x\n",
				program_header->vaddr,
				program_header->paddr,
				program_header->offset,
				program_header->filesz);

			// Get physical address which can be accessed by loader.
			dest = program_header->paddr;
			size = program_header->memsz;
			if (size < program_header->filesz) {
				size = program_header->filesz;
			}

			if (size != 0) {
				if (program_header->filesz != 0) {
					uint32_t endoffset;

					/* Check if file is completly loaded. */
					if (program_header->filesz > (128 * 1024 * 1024)) {
						error_printf("The %s file has a section with a bad file size.",
							name, program_header->filesz);
						return -31;
					}
					if (program_header->offset > (128 * 1024 * 1024)) {
						error_printf("The %s file has a section with a bad offset.",
							name, program_header->filesz);
						return -31;
					}

					endoffset = program_header->offset + program_header->filesz;

					if (endoffset >= filesize) {
						error_printf("The %s file with %u Bytes must be at least %d Bytes. Please redownload this file.",
							name, filesize, endoffset);
						return -30;
					}
				}
				uint32_t last;
				area = dest >> 28;
				switch(area) {
					case 0x8:
					case 0xa:
						/* Everything is fine. */
						break;
					default:
						error_printf("Bad memory address 0x%08x of %s is not accessable by loader.",
							dest, name);
						return -2;
				}
				dest = PHYSADDR(dest);

				if (dest < start) {
					error_printf("The memory region start address 0x%08x of %s is before 0x%08x.",
						dest, name, start);
					return -3;
				}

				last = dest + size;
				if (highest != NULL) {
					if (*highest < last) {
						*highest = last;
					}
				}
				if (last >= end) {
					error_printf("The memory region end address 0x%08x of %s is after 0x%08x.",
						last, name, end);
					return -4;
				}
				if ((dest <= entry)
					&& ((dest + size) > entry)) {
					copied_entry = -1;
				}

				if (destaddr != NULL) {
					uint8_t *d = &destaddr[dest - start];
					int size = program_header->memsz - program_header->filesz;

					if (program_header->filesz != 0)
					{
						kprintf("VAddr: 0x%08x PAddr: 0x%08x Offset 0x%08x Size 0x%08x\n",
							program_header->vaddr,
							program_header->paddr,
							program_header->offset,
							program_header->filesz);
						kmemcpy(d, &buffer[program_header->offset], program_header->filesz);
					}
					if (size > 0)
						memset(&d[program_header->filesz], 0, size);
				}
			}
		}
	}
	if (!copied_entry) {
		error_printf("The ELF doesn't contain code at the entry point.\n");
		return -5;
	}
	return 0;
}

int check_sections64(const char *name, const char *buffer, uint32_t filesize, uint32_t start, uint32_t end, uint32_t *highest, uint8_t *destaddr)
{
	const Elf64_Ehdr_t *file_header;
	int pos = 0;
	int i;
	uint32_t entry = 0;
	uint32_t area;
	int copied_entry = 0;

	if (highest != NULL) {
		*highest = 0;
	}

	file_header = (const Elf64_Ehdr_t *) &buffer[pos];
	pos += sizeof(Elf64_Ehdr_t);
	if (file_header->magic != ELFMAGIC) {
		error_printf("Magic 0x%08x is wrong.\n", file_header->magic);
		return -1;
	}
	entry = file_header->entry;
	kprintf("entry is 0x%08llx\n", (unsigned long long) file_header->entry);
	area = entry >> 28;
	switch(area) {
		case 0x8:
		case 0xa:
			/* Everything is fine. */
			break;
		default:
			error_printf("Bad entry address 0x%08x of %s is not accessible by loader.",
				entry, name);
			return -20;
	}
	entry = PHYSADDR(entry);
	if (entry < start) {
		error_printf("Entry point 0x%08x of %s is before 0x%08x.",
			entry, name, start);
		return -21;
	}
	if (entry >= end) {
		error_printf("Entry point 0x%08x of %s is after  0x%08x.",
			entry, name, end);
		return -21;
	}
	for (i = 0; i < file_header->phnum; i++)
	{
		Elf64_Phdr_t *program_header;
		program_header = (Elf64_Phdr_t *) &buffer[pos];
		pos += sizeof(Elf64_Phdr_t);
		if ( (program_header->type == PT_LOAD)
			&& (program_header->memsz != 0) )
		{
			uint32_t dest;
			uint32_t size;

			kprintf("VAddr: 0x%08llx PAddr: 0x%08llx Offset 0x%08llx Size 0x%08llx\n",
				(unsigned long long) program_header->vaddr,
				(unsigned long long) program_header->paddr,
				(unsigned long long) program_header->offset,
				(unsigned long long) program_header->filesz);

			// Get physical address which can be accessed by loader.
			dest = program_header->paddr;
			size = program_header->memsz;
			if (size < program_header->filesz) {
				size = program_header->filesz;
			}

			if (size != 0) {
				if (program_header->filesz != 0) {
					uint32_t endoffset;

					/* Check if file is completly loaded. */
					if (program_header->filesz > (128 * 1024 * 1024)) {
						error_printf("The %s file has a section with a bad file size.",
							name, program_header->filesz);
						return -31;
					}
					if (program_header->offset > (128 * 1024 * 1024)) {
						error_printf("The %s file has a section with a bad offset.",
							name, program_header->filesz);
						return -31;
					}

					endoffset = program_header->offset + program_header->filesz;

					if (endoffset >= filesize) {
						error_printf("The %s file with %u Bytes must be at least %d Bytes. Please redownload this file.",
							name, filesize, endoffset);
						return -30;
					}
				}
				uint32_t last;
				area = dest >> 28;
				switch(area) {
					case 0x8:
					case 0xa:
						/* Everything is fine. */
						break;
					default:
						error_printf("Bad memory address 0x%08x of %s is not accessable by loader.",
							dest, name);
						return -2;
				}
				dest = PHYSADDR(dest);

				if (dest < start) {
					error_printf("The memory region start address 0x%08x of %s is before 0x%08x.",
						dest, name, start);
					return -3;
				}

				last = dest + size;
				if (highest != NULL) {
					if (*highest < last) {
						*highest = last;
					}
				}
				if (last >= end) {
					error_printf("The memory region end address 0x%08x of %s is after 0x%08x.",
						last, name, end);
					return -4;
				}
				if ((dest <= entry)
					&& ((dest + size) > entry)) {
					copied_entry = -1;
				}

				if (destaddr != NULL) {
					uint8_t *d = &destaddr[dest - start];
					int size = program_header->memsz - program_header->filesz;

					if (program_header->filesz != 0)
					{
						kprintf("VAddr: 0x%08x PAddr: 0x%08x Offset 0x%08x Size 0x%08x\n",
							program_header->vaddr,
							program_header->paddr,
							program_header->offset,
							program_header->filesz);
						kmemcpy(d, &buffer[program_header->offset], program_header->filesz);
					}
					if (size > 0)
						memset(&d[program_header->filesz], 0, size);
				}
			}
		}
	}
	if (!copied_entry) {
		error_printf("The ELF doesn't contain code at the entry point.\n");
		return -5;
	}
	return 0;
}

int check_sections(const char *name, const char *buffer, uint32_t filesize, uint32_t start, uint32_t end, uint32_t *highest, uint8_t *destaddr)
{
	Elf32_Ehdr_t *file_header;
	int pos = 0;

	start = PHYSADDR(start);
	end = PHYSADDR(end);

	file_header = (Elf32_Ehdr_t *) &buffer[pos];
	if (file_header->info[0] == 2) {
		/* 64 Bit ELF file. */
		return check_sections64(name, buffer, filesize, start, end, highest, destaddr);
	} else {
		/* 32 Bit ELF file. */
		return check_sections32(name, buffer, filesize, start, end, highest, destaddr);
	}
}

/**
 * Copy program sections of ELF file to memory.
 * @param buffer Pointer to ELF file.
 */
entry_t *copy_sections32(const char *buffer)
{
	const Elf32_Ehdr_t *file_header;
	int pos = 0;
	int i;
	entry_t *entry = NULL;

	file_header = (const Elf32_Ehdr_t *) &buffer[pos];
	pos += sizeof(Elf32_Ehdr_t);
	if (file_header->magic != ELFMAGIC) {
		kprintf("Magic 0x%08x is wrong.\n", file_header->magic);
		return NULL;
	}
	entry = (entry_t *) file_header->entry;
	kprintf("entry is 0x%08x\n", (int) entry);
	for (i = 0; i < file_header->phnum; i++)
	{
		const Elf32_Phdr_t *program_header;
		program_header = (const Elf32_Phdr_t *) &buffer[pos];
		pos += sizeof(Elf32_Phdr_t);

		if ( (program_header->type == PT_LOAD)
			&& (program_header->memsz != 0) )
		{
			uint8_t *dest;

			// Copy to physical address which can be accessed by loader.
			dest = (uint8_t *) program_header->paddr;

			if (program_header->filesz != 0)
			{
				kprintf("VAddr: 0x%08x PAddr: 0x%08x Offset 0x%08x Size 0x%08x\n",
					program_header->vaddr,
					program_header->paddr,
					program_header->offset,
					program_header->filesz);
				kmemcpy(dest, &buffer[program_header->offset], program_header->filesz);
				kprintf("First bytes 0x%02x 0x%02x\n", (int) dest[0], (int) dest[1]);
			}
			int size = program_header->memsz - program_header->filesz;
			if (size > 0)
				memset(&dest[program_header->filesz], 0, size);
		}
	}
	return entry;
}

entry_t *copy_sections64(const char *buffer)
{
	const Elf64_Ehdr_t *file_header;
	int pos = 0;
	int i;
	entry_t *entry = NULL;

	file_header = (const Elf64_Ehdr_t *) &buffer[pos];
	pos += sizeof(Elf64_Ehdr_t);
	if (file_header->magic != ELFMAGIC) {
		kprintf("Magic 0x%08x is wrong.\n", file_header->magic);
		return NULL;
	}
	entry = (entry_t *) PHYSADDR(file_header->entry);
	kprintf("entry is 0x%08x\n", (int) entry);
	kprintf("phnum %d\n", (int) file_header->phnum);
	for (i = 0; i < file_header->phnum; i++)
	{
		const Elf64_Phdr_t *program_header;
		program_header = (const Elf64_Phdr_t *) &buffer[pos];
		pos += sizeof(Elf64_Phdr_t);

		if ( (program_header->type == PT_LOAD)
			&& (program_header->memsz != 0) )
		{
			uint8_t *dest;

			// Copy to physical address which can be accessed by loader.
			dest = (uint8_t *) ((uint32_t) program_header->paddr);

			if (program_header->filesz != 0)
			{
				kprintf("VAddr: 0x%08x PAddr: 0x%08x Offset 0x%08x Size 0x%08x\n",
					program_header->vaddr,
					program_header->paddr,
					program_header->offset,
					program_header->filesz);
				kmemcpy(dest, &buffer[program_header->offset], program_header->filesz);
				kprintf("First bytes 0x%02x 0x%02x\n", (int) dest[0], (int) dest[1]);
			}
			int size = program_header->memsz - program_header->filesz;
			if (size > 0)
				memset(&dest[program_header->filesz], 0, size);
		}
	}
	return entry;
}

entry_t *copy_sections(const char *buffer)
{
	const Elf32_Ehdr_t *file_header;
	int pos = 0;

	file_header = (const Elf32_Ehdr_t *) &buffer[pos];
	if (file_header->info[0] == 2) {
		/* 64 Bit ELF file. */
		return copy_sections64(buffer);
	} else {
		/* 32 Bit ELF file. */
		return copy_sections32(buffer);
	}
}

/**
 * Verify if program sections of ELF are copied correctly to memory.
 * Required to check if kernel can be started without problems.
 * @param buffer Pointer to ELF file.
 */
void verify_sections32(const char *buffer)
{
	const Elf32_Ehdr_t *file_header;
	int pos = 0;
	int i;
	entry_t *entry = NULL;

	file_header = (const Elf32_Ehdr_t *) &buffer[pos];
	pos += sizeof(Elf32_Ehdr_t);
	if (file_header->magic != ELFMAGIC) {
		kprintf("Magic 0x%08x is wrong.\n", file_header->magic);
		panic();
	}
	entry = (entry_t *) file_header->entry;
	kprintf("entry is 0x%08x\n", (int) entry);
	for (i = 0; i < file_header->phnum; i++)
	{
		const Elf32_Phdr_t *program_header;
		program_header = (const Elf32_Phdr_t *) &buffer[pos];
		pos += sizeof(Elf32_Phdr_t);
		if ( (program_header->type == PT_LOAD)
			&& (program_header->memsz != 0) )
		{
			unsigned char *dest;

			// Copied to physical address which can be accessed by loader.
			dest = (unsigned char *) (program_header->paddr);

			if (program_header->filesz != 0)
			{
				kprintf("VAddr: 0x%08x PAddr: 0x%08x Offset 0x%08x Size 0x%08x\n",
					program_header->vaddr,
					program_header->paddr,
					program_header->offset,
					program_header->filesz);
				if (memcmp(dest, &buffer[program_header->offset], program_header->filesz) != 0) {
					kputs("Verify failed");
					panic();
				};
				kprintf("First bytes 0x%02x 0x%02x\n", (int) dest[0], (int) dest[1]);
			}
			unsigned int size = program_header->memsz - program_header->filesz;
			if (size > 0) {
				unsigned int i;

				for (i = 0; i < size; i++) {
					if (dest[program_header->filesz] != 0) {
						kputs("Verify failed in memset");
						panic();
					}
				}
			}
		}
	}
	return;
}

void verify_sections64(const char *buffer)
{
	const Elf64_Ehdr_t *file_header;
	int pos = 0;
	int i;
	entry_t *entry = NULL;

	file_header = (const Elf64_Ehdr_t *) &buffer[pos];
	pos += sizeof(Elf64_Ehdr_t);
	if (file_header->magic != ELFMAGIC) {
		kprintf("Magic 0x%08x is wrong.\n", file_header->magic);
		panic();
	}
	entry = (entry_t *) PHYSADDR(file_header->entry);
	kprintf("entry is 0x%08x\n", (int) entry);
	for (i = 0; i < file_header->phnum; i++)
	{
		const Elf64_Phdr_t *program_header;
		program_header = (const Elf64_Phdr_t *) &buffer[pos];
		pos += sizeof(Elf64_Phdr_t);
		if ( (program_header->type == PT_LOAD)
			&& (program_header->memsz != 0) )
		{
			unsigned char *dest;

			// Copied to physical address which can be accessed by loader.
			dest = (unsigned char *) PHYSADDR(program_header->paddr);

			if (program_header->filesz != 0)
			{
				kprintf("VAddr: 0x%08x PAddr: 0x%08x Offset 0x%08x Size 0x%08x\n",
					program_header->vaddr,
					program_header->paddr,
					program_header->offset,
					program_header->filesz);
				if (memcmp(dest, &buffer[program_header->offset], program_header->filesz) != 0) {
					kputs("Verify failed");
					panic();
				};
				kprintf("First bytes 0x%02x 0x%02x\n", (int) dest[0], (int) dest[1]);
			}
			unsigned int size = program_header->memsz - program_header->filesz;
			if (size > 0) {
				unsigned int i;

				for (i = 0; i < size; i++) {
					if (dest[program_header->filesz] != 0) {
						kputs("Verify failed in memset");
						panic();
					}
				}
			}
		}
	}
	return;
}

void verify_sections(const char *buffer)
{
	const Elf32_Ehdr_t *file_header;
	int pos = 0;

	file_header = (const Elf32_Ehdr_t *) &buffer[pos];
	if (file_header->info[0] == 2) {
		/* 64 Bit ELF file. */
		return verify_sections64(buffer);
	} else {
		/* 32 Bit ELF file. */
		return verify_sections32(buffer);
	}
}

/**
 * Load file and return pointer to it.
 * @param filename Path to file.
 * @param size Pointer where file size will be stored, can be NULL.
 * @return Pointer to memory where file is loaded to.
 */
char *load_file(const char *filename, int *size, void *addr)
{
	char *buffer = NULL;
	int s;
	int filesize;
	FILE *fin;

	if (size == NULL) {
		size = &s;
		addr = NULL;
	}

	setEnableDisc(-1);
	graphic_setPercentage(0, filename);
	fin = fopen(filename, "rb");
	if (fin == NULL) {
		error_printf("Error cannot open file %s.", filename);
		setEnableDisc(0);
		return NULL;
	}

	fseek(fin, 0, SEEK_END);
	filesize = s = ftell(fin);
	fseek(fin, 0, SEEK_SET);
	dprintf("filesize %d\n", *size);
	if (addr != NULL) {
		if (filesize <= *size) {
			buffer = addr;
		}
	} else {
		// memalign() begins at &_end behind elf file. This ensures that the first 5 MByte are not used.
		buffer = (char *) memalign(64, filesize);
	}
	dprintf("buffer 0x%08x\n", (unsigned int) buffer);
	*size = filesize;
	if (buffer == NULL) {
		fclose(fin);
		setEnableDisc(0);
		return NULL;
	}
	if (addr == NULL) {
		if ((u32) buffer < (u32) &_end) {
			/* This will lead to problems. */
			error_printf("memalign() is unusable!");
			setEnableDisc(0);
			return NULL;
		}
	}

	dprintf("Loading...\n");
	int pos = 0;
	int n;
	int next = 10000;
	kprintf("%s size %d\n", filename, *size);
	while ((n = fread(&buffer[pos], 1, next, fin)) > 0) {
		pos += n;
		s -= n;
		if (s < next)
			next = s;
		if (next <= 0)
			break;
		dprintf(".");
		graphic_setPercentage(pos / (filesize / 100), filename);
	}
	fclose(fin);
	setEnableDisc(0);
	dprintf("Loaded %d\n", pos);
	graphic_setPercentage(100, filename);

	return buffer;
}

/**
 * Load compressed kernel file and return pointer to it.
 * @param filename Path to file.
 * @param size Pointer where file size will be stored, can be NULL.
 * @return Pointer to memory where file is loaded to.
 */
char *load_kernel_file(const char *filename, int *size)
{
	char *buffer = NULL;
	gzFile fin;
	int maxsize = 6 * 1024 * 1024;

	if (size == NULL) {
		return NULL;
	}

	setEnableDisc(-1);
	graphic_setPercentage(0, filename);
	fin = gzopen(filename, "rb");
	if (fin == NULL) {
		error_printf("Error cannot open file %s.", filename);
		kprintf("Failed to open file %s.\n", filename);
		setEnableDisc(0);
		return NULL;
	}

	// memalign() begins at &_end behind elf file. This ensures that the first 5 MByte are not used.
	// Don't know the size just use 6 MB.
	do {
		buffer = (char *) memalign(64, maxsize);
		if (buffer == NULL) {
			/* Check if 512KByte less is available. */
			maxsize -= 512 * 1024;
		}
	} while ((buffer == NULL) && (maxsize > (1024 * 1024)));

	dprintf("buffer 0x%08x\n", (unsigned int) buffer);
	if (buffer == NULL) {
		gzclose(fin);
		error_printf("Failed to allocate memory (memory leak)! Please restart kernelloader and boot kernel directly.");
		kprintf("Failed to allocate memory.\n");
		setEnableDisc(0);
		return NULL;
	}
	if ((u32) buffer < (u32) &_end) {
		/* This will lead to problems. */
		error_printf("memalign() is unusable!");
		kprintf("memalign() is unusable!\n");
		setEnableDisc(0);
		return NULL;
	}

	dprintf("Loading...\n");
	int pos = 0;
	int n;
	int next = 10 * 1024;
	while ((n = gzread(fin, &buffer[pos], next)) > 0) {
		dprintf("n = %d\n", n);
		pos += n;
		dprintf("pos = %d\n", pos);
		graphic_setPercentage(pos / (maxsize / 100), filename);
		if ((pos + next) > maxsize) {
			error_printf("Error file %s is too big (free %ukB).\n", filename, maxsize / 1024);
			if (maxsize < (6 * 1024 * 1024)) {
				error_printf("Restart kernelloader for more free memory (max. 6MB for video mode auto).");
			} else {
				error_printf("You should use ee-strip to make the kernel smaller (gzip doesn't help).");
			}
			kprintf("Error file %s is too large (> %ukByte).\n", filename, maxsize / 1024);
			setEnableDisc(0);
			gzclose(fin);
			free(buffer);
			return NULL;
		}
	}
	dprintf("last n = %d\n", n);
	*size = pos;
	kprintf("%s size %d\n", filename, *size);
	gzclose(fin);

	/* Make it smaller, so remaining memory can be used for other stuff. */
	buffer = realloc(buffer, *size);
	dprintf("buffer 0x%08x\n", (unsigned int) buffer);
	setEnableDisc(0);
	dprintf("Loaded %d\n", pos);
	graphic_setPercentage(100, filename);

	return buffer;
}

int getNumberOfModules(void)
{
	return sizeof(modules) / sizeof(moduleEntry_t);
}

moduleEntry_t *getModuleEntry(int idx)
{
	return &modules[idx];
}


/** Load all IOP modules (from host). */
int loadModules(void)
{
	int i;
	int j;

	for (i = 0; i < getNumberOfModules(); i++)
	{
		if (modules[i].network) {
			if (!hasNetworkSupport()) {
				/* Network not working. */
				continue;
			}
		}
		if ((modules[i].buffered) && (modules[i].load)) {
			const rom_entry_t *romfile = NULL;

			dprintf("Loading module %s.\n", modules[i].path);
			if (strncmp(modules[i].path, "host:", 5) == 0) {
				kprintf("Try to load %s\n", &modules[i].path[5]);
				romfile = rom_getFile(&modules[i].path[5]);
			}
			if (romfile == NULL) {
				char *buffer;
				buffer = load_file(modules[i].path, &modules[i].size, NULL);
				modules[i].buffer = buffer;
				if (buffer == NULL) {
					error_printf("Failed to load module '%s'.", modules[i].path);

					/* Free memory. */
					for (j = 0; j < i; j++) {
						if ((modules[j].buffered) && (modules[j].load)) {
							if (modules[j].allocated) {
								free(buffer);
								buffer = NULL;
								modules[i].buffer = NULL;
								modules[j].allocated = 0;
							}
						}
					}
					return -1;
				} else {
					modules[i].allocated = 1;
					if (modules[i].libsd && loaderConfig.patchLibsd) {
						patchLibsd(buffer, modules[i].size);
					}
				}
			} else {
				modules[i].allocated = 0;
				modules[i].buffer = romfile->start;
				modules[i].size = romfile->size;
			}
		} else {
			dprintf("Not loading module %s.\n", modules[i].path);
			modules[i].buffer = NULL;
		}
	}
	return 0;
}

/** Start all IOP modules. */
void startModules(struct ps2_bootinfo *bootinfo)
{
	int i;
	int rv;

	for (i = 0; i < getNumberOfModules(); i++)
	{
		if (modules[i].load) {
			if (modules[i].eromdrv) {
				if (!isDVDVSupported()) {
					/* DVD video will not work, so don't load it. */
					continue;
				}
				modules[i].args = get_eromdrvpath();
				modules[i].argLen = strlen(modules[i].args) + 1;
			}
			if (modules[i].network) {
				if (!hasNetworkSupport()) {
					/* Network not working. */
					continue;
				}
			}
			if (modules[i].ps2smap) {
				modules[i].args = getPS2MAPParameter(&modules[i].argLen);
			}
			if (modules[i].buffered) {
				int ret;

				ret = SifExecModuleBuffer((void *) modules[i].buffer, modules[i].size, modules[i].argLen, modules[i].args, &rv);
				if (ret < 0) {
					rv = ret;
				} else {
					if (modules[i].dev9init) {
						if (rv < 0) {
							/* Tell Linux that DEV9 is disabled. */
							bootinfo->pccard_type = 0;
						} else {
							if (loaderConfig.enableDev9 && hasNetworkSupport()) {
								const char *pcicType;

								/* Tell Linux that DEV9 is enabled. */
								bootinfo->pccard_type = 0x0100;

								pcicType = getPcicType();
								if (strlen(pcicType) > 0) {
									/* User configured calue in menu. */
									bootinfo->pcic_type = atoi(pcicType);
								} else {
									/* Auto detect type. */
									bootinfo->pcic_type = rv;
								}
							}

						}
						rv = 0;
					}
				}
			} else {
				rv = SifLoadModule(modules[i].path, modules[i].argLen, modules[i].args);
			}
			if (rv < 0) {
				if (modules[i].eromdrv) {
					error_printf("Failed to start module \"%s\" (rv = %d).", modules[i].args, rv);
				} else {
					error_printf("Failed to start module \"%s\" (rv = %d).", modules[i].path, rv);
				}
			}
		}
	}
}

const char *pagemask2text(uint32_t pagemask)
{
	switch((pagemask >> 13) & 0xFFF) {
		case 0x000:
			return U2K("4 Kbyte");
		case 0x003:
			return U2K("16 Kbyte");
		case 0x00f:
			return U2K("64 Kbyte");
		case 0x03f:
			return U2K("256 Kbyte");
		case 0x0ff:
			return U2K("1 MByte");
		case 0x3ff:
			return U2K("4 MByte");
		case 0xfff:
			return U2K("16 MByte");
		default:
			return U2K("Unknown page size");
	}
}

const char *entrylo2cachetext(uint32_t entrylo)
{
	switch((entrylo >> 3) & 0x7)
	{
		case 2:
			return U2K("Uncached");
		case 3:
			return U2K("Cacheable, write-back, write allocate");
		case 7:
			return U2K("Uncached Accelerated");
		default:
			return U2K("Reserved");
	}
}

/** Print all TLBs. */
void print_tlbs(void)
{
	int entry;
	uint32_t wired;

	/* Get wired. */
	__asm__ __volatile__(
		"sync.p\n"
		"mfc0 %0, $6\n":"=r" (wired):);

	kprintf("wired %d\n", wired);
	for (entry = 0; entry < 48; entry++) {
		uint32_t entryhi;
		uint32_t pagemask;
		uint32_t entrylo0;
		uint32_t entrylo1;
		uint32_t index;

		/* Set index. */
		__asm__ __volatile__ (
			".set push\n"
			".set noreorder\n"
			"mtc0 %0, $0\n"
			"sync.p\n"
			"nop\n"
			".set pop\n"
			::"r" (entry));

		/* Read tlb entry. */
		__asm__ __volatile__(
			".set push\n"
			".set noreorder\n"
			"nop\n"
			"nop\n"
			"nop\n"
			"nop\n"
			"tlbr\n"
			"sync.p\n"
			"nop\n"
			"nop\n"
			"nop\n"
			"nop\n"
			".set pop\n"
			:::"memory");
		/* Get index. */
		__asm__ __volatile__(
			"sync.p\n"
			"mfc0 %0, $0\n":"=r" (index)::"memory");
		/* Get entryhi. */
		__asm__ __volatile__(
			"sync.p\n"
			"mfc0 %0, $10\n":"=r" (entryhi)::"memory");
		/* Get page mask. */
		__asm__ __volatile__(
			"sync.p\n"
			"mfc0 %0, $5\n":"=r" (pagemask)::"memory");
		/* Get entrylo0. */
		__asm__ __volatile__(
			"sync.p\n"
			"mfc0 %0, $2\n":"=r" (entrylo0)::"memory");
		/* Get entrylo1. */
		__asm__ __volatile__(
			"sync.p\n"
			"mfc0 %0, $3\n":"=r" (entrylo1)::"memory");
		/* Barrier. */
		__asm__ __volatile__("sync.p"::: "memory");

		kprintf("index %d\n", index);
		kprintf("entryhi 0x%x vaddr 0x%x asid %d\n", entryhi, entryhi & 0xffffe000, entryhi & 0xFF);
		kprintf("pagemask 0x%x %s\n", pagemask, pagemask2text(pagemask));
		kprintf("entrylo0 0x%x paddr 0x%x %s, %s, %s, %s\n",
			entrylo0, (entrylo0 & 0x03ffffffc0) << 6,
			(entrylo0 & 2) ? U2K("valid") : U2K("invalid"),
			entrylo2cachetext(entrylo0),
			(entrylo0 & 4) ? U2K("dirty") : U2K("not dirty"),
			(entrylo0 & 1) ? U2K("global") : U2K("not global"));
		kprintf("entrylo1 0x%x paddr 0x%x %s, %s, %s, %s\n",
			entrylo1, (entrylo1 & 0x03ffffffc0) << 6,
			(entrylo1 & 2) ? U2K("valid") : U2K("invalid"),
			entrylo2cachetext(entrylo1),
			(entrylo1 & 4) ? U2K("dirty") : U2K("not dirty"),
			(entrylo1 & 1) ? U2K("global") : U2K("not global"));
		kprintf("\n");
	}
}

/** Flush all TLBs. User space cannot be accessed after calling this function. */
void flush_tlbs(void)
{
	int entry;

	/* Set wired to 0. */
	__asm__ __volatile__(
		"mtc0 %0, $6\n"
		"sync.p\n"::"r" (0));
	/* Set page mask to 4k. */
	__asm__ __volatile__(
		"mtc0 %0, $5\n"
		"sync.p\n"::"r" (0));
	/* Set entrylo0 to zero. */
	__asm__ __volatile__(
		"mtc0 %0, $2\n"
		"sync.p\n"::"r" (0));
	/* Set entrylo1 to zero. */
	__asm__ __volatile__(
		"mtc0 %0, $3\n"
		"sync.p\n"::"r" (0));
	/* Barrier. */
	__asm__ __volatile__("sync.p"::);
	for (entry = 0; entry < 48; entry++) {
		/* Set entryhi. */
		__asm__ __volatile__(
			"mtc0 %0, $10\n"
			"sync.p\n"::"r" (KSEG0 + entry * 0x2000));
		/* Set index. */
		__asm__ __volatile__(
			"mtc0 %0, $0\n"
			"sync.p\n"::"r" (entry));
		/* Barrier. */
		__asm__ __volatile__("sync.p"::);
		/* Write tlb entry. */
		__asm__ __volatile__("tlbwi"::);
		/* Barrier. */
		__asm__ __volatile__("sync.p"::);
	}
}

/** Print loaded modules on screen. */
void printAllModules(void)
{
	smod_mod_info_t *current;
	smod_mod_info_t module;
	char name[256];
	int i;

	current = NULL;
	i = 0;
	while (smod_get_next_mod(current, &module) != 0)
	{
		smem_read(module.name, name, 256);
		kprintf("modules id %d \"%s\" version 0x%02x\n", module.id, name, module.version);
		current = &module;
		i++;
	}
}

char magic_string[] __attribute__((aligned(4))) = "PS2b";
uint32_t *magic_check = (uint32_t *) magic_string;

uint32_t *getSBIOSCallTable(char *addr)
{
	void *code;
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

	kprintf("Entrypoint 0x%08x magic 0x%08x\n", *entrypoint, *magic);

	if (*magic != *magic_check) {
		error_printf("SBIOS file is incorrect (magic is wrong).");
		return NULL;
	}

	entrypoint = (uint32_t *) ((((uint32_t) *entrypoint) - SBIOS_START_ADDRESS) + ((uint32_t) addr));

	for (code = entrypoint; code < (void *)(entrypoint + NUMBER_OF_INSTRUCTIONS_CHECKED); code += 4) {
		uint32_t value;

		value = *((uint32_t *)code);
		//kprintf("Check code at 0x%08x 0x%08x op %d\n", code, value, value >> 26);
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
				kprintf("SBIOS size is 0x%08x.\n", sbiosSize);
				break;
			}
#endif
		}
	}
	kprintf("SBIOS Call table offset is at 0x%08x.\n", jumpBase);
	if (jumpBase != 0) {
		jumpBase = jumpBase - SBIOS_START_ADDRESS + ((uint32_t) addr);
	} else {
		error_printf("SBIOS call table not found.");
	}
	return (uint32_t *) jumpBase;
}

void disableTimers(void)
{
	volatile uint32_t *tm0_mode = (uint32_t *) 0xB0000010;
	volatile uint32_t *tm1_mode = (uint32_t *) 0xB0000810;
	volatile uint32_t *tm2_mode = (uint32_t *) 0xB0001010;
	volatile uint32_t *tm3_mode = (uint32_t *) 0xB0001810;

	*tm0_mode = 0;
	*tm1_mode = 0;
	*tm2_mode = 0;
	*tm3_mode = 0;
}

static void SetSysConf(struct ps2_sysconf *sysconf)
{
	ConfigParam osdconfig;
	GetOsdConfigParam(&osdconfig);
	sysconf->timezone = osdconfig.timezoneOffset;
	sysconf->aspect = osdconfig.screenType;
	sysconf->language = osdconfig.language;
	sysconf->spdif = osdconfig.spdifMode;
	sysconf->video = osdconfig.videoOutput;

	/* Check if it is an early Japan console. */
	if (osdconfig.region != 0) {
		Config2Param osdconfig2;

		/* Later models support osdconfig2. */
		GetOsdConfigParam2(&osdconfig2, 1, 1);
		sysconf->timenotation = osdconfig2.timeFormat;
		sysconf->datenotation = osdconfig2.dateFormat;
		sysconf->summertime = osdconfig2.daylightSaving;
	}
}

const char *load_sbios(struct ps2_bootinfo *bootinfo)
{
	const char *sbios_filename = NULL;
	int sbios_size = 0;
	const char *sbios = NULL;
	char *sbiosbuffer = NULL;
	static uint8_t sbiosbin[MAX_SBIOS_SIZE + SBIOS_RESERVED] __attribute__((aligned(4)));
	uint32_t *SBIOSCallTable = NULL;
	int found = 0;
	int i;

	memset(sbiosbin, 0, sizeof(sbiosbin));

	sbios_filename = getSBIOSFilename();
	if (strncmp(sbios_filename, "host:", 5) == 0) {
		const rom_entry_t *romfile;

		romfile = rom_getFile(&sbios_filename[5]);
		if (romfile != NULL) {
			sbios = romfile->start;
			sbios_size = romfile->size;
		}
	}
	if (sbios == NULL) {
		sbiosbuffer = load_file(sbios_filename, &sbios_size, NULL);
		sbios = sbiosbuffer;
	}
	if (sbios == NULL) {
		error_printf("Failed to load sbios.elf");
		return NULL;
	}

	graphic_setStatusMessage("Checking SBIOS...");

	/* Test if this is a ELF or a binary file.*/
	if ((sbios_size < 8) || (((const uint32_t *) sbios)[1] != *magic_check)) {
		int rv;
		uint32_t highest;

		/* SBIOS in ELF format. */

		/* Check for errors in ELF file and
		 * copy SBIOS into buffer sbiosbin.
		 */
		rv = check_sections("SBIOS", sbios, sbios_size,
			SBIOS_START_ADDRESS,
			SBIOS_START_ADDRESS + MAX_SBIOS_SIZE,
			&highest, sbiosbin);
		if (sbiosbuffer != NULL) {
			free(sbiosbuffer);
			sbiosbuffer = NULL;
		}
		if (rv != 0) {
			/* Error in ELF file for SBIOS. */
			return NULL;
		}
		sbios_size = highest - PHYSADDR(SBIOS_START_ADDRESS);
	} else {
		/* SBIOS in binary format. */
		if (sbios_size >= MAX_SBIOS_SIZE) {
			error_printf("SBIOS file is too large.");
			return NULL;
		}
		/* Copy SBIOS into buffer sbiosbin. */
		kmemcpy(sbiosbin, sbios, sbios_size);
		if (sbiosbuffer != NULL) {
			free(sbiosbuffer);
			sbiosbuffer = NULL;
		}
	}
	sbios = NULL;

	for (i = 1; i < (sbios_size / 4); i++) {
		if (((uint32_t *) sbiosbin)[i] == *magic_check) {
			found = 1;

			/* Found magic, store location in bootinfo structure. */
			bootinfo->sbios_base = SBIOS_START_ADDRESS + (i - 1) * 4;
			kprintf("SBIOS base 0x%08x\n", bootinfo->sbios_base);
			SBIOSCallTable = getSBIOSCallTable((char *) (&(((uint32_t *) sbiosbin)[i - 1])));
			if (SBIOSCallTable != NULL) {
				break;
			}
		}
	}
	if (SBIOSCallTable == NULL) {
		if (!found) {
			error_printf("SBIOS file is invalid, magic not found.");
		}
		return NULL;
	}

	graphic_setStatusMessage("Setup SBIOS...");
	kprintf("Using address 0x%08x as SBIOSCallTable (SBIOS is at 0x%08x).\n",
		(uint32_t) SBIOSCallTable, (uint32_t) sbiosbin);
	disableSBIOSCalls(SBIOSCallTable);
	graphic_setStatusMessage(NULL);

	/* Check if there is free memory for the console type. */
	if (strlen(ps2_console_type) >= SBIOS_RESERVED) {
		error_printf("PS2 console string is to long!");
		return NULL;
	}
	/* Add PS2 console type behind SBIOS. */
	strcpy(((void *) sbiosbin) + sbios_size, ps2_console_type);

	/* sbiosbin will be copied later to SBIOS_START_ADDRESS.
	 * Access data from kernel space, because this will be accessed by
	 * the Linux kernel. */
	bootinfo->ver_model = (void *) (SBIOS_START_ADDRESS + sbios_size);

	return sbiosbin;
}

/**
 * Load kernel, initrd and required modules. Then start kernel.
 * @param mode Graphic mode that should be used.
 */
static int real_loader(void)
{
	entry_t *entry;
	int ret;
	char *buffer = NULL;
	const char *sbios = NULL;
	struct ps2_bootinfo bootinfo;
	register int sp asm("sp");
	static char commandline[2 * MAX_INPUT_LEN] = "";
	uint32_t iopaddr;
	volatile uint32_t *sbios_iopaddr = (uint32_t *) 0x80001008;
#if 0
	volatile uint32_t *sbios_osdparam = (uint32_t *) 0x8000100c;
#endif
	int kernel_size = 0;
	const char *kernel_filename = NULL;
	uint32_t *initrd_header = NULL;
	uint32_t initrd_start = 0;
	uint32_t initrd_size = 0;

	/* Initialize memboot parameter. */
	memset(&bootinfo, 0, sizeof(struct ps2_bootinfo));

	strcpy(commandline, getKernelParameter());

	bootinfo.size = sizeof(struct ps2_bootinfo);
	if (IsT10K()) {
		bootinfo.mach_type = PS2_BOOTINFO_MACHTYPE_T10K;
		bootinfo.maxmem = 128 * 1024 * 1024;
	} else {
		bootinfo.mach_type = PS2_BOOTINFO_MACHTYPE_PS2;
		bootinfo.maxmem = 32 * 1024 * 1024 - 4096;
	}
	bootinfo.opt_string = (char *) (((unsigned int) commandline) | KSEG0_MASK); /* Command line parameters. */

	SetSysConf(&bootinfo.sysconf);

	kprintf("Started loader\n");

	kprintf("Load kernel......\n");
	kprintf("Stack 0x%08x\n", sp);
	if ((u32) sp < (u32) &_end) {
		/* This will lead to problems. */
		error_printf("Stack is unusable!");
		return -1;
	}
	sbios = load_sbios(&bootinfo);
	if (sbios == NULL) {
		return -35;
	}
	FlushCache(0);
	kernel_filename = getKernelFilename();
	if (strncmp(kernel_filename, "host:", 5) == 0) {
		const rom_entry_t *romfile;

		romfile = rom_getFile(&kernel_filename[5]);
		if (romfile != NULL) {
			buffer = memalign(64, romfile->size);
			if (buffer != NULL) {
				kmemcpy(buffer, romfile->start, romfile->size);
				kernel_size = romfile->size;
			} else {
				error_printf("Failed to allocate memory for kernel.");
				return -14;
			}
		}
	}
	if (buffer == NULL) {
		buffer = load_kernel_file(kernel_filename, &kernel_size);
	}
	if (buffer != NULL) {
		const char *initrd_filename;
		uint32_t lowestAddress;
		uint32_t highest;
		uint32_t base = 0;

		lowestAddress = (uint32_t) _ftext;
		if (lowestAddress > ((uint32_t) buffer)) {
			lowestAddress = (uint32_t) buffer;
		}

		/* Check for errors in ELF file. */
		graphic_setStatusMessage("Checking Kernel...");
		if (check_sections("kernel", buffer, kernel_size, 0x10000, lowestAddress, &highest, NULL) != 0) {
			free(buffer);
			buffer = NULL;
			return -2;
		}

		/* Check if loading of initrd is possible. */
		if ((highest < lowestAddress) && (highest >= NORMAL_MEMORY_START)) {
			base = ((highest + 4096 - 1) & ~(4096 - 1)) - 8;
			if (base > highest) {
				initrd_header = (uint32_t *) base;
			} else {
				initrd_header = NULL;
			}
		} else {
			initrd_header = NULL;
		}

		graphic_setStatusMessage(NULL);
		if (loadModules()) {
			free(buffer);
			buffer = NULL;
			return -2;
		}

#if 0
		if (crc32check("Memory corrupted by loading kernel.") != 0) {
			free(buffer);
			buffer = NULL;
			return -13;
		}
#endif

		initrd_filename = getInitRdFilename();
		if (initrd_filename != NULL) {
			if (initrd_header == NULL) {
				/* We don't want to overwrite end of kernel. */
				error_printf("Can't load initrd. End of kernel must not be within the last 8 bytes of a page. "
					"Kernel must be larger than %d Bytes.\n", NORMAL_MEMORY_START);
				free(buffer);
				buffer = NULL;
				return -12;
			}
			initrd_size = ((uint32_t) lowestAddress) - ((uint32_t) &initrd_header[2]);
			kprintf("%d bytes for initrd available.\n", initrd_size);
			initrd_start = ((unsigned int) load_file(initrd_filename, &initrd_size, &initrd_header[2]));
			if (initrd_start != 0) {
				if (initrd_size != 0) {
					if ((base + 8 + initrd_size) >= lowestAddress) {
						error_printf("Initrd is to big to move behind kernel %d <= %d bytes.\n",
							lowestAddress, (highest + 8 + initrd_size));
						free(buffer);
						buffer = NULL;
						return -11;
					}
					/* Magic number is used by kernel to detect initrd. */
					initrd_header[0] = 0x494E5244;
					initrd_header[1] = initrd_size;

					initrd_start |= KSEG0_MASK;

					/* Linux 2.6 has parameters for initrd. */
					snprintf(commandline, sizeof(commandline), "%s rd_start=0x%08x rd_size=0x%08x",
						getKernelParameter(), initrd_start, initrd_size);
				} else {
					error_printf("Loading of initrd failed (1).");
					free(buffer);
					buffer = NULL;
					return -9;
				}
			} else {
				error_printf("Loading of initrd failed (out of memory).");
				free(buffer);
				buffer = NULL;
				return -10;
			}
			kprintf("initrd_start 0x%08x 0x%08x\n", initrd_start, initrd_size);
		}

		/* Check integrity of kloader ELF. */
		if (crc32check("Memory corrupted by loading kernel, initrd or SBIOS.") != 0) {
			free(buffer);
			buffer = NULL;
			return -13;
		}

#if 0
		CDVD_Stop();
		CDVD_FlushCache();
#endif
		if (isDVDVSupported()) {
			graphic_setStatusMessage("Stopping DVD");

			/* Stop CD/DVD. */
			CDVD_Stop();
			CDVD_FlushCache();

			CDDA_Exit();
		}

		if (debug_mode == -1) {
			kprintf("Try to reboot IOP.\n");
			graphic_setStatusMessage("Reseting IOP");
		}

		PS2KbdClose();
		deinitializeController();

		FlushCache(0);

		if (debug_mode == -1) {
			graphic_setStatusMessage("Exit IOP Heap");
			SifExitIopHeap();
			graphic_setStatusMessage("Exit LoadFile");
			SifLoadFileExit();
			graphic_setStatusMessage("Exit FIO");
			fioExit();
			graphic_setStatusMessage("Exit RPC");
			SifExitRpc();
			graphic_setStatusMessage("Stop DMA");
			SifStopDma();
			graphic_setStatusMessage("PreReset Init RPC");
			SifInitRpc(0);

			graphic_setStatusMessage("CDVD_INIT_NOCHECK\n");
			cdInit(CDVD_INIT_NOCHECK);
			graphic_setStatusMessage("CDVD_INIT_EXIT\n");
			cdInit(CDVD_INIT_EXIT);

			graphic_setStatusMessage("Reseting IOP");
			while (!SifIopReset(iop_reset_param, 0));

			graphic_setStatusMessage("IOP Sync");
			while (!SifIopSync());

			graphic_setStatusMessage("Initialize RPC");
			SifInitRpc(0);

			graphic_setStatusMessage("Patching enable LMB");
			sbv_patch_enable_lmb();
			graphic_setStatusMessage("Patching disable prefix check");
			sbv_patch_disable_prefix_check();
		}

		graphic_setStatusMessage("Adding eedebug handler");

		addEEDebugHandler();

		graphic_setStatusMessage("Starting modules");
		//kprintf("Starting modules\n");

		startModules(&bootinfo);

		graphic_setStatusMessage("Started all modules");

		iop_init_shared();

#ifdef USER_SPACE_SUPPORT
		iop_prints("User mode sharedmem test\n");
#endif
		printAllModules();

		graphic_setStatusMessage("Setup iopaddr");

		iopaddr = SifGetReg(0x80000000);

		graphic_setStatusMessage("Buffer check");

		if (!isInfoBufferEmpty() || (getErrorMessage() != NULL)) {
			/* Print queued eedebug messages. (Anything printed by IOP). */
			waitForUser();
		}

		graphic_setStatusMessage("Stop RPC");

		SifExitIopHeap();
		SifLoadFileExit();
		SifExitRpc();
		SifStopDma();

		graphic_setStatusMessage("Copying files and start...");

		disable_interrupts();

		ee_kmode_enter();
#ifdef USER_SPACE_SUPPORT
		iop_kmode_enter();
#endif
		/* Access data from kernel space, because TLB misses can't be handled here. */
		buffer = (char *) (((uint32_t) buffer) | KSEG0_MASK);

		/* Access data from kernel space, because TLB misses can't be handled here. */
		sbios = U2K(sbios);


		/* Be sure that all interrupts are disabled. */
		__asm__ __volatile__(
			"mtc0 %0, $12\n"
			"sync.p\n"::"r" (
			(0<<30) /* Deactivate COP2: VPU0 -> not used by loader */
			| (1<<29) /* Activate COP1: FPU -> used by compiler and functions like printf. */
			| (1<<28) /* Activate COP0: EE Core system control processor. */
			| (1<<16) /* eie */));

		/* Disable performance counters. */
		SET_PCCR(0);

		/* Disable all timers. */
		disableTimers();

		/* Flush cache to be sure that jump2kernelspace() will work. */
		flushDCacheAll();
		invalidateICacheAll();

		/* Need to change current address to kernel space address,
		 * because page handler will not work, when PS2 kernel is
		 * overwritten.
		 * TLB commmands need to be executed from KSEG address.
		 */
		jump2kernelspace(KSEG0_MASK);

		/* Be sure that local variables are not lost after changing stack pointer. */
		flushDCacheAll();

		kputs("Kernel mode print\n");
		kprintf("Stack 0x%08x\n", sp);

		if (isSlimPSTwo()) {
			if (loaderConfig.enableDev9 && hasNetworkSupport()) {
				/* Use value 0x0200 to inform Linux about slim PSTwo. This was not part of Sony's Linux. */
				bootinfo.pccard_type = 0x0200;
			}
		} else {
			if (loaderConfig.enableDev9 && hasNetworkSupport()) {
				/* DEV9 can be only used by Linux, when PS2LINK is not loaded. */
				if (ps2dev9_init() == 0) {
					const char *pcicType;
	
					/* Activate hard disc. */
					ata_setup();
	
					/* Tell Linux to activate HDD and Network. */
					bootinfo.pccard_type = 0x0100;
					pcicType = getPcicType();
					if (strlen(pcicType) > 0) {
						/* User configured calue in menu. */
						bootinfo.pcic_type = atoi(pcicType);
					} else {
						/* Auto detect type. */
						bootinfo.pcic_type = pcic_get_cardtype();
					}
				}
			}
		}

		/* Setup exceptions: */
		/* TLB Refill */
		installExceptionHandler(0);
		/* Performance Counter */
		installExceptionHandler(1);
		/* Debug */
		installExceptionHandler(2);
		/* All other exceptions */
		installExceptionHandler(3);
		/* Interrupt */
		installExceptionHandler(4);

		/* Flush cache to activate exception handlers.
		 * This is also required, because we remove all TLBs and writing back cache
		 * entries can fail when nothing is mapped.
		 */
		flushDCacheAll();
		invalidateICacheAll();

		/* Set status register. */
		__asm__ __volatile__(
			"mtc0 %0, $12\n"
			"sync.p\n"::"r" (
			(0<<30) /* Deactivate COP2: VPU0 -> not used by loader */
			| (1<<29) /* Activate COP1: FPU -> used by compiler and functions like printf. */
			| (1<<28) /* Activate COP0: EE Core system control processor. */
			| (1<<16) /* eie */));

		/* Set config register. */
		__asm__ __volatile__(
			"mtc0 %0, $16\n"
			"sync.p\n"::"r" ((3<<0) /* Kseg0 cache */
						| (1<<12) /* Branch prediction */
						| (1<<13) /* Non blocking loads */
						| (1<<16) /* Data cache enable */
						| (1<<17) /* Instruction cache enable */
						| (1<<18) /* Enable pipeline */));

		/* Disable all breakpoints. */
		__asm__ __volatile__(
			"mtc0 %0, $24\n"
			"sync.p\n"::"r" (0));

		/* Disable performance counter. */
		__asm__ __volatile__("mtps %0, 0\nsync.p\n"::"r" (0));

		__asm__ __volatile__(
			"mtc0 %0, $28\n"
			"sync.p\n"::"r" (0x011c1020));
		__asm__ __volatile__(
			"mtc0 %0, $29\n"
			"sync.p\n"::"r" (0x40004190));

		/* Disable all INTC interrupts. */
		*((volatile uint32_t *) I_MASK) = *((volatile uint32_t *) I_MASK);
		BARRIER();
		/* Acknowledge all interrupts. */
		*((volatile uint32_t *) I_STAT) = *((volatile uint32_t *) I_STAT);
		BARRIER();
#if 0
		/* Suspend all DMA channels. */
		*((volatile unsigned int *) D_ENABLEW) = 1 << 16;
		BARRIER();

		/* Disable each DMA channel. */
		*((volatile unsigned int *) D0_CHCR) = 0;
		BARRIER();
		*((volatile unsigned int *) D1_CHCR) = 0;
		BARRIER();
		*((volatile unsigned int *) D2_CHCR) = 0;
		BARRIER();
		*((volatile unsigned int *) D3_CHCR) = 0;
		BARRIER();
		*((volatile unsigned int *) D4_CHCR) = 0;
		BARRIER();
		*((volatile unsigned int *) D5_CHCR) = 0;
		BARRIER();
		*((volatile unsigned int *) D6_CHCR) = 0;
		BARRIER();
		*((volatile unsigned int *) D7_CHCR) = 0;
		BARRIER();
		*((volatile unsigned int *) D8_CHCR) = 0;
		BARRIER();
		*((volatile unsigned int *) D9_CHCR) = 0;
		BARRIER();

		/* Disable all DMA channels. */
		*((volatile unsigned int *) D_CTRL) = 0;
		BARRIER();
#endif
#if 0
		/* XXX: Crash to test exception handlers. */
		kputs("Try to crash\n");
		*((volatile unsigned int *) 0x40000000) = 0;
#endif
		/* Install SBIOS. */
		/* Copy binary data. */
		kmemcpy((void *) SBIOS_START_ADDRESS, sbios, MAX_SBIOS_SIZE + SBIOS_RESERVED);

		/* Install linux kernel. */
		kputs("Copy kernel\n");
		entry = copy_sections(buffer);

		verify_sections(buffer);
		kputs("Code verified\n");

		/* Set iopaddr for SifRPC. */
		*sbios_iopaddr = iopaddr;
#if 0
		/* XXX: Need to check parameter. */
		*sbios_osdparam = 0x0107ddc8;
#endif
		kprintf("Patched sbios_iopaddr 0x%08x\n", iopaddr);

		kmemcpy((void *) PS2_BOOTINFO_OLDADDR, &bootinfo, sizeof(bootinfo));

		if (entry != NULL) {
			/* Activate SBIOS and linux kernel. */
			flushDCacheAll();
			invalidateICacheAll();

			//print_tlbs();
#if 1
			/* XXX: not done by original loader, only for testing. */
			/* Flush all tlb entries. */
			kputs("Flush TLBs (printf will not work after this).\n");
			/* printf is not working, because it uses a function pointer, which lead to a jump into kernelspace. */
			flush_tlbs();
			kputs("TLBs flushed.\n");
#endif
#if 0
			/* XXX: Crash to test exception handlers. */
			kputs("Try to crash\n");
			*((volatile unsigned int *) 0x100000) = 0;
#endif

			kputs("Jump to kernel!\n");
			ret = entry(0 /* unused */, NULL /* unused */, (char **) PS2_BOOTINFO_OLDADDR, NULL /* unused */);
			kputs("Back from kernel!\n");
			panic();
		} else {
			kputs("ELF not loaded.\n");
		}
	} else {
		error_printf("Failed to load kernel.");
		return -1;
	}
	kputs("End reached?\n");
	panic();

	return -2;
}

static inline void nopdelay1ms(void)
{
	/* (300 MHz CPU / 1ms) / 7 CPU cycles per loop (4 * nop + assumed loop overhead), so we will wait for 1ms. */
	int i = (300000000 / 1000) / 7;

	do {
		__asm__ ("nop\nnop\nnop\nnop\nnop\n");
	} while (i-- != -1);
}

void DelayThread(int delay)
{
	int i;

	for (i = 0; i < delay; i++) {
		nopdelay1ms();
	}
}

int loader(void *arg)
{
	DiskType type;
	int rv;

	arg = arg;

	/* Check for kloader integrity. */
	if (crc32check("Memory corupted before loading kernel.") != 0) {
		return -1;
	}

	if (isDVDVSupported()) {
		type = CDDA_DiskType();
	
		/* Detect disk type, so loading will work. */
		if (type == DiskType_DVDV) {
			CDVD_SetDVDV(1);
		} else {
			CDVD_SetDVDV(0);
		}
	}

	rv = real_loader();

	if (isDVDVSupported()) {
		/* Always stop CD/DVD when an error happened. */
		CDVD_Stop();
		CDVD_FlushCache();
	}

	return rv;
}


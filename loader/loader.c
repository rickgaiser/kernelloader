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
#include "libkbd.h"
#include "SMS_CDVD.h"
#include "SMS_CDDA.h"

#include "cache.h"
#include "elf.h"
#include "bootinfo.h"
#include "iopmem.h"
#include "jump.h"
#include "entry.h"
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
#include "kernelgraphic.h"
#include "zlib.h"
#include "configuration.h"
#include "ps2dev9.h"
#include "hdd.h"
#include "modules.h"

#define SET_PCCR(val) \
	__asm__ __volatile__("mtc0 %0, $25"::"r" (val))

#define I_MASK 0x1000F010
#define D_CTRL 0x1000E000
#define D_ENABLEW 0x1000F590
#define D0_CHCR  0x10008000
#define D1_CHCR  0x10009000
#define D2_CHCR  0x1000A000
#define D3_CHCR  0x1000B000
#define D4_CHCR  0x1000B400
#define D5_CHCR  0x1000C000
#define D6_CHCR  0x1000C400
#define D7_CHCR  0x1000C800
#define D8_CHCR  0x1000D000
#define D9_CHCR  0x1000D400
#define BARRIER() \
	/* Barrier. */ \
	__asm__ __volatile__("sync.p"::);

#if 0
/** Debug print. */
#define dprintf printf
/** Debug print on IOP. */
#define iop_dprintf iop_printf
/** Debug print on IOP. */
#define iop_dprints iop_prints
#else
/** Debug print. */
#define dprintf(args...)
/** Debug print on IOP. */
#define iop_dprintf(args...)
/** Debug print on IOP. */
#define iop_dprints(args...)

#endif

/** Number of instructions checked to find SBIOS call table. */
#define NUMBER_OF_INSTRUCTIONS_CHECKED 128
/** Base address for SBIOS. */
#define SBIOS_START_ADDRESS 0x80001000
/** Normal usable memroy starts here.*/
#define NORMAL_MEMORY_START 0x80000

/** Definition of kernel entry function. */
typedef int (entry_t)(int argc, char **argv, char **envp, int *prom_vec);

/** Parameter for IOP reset. */
static char s_pUDNL   [] __attribute__(   (  section( ".data" ), aligned( 1 )  )   ) = "rom0:UDNL rom0:EELOADCNF";

/** Modules that should be loaded. */
moduleEntry_t modules[] = {
	{
		.path = "host:eedebug.irx",
		.buffered = -1,
		.argLen = 0,
		.args = NULL,
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
	},
	{
		.path = "host:eromdrvloader.irx",
		.buffered = -1,
		.argLen = 0,
		.args = NULL,
		.defaultmod = 1,
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
	},
	{
		.path = "rom0:XSIO2MAN",
		.buffered = 0,
		.argLen = 0,
		.args = NULL,
		.defaultmod = 1,
		.slim = 1 /* XXX: New modules seems to be more stable on heavy USB usage. */
	},
	{
		.path = "host:freesio2.irx",
		.buffered = -1,
		.argLen = 0,
		.args = NULL,
	},
	{
		.path = "rom0:SIO2MAN",
		.buffered = 0,
		.argLen = 0,
		.args = NULL,
		.defaultmod = 1,
		.slim = -1 /* XXX: SBIOS causes system hang on heavy USB usage. */
	},
	{
		.path = "rom1:SIO2MAN",
		.buffered = 0,
		.argLen = 0,
		.args = NULL,
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
	},
	{
		.path = "rom0:XMCMAN",
		.buffered = 0,
		.argLen = 0,
		.args = NULL,
		.defaultmod = 1,
		.slim = 1 /* XXX: New modules seems to be more stable on heavy USB usage. */
	},
	{
		.path = "rom0:MCMAN",
		.buffered = 0,
		.argLen = 0,
		.args = NULL,
		.defaultmod = 1,
		.slim = -1 /* XXX: SBIOS causes system hang on heavy USB usage. */
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
	},
	{
		.path = "rom0:XMCSERV",
		.buffered = 0,
		.argLen = 0,
		.args = NULL,
		.defaultmod = 1,
		.slim = 1 /* XXX: New modules seems to be more stable on heavy USB usage. */
	},
	{
		.path = "rom0:MCSERV",
		.buffered = 0,
		.argLen = 0,
		.args = NULL,
		.defaultmod = 1,
		.slim = -1 /* XXX: SBIOS causes system hang on heavy USB usage. */
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
	},
	{
		.path = "rom0:XPADMAN",
		.buffered = 0,
		.argLen = 0,
		.args = NULL,
	},
	{
		.path = "host:freepad.irx",
		.buffered = -1,
		.argLen = 0,
		.args = NULL,
	},
	{
		.path = "rom0:PADMAN",
		.buffered = 0,
		.argLen = 0,
		.args = NULL,
		.defaultmod = 1,
		.slim = -1 /* XXX: SBIOS causes system hang on heavy USB usage. */
	},
	{
		.path = "rom1:PADMAN",
		.buffered = 0,
		.argLen = 0,
		.args = NULL,
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
	},
	{
		.path = "host:freesd.irx",
		.buffered = -1,
		.argLen = 0,
		.args = NULL,
		.libsd = -1,
	},
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
		.path = "rom1:SDRDRV",
		.buffered = 0,
		.argLen = 0,
		.args = NULL,
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
	},
	{
		.path = "host:iomanX.irx",
		.buffered = -1,
		.argLen = 0,
		.args = NULL,
		.defaultmod = 1,
		.slim = 1,
	},
	{
		.path = "host:poweroff.irx",
		.buffered = -1,
		.argLen = 0,
		.args = NULL,
		.defaultmod = 1,
		.slim = 1,
	},
	{
		.path = "host:ps2dev9.irx",
		.buffered = -1,
		.argLen = 0,
		.args = NULL,
		.defaultmod = 1,
		.slim = 1,
	},
	{
		.path = "host:ps2ip.irx",
		.buffered = -1,
		.argLen = 0,
		.args = NULL,
	},
	{
		.path = "host:ps2smap.irx",
		.buffered = -1,
		.argLen = 0,
		.args = NULL,
		.ps2smap = 1
	},
 	{
		.path = "host:smaprpc.irx",
		.buffered = -1,
		.argLen = 0,
		.args = NULL,
		.defaultmod = 1,
		.slim = 1,
	},
	{
		.path = "host:ps2link.irx",
		.buffered = -1,
		.argLen = 0,
		.args = NULL,
	},
#ifdef NAPLINK
	{
		.path = "host:npm-usbd.irx",
		.buffered = -1,
		.argLen = 0,
		.args = NULL,
	},
	{
		.path = "host:npm-2301.irx",
		.buffered = -1,
		.argLen = 0,
		.args = NULL,
	},
#endif
#if 0 /* Debug module is unstable. */
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
#endif
	{
#ifdef RTE
		.path = "host:RTE/iopintr.irx",
#else
		.path = CONFIG_DIR "/iopintr.irx",
#endif
		.buffered = -1,
		.argLen = 0,
		.args = NULL,
	},
	{
		/* Interrupt relay when DEV9 is not loaded. */
		.path = "host:TGE/intrelay-direct.irx",
		.buffered = -1,
		.argLen = 0,
		.args = NULL,
		.defaultmod = 1,
		.slim = -1
	},
	{
		/* Interrupt relay when DEV9 is not loaded and slim PSTwo. */
		.path = "host:TGE/intrelay-direct-rpc.irx",
		.buffered = -1,
		.argLen = 0,
		.args = NULL,
	},
	{
		/* Interrupt relay when DEV9 is loaded. */
		.path = "host:TGE/intrelay-dev9.irx",
		.buffered = -1,
		.argLen = 0,
		.args = NULL,
		/* Only hard disc and USB is working. */
	},
	{
		/* Interrupt relay when DEV9 is loaded and slim PSTwo. */
		.path = "host:TGE/intrelay-dev9-rpc.irx",
		.buffered = -1,
		.argLen = 0,
		.args = NULL,
		.defaultmod = 1,
		.slim = 1
		/* Only hard disc and USB is working. */
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
	},
	{
#ifdef RTE
		.path = "host:RTE/cdvdman.irx",
#else
		.path = CONFIG_DIR "/cdvdman.irx",
#endif
		.buffered = -1,
		.argLen = 0,
		.args = NULL,
	},
	{
		.path = "rom0:XCDVDMAN",
		.buffered = 0,
		.argLen = 0,
		.args = NULL,
		.defaultmod = 1,
		.slim = 1 /* XXX: New modules seems to be more stable on heavy USB usage. */
	},
	{
		.path = "rom0:CDVDMAN",
		.buffered = 0,
		.argLen = 0,
		.args = NULL,
		.defaultmod = 1,
		.slim = -1 /* XXX: SBIOS causes system hang on heavy USB usage. */
	},
	{
		.path = "rom1:CDVDMAN",
		.buffered = 0,
		.argLen = 0,
		.args = NULL,
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
	},
	{
		.path = "rom0:XCDVDFSV",
		.buffered = 0,
		.argLen = 0,
		.args = NULL,
	},
	{
		.path = "rom0:CDVDFSV",
		.buffered = 0,
		.argLen = 0,
		.args = NULL,
	},
	{
		.path = "rom1:CDVDFSV",
		.buffered = 0,
		.argLen = 0,
		.args = NULL,
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
/** End address of this loader. */
void _end(void);

/** Called if system is in an unrecoverable state. */
void panic()
{
	/* Nothing can be done here. */
	while(1);
}

/** Patch libsd, so that sound and USB is working at the same time. */
void patchLibsd(uint8_t *buffer, uint32_t size)
{
	uint32_t i;

	for (i = 0; i < (size - sizeof(libsdOrig)); i++) {
		if (memcmp(buffer + i, libsdOrig, sizeof(libsdOrig)) == 0) {
			memcpy(buffer + i, libsdPatched, sizeof(libsdPatched));
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
int check_sections(const char *name, char *buffer, uint32_t filesize, uint32_t start, uint32_t end, uint32_t *highest)
{
	Elf32_Ehdr_t *file_header;
	int pos = 0;
	int i;
	uint32_t entry = 0;
	uint32_t area;

	start = start & 0x0FFFFFFF;
	end = end & 0x0FFFFFFF;
	if (highest != NULL) {
		*highest = 0;
	}

	file_header = (Elf32_Ehdr_t *) &buffer[pos];
	pos += sizeof(Elf32_Ehdr_t);
	if (file_header->magic != ELFMAGIC) {
		error_printf("Magic 0x%08x is wrong.\n", file_header->magic);
		return -1;
	}
	entry = file_header->entry;
	printf("entry is 0x%08x\n", entry);
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
	entry = entry & 0x0FFFFFFF;
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

			printf("VAddr: 0x%08x PAddr: 0x%08x Offset 0x%08x Size 0x%08x\n",
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
					char *startaddr;
					char *endaddr;
					/* Check if file is completly loaded. */
					startaddr = &buffer[program_header->offset];
					endaddr = startaddr + program_header->filesz;

					if ((startaddr < buffer) || (startaddr >= &buffer[filesize])) {
						error_printf("The %s file must be at least %d Bytes. Please redownload this file.", name, startaddr - buffer);
						return -30;
					}
					if ((endaddr < buffer) || (endaddr >= &buffer[filesize])) {
						error_printf("The %s file must be at least %d Bytes. Please redownload this file.", name, endaddr - buffer);
						return -31;
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
				dest = dest & 0x0FFFFFFF;

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
			}
		}
	}
	return 0;
}

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

	file_header = (Elf32_Ehdr_t *) &buffer[pos];
	pos += sizeof(Elf32_Ehdr_t);
	if (file_header->magic != ELFMAGIC) {
		iop_printf(U2K("Magic 0x%08x is wrong.\n"), file_header->magic);
		return NULL;
	}
	entry = (entry_t *) file_header->entry;
	iop_dprintf(U2K("entry is 0x%08x\n"), (int) entry);
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
				iop_dprintf(U2K("VAddr: 0x%08x PAddr: 0x%08x Offset 0x%08x Size 0x%08x\n"),
					program_header->vaddr,
					program_header->paddr,
					program_header->offset,
					program_header->filesz);
				memcpy(dest, &buffer[program_header->offset], program_header->filesz);
				iop_dprintf(U2K("First bytes 0x%02x 0x%02x\n"), (int) dest[0], (int) dest[1]);
			}
			int size = program_header->memsz - program_header->filesz;
			if (size > 0)
				memset(&dest[program_header->filesz], 0, size);
		}
	}
	return entry;
}

/**
 * Verify if program sections of ELF are copied correctly to memory.
 * Required to check if kernel can be started without problems.
 * @param buffer Pointer to ELF file.
 */
void verify_sections(char *buffer)
{
	Elf32_Ehdr_t *file_header;
	int pos = 0;
	int i;
	entry_t *entry = NULL;

	file_header = (Elf32_Ehdr_t *) &buffer[pos];
	pos += sizeof(Elf32_Ehdr_t);
	if (file_header->magic != ELFMAGIC) {
		iop_printf(U2K("Magic 0x%08x is wrong.\n"), file_header->magic);
		panic();
	}
	entry = (entry_t *) file_header->entry;
	iop_dprintf(U2K("entry is 0x%08x\n"), (int) entry);
	for (i = 0; i < file_header->phnum; i++)
	{
		Elf32_Phdr_t *program_header;
		program_header = (Elf32_Phdr_t *) &buffer[pos];
		pos += sizeof(Elf32_Phdr_t);
		if ( (program_header->type == PT_LOAD)
			&& (program_header->memsz != 0) )
		{
			unsigned char *dest;

			// Copied to physical address which can be accessed by loader.
			dest = (unsigned char *) (program_header->paddr);

			if (program_header->filesz != 0)
			{
				iop_dprintf(U2K("VAddr: 0x%08x PAddr: 0x%08x Offset 0x%08x Size 0x%08x\n"),
					program_header->vaddr,
					program_header->paddr,
					program_header->offset,
					program_header->filesz);
				if (memcmp(dest, &buffer[program_header->offset], program_header->filesz) != 0) {
					iop_prints(U2K("Verify failed"));
					panic();
				};
				iop_dprintf(U2K("First bytes 0x%02x 0x%02x\n"), (int) dest[0], (int) dest[1]);
			}
			unsigned int size = program_header->memsz - program_header->filesz;
			if (size > 0) {
				unsigned int i;

				for (i = 0; i < size; i++) {
					if (dest[program_header->filesz] != 0) {
						iop_prints(U2K("Verify failed in memset"));
						panic();
					}
				}
			}
		}
	}
	return;
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
	printf("%s size %d\n", filename, *size);
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
char *load_kernel_file(const char *filename, int *size, void *addr)
{
	char *buffer = NULL;
	int s;
	gzFile fin;
	int maxsize = 6 * 1024 * 1024;

	if (size == NULL) {
		size = &s;
		addr = NULL;
	}

	setEnableDisc(-1);
	graphic_setPercentage(0, filename);
	fin = gzopen(filename, "rb");
	if (fin == NULL) {
		error_printf("Error cannot open file %s.", filename);
		setEnableDisc(0);
		return NULL;
	}

	// memalign() begins at &_end behind elf file. This ensures that the first 5 MByte are not used.
	// Don't know the size just use 7 MB.
	buffer = (char *) memalign(64, maxsize);

	dprintf("buffer 0x%08x\n", (unsigned int) buffer);
	if (buffer == NULL) {
		gzclose(fin);
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
	int next = 10 * 1024;
	while ((n = gzread(fin, &buffer[pos], next)) > 0) {
		pos += n;
		s -= n;
		if (s < next)
			next = s;
		if (next <= 0)
			break;
		dprintf(".");
		graphic_setPercentage(pos / (maxsize / 100), filename);
		if ((pos + next) > maxsize) {
			error_printf("Error file %s is too large (> 8MB).", filename);
			setEnableDisc(0);
			gzclose(fin);
			free(buffer);
			return NULL;
		}
	}
	*size = pos;
	printf("%s size %d\n", filename, *size);
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
		if ((modules[i].buffered) && (modules[i].load)) {
			rom_entry_t *romfile = NULL;

			dprintf("Loading module %s.\n", modules[i].path);
			if (strncmp(modules[i].path, "host:", 5) == 0) {
				printf("Try to load %s\n", &modules[i].path[5]);
				romfile = rom_getFile(&modules[i].path[5]);
			}
			if (romfile == NULL) {
				modules[i].buffer = load_file(modules[i].path, &modules[i].size, NULL);
				if (modules[i].buffer == NULL) {
					error_printf("Failed to load module '%s'.", modules[i].path);

					/* Free memory. */
					for (j = 0; j < i; j++) {
						if ((modules[j].buffered) && (modules[j].load)) {
							if (modules[j].allocated) {
								free(modules[i].buffer);
								modules[j].allocated = 0;
							}
						}
					}
					return -1;
				} else {
					modules[i].allocated = 1;
					if (modules[i].libsd && loaderConfig.patchLibsd) {
						patchLibsd(modules[i].buffer, modules[i].size);
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
			if (modules[i].ps2smap) {
				modules[i].args = getPS2MAPParameter(&modules[i].argLen);
			}
			if (modules[i].buffered) {
				int ret;

				ret = SifExecModuleBuffer(modules[i].buffer, modules[i].size, modules[i].argLen, modules[i].args, &rv);
				if (ret < 0) {
					rv = ret;
				} else {
					if (modules[i].dev9init) {
						if (rv < 0) {
							/* Tell Linux that DEV9 is disabled. */
							bootinfo->pccard_type = 0;
						} else {
							if (loaderConfig.enableDev9) {
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
				error_printf("Failed to start module \"%s\" (rv = %d).", modules[i].path, rv);
			}
		}
	}
}

/**
 * Install an exception handler
 * @param number The number of the exception handler to install.
 */
void installExceptionHandler(int number)
{
	void *dstAddr;
	void *srcAddr;
	uint32_t *patch;
	uint32_t size;

	if (number > 4) {
		iop_prints(U2K("Exception number is too big.\n"));
		panic();
	}
	dstAddr = (void *) (KSEG0_MASK + 0x80 * number);
	srcAddr = (void *) (((uint32_t) exceptionHandlerStart) | KSEG0_MASK);
	size = ((uint32_t) exceptionHandlerEnd) - ((uint32_t) exceptionHandlerStart);

	patch = (uint32_t *) exceptionHandlerPatch2;
	iop_dprintf(U2K("exceptionHandlerPatch2 = 0x%08x\n"), *patch);
	*patch = ((*patch) & 0xFFFF0000) | number;
	iop_dprintf(U2K("exceptionHandlerPatch2 = 0x%08x\n"), *patch);

	memcpy(dstAddr, srcAddr, size);
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
	__asm__ __volatile__("mfc0 %0, $6":"=r" (wired):);

	iop_printf(U2K("wired %d\n"), wired);
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
		__asm__ __volatile__("mfc0 %0, $0":"=r" (index)::"memory");
		/* Get entryhi. */
		__asm__ __volatile__("mfc0 %0, $10":"=r" (entryhi)::"memory");
		/* Get page mask. */
		__asm__ __volatile__("mfc0 %0, $5":"=r" (pagemask)::"memory");
		/* Get entrylo0. */
		__asm__ __volatile__("mfc0 %0, $2":"=r" (entrylo0)::"memory");
		/* Get entrylo1. */
		__asm__ __volatile__("mfc0 %0, $3":"=r" (entrylo1)::"memory");
		/* Barrier. */
		__asm__ __volatile__("sync.p"::: "memory");

		iop_printf(U2K("index %d\n"), index);
		iop_printf(U2K("entryhi 0x%x vaddr 0x%x asid %d\n"), entryhi, entryhi & 0xffffe000, entryhi & 0xFF);
		iop_printf(U2K("pagemask 0x%x %s\n"), pagemask, pagemask2text(pagemask));
		iop_printf(U2K("entrylo0 0x%x paddr 0x%x %s, %s, %s, %s\n"),
			entrylo0, (entrylo0 & 0x03ffffffc0) << 6,
			(entrylo0 & 2) ? U2K("valid") : U2K("invalid"),
			entrylo2cachetext(entrylo0),
			(entrylo0 & 4) ? U2K("dirty") : U2K("not dirty"),
			(entrylo0 & 1) ? U2K("global") : U2K("not global"));
		iop_printf(U2K("entrylo1 0x%x paddr 0x%x %s, %s, %s, %s\n"),
			entrylo1, (entrylo1 & 0x03ffffffc0) << 6,
			(entrylo1 & 2) ? U2K("valid") : U2K("invalid"),
			entrylo2cachetext(entrylo1),
			(entrylo1 & 4) ? U2K("dirty") : U2K("not dirty"),
			(entrylo1 & 1) ? U2K("global") : U2K("not global"));
		iop_printf(U2K("\n"));
	}
}

/** Flush all TLBs. User space cannot be accessed after calling this function. */
void flush_tlbs(void)
{
	int entry;

	/* Set wired to 0. */
	__asm__ __volatile__("mtc0 %0, $6"::"r" (0));
	/* Set page mask to 4k. */
	__asm__ __volatile__("mtc0 %0, $5"::"r" (0));
	/* Set entrylo0 to zero. */
	__asm__ __volatile__("mtc0 %0, $2"::"r" (0));
	/* Set entrylo1 to zero. */
	__asm__ __volatile__("mtc0 %0, $3"::"r" (0));
	/* Barrier. */
	__asm__ __volatile__("sync.p"::);
	for (entry = 0; entry < 48; entry++) {
		/* Set entryhi. */
		__asm__ __volatile__("mtc0 %0, $10"::"r" (KSEG0 + entry * 0x2000));
		/* Set index. */
		__asm__ __volatile__("mtc0 %0, $0"::"r" (entry));
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
		printf("modules id %d \"%s\" version 0x%02x\n", module.id, name, module.version);
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

	printf("Entrypoint 0x%08x magic 0x%08x\n", *entrypoint, *magic);

	if (*magic != *magic_check) {
		error_printf("SBIOS file is incorrect (magic is wrong).");
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
	} else {
		error_printf("SBIOS call table not found.");
	}
	return (uint32_t *) jumpBase;
}

/**
 * Load kernel, initrd and required modules. Then start kernel.
 * @param mode Graphic mode that should be used.
 */
int real_loader(void)
{
	entry_t *entry;
	int ret;
	char *buffer = NULL;
	char *sbios = NULL;
	struct ps2_bootinfo *bootinfo = (struct ps2_bootinfo *) PS2_BOOTINFO_OLDADDR;
	register int sp asm("sp");
	const char *commandline;
	uint32_t *patch;
	uint32_t iopaddr;
	volatile uint32_t *sbios_iopaddr = (uint32_t *) 0x80001008;
#if 0
	volatile uint32_t *sbios_osdparam = (uint32_t *) 0x8000100c;
#endif
	int sbios_size = 0;
	int kernel_size = 0;
	const char *sbios_filename = NULL;
	const char *kernel_filename = NULL;
	uint32_t *initrd_header = NULL;
	uint32_t initrd_start = 0;
	uint32_t initrd_size = 0;
	const char *gmode = NULL;

	commandline = getKernelParameter();

	/* Initialize memboot parameter. */
	memset(bootinfo, 0, sizeof(struct ps2_bootinfo));

	bootinfo->size = sizeof(struct ps2_bootinfo);
	bootinfo->maxmem = 32 * 1024 * 1024 - 4096;
	bootinfo->mach_type = PS2_BOOTINFO_MACHTYPE_PS2;
	bootinfo->opt_string = (char *) (((unsigned int) commandline) | KSEG0_MASK); /* Command line parameters. */

	printf("Started loader\n");

	patch = (uint32_t *) exceptionHandlerPatch1;
	dprintf("exceptionHandlerPatch = 0x%08x\n", *patch);
	*patch = ((*patch) & 0xFFFF0000) | (((((uint32_t) exceptionHandleStage2) & 0xFFFF0000) | KSEG0_MASK)>> 16);
	dprintf("exceptionHandlerPatch = 0x%08x\n", *patch);

	printf("Load kernel......\n");
	printf("Stack 0x%08x\n", sp);
	if ((u32) sp < (u32) &_end) {
		/* This will lead to problems. */
		error_printf("Stack is unusable!");
		return -1;
	}
	sbios_filename = getSBIOSFilename();
	if (strncmp(sbios_filename, "host:", 5) == 0) {
		rom_entry_t *romfile;

		romfile = rom_getFile(&sbios_filename[5]);
		if (romfile != NULL) {
			sbios = romfile->start;
			sbios_size = romfile->size;
		}
	}
	if (sbios == NULL) {
		sbios = load_file(sbios_filename, &sbios_size, NULL);
	}
	if (sbios == NULL) {
		error_printf("Failed to load sbios.elf");
		return -1;
	} else {
		uint32_t *SBIOSCallTable = NULL;
		int found = 0;
		int i;

		graphic_setStatusMessage("Checking SBIOS...");

		/* Don't check binary file.*/
		if ((sbios_size < 8) || (((uint32_t *) sbios)[1] != *magic_check)) {
			/* Check for errors in ELF file. */
			if (check_sections("SBIOS", sbios, sbios_size, 0x1000, 0x10000, NULL) != 0) {
				free(sbios);
				return -2;
			}
		} else {
			if (sbios_size >= 0x10000) {
				error_printf("SBIOS file is too large.");
				return -2;
			}
		}

		for (i = 0; i < (sbios_size / 4); i++) {
			if (((uint32_t *) sbios)[i] == *magic_check) {
				found = 1;
				SBIOSCallTable = getSBIOSCallTable((char *) (&(((uint32_t *) sbios)[i - 1])));
				if (SBIOSCallTable != NULL) {
					break;
				}
			}
		}
		if (SBIOSCallTable == NULL) {
			if (!found) {
				error_printf("SBIOS file is invalid, magic not found.");
			}
			return -3;
		} else {
			graphic_setStatusMessage("Setup SBIOS...");
			printf("Using address 0x%08x as SBIOSCallTable (SBIOS is at 0x%08x).\n", (uint32_t) SBIOSCallTable, (uint32_t) sbios);
			disableSBIOSCalls(SBIOSCallTable);
			graphic_setStatusMessage(NULL);
		}
		/* Access data from kernel space, because TLB misses can't be handled here. */
		sbios = (char *) (((unsigned int) sbios) | KSEG0_MASK);
	}
	FlushCache(0);
	kernel_filename = getKernelFilename();
	if (strncmp(kernel_filename, "host:", 5) == 0) {
		rom_entry_t *romfile;

		romfile = rom_getFile(&kernel_filename[5]);
		if (romfile != NULL) {
			buffer = memalign(64, romfile->size);
			if (buffer != NULL) {
				memcpy(buffer, romfile->start, romfile->size);
				kernel_size = romfile->size;
			}
		}
	}
	if (buffer == NULL) {
		buffer = load_kernel_file(kernel_filename, &kernel_size, NULL);
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
		if (check_sections("kernel", buffer, kernel_size, 0x10000, lowestAddress, &highest) != 0) {
			free((void *) (((unsigned int) sbios) & 0x0FFFFFFF));
			free(buffer);
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

		/* Access data from kernel space, because TLB misses can't be handled here. */
		buffer = (char *) (((unsigned int) buffer) | KSEG0_MASK);
		graphic_setStatusMessage(NULL);
		if (loadModules()) {
			free((void *) (((unsigned int) sbios) & 0x0FFFFFFF));
			free((void *) (((unsigned int) buffer) & 0x0FFFFFFF));
			return -2;
		}

		initrd_filename = getInitRdFilename();
		if (initrd_filename != NULL) {
			if (initrd_header == NULL) {
				/* We don't want to overwrite end of kernel. */
				error_printf("Can't load initrd. End of kernel must not be within the last 8 bytes of a page. "
					"Kernel must be larger than %d Bytes.\n", NORMAL_MEMORY_START);
				free((void *) (((unsigned int) sbios) & 0x0FFFFFFF));
				free((void *) (((unsigned int) buffer) & 0x0FFFFFFF));
				return -12;
			}
			initrd_size = ((uint32_t) lowestAddress) - ((uint32_t) &initrd_header[2]);
			printf("%d bytes for initrd available.\n", initrd_size);
			initrd_start = ((unsigned int) load_file(initrd_filename, &initrd_size, &initrd_header[2]));
			if (initrd_start != 0) {
				if (initrd_size != 0) {
					if ((base + 8 + initrd_size) >= lowestAddress) {
						error_printf("Initrd is to big to move behind kernel %d <= %d bytes.\n",
							lowestAddress, (highest + 8 + initrd_size));
						free((void *) (((unsigned int) sbios) & 0x0FFFFFFF));
						free((void *) (((unsigned int) buffer) & 0x0FFFFFFF));
						return -11;
					}
					/* Magic number is used by kernel to detect initrd. */
					initrd_header[0] = 0x494E5244;
					initrd_header[1] = initrd_size;

					initrd_start |= KSEG0_MASK;
				} else {
					error_printf("Loading of initrd failed (1).");
					return -9;
				}
			} else {
				error_printf("Loading of initrd failed (out of memory).");
				return -10;
			}
			printf("initrd_start 0x%08x 0x%08x\n", initrd_start, initrd_size);
		}

#if 0
		CDVD_Stop();
		CDVD_FlushCache();
#endif

		printf("Try to reboot IOP.\n");
		graphic_setStatusMessage("Reseting IOP");

		PS2KbdClose();
		deinitializeController();

		FlushCache(0);

		SifExitIopHeap();
		SifLoadFileExit();
		SifExitRpc();
		SifStopDma();

		SifIopReset(s_pUDNL, 0);

		while (SifIopSync());

		graphic_setStatusMessage("Initialize RPC");
		//printf("RPC");
		SifInitRpc(0);

		graphic_setStatusMessage("Patching enable LMB");
		sbv_patch_enable_lmb();
		graphic_setStatusMessage("Patching disable prefix check");
		sbv_patch_disable_prefix_check();

		graphic_setStatusMessage("Adding eedebug handler");

		addEEDebugHandler();

		graphic_setStatusMessage("Starting modules");
		//printf("Starting modules\n");

		startModules(bootinfo);

		graphic_setStatusMessage("Started all modules");

		//printf("Started modules\n");

		printAllModules();
		iopaddr = SifGetReg(0x80000000);

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

		/* Be sure that all interrupts are disabled. */
		__asm__ __volatile__("mtc0 %0, $12\nsync.p\n"::"r" (
			(0<<30) /* Deactivate COP2: VPU0 -> not used by loader */
			| (1<<29) /* Activate COP1: FPU -> used by compiler and functions like printf. */
			| (1<<28) /* Activate COP0: EE Core system control processor. */
			| (1<<16) /* eie */));

		gmode = getGraphicMode();
		if (gmode[0] != 0) {
			int mode = atoi(gmode);

			iop_printf("Setting graphic mode %d.\n", mode);
			setGraphicMode(mode);
		}

		/* Disable performance counters. */
		SET_PCCR(0);

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

		iop_dprints(U2K("Kernel mode print\n"));
		iop_dprintf(U2K("Stack 0x%08x\n"), sp);

		if (isSlimPSTwo()) {
			if (loaderConfig.enableDev9) {
				/* Use value 0x0200 to inform Linux about slim PSTwo. This was not part of Sony's Linux. */
				bootinfo->pccard_type = 0x0200;
			}
		} else {
			if (loaderConfig.enableDev9) {
				/* DEV9 can be only used by Linux, when PS2LINK is not loaded. */
				if (ps2dev9_init() == 0) {
					const char *pcicType;
	
					/* Activate hard disc. */
					ata_setup();
	
					/* Tell Linux to activate HDD and Network. */
					bootinfo->pccard_type = 0x0100;
					pcicType = getPcicType();
					if (strlen(pcicType) > 0) {
						/* User configured calue in menu. */
						bootinfo->pcic_type = atoi(pcicType);
					} else {
						/* Auto detect type. */
						bootinfo->pcic_type = pcic_get_cardtype();
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
		__asm__ __volatile__("mtc0 %0, $12\nsync.p\n"::"r" (
			(0<<30) /* Deactivate COP2: VPU0 -> not used by loader */
			| (1<<29) /* Activate COP1: FPU -> used by compiler and functions like printf. */
			| (1<<28) /* Activate COP0: EE Core system control processor. */
			| (1<<16) /* eie */));

		/* Set config register. */
		__asm__ __volatile__("mtc0 %0, $16\nsync.p\n"::"r" ((3<<0) /* Kseg0 cache */
						| (1<<12) /* Branch prediction */
						| (1<<13) /* Non blocking loads */
						| (1<<16) /* Data cache enable */
						| (1<<17) /* Instruction cache enable */
						| (1<<18) /* Enable pipeline */));

		/* Disable all breakpoints. */
		__asm__ __volatile__("mtc0 %0, $24\nsync.p\n"::"r" (0));

		/* Disable performance counter. */
		__asm__ __volatile__("mtps %0, 0\nsync.p\n"::"r" (0));

		__asm__ __volatile__("mtc0 %0, $28\nsync.p\n"::"r" (0x011c1020));
		__asm__ __volatile__("mtc0 %0, $29\nsync.p\n"::"r" (0x40004190));

#if 0
		/* Disable all INTC interrupts. */
		*((volatile unsigned int *) I_MASK) = 0;
		BARRIER();

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
		iop_prints(U2K("Try to crash\n"));
		//patch = (void *) 0x04000000;
		patch = (void *) 0x00100000;
		*patch = 0;
#endif

		/* Install SBIOS. */
		if ((sbios_size < 8) || (((uint32_t *) sbios)[1] != *magic_check)) {
			bootinfo->sbios_base = (uint32_t) copy_sections(sbios);
		} else {
			/* Copy binary data. */
			memcpy((void *) SBIOS_START_ADDRESS, sbios, sbios_size);
			bootinfo->sbios_base = SBIOS_START_ADDRESS;
		}

		/* Install linux kernel. */
		entry = copy_sections(buffer);

		/* Can only verify ELF files. */
		if ((sbios_size < 8) || (((uint32_t *) sbios)[1] != *magic_check)) {
			/* Verify if everything is correct. */
			verify_sections(sbios);
		}
		verify_sections(buffer);
		iop_prints(U2K("Code verified\n"));

		/* Set iopaddr for SifRPC. */
		*sbios_iopaddr = iopaddr;
#if 0
		/* XXX: Need to check parameter. */
		*sbios_osdparam = 0x0107ddc8;
#endif
		iop_printf(U2K("Patched sbios_iopaddr 0x%08x\n"), iopaddr);

		if (entry != NULL) {
			/* Activate SBIOS and linux kernel. */
			flushDCacheAll();
			invalidateICacheAll();

			//print_tlbs();
#if 1
			/* XXX: not done by original loader, only for testing. */
			/* Flush all tlb entries. */
			iop_prints(U2K("Flush TLBs (iop_printf will not work after this).\n"));
			/* printf is not working, because it uses a function pointer, which lead to a jump into kernelspace. */
			flush_tlbs();
			iop_prints(U2K("TLBs flushed.\n"));
#endif

			iop_prints(U2K("Jump to kernel!\n"));
			ret = entry(0 /* unused */, NULL /* unused */, (char **) ((u32) bootinfo | KSEG0_MASK), NULL /* unused */);
			iop_prints(U2K("Back from kernel!\n"));
			panic();
		} else {
			iop_prints(U2K("ELF not loaded.\n"));
		}
	} else {
		error_printf("Failed to load kernel.");
		free((void *) (((unsigned int) sbios) & 0x0FFFFFFF));
		return -1;
	}
	iop_prints(U2K("End reached?\n"));
	error_printf("Unknown program state.");

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


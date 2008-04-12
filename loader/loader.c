/* Copyright (c) 2007 Mega Man */
#include <stdio.h>
#include <string.h>
#include <kernel.h>
#include <iopheap.h>
#include <loadfile.h>
#include <sbv_patches.h>
#include <sifrpc.h>
#include <iopcontrol.h>
#include <debug.h>

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
#include "usb.h"

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


/** Debug print. */
#define dprintf(args...)
/** Debug print on IOP. */
#define iop_dprintf(args...)
/** Debug print on IOP. */
#define iop_dprints(args...)

/** Structure describing module that should be loaded. */
typedef struct
{
	/** Path to module file. */
	const char *path;
	/** Buffer used for loading the module. */
	unsigned char *buffer;
	/** Module size. */
	unsigned int size;
	/** True, if module must be buffered and can't be loaded after IOP reset. */
	int buffered;
	/** Parameter length. */
	unsigned int argLen;
	/** Module parameters. */
	const char *args;
} moduleEntry_t;

/** Definition of kernel entry function. */
typedef int (entry_t)(int argc, char **argv, char **envp, int *prom_vec);

/** Parameter for IOP reset. */
static char s_pUDNL   [] __attribute__(   (  section( ".data" ), aligned( 1 )  )   ) = "rom0:UDNL rom0:EELOADCNF";
/** Linux parameter for PAL mode. */
const char commandline_pal[] = "crtmode=pal ramdisk_size=16384";
/** Linux parameter for NTSC mode. */
const char commandline_ntsc[] = "crtmode=ntsc ramdisk_size=16384";
/* IP + Netmask + Gateway. */
const char ifcfg[] = "192.168.0.23\000255.255.255.0\000192.168.0.1";

/** Modules that should be loaded. */
moduleEntry_t modules[] = {
#if 0
	/* RTE modules. */
	{
		.path = "host:sio2man.irx",
		.buffered = -1,
		.argLen = 0,
		.args = NULL
	},
	{
		.path = "host:mcman.irx",
		.buffered = -1,
		.argLen = 0,
		.args = NULL
	},
	{
		.path = "host:mcserv.irx",
		.buffered = -1,
		.argLen = 0,
		.args = NULL
	},
	{
		.path = "host:padman.irx",
		.buffered = -1,
		.argLen = 0,
		.args = NULL
	},
	{
		.path = "host:libsd.irx",
		.buffered = -1,
		.argLen = 0,
		.args = NULL
	},
	{
		.path = "host:sdrdrv.irx",
		.buffered = -1,
		.argLen = 0,
		.args = NULL
	},
	{
		.path = "host:iopintr.irx",
		.buffered = -1,
		.argLen = 0,
		.args = NULL
	},
	{
		.path = "host:dmarelay.irx",
		.buffered = -1,
		.argLen = 0,
		.args = NULL
	},
#else
	/* TGE modules. */
#if 0
	{
		.path = "rom0:XSIO2MAN",
		.buffered = 0,
		.argLen = 0,
		.args = NULL
	},
	{
		.path = "rom0:XMCMAN",
		.buffered = 0,
		.argLen = 0,
		.args = NULL
	},
	{
		.path = "rom0:XMCSERV",
		.buffered = 0,
		.argLen = 0,
		.args = NULL
	},
#else
	{
		.path = "rom0:SIO2MAN",
		.buffered = 0,
		.argLen = 0,
		.args = NULL
	},
	{
		.path = "rom0:MCMAN",
		.buffered = 0,
		.argLen = 0,
		.args = NULL
	},
	{
		.path = "rom0:MCSERV",
		.buffered = 0,
		.argLen = 0,
		.args = NULL
	},
#endif
	{
		.path = "host:ioptrap.irx",
		.buffered = -1,
		.argLen = 0,
		.args = NULL
	},
	{
		.path = "host:iomanX.irx",
		.buffered = -1,
		.argLen = 0,
		.args = NULL
	},
#ifdef PS2LINK
	{
		.path = "host:ps2dev9.irx",
		.buffered = -1,
		.argLen = 0,
		.args = NULL
	},
	{
		.path = "host:ps2ip.irx",
		.buffered = -1,
		.argLen = 0,
		.args = NULL
	},
	{
		.path = "host:ps2smap.irx",
		.buffered = -1,
		.argLen = sizeof(ifcfg),
		.args = ifcfg
	},
	{
		.path = "host:poweroff.irx",
		.buffered = -1,
		.argLen = 0,
		.args = NULL
	},
	{
		.path = "host:ps2link.irx",
		.buffered = -1,
		.argLen = 0,
		.args = NULL
	},
#endif
#ifdef SHARED_MEM_DEBUG
	{
		.path = "host:sharedmem.irx",
		.buffered = -1,
		.argLen = 0,
		.args = NULL
	},
#endif
#ifdef TGE
	{
		.path = "host:intrelay.irx",
		.buffered = -1,
		.argLen = 0,
		.args = NULL
	},
	{
		.path = "host:dmarelay.irx",
		.buffered = -1,
		.argLen = 0,
		.args = NULL
	},
#endif
#endif
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
			int size = program_header->memsz - program_header->filesz;
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
char *load_file(const char *filename, int *size)
{
	char *buffer;
	int s;
	int filesize;
	FILE *fin;

	if (size == NULL)
		size = &s;

	fin = fopen(filename, "rb");
	if (fin == NULL) {
		printf("Error cannot open elf file \"%s\".\n", filename);
		return NULL;
	}

	fseek(fin, 0, SEEK_END);
	filesize = s = *size = ftell(fin);
	fseek(fin, 0, SEEK_SET);
	dprintf("filesize %d\n", *size);
	// memalign() begins at &_end behind elf file. This ensures that the first 5 MByte are not used.
	buffer = (char *) memalign(64, *size);
	dprintf("buffer 0x%08x\n", (unsigned int) buffer);
	if (buffer == NULL) {
		fclose(fin);
		return NULL;
	}
	if ((u32) buffer < (u32) &_end) {
		/* This will lead to problems. */
		printf("memalign() is unusable!\n");
		panic();
	}

	dprintf("Loading...\n");
	int pos = 0;
	int n;
	int next = 10000;
	printf("%s size %d\n", filename, *size);
	graphic_setPercentage(0, filename);
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
	dprintf("Loaded %d\n", pos);
	graphic_setPercentage(100, filename);

	return buffer;
}

/** Load all IOP modules (from host). */
void loadModules(void)
{
	int i;

	for (i = 0; i < sizeof(modules) / sizeof(moduleEntry_t); i++)
	{
		if (modules[i].buffered) {
			dprintf("Loading module %s.\n", modules[i].path);
			modules[i].buffer = load_file(modules[i].path, &modules[i].size);
			if (modules[i].buffer == NULL) {
				printf("Failed to load module '%s'.\n", modules[i].path);
				SleepThread();
				panic();
			}
		} else {
			dprintf("Not loading module %s.\n", modules[i].path);
			modules[i].buffer = NULL;
		}
	}
}

/** Start all IOP modules. */
void startModules(void)
{
	int i;
	int rv;

	for (i = 0; i < sizeof(modules) / sizeof(moduleEntry_t); i++)
	{
		if (modules[i].buffered) {
			SifExecModuleBuffer(modules[i].buffer, modules[i].size, modules[i].argLen, modules[i].args, &rv);
		} else {
			SifLoadModule(modules[i].path, modules[i].argLen, modules[i].args);
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
		printf("modules id %d\n", module.id);
		smem_read(module.name, name, 256);
		printf("Name: %s\n", name);
#if 1
		printf("mod: %s\n", name);
#else
		scr_printf("mod: %s\n", name);
#endif
		current = &module;
		i++;
	}
}

/**
 * Load kernel, initrd and required modules. Then start kernel.
 * @param mode Graphic mode that should be used.
 */
int loader(graphic_mode_t mode)
{
	entry_t *entry;
	int ret;
	char *buffer;
	char *sbios;
	struct ps2_bootinfo *bootinfo = (struct ps2_bootinfo *) PS2_BOOTINFO_OLDADDR;
	register int sp asm("sp");
	const char *commandline;
	uint32_t *patch;
	uint32_t iopaddr;
	uint32_t *sbios_iopaddr = (uint32_t *) 0x80001008;
	uint32_t *sbios_osdparam = (uint32_t *) 0x8000100c;

	/* Set commandline for correct video mode. */
	if(mode == MODE_NTSC) {
		commandline = commandline_ntsc;
	} else {
		commandline = commandline_pal;
	}

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
		printf("Stack is unusable!\n");
		panic();
	}
	sbios = load_file("host:sbios.elf", NULL);
	if (sbios == NULL) {
		printf("Failed to load sbios.elf\n");
		return 0;
	} else {
		/* Access data from kernel space, because TLB misses can't be handled here. */
		sbios = (char *) (((unsigned int) sbios) | KSEG0_MASK);
	}
	FlushCache(0);
	buffer = load_file("host:kernel.elf", NULL);
	if (buffer != NULL) {
		/* Access data from kernel space, because TLB misses can't be handled here. */
		buffer = (char *) (((unsigned int) buffer) | KSEG0_MASK);
		loadModules();

		bootinfo->initrd_start = ((unsigned int) load_file("host:initrd.gz", &bootinfo->initrd_size));
		if (bootinfo->initrd_size == 0) {
			bootinfo->initrd_start = ((unsigned int) load_file("host:initrd", &bootinfo->initrd_size));
		}
		if (bootinfo->initrd_size != 0) {
			bootinfo->initrd_start |= KSEG0_MASK;
		}
		printf("initrd_start 0x%08x 0x%08x\n", bootinfo->initrd_start, bootinfo->initrd_size);

		printf("Try to reboot IOP.\n");
		FlushCache(0);

		SifExitIopHeap();
		SifLoadFileExit();
		SifExitRpc();
		SifStopDma();

		SifIopReset(s_pUDNL, 0);

		while (SifIopSync());

		printf("RPC");
		SifInitRpc(0);

		sbv_patch_enable_lmb();
		sbv_patch_disable_prefix_check();

		printf("Starting modules\n");

		startModules();
		printf("Started modules\n");

		printAllModules();
		iopaddr = SifGetReg(0x80000000);

		SifExitIopHeap();
		SifLoadFileExit();
		SifExitRpc();
		SifStopDma();

		disable_interrupts();
		ee_kmode_enter();
#ifdef USER_SPACE_SUPPORT
		iop_kmode_enter();
#endif

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
		bootinfo->sbios_base = (uint32_t) copy_sections(sbios);

		/* Install linux kernel. */
		entry = copy_sections(buffer);

		/* Verify if everything is correct. */
		verify_sections(sbios);
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

			/* Setup USB. */
			initUSB();

			iop_prints(U2K("Jump to kernel!\n"));
			ret = entry(0 /* unused */, NULL /* unused */, (char **) ((u32) bootinfo | KSEG0_MASK), NULL /* unsused */);
			iop_prints(U2K("Back from kernel!\n"));
			panic();
		} else {
			iop_prints(U2K("ELF not loaded.\n"));
		}
	} else {
		iop_prints(U2K("ELF not loaded.\n"));
	}
	iop_prints(U2K("End reached?\n"));

	panic();

	return(0);
}


/*
 * linux/include/asm-mips/ps2/bootinfo.h
 *
 *	Copyright (C) 2000, 2001  Sony Computer Entertainment Inc.
 *
 * This file is subject to the terms and conditions of the GNU General
 * Public License Version 2. See the file "COPYING" in the main
 * directory of this archive for more details.
 *
 * $Id$
 */

#ifndef __ASM_PS2_BOOTINFO_H
#define __ASM_PS2_BOOTINFO_H

#include "sysconf.h"
#include "memory.h"

#define PS2_BOOTINFO_MAGIC	0x50324c42	/* "P2LB" */
#define PS2_BOOTINFO_OLDADDR	(0x01fff000 | KSEG0_MASK)
#define PS2_BOOTINFO_MACHTYPE_PS2	0
#define PS2_BOOTINFO_MACHTYPE_T10K	1

struct ps2_rtc {
    uint8_t padding_1;
    uint8_t sec;
    uint8_t min;
    uint8_t hour;
    uint8_t padding_2;
    uint8_t day;
    uint8_t mon;
    uint8_t year;
};

struct ps2_bootinfo {
    uint32_t		pccard_type;
    char		*opt_string;
    uint32_t		reserved0;
    uint32_t		reserved1;
    struct ps2_rtc	boot_time;
    uint32_t		mach_type;
    uint32_t		pcic_type;
    struct ps2_sysconf	sysconf;
    uint32_t		magic;
    uint32_t		size;
    uint32_t		sbios_base;
    uint32_t		maxmem;
    uint32_t		stringsize;
    char		*stringdata;
    char		*ver_vm;
    char		*ver_rb;
    char		*ver_model;
    char		*ver_ps1drv_rom;
    char		*ver_ps1drv_hdd;
    char		*ver_ps1drv_path;
    char		*ver_dvd_id;
    char		*ver_dvd_rom;
    char		*ver_dvd_hdd;
    char		*ver_dvd_path;
    /* Special defines, not used anymore. */
    uint32_t		initrd_start;
    uint32_t		initrd_size;
};
#define PS2_BOOTINFO_OLDSIZE	((int)(&((struct ps2_bootinfo*)0)->magic))

#endif /* __ASM_PS2_BOOTINFO_H */

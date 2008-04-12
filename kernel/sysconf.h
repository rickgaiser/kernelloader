/*
 * linux/include/asm-mips/ps2/sysconf.h
 *
 *	Copyright (C) 2000, 2001  Sony Computer Entertainment Inc.
 *
 * This file is subject to the terms and conditions of the GNU General
 * Public License Version 2. See the file "COPYING" in the main
 * directory of this archive for more details.
 *
 * $Id$
 */

#ifndef __ASM_PS2_SYSCONF_H
#define __ASM_PS2_SYSCONF_H

struct ps2_sysconf {
    short timezone;
    uint8_t aspect;
    uint8_t datenotation;
    uint8_t language;
    uint8_t spdif;
    uint8_t summertime;
    uint8_t timenotation;
    uint8_t video;
};

#endif /* __ASM_PS2_SYSCONF_H */

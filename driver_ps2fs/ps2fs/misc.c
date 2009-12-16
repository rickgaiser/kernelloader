/*
 * ps2fs/misc.c
 *
 * Copyright (c) 2002 Andrew Church <achurch@achurch.org>
 *
 * Error routines shamelessly borrowed from ext2/super.c
 */

#include <linux/config.h>
#include <linux/string.h>
#include <linux/fs.h>
#include <linux/blkdev.h>

#include "ps2fs_fs.h"

/*************************************************************************/
/*************************************************************************/

static char error_buf[1024];

/*************************************************************************/

/* Report a warning on the given filesystem. */

void ps2fs_warning(struct super_block *sb, const char *function,
		   const char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    vsprintf(error_buf, fmt, args);
    va_end(args);
    printk(KERN_WARNING "ps2fs WARNING (device %s): %s: %s\n",
	   bdevname(sb->s_dev), function, error_buf);
}

/*************************************************************************/

/* Report an error on the given filesystem. */

void ps2fs_error(struct super_block *sb, const char *function,
		 const char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    vsprintf(error_buf, fmt, args);
    va_end(args);
    printk(KERN_ERR "ps2fs ERROR (device %s): %s: %s\n",
	   bdevname(sb->s_dev), function, error_buf);
}

/*************************************************************************/

/* Report a fatal error on the given filesystem and halt. */

void ps2fs_panic(struct super_block *sb, const char *function,
		 const char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    vsprintf(error_buf, fmt, args);
    va_end(args);
    panic("ps2fs PANIC (device %s): %s: %s\n",
	  bdevname(sb->s_dev), function, error_buf);
}

/*************************************************************************/
/*************************************************************************/

static int mdays[12] = {0,31,59,90,120,151,181,212,243,273,304,334};

/*************************************************************************/

/* Convert a ps2fs_datetime structure to an ordinary time value. */

__s32 from_ps2time(const struct ps2fs_datetime *dt)
{
    __s32 t = 0;
    __u16 year = le16_to_cpu(dt->year);
    int i;

    for (i = 1970; i < year; i++) {
	t += 365*86400;
	if (i%4 == 0 && (t%100 != 0 || t%400 == 0))
	    t += 86400;
    }
    t += mdays[dt->month-1] * 86400;
    if (dt->month > 2 && (year%4 == 0 && (year%100 != 0 || year%400 == 0)))
	t += 86400;
    return t + (dt->day-1)*86400 + dt->hour*3600 + dt->min*60 + dt->sec;
}

/*************************************************************************/

/* Convert an ordinary time value to a ps2fs_datetime structure. */

void to_ps2time(struct ps2fs_datetime *dt, __s32 time)
{
    __u16 year;
    int days, i;

    year = 1969;
    do {
	year++;
	if (year%4 == 0 && (year%100 != 0 || year%400 == 0))
	    days = 366;
	else
	    days = 365;
	time -= days*86400;
    } while (time >= 0);
    time += days*86400;
    dt->year = cpu_to_le16(year);
    for (i = 1; i < 12; i++) {
	if (time < mdays[i]*86400)
	    break;
    }
    dt->month = i;
    time -= mdays[i-1]*86400;
    dt->day = time/86400 + 1;
    dt->hour = time%86400 / 3600;
    dt->min = time%3600 / 60;
    dt->sec = time%60;
}

/*************************************************************************/
/*************************************************************************/

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,4,0)

/* simple_strtol() for kernel 2.2.  Strangely enough, this is declared in
 * <linux/kernel.h> but isn't exported (at least in kernel 2.2.1), so we
 * have to define it ourselves.
 */

long simple_strtol(const char *str, char **ret, unsigned int base)
{
    int negative;
    unsigned long val;

    if (str && *str == '-') {
	negative = 1;
	str++;
    } else {
	negative = 0;
    }
    val = simple_strtoul(str, ret, base);
    if (val > 0x7FFFFFFF) {
	if (negative)
	    return -0x80000000;
	else
	    return 0x7FFFFFFF;
    } else {
	if (negative)
	    return -val;
	else
	    return val;
    }
}

#endif  /* kernel 2.2 */

/*************************************************************************/

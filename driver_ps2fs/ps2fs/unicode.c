/*
 * ps2fs/unicode.c (Unicode -> EUC-JP translation)
 *
 * Copyright (c) 2002 Andrew Church
 */

#include <linux/fs.h>

/*************************************************************************/

static unsigned short uni2euc[65536] = {
#include "unitable.h"
};

static unsigned short euc2uni[94*94] = {
#include "euctable.h"
};

/*************************************************************************/

/* Converts all Unicode (UTF-8) characters in the given null-terminated
 * buffer to EUC-JP, overwriting the old buffer (the new string will always
 * be no longer than the old one).  Characters that cannot be represented
 * in EUC-JP will be translated into a '?' character.
 */

void translate_from_unicode(unsigned char *buf)
{
    unsigned char *dest = buf;
    int nbytes, i;
    unsigned long codepoint;
    unsigned int eucval;

    while (*buf) {
	if (!(*buf & 0x80)) {
	    /* Ordinary character, pass it through */
	    *dest++ = *buf++;
	    continue;
	}
	/* Unicode character, process */
	if (!(*buf & 0x40)) {
	    /* Invalid Unicode character (10xxxxxx should only be used for
	     * intermediate characters)--replace by '?' and continue */
	    *dest++ = '?';
	    buf++;
	    continue;
	} else if (!(*buf & 0x20)) {
	    nbytes = 2;
	} else if (!(*buf & 0x10)) {
	    nbytes = 3;
	} else if (!(*buf & 0x08)) {
	    nbytes = 4;
	} else if (!(*buf & 0x04)) {
	    nbytes = 5;
	} else if (!(*buf & 0x02)) {
	    nbytes = 6;
	} else {
	    /* 1111111x is an invalid character */
	    *dest++ = '?';
	    buf++;
	    continue;
	}
	/* Make sure there are enough bytes left in the string and that
	 * they're valid; otherwise replace by a single '?' and stop.
	 * Convert to a single Unicode code point as we go. */
	codepoint = *buf++ & (0x7F >> nbytes);
	for (i = 1; i < nbytes; i++) {
	    if (!*buf || (*buf & 0xC0) != 0x80) {
		*dest++ = '?';
		break;
	    }
	    codepoint = codepoint<<6 | (*buf++ & 0x3F);
	}
	/* Code points over 65535 are automatically out of range, replace
	 * them with a '?'. */
	if (codepoint > 0xFFFF) {
	    *dest++ = '?';
	    continue;
	}
	/* Success, actually convert the character. */
	eucval = uni2euc[codepoint];
	*dest++ = eucval >> 8;
	*dest++ = eucval & 0xFF;
    }
    *dest = 0;
}

/*************************************************************************/

/* Converts all EUC-JP characters in the given `dentry' to Unicode (UTF-8),
 * storing the result in `dest'.  If the result would exceed `destlen'
 * bytes (including the trailing null), return 0, else return 1.
 * Characters 0x80-0xA0 and 0xFF, as well as EUC-JP characters not followed
 * by another EUC-JP character, will be translated as ISO-8859-1.
 */

int translate_to_unicode(const struct dentry *dentry, unsigned char *dest,
			 int destlen)
{
    const unsigned char *buf = dentry->d_name.name;
    const unsigned char *end = buf + dentry->d_name.len;
    int i;

    if (destlen <= 0)
	return 0;
    while (buf < end) {
	if (!(*buf & 0x80)) {
	    /* Ordinary character, pass it through */
	    if (--destlen <= 0)
		goto fail;
	    *dest++ = *buf++;
	    continue;
	}
	if (*buf<0xA1 || *buf>0xFE || buf+1>end || buf[1]<0xA1 || buf[1]>0xFE) {
	    /* Not EUC-JP, or next character is not EUC-JP; treat as ISO8859 */
	    if ((destlen -= 2) <= 0)
		goto fail;
	    *dest++ = 0xC0 | (*buf >> 6);
	    *dest++ = 0x80 | (*buf++ & 0x3F);
	    continue;
	}
	/* Process as EUC-JP */
	i = euc2uni[(buf[0]-0xA1)*94+(buf[1]-0xA1)];
	buf += 2;
	    if ((destlen -= 3) <= 0)
		goto fail;
	*dest++ = 0xE0 | i>>12;
	*dest++ = 0x80 | ((i>>6) & 0x3F);
	*dest++ = 0x80 | (i & 0x3F);
    }
    *dest = 0;
    return 1;
  fail:
    return 0;
}

/*************************************************************************/

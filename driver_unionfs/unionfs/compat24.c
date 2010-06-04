/*
 * Copyright (c) 2003-2005 Erez Zadok
 * Copyright (c) 2003-2005 Charles P. Wright
 * Copyright (c) 2005      Arun M. Krishnakumar
 * Copyright (c) 2005      David P. Quigley
 * Copyright (c) 2003-2004 Mohammad Nayyer Zubair
 * Copyright (c) 2003-2003 Puja Gupta
 * Copyright (c) 2003-2003 Harikesavan Krishnan
 * Copyright (c) 2003-2005 Stony Brook University
 * Copyright (c) 2003-2005 The Research Foundation of State University of New York
 *
 * For specific licensing information, see the COPYING file distributed with
 * this package.
 *
 * This Copyright notice must be kept intact and distributed with all sources.
 */
/*
 *  $Id$
 */

#include "fist.h"
#include "unionfs.h"

static spinlock_t buflock = SPIN_LOCK_UNLOCKED;

int vprintk(const char *fmt, va_list args)
{
	static char buf[PAGE_SIZE];
	unsigned long flags;
	int ret;

	spin_lock_irqsave(&buflock, flags);
	vsnprintf(buf, sizeof(buf), fmt, args);
	ret = printk(buf);
	spin_unlock_irqrestore(&buflock, flags);

	return ret;
}

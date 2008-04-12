/* Copyright (c) 2008 Mega Man */
#ifndef _MUTEX_H_
#define _MUTEX_H_

/** Initialize value for locks. */
#define SBIOS_MUTEX_INIT ((sbios_mutex_t) 0)

/** Mutex is used to protect cirtical sections. */
typedef int sbios_mutex_t;

int sbios_tryLock(sbios_mutex_t *mutex);
void sbios_unlock(sbios_mutex_t *mutex);

#endif

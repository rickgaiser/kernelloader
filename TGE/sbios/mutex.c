/* Copyright (c) 2008 Mega Man */
#include "mutex.h"
#include "core.h"
#include "stdio.h"
#include "tge_defs.h"


/**
 * Try to lock a mutex.
 * @param mutex Mutex to be locked.
 * @returns
 * @retval 0 Mutex is now locked.
 * @retval -1 Error, mutex is invalid.
 * @retval -2 Error, mutex was alredy locked.
 */
int sbios_tryLock(sbios_mutex_t *mutex) {
	u32 status;

	if (mutex == NULL) {
		printf("mutex inval\n");
		return -1;
	}
	core_save_disable(&status);
	if (*mutex) {
		/* Currently used by other call. */
		core_restore(status);
		return -2;
	} else {
		*mutex = -1;
		core_restore(status);
		return 0;
	}
}

/**
 * Unlock a mutex previously locked by sbios_tryLock.
 */
void sbios_unlock(sbios_mutex_t *mutex) {
	u32 status;

	if (mutex == NULL) {
		printf("mutex inval\n");
		return;
	}

	core_save_disable(&status);
	if (*mutex) {
		*mutex = 0;
		core_restore(status);
	} else {
		core_restore(status);
		printf("mutex err\n");
	}
}


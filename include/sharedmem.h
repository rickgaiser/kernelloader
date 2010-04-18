#ifndef _SHAREDMEM_H_
#define _SHAREDMEM_H_

/** Name of sharedmem module. */
#define SHAREDMEM_MODULE_NAME "sharedmem"

/** Magic value to find shared memory area. */
#define SHAREDMEM_MAGIC "sharedmem debug area structure."

typedef struct {
	/** Magic number to find structure in memory. */
	const char magic[32];
	/** Shared memory communicationarea. */
	unsigned char shared[32];
} sharedmem_dbg_t;

#endif

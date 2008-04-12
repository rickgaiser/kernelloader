#include "iopmem.h"
#include "core.h"
#include "string.h"

/** IOP RAM is mysteriously mapped into EE HW space at this address. */
#define SUB_VIRT_MEM    0xbc000000

/** Read iop memory address. */
unsigned int iop_read(void *addr, void *buf, unsigned int size)
{
	memcpy(buf, addr + SUB_VIRT_MEM, size);

	return size;
}

/** Write to iop memory address. */
unsigned int iop_write(void *addr, void *buf, unsigned int size)
{
	memcpy(addr + SUB_VIRT_MEM, buf, size);

	return size;
}


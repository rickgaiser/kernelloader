#ifndef _IOPMEM_H_
#define _IOPMEM_H_

/** Read IOP memory. */
unsigned int iop_read(void *addr, void *buf, unsigned int size);
/** Write to IOP memory. */
unsigned int iop_write(void *addr, void *buf, unsigned int size);

#endif /* _IOPMEM_H_ */

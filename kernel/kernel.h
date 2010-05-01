/* Copyright (c) 2007 Mega Man */
#ifndef _KERNEL_H_
#define _KERNEL_H_

#include "stdint.h"

#if 0
#define DBG printf
#else
#define DBG(args...)
#endif

/** Interface for creating. */
typedef struct t_ee_sema
{
	int count;
	int max_count;
	int init_count;
	int wait_threads;
	uint32_t attr;
	uint32_t option;
} ee_sema_t;

/** Interface for program argument passing. */
typedef struct {
	uint32_t argc;
	char *argv[1];
	char commandline[0];
} args_t;

typedef int32_t dmac_useg_handler_t(int32_t channel);


/* thread.c */

/** Create a new semaphore. */
int32_t syscallCreateSema(ee_sema_t *sema);

/** Delete semaphore. */
int32_t iDeleteSema(int32_t sema_id);

/** Wait on semaphore. */
int32_t WaitSema(uint32_t sid);

/** Signal semaphore. */
int32_t iSignalSema(int32_t sid);

/** Exit current thread. */
uint32_t syscallExit(void);

/* memory.c */

/** Setup thread parameters. */
uint32_t syscallRFU60(uint32_t gp, uint32_t stack, uint32_t stack_size, args_t *args);

/** Return heap address. */
uint32_t syscallRFU61(void);

/** Flush data cache. */
uint32_t syscallRFU100(void);

/* graphic.c */


/** Setup graphic mode. */
uint32_t syscallSetCrtc(int int_mode, int ntsc_pal, int field_mode);

/** Set GS IMR register. */
uint32_t syscallSetGsIMR(uint32_t imr);

/* rtesif.c */
void syscallSifSetDChain(void);

/* dmac.c */
/** Install user space handler for DMAC Channel. */
int32_t syscallAddDmacHandler(uint32_t channel, dmac_useg_handler_t *handler, int32_t next);
/** Enable DMAC interrupt. */
int32_t syscallEnableDmac(uint32_t channel);
/** Disable DMAC interrupt. */
int32_t syscallDisableDmac(uint32_t channel);
/** Get sif register. */
int syscallSifGetReg(uint32_t register_num);
/** Set sif register. */
int syscallSifSetReg(uint32_t register_num, uint32_t register_value);
/** Start DMA request. */
uint32_t syscallSifSetDma(void *sdd, int32_t len);
#endif

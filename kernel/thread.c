/* Copyright (c) 2007 Mega Man */
#include "kernel.h"
#include "stdio.h"
#include "memory.h"
#include "cp0register.h"
#include "interrupts.h"

#define NUMBER_OF_SEMAPHORES 32

typedef struct kernel_sema kernel_sema_t;

struct kernel_sema
{
	int count;
	int max_count;
	int init_count;
	int wait_threads;
	uint32_t attr;
	uint32_t option;
	kernel_sema_t *next;
	int index;
	int free;
};

kernel_sema_t semaphoreArray[NUMBER_OF_SEMAPHORES];

kernel_sema_t *semaphoreListHead = NULL;

int32_t syscallCreateSema(ee_sema_t *sema)
{
	kernel_sema_t *s;

	if (sema->init_count < 0) {
		return -1;
	}
	s = semaphoreListHead;
	if (s == NULL) {
		return -1;
	}
	semaphoreListHead = s->next;
	s->count = sema->init_count;
	s->max_count = sema->max_count;
	s->wait_threads = sema->wait_threads;
	s->attr = sema->attr;
	s->option = sema->option;
	s->free = 0;
	s->next = NULL;

	return s->index;
}

int32_t iDeleteSema(int32_t sema_id)
{
	kernel_sema_t *s;

	if (sema_id >= NUMBER_OF_SEMAPHORES) {
		return -1;
	}
	if (sema_id < 0) {
		return -1;
	}
	s = &semaphoreArray[sema_id];
	if (s->free) {
		return -1;
	}

	/* XXX: Check for waiting threads. */

	s->next = semaphoreListHead;
	semaphoreListHead = s;
	s-> free = -1;

	return sema_id;
}

int32_t iWaitSema(uint32_t sid)
{
	if ((sid >= NUMBER_OF_SEMAPHORES) ||
		(semaphoreArray[sid].count < 0)) {
		return -1;
	}
	if (semaphoreArray[sid].count > 0) {
		semaphoreArray[sid].count--;
		return sid;
	}

	/* XXX: Put thread into wait queue. */
	return -2;
}

int32_t WaitSema(uint32_t sid)
{
	int ret;
	uint32_t old;

	CP0_GET_STATUS(old);
	/* Change to kernel mode and switch off interrupts. */
	CP0_SET_STATUS(old & (~0x1f));

	ret = iWaitSema(sid);
	while (ret == -2) {
		enableInterrupts();
		disableInterrupts();
		/* XXX: Make thread scheduling instead. */
		ret = iWaitSema(sid);
	}
	CP0_SET_STATUS(old);
	return ret;
}

int32_t iSignalSema(int32_t sid)
{
	if ((sid >= NUMBER_OF_SEMAPHORES) ||
		(semaphoreArray[sid].count < 0)) {
		return -1;
	}
	/* XXX: Wakeup threaqd. */
	semaphoreArray[sid].count++;
}

uint32_t syscallExit(void)
{
	printf("Program exited.\n");
	while(1);
}

void init_semaphores(void)
{
	int i;

	/* Setup a list by an array. */
	semaphoreListHead = &semaphoreArray[0];

	for (i = 0; i < (NUMBER_OF_SEMAPHORES - 1); i++) {
		semaphoreArray[i].next = &semaphoreArray[i + 1];
		semaphoreArray[i].index = i;
		semaphoreArray[i].free = -1;
	}
	semaphoreArray[NUMBER_OF_SEMAPHORES - 1].next = NULL;
	semaphoreArray[NUMBER_OF_SEMAPHORES - 1].index = NUMBER_OF_SEMAPHORES - 1;
	semaphoreArray[NUMBER_OF_SEMAPHORES - 1].free = -1;
}


void init_thread_module(void)
{
	init_semaphores();
}

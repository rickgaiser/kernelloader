/* Copyright (c) 2007 Mega Man */
#include "kernel.h"
#include "stdio.h"
#include "memory.h"

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
	s->count = sema->count;
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

uint32_t syscallExit(void)
{
	printf("Program exited.\n");
	while(1);
}

void init_seamphores(void)
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
	init_seamphores();
}

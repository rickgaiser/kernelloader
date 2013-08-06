/* Copyright (c) 2007 Mega Man */
#include "string.h"
#include "stdint.h"

void *memcpy(void *dest, const void *src, unsigned int size)
{
	uint8_t *d = (uint8_t *) dest;
	uint8_t *s = (uint8_t *) src;

	while (size--) {
		*d++ = *s++;
	}

	return dest;
}

void *memset(void *dest, int c, unsigned int size)
{
	uint8_t *d = (uint8_t *) dest;

	while (size-- != 0) {
		*d++ = (uint8_t) c;
	}

	return dest;
}

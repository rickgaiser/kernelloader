/* Copyright (c) 2007 Mega Man */
#ifndef _STRING_H_
#define _STRING_H_

void *memcpy(void *dest, const void *src, unsigned int size);
void *memset(void *dest, int c, unsigned int size);
unsigned int strlen(const char *s);
int memcmp(const void *s1, const void *s2, unsigned int n);

#endif

/* Copyright (c) 2008 Mega Man */
#ifndef _STRING_H_
#define _STRING_H_

void *memcpy(void *dest, const void *src, int size);
void *memset(void *dest, int c, int size);
unsigned int strlen(const char *s);
int memcmp(const void *s1, const void *s2, unsigned int n);
char *strncpy(char *dst, const char *src, unsigned int len);
char *strcpy(char *dst, const char *src);

#endif

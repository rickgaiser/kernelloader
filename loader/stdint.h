/* Copyright (c) 2007 Mega Man */
#ifndef _STDINT_H_
#define _STDINT_H_

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned /*long*/ long uint64_t;
typedef unsigned int uint128_t __attribute__((mode(TI), aligned(16)));
typedef signed char int8_t;
typedef signed short int16_t;
typedef signed int int32_t;
typedef signed /*long*/ long int64_t;
typedef __signed__ int int128_t __attribute__((mode(TI), aligned(16)));

#endif

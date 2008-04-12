/* Copyright (c) 2007 Mega Man */
#ifndef __PAD_H__
#define __PAD_H__

#include <libpad.h>

#ifdef __cplusplus
extern "C" {
#endif
void initializeController();
void deinitializeController(void);
int readPad(int port);
#ifdef __cplusplus
}
#endif

#endif /* __PAD_H__ */

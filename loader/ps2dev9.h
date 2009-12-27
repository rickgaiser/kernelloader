#ifndef _PS2DEV9_H_
#define _PS2DEV9_H_
/* Copyright (c) 2007 Mega Man */

int ps2dev9_init(void);
int pcic_get_cardtype(void);
void dev9IntrEnable(int mask);

#endif

/* Copyright (c) 2007 Mega Man */
#ifndef _JUMP_H_
#define _JUMP_H_

/** Jump to kernel space by adding ksegOffset. Change stack and instruction pointer. */
void jump2kernelspace(unsigned int ksegOffset);

#endif /* _JUMP_H_ */

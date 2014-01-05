/* Copyright (c) 2007-2014 Mega Man */
#ifndef _EXCEPTION_H_
#define _EXCEPTION_H_

void installExceptionHandler(int number);
void exception(int nr, uint32_t *regs);

#endif /* _EXCEPTION_H_ */

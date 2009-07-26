#ifndef _DEV9INIT_H_
#define _DEV9INIT_H_

#define MODNAME "dev9init"
#define M_PRINTF(format, args...)	\
	printf(MODNAME ": " format, ## args)

void dev9IntrEnable(int mask);

#endif

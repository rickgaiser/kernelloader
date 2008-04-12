/* Copyright (c) 2007 Mega Man */
#ifndef _IOPPRINTDTATA_H_
#define _IOPPRINTDTATA_H_

typedef struct {
	struct t_SifCmdHeader    sifcmd;
	char text[80];
} iop_text_data_t;

#endif

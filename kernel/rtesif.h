/* Copyright (c) 2007 Mega Man */
#ifndef _RTE_SIF_H_
#define _RTE_SIF_H_

int sif_reg_set(int reg, int val);
int sif_reg_get(int reg);
int sif_dma_request(void *dmareq, int count);
void sif_init(void);
void setdve(int mode);

#endif

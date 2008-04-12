/*
 * tge_sifdma.h - TGE SIF DMA header file
 *
 * Copyright (c) 2003 Marcus R. Brown <mrbrown@0xd6.org>
 *
 * See the file LICENSE, located within this directory, for licensing terms.
 */

#ifndef TGE_SIFDMA_H
#define TGE_SIFDMA_H

/** Enable interrupt after transfer. */
#define TGE_SIFDMA_ATTR_INT_I	0x02
#define TGE_SIFDMA_ATTR_INT_O	0x04
/** Transfer IOP DMAtag without data. */
#define TGE_SIFDMA_ATTR_DMATAG	0x20
#define TGE_SIFDMA_ATTR_ERT		0x40

/**
 * Structure describing data transfered from EE to IOP.
 */
typedef struct {
	/** Data to be transfered (located in EE memory). */
	void	*src;
	/** Where to copy the data in IOP memory. */
	void	*dest;
	/** Size in bytes to transfer. */
	u32	size;
	/** Flags:
	 *   TGE_SIFDMA_ATTR_INT_I
	 *   TGE_SIFDMA_ATTR_DMATAG
	 *   TGE_SIFDMA_ATTR_INT_O
	 *   TGE_SIFDMA_ATTR_ERT
	 */
	u32	attr;
} tge_sifdma_transfer_t;

#endif /* TGE_SIFDMA_H */

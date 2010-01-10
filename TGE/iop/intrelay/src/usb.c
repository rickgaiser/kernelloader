/* Copyright (c) 2009 Mega Man, copied code from usb driver in ps2sdk. */
#include <tamtypes.h>
#include <stdio.h>

#include "usb.h"
#include "irx_imports.h"

/** UB OHCI register base address. */
#define USB_OHCI_REGBASE 0xBF801600
#define USB_REG_UNKNOWN1680 ((volatile u16 *) 0xbf801680)

#define BIT(x) (1 << (x))
#define OHCI_COM_HCR BIT(0)

typedef struct _hcTd {
	u32 HcArea;
	void   *curBufPtr;
	struct _hcTd *next;
	void   *bufferEnd;
} HcTD_t __attribute__ ((packed));

typedef struct _hcEd {
	u16 hcArea;
	u16 maxPacketSize;
	HcTD_t  *tdTail;
	HcTD_t  *tdHead;
	struct _hcEd *next;
} HcED_t __attribute__ ((packed));

typedef struct {
	volatile HcED_t   *InterruptTable[32];
	volatile u16 FrameNumber;
	volatile u16 pad;
	volatile HcTD_t   *DoneHead;
	volatile u8  reserved[116];
	volatile u32 pad2; // expand struct to 256 bytes for alignment
} HcCA_t __attribute__ ((packed));

typedef struct {
	volatile u32 HcRevision;
	volatile u32 HcControl;
	volatile u32 HcCommandStatus;
	volatile u32 HcInterruptStatus;
	volatile u32 HcInterruptEnable;
	volatile u32 HcInterruptDisable;
	volatile HcCA_t  *HcHCCA;
	volatile HcED_t  *HcPeriodCurrentEd;
	volatile HcED_t  *HcControlHeadEd;
	volatile HcED_t  *HcControlCurrentEd;
	volatile HcED_t  *HcBulkHeadEd;
	volatile HcED_t  *HcBulkCurrentEd;
	volatile u32 HcDoneHead;
	volatile u32 HcFmInterval;
	volatile u32 HcFmRemaining;
	volatile u32 HcFmNumber;
	volatile u32 HcPeriodicStart;
	volatile u32 HcLsThreshold;
	volatile u32 HcRhDescriptorA;
	volatile u32 HcRhDescriptorB;
	volatile u32 HcRhStatus;
	volatile u32 HcRhPortStatus[2];
} OhciRegs_t __attribute__ ((packed));

/** Initialize USB for later use and reset OHCI chip. */
void initUSB(void)
{
	volatile OhciRegs_t *ohciRegs = (OhciRegs_t *) USB_OHCI_REGBASE;

	printf("Host Controller...\n");
	ohciRegs->HcInterruptDisable = ~0;
	ohciRegs->HcCommandStatus = OHCI_COM_HCR;
	ohciRegs->HcControl = 0;
	while (ohciRegs->HcCommandStatus & OHCI_COM_HCR) {
		// add timeout stuff
	}

	printf("HC reset done\n");

	/* The following lines will activate the OHCI chip. */
	dmac_set_dpcr2(dmac_get_dpcr2() | 0x08000000);
	*USB_REG_UNKNOWN1680 = 1;
}

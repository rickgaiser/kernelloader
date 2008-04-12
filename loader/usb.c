/* Copyright (c) 2007 Mega Man, copied code from usb driver in ps2sdk. */
#include "stdint.h"
#include "iopmem.h"
#include "usb.h"
#include "memory.h"

/** UB OHCI register base address. */
#define USB_OHCI_REGBASE 0xBF801600
#define USB_REG_UNKNOWN1570 ((volatile uint32_t *) 0xbf801570)
#define USB_REG_UNKNOWN1680 ((volatile uint16_t *) 0xbf801680)

#define BIT(x) (1 << (x))
#define OHCI_COM_HCR BIT(0)

typedef struct _hcTd {
	uint32_t HcArea;
	void   *curBufPtr;
	struct _hcTd *next;
	void   *bufferEnd;
} HcTD_t __attribute__ ((packed));

typedef struct _hcEd {
	uint16_t hcArea;
	uint16_t maxPacketSize;
	HcTD_t  *tdTail;
	HcTD_t  *tdHead;
	struct _hcEd *next;
} HcED_t __attribute__ ((packed));

typedef struct {
	volatile HcED_t   *InterruptTable[32];
	volatile uint16_t FrameNumber;
	volatile uint16_t pad;
	volatile HcTD_t   *DoneHead;
	volatile uint8_t  reserved[116];
	volatile uint32_t pad2; // expand struct to 256 bytes for alignment
} HcCA_t __attribute__ ((packed));

typedef struct {
	volatile uint32_t HcRevision;
	volatile uint32_t HcControl;
	volatile uint32_t HcCommandStatus;
	volatile uint32_t HcInterruptStatus;
	volatile uint32_t HcInterruptEnable;
	volatile uint32_t HcInterruptDisable;
	volatile HcCA_t  *HcHCCA;
	volatile HcED_t  *HcPeriodCurrentEd;
	volatile HcED_t  *HcControlHeadEd;
	volatile HcED_t  *HcControlCurrentEd;
	volatile HcED_t  *HcBulkHeadEd;
	volatile HcED_t  *HcBulkCurrentEd;
	volatile uint32_t HcDoneHead;
	volatile uint32_t HcFmInterval;
	volatile uint32_t HcFmRemaining;
	volatile uint32_t HcFmNumber;
	volatile uint32_t HcPeriodicStart;
	volatile uint32_t HcLsThreshold;
	volatile uint32_t HcRhDescriptorA;
	volatile uint32_t HcRhDescriptorB;
	volatile uint32_t HcRhStatus;
	volatile uint32_t HcRhPortStatus[2];
} OhciRegs_t __attribute__ ((packed));

/** Initialize USB for later use and reset OHCI chip. */
void initUSB(void)
{
	volatile OhciRegs_t *ohciRegs = (OhciRegs_t *) USB_OHCI_REGBASE;

	iop_prints(U2K("Host Controller...\n"));
	ohciRegs->HcInterruptDisable = ~0;
	ohciRegs->HcCommandStatus = OHCI_COM_HCR;
	ohciRegs->HcControl = 0;
	while (ohciRegs->HcCommandStatus & OHCI_COM_HCR) {
		// add timeout stuff
	}

	iop_prints(U2K("HC reset done\n"));

	/* The following lines will activate the OHCI chip. */
	*USB_REG_UNKNOWN1570 = *USB_REG_UNKNOWN1570 | 0x08000000;
	*USB_REG_UNKNOWN1680 = 1;
}

/*
 * Copyright (c) Tord Lindstrom (pukko@home.se)
 * Copyright (c) adresd ( adresd_ps2dev@yahoo.com )
 * Copyright 2009 Mega Man
 *
 * Based on ps2smap. Code changed for kernelloader.
 *
 * Branched from:
 *    svn://svn.ps2dev.org/ps2/trunk/ps2eth
 *    Revision: 1588
 *
 * Purpose: Forward ethernet frame between EE and IOP,
 *          because slim PSTwo doesn't have direct access
 *          to the IOP hardware.
 */

#include <stdio.h>
#include <sysclib.h>
#include <loadcore.h>
#include <thbase.h>
#include <thevent.h>
#include <thsemap.h>
#include <vblank.h>
#include <intrman.h>
#include <sifrpc.h>

#include "smap.h"
#include "dev9.h"
#include "smap_rpc.h"
#include "pbuf.h"
#include "main.h"

IRX_ID("smaprpc", 1, 0);

#define	UNKN_1464   *(u16 volatile*)0xbf801464

#define	IFNAME0	's'
#define	IFNAME1	'm'

#define	TIMER_INTERVAL		(100*1000)
#define	TIMEOUT				(300*1000)
#define	MAX_REQ_CNT			8
#define NR_OF_SMAP_RX_MSGS 16


static int		iSendMutex;
static int		iSendReqMutex;
static int		iSendReq=-1;
static int 		iReqNR=0;
static int 		iReqCNT=0;
static PBuf*	apReqQueue[MAX_REQ_CNT];
static int		iTimeoutCNT=0;

u32 ee_buffer;
u32 ee_buffer_pos;
u32 volatile ee_buffer_size = 0;


//From lwip/err.h and lwip/tcpip.h

#define	ERR_OK		0		//No error, everything OK
#define	ERR_CONN		-6		//Not connected
#define	ERR_IF		-11	//Low-level netif error


static void
StoreLast(PBuf* pBuf)
{

	//Store pBuf last in the request-queue.

	apReqQueue[(iReqNR+iReqCNT)%MAX_REQ_CNT]=pBuf;
	++iReqCNT;

	//Since pBuf won't be sent right away, increase the reference-count to prevent it from being deleted before it's sent.

	++(pBuf->ref);
}


static SMapStatus
AddToQueue(PBuf* pBuf)
{

	//Add pBuf to the request-queue.

	int			iIntFlags;
	SMapStatus	Ret;

	//Due to synchronization issues, disable the interrupts.

	CpuSuspendIntr(&iIntFlags);

	if	(iReqCNT==0)
	{

		//The queue is empty, try to send the packet right away.

		Ret=SMap_Send(pBuf);

		//Did a TX-resource exhaustion occur?

		if	(Ret==SMap_TX)
		{

			//Yes, store pBuf last in the queue so it's sent when there are enough TX-resources.

			StoreLast(pBuf);

			//Clear the timout-timer.

			iTimeoutCNT=0;

			//Set the return-value to SMap_OK to indicate that pBuf has been either sent or added to the queue.

			Ret=SMap_OK;
		}
		if	(Ret==SMap_OK)
		{
			pbuf_free(pBuf);
		}
	}
	else if	(iReqCNT<MAX_REQ_CNT)
	{

		//There queue isn't empty but there is atleast one free entry. Store pBuf last in the queue.

		StoreLast(pBuf);

		//Set the return-value to SMap_OK to indicate that pBuf has been either sent or added to the queue.

		Ret=SMap_OK;
	}
	else
	{

		//The queue is full, return SMap_TX to indicate that.

		Ret=SMap_TX;
	}

	//Restore the interrupts.

	CpuResumeIntr(iIntFlags);
	return	Ret;
}


static void
SendRequests(void)
{

	//This function should only be called from QueueHandler. It tries to send as many requests from the queue until there is an
	//TX-resource exhaustion.

	while	(iReqCNT>0)
	{

		//Retrieve the first request in the queue.

		PBuf*		pReq=apReqQueue[iReqNR];

		//Try to send the packet!

		SMapStatus	Status=SMap_Send(pReq);

		//Clear the timout-timer.

		iTimeoutCNT=0;

		//Did a TX-resource exhaustion occur?

		if	(Status==SMap_TX)
		{

			//Yes, we'll try to re-send the packet the next time a TX-interrupt occur, exit!

			return;
		}

		//No resource-exhaustion occured. Regardless if the packet was successfully sent or nor, process the next request. If it's
		//an important package and ps2ip won't receive an ack, it'll resend the package. We are done with pReq and should invoke
		//pbuf_free to decrease the ref-count.

		pbuf_free(pReq);

		//pReq has been sent, advance the queue-index one step.

		iReqNR=(iReqNR+1)%MAX_REQ_CNT;
		--iReqCNT;
	}
}


static void
QueueHandler(void)
{

	//This function should only be called from an interrupt-context. It tries to send as many requests as possible from the queue
	//and signals any waiting thread if the queue becomes non-full. Start with trying to send the reqs.

	SendRequests();

	//Is there a thread waiting for the queue to become non-full?

	if	(iSendReq!=-1)
	{

		//Yes, send it a signal that it should try to add to the queue now.

		iSignalSema(iSendReq);
		iSendReq=-1;
	}
}


static int 
SMapInterrupt(int iFlag)
{
	int	iFlags=SMap_GetIRQ();

	if	(iFlags&(INTR_TXDNV|INTR_TXEND))
	{

		//It's a TX-interrupt, handle it now!

		SMap_HandleTXInterrupt(iFlags&(INTR_TXDNV|INTR_TXEND));

		//Handle the request-queue.

		QueueHandler();

		//Several packets might have been sent during QueueHandler and we might have spent a couple of 1000 usec. Re-read the
		//interrupt-flags to more accurately reflect the current interrupt-status.

		iFlags=SMap_GetIRQ();
	}

	if	(iFlags&(INTR_EMAC3|INTR_RXDNV|INTR_RXEND))
	{

		//It's a RX- or a EMAC-interrupt, handle it!

		SMap_HandleRXEMACInterrupt(iFlags&(INTR_EMAC3|INTR_RXDNV|INTR_RXEND));
	}
	return	1;
}


static unsigned int 
Timer(void* pvArg)
{

	//Are there any requests in the queue?

	if	(iReqCNT==0)
	{

		//No, exit!

		return	(unsigned int)pvArg;
	}

	//Yes, If no TX-interrupt has occured for the last TIMEOUT usec, process the request-queue.

	iTimeoutCNT+=TIMER_INTERVAL;
	if	(iTimeoutCNT>=TIMEOUT&&iTimeoutCNT<(TIMEOUT+TIMER_INTERVAL))
	{

		//Timeout, handle the request-queue.

		QueueHandler();
	}
	return	(unsigned int)pvArg;
}


static void
InstallIRQHandler(void)
{

	//Install the SMap interrupthandler for all of the SMap-interrupts.

	int	iA;

	for	(iA=2;iA<7;++iA)
	{
		dev9RegisterIntrCb(iA,SMapInterrupt);
	}
}


static void
InstallTimer(void)
{
	iop_sys_clock_t	ClockTicks;

	USec2SysClock(TIMER_INTERVAL,&ClockTicks);
	SetAlarm(&ClockTicks,Timer,(void*)ClockTicks.lo);
}


static void
DetectAndInitDev9(void)
{
	SMap_DisableInterrupts(INTR_BITMSK);
	EnableIntr(IOP_IRQ_DEV9);
	CpuEnableIntr();

	UNKN_1464=3;
}


static err_t
Send(PBuf* pBuf)
{

	//Send the packet in pBuf.

	while	(1)
	{
		int			iFlags;
		SMapStatus	Res;

		//Try to add the packet to the request-queue.

		if	((Res=AddToQueue(pBuf))==SMap_OK)
		{

			//The packet was sent successfully or added to the queue, return ERR_OK.

			return	ERR_OK;
		}
		else if	(Res==SMap_Err)
		{

			//SMap_Send wasn't able to send the packet due to some hardware limitation, return ERR_IF to indicate that.

			return	ERR_IF;
		}
		else if	(Res==SMap_Con)
		{

			//SMap_Send wasn't able to send the packet due to not being connected, return ERR_CONN to indicate that.

			return	ERR_CONN;
		}

		//The request-queue is full. If iSendReq is assigned a mutex-id when an TX-interrupt occur, SMapInterrupt will signal that
		//mutex when there is room in the queue. Due to synchronization issues, we must first acquire the iSendMutex before we can
		//assign iSendReq.

		WaitSema(iSendMutex);

		//There is a possibility that the queue has become non-full during the acquisition of the mutex. Verify that it still is
		//full.

		CpuSuspendIntr(&iFlags);
		if	(iReqCNT==MAX_REQ_CNT)
		{

			//It's still full. Assign iSendReqMutex to iSendReq.

			iSendReq=iSendReqMutex;
			CpuResumeIntr(iFlags);

			//Wait for the SMapInterrupt to signal us.

			WaitSema(iSendReqMutex);
		}
		else
		{

			//The queue isn't full anymore, try to add pBuf to it.

			CpuResumeIntr(iFlags);
		}

		//Release the iSendMutex

		SignalSema(iSendMutex);
	}
}


//SMapLowLevelOutput():

//This function is called by the TCP/IP stack when a low-level packet should be sent. It'll be invoked in the context of the
//tcpip-thread.

err_t SMapLowLevelOutput(PBuf* pOutput)
{
	return	Send(pOutput);
}


//SMapIFInit():

//Should be called at the beginning of the program to set up the network interface.

static err_t
SMapIFInit(void)
{
	//Enable sending and receiving of data.

	SMap_Start();
	return	ERR_OK;
}


void
SMapLowLevelInput(PBuf* pBuf)
{
	//When we receive data, the interrupt-handler will invoke this function, which means we are in an interrupt-context. Pass on
	//the received data to EE.
	
	if (pBuf != NULL) {
		u8 *data = pBuf->payload;

		if (data != NULL) {

			if (ee_buffer_size > 0) {
				static smap_message_t smap_msg[NR_OF_SMAP_RX_MSGS] __attribute__((aligned(64)));
				static int nr = 0;
				int ret;

				if ((ee_buffer_pos + pBuf->tot_len) > ee_buffer_size) {
					ee_buffer_pos = 0;
				}
				smap_msg[nr].payload = ee_buffer + ee_buffer_pos;
				smap_msg[nr].size = pBuf->tot_len;
				ret = sceSifSendCmd(SIF_SMAP_RECEIVE, &smap_msg[nr], sizeof(smap_msg[nr]), data, (void *) smap_msg[nr].payload, smap_msg[nr].size);
				if (ret == 0) {
					printf("Failed to send message (ret = %d).\n", ret);
				}
				pBuf->id = ret;
				ee_buffer_pos = (ee_buffer_pos + pBuf->tot_len + DMA_TRANSFER_SIZE - 1) & ~(DMA_TRANSFER_SIZE - 1);
				nr = (nr + 1) % NR_OF_SMAP_RX_MSGS;
			} else {
				printf("Loosing ethernet frame. No EE rx buffer.\n");
			}
		} else {
			printf("Receive data error.\n");
		}
	} else {
		printf("Receive pBuf error.\n");
	}
}


static int
SMapInit(void)
{
	DetectAndInitDev9();
	dbgprintf("SMapInit: Dev9 detected & initialized\n");

	if	((iSendMutex=CreateMutex(IOP_MUTEX_UNLOCKED))<0)
	{
		printf("SMapInit: Fatal error - unable to create iSendMutex\n");
		return	0;
	}

	if	((iSendReqMutex=CreateMutex(IOP_MUTEX_UNLOCKED))<0)
	{
		printf("SMapInit: Fatal error - unable to create iSendReqMutex\n");
		return	0;
	}

	if	(!SMap_Init())
	{
		return	0;
	}
	dbgprintf("SMapInit: SMap initialized\n");

	InstallIRQHandler();
	dbgprintf("SMapInit: Interrupt-handler installed\n");

	InstallTimer();
	dbgprintf("SMapInit: Timer installed\n");

	SMapIFInit();
	dbgprintf("SMapInit: SMapIFInit\n");

	//Return 1 (true) to indicate success.

	return	1;
}


int
_start(int iArgC,char** ppcArgV)
{
	if	(!SMapInit())
	{

		//Something went wrong, return -1 to indicate failure.

		return	-1;
	}

	printf("SMap: Initialized OK\n");

	//Initialized ok.
	rpc_start();

	return	0;
}

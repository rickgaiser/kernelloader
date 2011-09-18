/*
# _____     ___ ____     ___ ____
#  ____|   |    ____|   |        | |____|
# |     ___|   |____ ___|    ____| |    \    PS2DEV Open Source Project.
#-----------------------------------------------------------------------
# Copyright 2001-2004, ps2dev - http://www.ps2dev.org
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.
#
# (c) 2007 Mega Man
#
# $Id$
# Function defenitions for mclib.
*/

#include "tge_types.h"
#include "tge_defs.h"

#include "tge_sbios.h"
#include "tge_sifdma.h"

#include "core.h"

#include "sif.h"
#include "sifdma.h"
#include "sifcmd.h"
#include "sifrpc.h"
#include "mc.h"
#include "sbcalls.h"
#include "ps2lib_err.h"
#include "iopmemdebug.h"
#include "stdio.h"
#include "string.h"
#include "smod.h"

//#define MC_DEBUG

#define MC_INITIALIZED 2

#ifdef MC_DEBUG
#include <stdio.h>
#endif


// rpc command function numbers
#define MC_RPCCMD_INIT		0x00
#define MC_RPCCMD_GET_INFO	0x01
#define MC_RPCCMD_OPEN		0x02
#define MC_RPCCMD_CLOSE		0x03
#define MC_RPCCMD_SEEK		0x04
#define MC_RPCCMD_READ		0x05
#define MC_RPCCMD_WRITE		0x06
#define MC_RPCCMD_FLUSH		0x07
#define MC_RPCCMD_CH_DIR	0x08
#define MC_RPCCMD_GET_DIR	0x09
#define MC_RPCCMD_SET_INFO	0x0A
#define MC_RPCCMD_DELETE	0x0B
#define MC_RPCCMD_FORMAT	0x0C
#define MC_RPCCMD_UNFORMAT	0x0D
#define MC_RPCCMD_GET_ENT	0x0E
#define MC_RPCCMD_CHG_PRITY	0x0F
#define MC_RPCCMD_UNKNOWN_1	0x10

// rpc command function numbers
// mcRpcCmd[MC_TYPE_??][MC_RPCCMD_???]
static const int mcRpcCmd[2][17] =
{
	// MCMAN/MCSERV values
	{
	0x70,	// MC_RPCCMD_INIT
	0x78,	// MC_RPCCMD_GET_INFO
	0x71,	// MC_RPCCMD_OPEN
	0x72,	// MC_RPCCMD_CLOSE
	0x75,	// MC_RPCCMD_SEEK
	0x73,	// MC_RPCCMD_READ
	0x74,	// MC_RPCCMD_WRITE
	0x7A,	// MC_RPCCMD_FLUSH
	0x7B,	// MC_RPCCMD_CH_DIR
	0x76,	// MC_RPCCMD_GET_DIR
	0x7C,	// MC_RPCCMD_SET_INFO
	0x79,	// MC_RPCCMD_DELETE
	0x77,	// MC_RPCCMD_FORMAT
	0x80,	// MC_RPCCMD_UNFORMAT
	0x7E,	// MC_RPCCMD_GET_ENT
	0x7D,	// MC_RPCCMD_UNKNOWN_1 (calls mcman_funcs: 39, 17, 20, 19, 30)
	0x7F,	// MC_RPCCMD_UNKNOWN_2 (calls mcman_funcs: 20, 19)
	},
	// XMCMAN/XMCSERV values
	{
	0xFE,	// MC_RPCCMD_INIT
	0x01,	// MC_RPCCMD_GET_INFO
	0x02,	// MC_RPCCMD_OPEN
	0x03,	// MC_RPCCMD_CLOSE
	0x04,	// MC_RPCCMD_SEEK
	0x05,	// MC_RPCCMD_READ
	0x06,	// MC_RPCCMD_WRITE
	0x0A,	// MC_RPCCMD_FLUSH
	0x0C,	// MC_RPCCMD_CH_DIR
	0x0D,	// MC_RPCCMD_GET_DIR
	0x0E,	// MC_RPCCMD_SET_INFO
	0x0F,	// MC_RPCCMD_DELETE
	0x10,	// MC_RPCCMD_FORMAT
	0x11,	// MC_RPCCMD_UNFORMAT
	0x12,	// MC_RPCCMD_GET_ENT
	0x14,	// MC_RPCCMD_CHG_PRITY
	0x33,	// MC_RPCCMD_UNKNOWN_1 (calls xmcman_funcs: 45)
	}
};

// client data
static SifRpcClientData_t cdata __attribute__((aligned(64)));

// receive data
#define RSIZE 2048
static u8 rdata[RSIZE] __attribute__((aligned(64)));

static int* typeAddr;
static int* freeAddr;
static int* formatAddr;

static int  endParameter[48];
//static char curDir[1024];
static char buffFileInfo[64];

static struct {		// size = 1044
	int port;		// 0
	int slot;		// 4
	int flags;		// 8
	int maxent;		// 12
	int table;		// 16
	char name[1024];// 20
} mcCmd __attribute__((aligned(64)));

static struct {		// size = 48
	int fd;			// 0
	int pad1;		// 4
	int pad2;		// 8
	int size;		// 12
	int offset;		// 16
	int origin;		// 20
	int buffer;		// 24
	int param;		// 28
	char data[16];	// 32
} mcFileCmd __attribute__((aligned(64)));

static unsigned int mcInfoCmd[12] __attribute__((aligned(64)));

// whether or not mc lib has been inited
static int mcInitializeCounter = 0;

// stores last command executed
static unsigned int lastCmd = 0;

// specifies whether using MCSERV or XMCSERV modules
static int mcType = -1;

static void mcCallback(void* rarg)
{
	tge_sbcall_rpc_arg_t *carg = (tge_sbcall_rpc_arg_t *) rarg;

	/* End of transaction. */
	lastCmd = 0;

	carg->result = *(int*)rdata;
	carg->endfunc(carg->efarg, carg->result);
}



// function that gets called when mcGetInfo ends
// and interrupts are disabled
static void mcGetInfoApdx(void *rarg)
{
	tge_sbcall_rpc_arg_t *carg = (tge_sbcall_rpc_arg_t *) rarg;
	volatile int *endp = (volatile int *) KSEG1ADDR(endParameter);
	
	if(typeAddr		!= NULL)
	{
		*typeAddr	= endp[0];
	}
	
	if(freeAddr		!= NULL)
	{
		*freeAddr	= endp[1];
	}
	
	if(formatAddr	!= NULL)
	{
		// if using old mcman, make it always return 'formatted'
		if(mcType == MC_TYPE_MC)
		{
			if(endp[0] == 0)
				*formatAddr	= 0;
			else
				*formatAddr	= 1;
		}
		else if(mcType == MC_TYPE_XMC)
		{
			*formatAddr	= endp[36];
		}
	}
	/* End of transaction. */
	lastCmd = 0;

	carg->result = *(int*)rdata;
	carg->endfunc(carg->efarg, carg->result);
}

// function that gets called when mcRead ends
// and interrupts are disabled
static void mcReadFixAlign(void *rarg)
{
	tge_sbcall_rpc_arg_t *carg = (tge_sbcall_rpc_arg_t *) rarg;
	volatile int* data_raw = endParameter;
	int	*pkt = (u32*)UNCACHED_SEG(data_raw);
	u8	*src, *dest;
	int  i;
	
	dest = (u8*)pkt[2];
	src = (u8 *)&pkt[4];
	if(dest)
	{
		for(i=0; i<pkt[0]; i++)
		{
			dest[i] = src[i];
		}
	}
	
	dest = (u8*)pkt[3];
	if(mcType == MC_TYPE_MC)
		src = (u8 *)&pkt[8];
	else if(mcType == MC_TYPE_XMC)
		src = (u8 *)&pkt[20];
	if(dest)
	{
		for(i=0; i<pkt[1]; i++)
		{
			dest[i] = src[i];
		}
	}
	mcCallback(carg);
}

#if 0
// function that gets called when mcChDir ends
// and interrupts are disabled
static void mcStoreDir(volatile int* rarg)
{
	tge_sbcall_rpc_arg_t *carg = (tge_sbcall_rpc_arg_t *) rarg;
	int len;
	int currentDir = (int)curDir | 0x20000000;
	len = strlen((char*)currentDir);
	if(len >= 1024)
		len = strlen((char*)(currentDir+1023));
	memcpy((void*)data->optArg, (void*)currentDir, len);
	*(u8*)(currentDir+len) = 0;
	mcCallback(carg);
}
#endif

void mcInitCallback(void *rarg)
{
	tge_sbcall_rpc_arg_t *carg = (tge_sbcall_rpc_arg_t *) rarg;

	switch(mcInitializeCounter) {
	case 0:
		if (cdata.server == NULL) {
			/* Failed. */
			carg->result = -1;
			break;
		}
		mcInitializeCounter++;
		carg->result = 0;

		// for some reason calling this init sif function with 'mcserv' makes all other
		// functions not work properly. although NOT calling it means that detecting
		// whether or not cards are formatted doesnt seem to work :P
		if(mcType == MC_TYPE_MC)
		{
#ifdef MC_DEBUG
			printf("libmc: using MCMAN & MCSERV\n");
#endif
		} else {
			break;
		}

	case 1:
		if(mcType == MC_TYPE_XMC)
		{
			// check if old version of mcserv loaded
			if(*(s32*)UNCACHED_SEG(rdata+4) < 0x200)
			{
#ifdef MC_DEBUG
				printf("libmc: mcserv is too old (%x)\n", *(s32*)UNCACHED_SEG(rdata+4));
#endif
				carg->result = -120;
				carg->endfunc(carg->efarg, carg->result);
				return;
			}
		
			// check if old version of mcman loaded
			if(*(s32*)UNCACHED_SEG(rdata+8) < 0x200)
			{
#ifdef MC_DEBUG
				printf("libmc: mcman is too old (%x)\n", *(s32*)UNCACHED_SEG(rdata+8));
#endif
				carg->result = -121;
				carg->endfunc(carg->efarg, carg->result);
				return;
			}
			carg->result = *(s32*)UNCACHED_SEG(rdata+0);
		} else {
			carg->result = 1;
		}
		// successfully inited
		lastCmd = 0;
		mcInitializeCounter++;
		break;

	}
	carg->endfunc(carg->efarg, carg->result);
}

// init memcard lib
// 
// args:	MC_TYPE_MC  = use MCSERV/MCMAN
//			MC_TYPE_XMC = use XMCSERV/XMCMAN
// returns:	0   = successful
//			< 0 = error
int mcInit(tge_sbcall_rpc_arg_t *carg)
{
	int ret=0;

#if 0
	if	(mcType == MC_TYPE_RESET)
	{
		mclibInited = 0;
		cdata.server = NULL;
		return 0;
	}
#endif

	switch(mcInitializeCounter) {
	case 0:
		// bind to mc rpc on iop
		if((ret=SifBindRpc(&cdata, 0x80000400, SIF_RPC_M_NOWAIT, mcInitCallback, carg)) < 0)
		{
#ifdef MC_DEBUG
				printf("libmc: bind error\n");
#endif
			
			return -SIF_RPCE_SENDP;
		}
		break;

	case 1:
		if(mcType == MC_TYPE_XMC)
		{
#ifdef MC_DEBUG
			printf("libmc: using XMCMAN & XMCSERV\n");
#endif		
			// call init function
			if((ret = SifCallRpc(&cdata, mcRpcCmd[mcType][MC_RPCCMD_INIT], SIF_RPC_M_NOWAIT, &mcCmd, 48, rdata, 12, mcInitCallback, carg)) < 0)
			{
				// init error
#ifdef MC_DEBUG
				printf("libmc: initialisation error\n");
#endif
				return -SIF_RPCE_SENDP;
			}
		}
		break;
		
	default:
		//printf("mcInit: already initialized.\n");
		carg->result = 1;
		carg->endfunc(carg->efarg, carg->result);
		break;
	}
	
	
	return ret;
}

// get memcard state
// mcSync result:	 0 = same card as last getInfo call
//					-1 = formatted card inserted since last getInfo call
//					-2 = unformatted card inserted since last getInfo call
//					< -2 = memcard access error (could be due to accessing psx memcard)
// 
// args:    port number
//          slot number
//          pointer to get memcard type
//          pointer to get number of free clusters
//          pointer to get whether or not the card is formatted
// returns:	0   = successful
//			< 0 = error
int sbcall_mcgetinfo(tge_sbcall_rpc_arg_t *carg)
{
	tge_sbcall_mcgetinfo_arg_t *arg = carg->sbarg;
	int ret;
	u32 status;
	
	// check mc lib is inited
	if(mcInitializeCounter != MC_INITIALIZED) {
		return -1;
	}
	// check nothing else is processing
	core_save_disable(&status);
	if(lastCmd != 0) {
		core_restore(status);
		return -SIF_RPCE_SENDP;
	}
	lastCmd = MC_FUNC_GET_INFO;
	core_restore(status);
	
	// set global variables
	if(mcType == MC_TYPE_MC)
	{
		mcInfoCmd[0x01] = arg->port;
		mcInfoCmd[0x02] = arg->slot;
		mcInfoCmd[0x03] = (arg->type)	? 1 : 0;
		mcInfoCmd[0x04] = (arg->free)	? 1 : 0;
		mcInfoCmd[0x05] = (arg->format)	? 1 : 0;
		mcInfoCmd[0x07] = (int)endParameter;
	}
	else
	{
		mcInfoCmd[0x01] = arg->port;
		mcInfoCmd[0x02] = arg->slot;
		mcInfoCmd[0x03] = (arg->format)	? 1 : 0;
		mcInfoCmd[0x04] = (arg->free)	? 1 : 0;
		mcInfoCmd[0x05] = (arg->type)	? 1 : 0;
		mcInfoCmd[0x07] = (int)endParameter;
	}
	typeAddr	= arg->type;
	freeAddr	= arg->free;
	formatAddr	= arg->format;
	SifWriteBackDCache(endParameter,192);

	// send sif command
	if((ret = SifCallRpc(&cdata, mcRpcCmd[mcType][MC_RPCCMD_GET_INFO], 1, mcInfoCmd, 48, rdata, 4, mcGetInfoApdx, carg)) != 0) {
		lastCmd = 0;
		return -SIF_RPCE_SENDP;
	}
	return ret;
}

// open a file on memcard
// mcSync returns:	0 or more = file descriptor (success)
//					< 0 = error
// 
// args:	port number
//			slot number
//			filename to open
//			open file mode (O_RDWR, O_CREAT, etc)
// returns:	0   = successful
//			< 0 = error
int sbcall_mcopen(tge_sbcall_rpc_arg_t *carg)
{
	tge_sbcall_mcopen_arg_t *arg = carg->sbarg;
	int ret;
	u32 status;
	
	// check mc lib is inited
	if(mcInitializeCounter != MC_INITIALIZED) {
		return -1;
	}
	// check nothing else is processing
	core_save_disable(&status);
	if(lastCmd != 0) {
		core_restore(status);
		return -SIF_RPCE_SENDP;
	}
	lastCmd = MC_FUNC_OPEN;
	core_restore(status);
	
	// set global variables
	mcCmd.port		= arg->port;
	mcCmd.slot		= arg->slot;
	mcCmd.flags		= arg->mode;
	strncpy(mcCmd.name, arg->name, 1023);
	mcCmd.name[1023] = 0;
	
	// send sif command
	if((ret = SifCallRpc(&cdata, mcRpcCmd[mcType][MC_RPCCMD_OPEN], 1, &mcCmd, 0x414, rdata, 4, mcCallback, carg)) != 0) {
		lastCmd = 0;
		return -SIF_RPCE_SENDP;
	}
	return ret;
}

// close an open file on memcard
// mcSync returns:	0 if closed successfully
//					< 0 = error
// 
// args:	file descriptor of open file
// returns:	0   = successful
//			< 0 = error
int sbcall_mcclose(tge_sbcall_rpc_arg_t *carg)
{
	tge_sbcall_mcclose_arg_t *arg = carg->sbarg;
	int ret;
	u32 status;
	
	// check mc lib is inited
	if(mcInitializeCounter != MC_INITIALIZED) {
		return -1;
	}
	// check nothing else is processing
	core_save_disable(&status);
	if(lastCmd != 0) {
		core_restore(status);
		return -SIF_RPCE_SENDP;
	}
	lastCmd = MC_FUNC_CLOSE;
	core_restore(status);
	
	// set global variables
	mcFileCmd.fd	= arg->fd;
	
	// send sif command
	if((ret = SifCallRpc(&cdata, mcRpcCmd[mcType][MC_RPCCMD_CLOSE], 1, &mcFileCmd, 48, rdata, 4, mcCallback, carg)) != 0) {
		lastCmd = 0;
		return -SIF_RPCE_SENDP;
	}
	return ret;
}

// move memcard file pointer
// mcSync returns:	0 or more = offset of file pointer from start of file
//					< 0 = error
// 
// args:	file descriptor
//			number of bytes from origin
//			initial position for offset
// returns:	0   = successful
//			< 0 = error
int sbcall_mcseek(tge_sbcall_rpc_arg_t *carg)
{
	tge_sbcall_mcseek_arg_t *arg = carg->sbarg;
	int ret;
	u32 status;
	
	// check mc lib is inited
	if(mcInitializeCounter != MC_INITIALIZED) {
		return -1;
	}
	// check nothing else is processing
	core_save_disable(&status);
	if(lastCmd != 0) {
		core_restore(status);
		return -SIF_RPCE_SENDP;
	}
	lastCmd = MC_FUNC_SEEK;
	core_restore(status);
	
	// set global variables
	mcFileCmd.fd		= arg->fd;
	mcFileCmd.offset	= arg->offset;
	mcFileCmd.origin	= arg->whence;
	
	// send sif command
	if((ret = SifCallRpc(&cdata, mcRpcCmd[mcType][MC_RPCCMD_SEEK], 1, &mcFileCmd, 48, rdata, 4, mcCallback, carg)) != 0) {
		lastCmd = 0;
		return -SIF_RPCE_SENDP;
	}
	return ret;
}

// read from file on memcard
// mcSync returns:	0 or more = number of bytes read from memcard
//					< 0 = error
// 
// args:	file descriptor
//			buffer to read to
//			number of bytes to read
// returns:	0   = successful
//			< 0 = error
int sbcall_mcread(tge_sbcall_rpc_arg_t *carg)
{
	tge_sbcall_mcread_arg_t *arg = carg->sbarg;
	int ret;
	u32 status;
	
	// check mc lib is inited
	if(mcInitializeCounter != MC_INITIALIZED) {
		return -1;
	}
	// check nothing else is processing
	core_save_disable(&status);
	if(lastCmd != 0) {
		core_restore(status);
		return -SIF_RPCE_SENDP;
	}
	lastCmd = MC_FUNC_READ;
	core_restore(status);
	
	// set global variables
	mcFileCmd.fd	= arg->fd;
	mcFileCmd.size	= arg->size;
	mcFileCmd.buffer= (int) arg->buf;
	mcFileCmd.param	= (int)endParameter;
	SifWriteBackDCache(arg->buf, arg->size);
	SifWriteBackDCache(endParameter,192);
	
	// send sif command
	if((ret = SifCallRpc(&cdata, mcRpcCmd[mcType][MC_RPCCMD_READ], 1, &mcFileCmd, 48, rdata, 4, mcReadFixAlign, carg)) != 0) {
		lastCmd = 0;
		return -SIF_RPCE_SENDP;
	}
	return ret;
}

// write to file on memcard
// mcSync returns:	0 or more = number of bytes written to memcard
//					< 0 = error
// 
// args:	file descriptor
//			buffer to write from write
// returns:	0   = successful
//			< 0 = error
int sbcall_mcwrite(tge_sbcall_rpc_arg_t *carg)
{
	tge_sbcall_mcwrite_arg_t *arg = carg->sbarg;
	int i, ret;
	u32 status;
	
	// check mc lib is inited
	if(mcInitializeCounter != MC_INITIALIZED) {
		return -1;
	}
	// check nothing else is processing
	core_save_disable(&status);
	if(lastCmd != 0) {
		core_restore(status);
		return -SIF_RPCE_SENDP;
	}
	lastCmd = MC_FUNC_WRITE;
	core_restore(status);
	
	// set global variables
	mcFileCmd.fd	= arg->fd;
	if(arg->size < 17)
	{
		mcFileCmd.size		= 0;
		mcFileCmd.origin	= arg->size;
		mcFileCmd.buffer	= 0;
	}
	else
	{
		mcFileCmd.size		= arg->size   -   ( ((int)(arg->buf-1) & 0xFFFFFFF0) - (int)(arg->buf-16) );
		mcFileCmd.origin	=                 ( ((int)(arg->buf-1) & 0xFFFFFFF0) - (int)(arg->buf-16) );
		mcFileCmd.buffer	= (int)arg->buf + ( ((int)(arg->buf-1) & 0xFFFFFFF0) - (int)(arg->buf-16) );
	}
	for(i=0; i<mcFileCmd.origin; i++)
	{
		mcFileCmd.data[i] = *(char*)(arg->buf+i);
	}
	FlushCache(0);
	
	// send sif command
	if((ret = SifCallRpc(&cdata, mcRpcCmd[mcType][MC_RPCCMD_WRITE], 1, &mcFileCmd, 48, rdata, 4, mcCallback, carg)) != 0) {
		lastCmd = 0;
		return -SIF_RPCE_SENDP;
	}
	return ret;
}

// flush file cache to memcard
// mcSync returns:	0 if ok
//					< 0 if error
// 
// args:	file descriptor
// returns:	0   = successful
//			< 0 = error
int sbcall_mcflush(tge_sbcall_rpc_arg_t *carg)
{
	tge_sbcall_mcflush_arg_t *arg = carg->sbarg;
	int ret;
	u32 status;
	
	// check mc lib is inited
	if(mcInitializeCounter != MC_INITIALIZED) {
		return -1;
	}
	// check nothing else is processing
	core_save_disable(&status);
	if(lastCmd != 0) {
		core_restore(status);
		return -SIF_RPCE_SENDP;
	}
	lastCmd = MC_FUNC_FLUSH;
	core_restore(status);
	
	// set global variables
	mcFileCmd.fd	= arg->fd;
	
	// send sif command
	if((ret = SifCallRpc(&cdata, mcRpcCmd[mcType][MC_RPCCMD_FLUSH], 1, &mcFileCmd, 48, rdata, 4, mcCallback, carg)) != 0) {
		lastCmd = 0;
		return -SIF_RPCE_SENDP;
	}
	return ret;
}

// create a dir
// mcSync returns:	0 if ok
//					< 0 if error
// 
// args:	port number
//			slot number
//			directory name
// returns:	0   = successful
//			< 0 = error
int sbcall_mcmkdir(tge_sbcall_rpc_arg_t *carg)
{
	tge_sbcall_mcmkdir_arg_t *arg = carg->sbarg;
	int ret;
	u32 status;
	
	// check mc lib is inited
	if(mcInitializeCounter != MC_INITIALIZED) {
		return -1;
	}
	// check nothing else is processing
	core_save_disable(&status);
	if(lastCmd != 0) {
		core_restore(status);
		return -SIF_RPCE_SENDP;
	}
	lastCmd = MC_FUNC_MK_DIR;
	core_restore(status);
	
	// set global variables
	mcCmd.port		= arg->port;
	mcCmd.slot		= arg->slot;
	mcCmd.flags		= 0x40;
	strncpy(mcCmd.name, arg->name, 1023);
	mcCmd.name[1023] = 0;
	
	// send sif command
	if((ret = SifCallRpc(&cdata, mcRpcCmd[mcType][MC_RPCCMD_OPEN], 1, &mcCmd, 0x414, rdata, 4, mcCallback, carg)) != 0) {
		lastCmd = 0;
		return -SIF_RPCE_SENDP;
	}
	return ret;
}

#if 0
// change current dir
// (can also get current dir)
// mcSync returns:	0 if ok
//					< 0 if error
// 
// args:	port number
//			slot number
//			new dir to change to
//			buffer to get current dir (use 0 if not needed)
// returns:	0   = successful
//			< 0 = error
int mcChdir(int port, int slot, const char* newDir, char* currentDir, SifRpcEndFunc_t endfunc, void *efarg, int *result)
{
	int ret;
	u32 status;
	
	// check mc lib is inited
	if(mcInitializeCounter != MC_INITIALIZED) {
		return -1;
	}
	// check nothing else is processing
	core_save_disable(&status);
	if(lastCmd != 0) {
		core_restore(status);
		return -SIF_RPCE_SENDP;
	}
	lastCmd = MC_FUNC_CH_DIR;
	core_restore(status);
	
	// set global variables
	mcCmd.port		= port;
	mcCmd.slot		= slot;
	mcCmd.table		= (int)curDir;
	strncpy(mcCmd.name, newDir, 1023);
	mcCmd.name[1023] = 0;
	SifWriteBackDCache(curDir,1024);
	
	// send sif command
	data.optArg = currentDir;
	if((ret = SifCallRpc(&cdata, mcRpcCmd[mcType][MC_RPCCMD_CH_DIR], 1, &mcCmd, 0x414, rdata, 4, (SifRpcEndFunc_t)mcStoreDir, &data)) != 0) {
		lastCmd = 0;
		return -SIF_RPCE_SENDP;
	}
	return ret;
}
#endif

// get memcard filelist
// mcSync result:	 0 or more = number of file entries obtained (success)
//					-2 = unformatted card
//					-4 = dirname error
// 
// args:    port number of memcard
//          slot number of memcard
//          filename to search for (can use wildcard and relative dirs)
//          mode: 0 = first call, otherwise = followup call
//          maximum number of entries to be written to filetable in 1 call
//          mc table array
// returns:	0   = successful
//			< 0 = error
int sbcall_mcgetdir(tge_sbcall_rpc_arg_t *carg)
{
	tge_sbcall_mcgetdir_arg_t *arg = carg->sbarg;
	int ret;
	u32 status;
	
	// check mc lib is inited
	if(mcInitializeCounter != MC_INITIALIZED) {
		return -1;
	}
	// check nothing else is processing
	core_save_disable(&status);
	if(lastCmd != 0) {
		core_restore(status);
		return -SIF_RPCE_SENDP;
	}
	lastCmd = MC_FUNC_GET_DIR;
	core_restore(status);
	
	// set global variables
	mcCmd.port		= arg->port;
	mcCmd.slot		= arg->slot;
	mcCmd.flags		= arg->mode;
	mcCmd.maxent	= arg->max_entries;
	mcCmd.table		= (int)arg->dirents;
	strncpy(mcCmd.name, arg->name, 1023);
	mcCmd.name[1023] = 0;
	SifWriteBackDCache(arg->dirents, arg->max_entries * sizeof(mcTable));
	
	// send sif command
	if((ret = SifCallRpc(&cdata, mcRpcCmd[mcType][MC_RPCCMD_GET_DIR], 1, &mcCmd, 0x414, rdata, 4, mcCallback, carg)) != 0) {
		lastCmd = 0;
		return -SIF_RPCE_SENDP;
	}
   	return ret;
}

// change file information
// mcSync returns:	0 if ok
//					< 0 if error
// 
// args:	port number
//			slot number
//			filename to access
//			data to be changed
//			flags to show which data is valid
// returns:	0   = successful
//			< 0 = error
int sbcall_mcsetfileinfo(tge_sbcall_rpc_arg_t *carg)
{
	tge_sbcall_mcsetfileinfo_arg_t *arg = carg->sbarg;
	int ret;
	u32 status;
	
	// check mc lib is inited
	if(mcInitializeCounter != MC_INITIALIZED) {
		return -1;
	}
	// check nothing else is processing
	core_save_disable(&status);
	if(lastCmd != 0) {
		core_restore(status);
		return -SIF_RPCE_SENDP;
	}
	lastCmd = MC_FUNC_SET_INFO;
	core_restore(status);
	
	// set global variables
	mcCmd.port		= arg->port;
	mcCmd.slot		= arg->slot;
	mcCmd.flags		= arg->modes;	// NOTE: this was ANDed with 7 so that u cant turn off copy protect! :)
	mcCmd.table		= (int)buffFileInfo;
	// copy info to buffFileInfo
	// (usually uses ldr/ldl sdr/sdl to copy)
	memcpy(buffFileInfo, arg->dirent, 64);
	
	strncpy(mcCmd.name, arg->name, 1023);
	mcCmd.name[1023] = 0;
	FlushCache(0);
	
	// send sif command
	if((ret = SifCallRpc(&cdata, mcRpcCmd[mcType][MC_RPCCMD_SET_INFO], 1, &mcCmd, 0x414, rdata, 4, mcCallback, carg)) != 0) {
		lastCmd = 0;
		return -SIF_RPCE_SENDP;
	}
	return ret;
}

// delete file
// mcSync returns:	0 if deleted successfully
//					< 0 if error
// 
// args:	port number to delete from
//			slot number to delete from
//			filename to delete
// returns:	0   = successful
//			< 0 = error
int sbcall_mcdelete(tge_sbcall_rpc_arg_t *carg)
{
	tge_sbcall_mcdelete_arg_t *arg = carg->sbarg;
	int ret;
	u32 status;
	
	// check mc lib is inited
	if(mcInitializeCounter != MC_INITIALIZED) {
		return -1;
	}
	// check nothing else is processing
	core_save_disable(&status);
	if(lastCmd != 0) {
		core_restore(status);
		return -SIF_RPCE_SENDP;
	}
	lastCmd = MC_FUNC_DELETE;
	core_restore(status);
	
	// set global variables
	mcCmd.port = arg->port;
	mcCmd.slot = arg->slot;
	mcCmd.flags = 0;
	strncpy(mcCmd.name, arg->name, 1023);
	mcCmd.name[1023] = 0;
	
	// call delete function
	if((ret = SifCallRpc(&cdata, mcRpcCmd[mcType][MC_RPCCMD_DELETE], 1, &mcCmd, 0x414, rdata, 4, mcCallback, carg)) != 0) {
		lastCmd = 0;
		return -SIF_RPCE_SENDP;
	}
   	return ret;
}

// format memory card
// mcSync returns:	0 if ok
//					< 0 if error
// 
// args:    port number
//          slot number
// returns:	0   = successful
//			< 0 = error
int sbcall_mcformat(tge_sbcall_rpc_arg_t *carg)
{
	tge_sbcall_mcformat_arg_t *arg = carg->sbarg;
	int ret;
	u32 status;
	
	// check mc lib is inited
	if(mcInitializeCounter != MC_INITIALIZED) {
		return -1;
	}
	// check nothing else is processing
	core_save_disable(&status);
	if(lastCmd != 0) {
		core_restore(status);
		return -SIF_RPCE_SENDP;
	}
	lastCmd = MC_FUNC_FORMAT;
	core_restore(status);
	
	// set global variables
	mcCmd.port = arg->port;
	mcCmd.slot = arg->slot;
	
	// call format function
	if((ret = SifCallRpc(&cdata, mcRpcCmd[mcType][MC_RPCCMD_FORMAT], 1, &mcCmd, 48, rdata, 4, mcCallback, carg)) != 0) {
		lastCmd = 0;
		return -SIF_RPCE_SENDP;
	}
	return ret;
}

// unformat memory card
// mcSync returns:	0 if ok
//					< 0 if error
// 
// args:    port number
//          slot number
// returns:	0   = successful
//			< 0 = error
int sbcall_mcunformat(tge_sbcall_rpc_arg_t *carg)
{
	tge_sbcall_mcunformat_arg_t *arg = carg->sbarg;
	int ret;
	u32 status;
	
	// check mc lib is inited
	if(mcInitializeCounter != MC_INITIALIZED) {
		return -1;
	}
	// check nothing else is processing
	core_save_disable(&status);
	if(lastCmd != 0) {
		core_restore(status);
		return -SIF_RPCE_SENDP;
	}
	lastCmd = MC_FUNC_UNFORMAT;
	core_restore(status);
	
	// set global variables
	mcCmd.port = arg->port;
	mcCmd.slot = arg->slot;
	
	// call unformat function
	if((ret = SifCallRpc(&cdata, mcRpcCmd[mcType][MC_RPCCMD_UNFORMAT], 1, &mcCmd, 48, rdata, 4, mcCallback, carg)) != 0) {
		lastCmd = 0;
		return -SIF_RPCE_SENDP;
	}
	return ret;
}

// get free space info
// mcSync returns:	0 or more = number of free entries (success)
//					< 0 if error
// 
// args:	port number
//			slot number
//			path to be checked
// returns:	0   = successful
//			< 0 = error
int sbcall_mcgetentspace(tge_sbcall_rpc_arg_t *carg)
{
	tge_sbcall_mcgetentspace_arg_t *arg = carg->sbarg;
	int ret;
	u32 status;
	
	// check mc lib is inited
	if(mcInitializeCounter != MC_INITIALIZED) {
		return -1;
	}
	// check nothing else is processing
	core_save_disable(&status);
	if(lastCmd != 0) {
		core_restore(status);
		return -SIF_RPCE_SENDP;
	}
	lastCmd = MC_FUNC_GET_ENT;
	core_restore(status);
	
	// set global variables
	mcCmd.port = arg->port;
	mcCmd.slot = arg->slot;
	strncpy(mcCmd.name, arg->name, 1023);
	mcCmd.name[1023] = 0;
	
	// call sif function
	if((ret = SifCallRpc(&cdata, mcRpcCmd[mcType][MC_FUNC_GET_ENT], 1, &mcCmd, 0x414, rdata, 4, mcCallback, carg)) != 0) {
		lastCmd = 0;
		return -SIF_RPCE_SENDP;
	}
	return ret;
}

// rename file or dir on memcard
// mcSync returns:	0 if ok
//					< 0 if error
// 
// args:	port number
//			slot number
//			name of file/dir to rename
//			new name to give to file/dir
// returns:	0   = successful
//			< 0 = error
int sbcall_mcrename(tge_sbcall_rpc_arg_t *carg)
{
	tge_sbcall_mcrename_arg_t *arg = carg->sbarg;
	int ret;
	u32 status;
	
	// check mc lib is inited
	if(mcInitializeCounter != MC_INITIALIZED) {
		return -1;
	}
	// check nothing else is processing
	core_save_disable(&status);
	if(lastCmd != 0) {
		core_restore(status);
		return -SIF_RPCE_SENDP;
	}
	lastCmd = MC_FUNC_RENAME;
	core_restore(status);
	
	// set global variables
	mcCmd.port = arg->port;
	mcCmd.slot = arg->slot;
	mcCmd.flags = 0x10;
	mcCmd.table = (int)buffFileInfo;
	strncpy(mcCmd.name, arg->origname, 1023);
	mcCmd.name[1023] = 0;
	strncpy(&buffFileInfo[0x20], arg->newname, 31);
	buffFileInfo[63] = 0;
	FlushCache(0);
	
	// call sif function
	if((ret = SifCallRpc(&cdata, mcRpcCmd[mcType][MC_FUNC_SET_INFO], 1, &mcCmd, 0x414, rdata, 4, mcCallback, carg)) != 0) {
		lastCmd = 0;
		return -SIF_RPCE_SENDP;
	}
	return ret;
}

// change mcserv thread priority
// (i dont think this is implemented properly)
// mcSync returns:	0 if ok
//					< 0 if error
// 
// args:	thread priority
// returns:	0   = successful
//			< 0 = error
int mcChangeThreadPriority(int level)
{
	int ret;
	u32 status;
	
	// check mc lib is inited
	if(mcInitializeCounter != MC_INITIALIZED) {
		return -1;
	}
	// check nothing else is processing
	core_save_disable(&status);
	if(lastCmd != 0) {
		core_restore(status);
		return -SIF_RPCE_SENDP;
	}
	lastCmd = MC_FUNC_CHG_PRITY;
	core_restore(status);
	
	// set global variables
	*(u32*)mcCmd.name = level;
	
	// call sif function
	if((ret = SifCallRpc(&cdata, mcRpcCmd[mcType][MC_FUNC_CHG_PRITY], 1, &mcCmd, 48, rdata, 4, 0, 0)) != 0) {
		lastCmd = 0;
		return -SIF_RPCE_SENDP;
	}
	return ret;
}

#if 0 /* Sync is done by sbios_rpc() in linux. */
// wait for mc functions to finish
// or check if they have finished yet
// 
// args:	mode 0=wait till function finishes, 1=check function status
//			pointer for getting the number of the currently processing function (can be 0)
//			pointer for getting result of function if it finishes
// returns:	0  = function is still executing (mode=1)
//			1  = function has finished executing
//			-1 = no function registered
int mcSync(int mode, int *cmd, int *result)
{
	int funcIsExecuting, i;
	
	// check if any functions are registered
	if(lastCmd == 0)
		return -1;
	
	// check if function is still processing
	funcIsExecuting = SifCheckStatRpc(&cdata);
	
	// if mode = 0, wait for function to finish
	if(mode == 0)
	{
		while(SifCheckStatRpc(&cdata))
		{
			for(i=0; i<100000; i++)
    			;
		}
		// function has finished
		funcIsExecuting = 0;
	}
	
	// get the number of the function being processed
	if(cmd)
		*cmd = lastCmd;
	
	// if function is still processing, return 0
	if(funcIsExecuting == 1)
		return 0;
	
	// function has finished, so clear last command
	lastCmd = 0;
	
	// get result
	if(result)
	{
		*result = *(int*)rdata;
	}
	
	return 1;
}
#endif

int sbcall_mcinit(tge_sbcall_rpc_arg_t *carg)
{
	int rv;

	if (mcType < 0) {
		smod_mod_info_t mod;

		/* Detect module loaded on IOP. */
		if (smod_get_mod_by_name("mcserv", &mod) > 0) {
			if ((mod.version >> 8) > 1) {
				printf("MCSERV new version 0x%x.\n", mod.version);
				mcType = MC_TYPE_XMC;
			} else {
				printf("MCSERV old version 0x%x.\n", mod.version);
				mcType = MC_TYPE_MC;
			}
		} else {
			printf("Error: MCSERV not loaded.\n");
			return -1;
		}
	}
	rv = mcInit(carg);

	printf("mcInit() result %d.\n", rv);
	return rv;
}


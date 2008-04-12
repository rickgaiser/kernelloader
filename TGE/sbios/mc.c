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
static char curDir[1024];
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

typedef struct {
	SifRpcEndFunc_t endfunc;
	void *efarg;
	int *result;
	void *optArg;
} mcCallbackData_t;

static unsigned int mcInfoCmd[12] __attribute__((aligned(64)));

// whether or not mc lib has been inited
static int mclibInited = 0;

// stores last command executed
static unsigned int lastCmd = 0;

static unsigned int *lastResult = NULL;
static SifRpcEndFunc_t lastEndfunc;
static void *lastEfarg;

// specifies whether using MCSERV or XMCSERV modules
static int mcType = MC_TYPE_MC;

static void mcCallback(void* arg)
{
	mcCallbackData_t *data = (mcCallbackData_t *) arg;

	*data->result = *(int*)rdata;
	data->endfunc(data->efarg);

	/* End of transaction. */
	lastCmd = 0;
}



// function that gets called when mcGetInfo ends
// and interrupts are disabled
static void mcGetInfoApdx(volatile int* arg)
{
	arg = KSEG1ADDR(arg);
	
	if(typeAddr		!= NULL)
	{
		*typeAddr	= arg[0];
	}
	
	if(freeAddr		!= NULL)
	{
		*freeAddr	= arg[1];
	}
	
	if(formatAddr	!= NULL)
	{
		// if using old mcman, make it always return 'formatted'
		if(mcType == MC_TYPE_MC)
		{
			if(arg[0] == 0)
				*formatAddr	= 0;
			else
				*formatAddr	= 1;
		}
		else if(mcType == MC_TYPE_XMC)
		{
			*formatAddr	= arg[36];
		}
	}
	if (lastResult != NULL) {
		*lastResult = *(int*)rdata;
	}
	if (lastEndfunc) {
		lastEndfunc(lastEfarg);
	}

	/* End of transaction. */
	lastCmd = 0;
}

// function that gets called when mcRead ends
// and interrupts are disabled
static void mcReadFixAlign(void *arg)
{
	mcCallbackData_t *data = (mcCallbackData_t *) arg;
	volatile int* data_raw = data->optArg;
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
	mcCallback(data);
}

// function that gets called when mcChDir ends
// and interrupts are disabled
static void mcStoreDir(volatile int* arg)
{
	mcCallbackData_t *data = (mcCallbackData_t *) arg;
	int len;
	int currentDir = (int)curDir | 0x20000000;
	len = strlen((char*)currentDir);
	if(len >= 1024)
		len = strlen((char*)(currentDir+1023));
	memcpy((void*)data->optArg, (void*)currentDir, len);
	*(u8*)(currentDir+len) = 0;
	mcCallback(data);
}

void mcInitStage3(void *arg)
{
	int ret=1;
	mcCallbackData_t *data = (mcCallbackData_t *) arg;

	if(mcType == MC_TYPE_XMC)
	{
		// check if old version of mcserv loaded
		if(*(s32*)UNCACHED_SEG(rdata+4) < 0x205)
		{
#ifdef MC_DEBUG
			printf("libmc: mcserv is too old (%x)\n", *(s32*)UNCACHED_SEG(rdata+4));
#endif
			mclibInited = 0;
			*data->result = -120;
			data->endfunc(data->efarg);
			return;
		}
		
		// check if old version of mcman loaded
		if(*(s32*)UNCACHED_SEG(rdata+8) < 0x206)
		{
#ifdef MC_DEBUG
			printf("libmc: mcman is too old (%x)\n", *(s32*)UNCACHED_SEG(rdata+8));
#endif
			mclibInited = 0;
			*data->result = -121;
			data->endfunc(data->efarg);
			return;
		}
		ret = *(s32*)UNCACHED_SEG(rdata+0);
	}
	// successfully inited
	mclibInited = 1;
	lastCmd = 0;
	*data->result = ret;
	data->endfunc(data->efarg);
}

void mcInitStage2(void *arg)
{
	int ret=0;
	mcCallbackData_t *data = (mcCallbackData_t *) arg;

	if (cdata.server == NULL) {
		/* Failed. */
		*data->result = -1;

		data->endfunc(data->efarg);
		return;
	}

	// for some reason calling this init sif function with 'mcserv' makes all other
	// functions not work properly. although NOT calling it means that detecting
	// whether or not cards are formatted doesnt seem to work :P
	if(mcType == MC_TYPE_MC)
	{
#ifdef MC_DEBUG
		printf("libmc: using MCMAN & MCSERV\n");
#endif
		mcInitStage3(data);
	}
	else if(mcType == MC_TYPE_XMC)
	{
#ifdef MC_DEBUG
		printf("libmc: using XMCMAN & XMCSERV\n");
#endif		
		
		// call init function
		if((ret = SifCallRpc(&cdata, mcRpcCmd[mcType][MC_RPCCMD_INIT], SIF_RPC_M_NOWAIT, &mcCmd, 48, rdata, 12, mcInitStage3, data)) < 0)
		{
			// init error
#ifdef MC_DEBUG
			printf("libmc: initialisation error\n");
#endif
			mclibInited = 0;
			*data->result = ret - 100;
			data->endfunc(data->efarg);
			return;
		}
		
	}
}

// init memcard lib
// 
// args:	MC_TYPE_MC  = use MCSERV/MCMAN
//			MC_TYPE_XMC = use XMCSERV/XMCMAN
// returns:	0   = successful
//			< 0 = error
int mcInit(int type, SifRpcEndFunc_t endfunc, void *efarg, int *result)
{
	int ret=0;
	static mcCallbackData_t data;

	if	(type == MC_TYPE_RESET)
	{
		mclibInited = 0;
		cdata.server = NULL;
		return 0;
	}

	if(mclibInited)
		return -1;

	// set which modules to use
	mcType = type;
	
	// bind to mc rpc on iop
	data.endfunc = endfunc;
	data.efarg = efarg;
	data.result = result;
	if((ret=SifBindRpc(&cdata, 0x80000400, SIF_RPC_M_NOWAIT, mcInitStage2, &data)) < 0)
	{
		#ifdef MC_DEBUG
			printf("libmc: bind error\n");
		#endif
		
		return ret;
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
int mcGetInfo(int port, int slot, int* type, int* free, int* format, SifRpcEndFunc_t endfunc, void *efarg, int *result)
{
	int ret;
	
	// check mc lib is inited
	if(!mclibInited)
		return -1;
	// check nothing else is processing
	if(lastCmd != 0)
		return lastCmd;
	
	// set global variables
	if(mcType == MC_TYPE_MC)
	{
		mcInfoCmd[0x01] = port;
		mcInfoCmd[0x02] = slot;
		mcInfoCmd[0x03] = (type)	? 1 : 0;
		mcInfoCmd[0x04] = (free)	? 1 : 0;
		mcInfoCmd[0x05] = (format)	? 1 : 0;
		mcInfoCmd[0x07] = (int)endParameter;
	}
	else
	{
		mcInfoCmd[0x01] = port;
		mcInfoCmd[0x02] = slot;
		mcInfoCmd[0x03] = (format)	? 1 : 0;
		mcInfoCmd[0x04] = (free)	? 1 : 0;
		mcInfoCmd[0x05] = (type)	? 1 : 0;
		mcInfoCmd[0x07] = (int)endParameter;
	}
	typeAddr	= type;
	freeAddr	= free;
	formatAddr	= format;
	SifWriteBackDCache(endParameter,192);

	lastResult = result;
	lastEndfunc = endfunc;
	lastEfarg = efarg;	
	lastCmd = MC_FUNC_GET_INFO;
	// send sif command
	if((ret = SifCallRpc(&cdata, mcRpcCmd[mcType][MC_RPCCMD_GET_INFO], 1, mcInfoCmd, 48, rdata, 4, (SifRpcEndFunc_t)mcGetInfoApdx, endParameter)) != 0) {
		lastCmd = 0;
		return ret;
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
int mcOpen(int port, int slot, const char *name, int mode, SifRpcEndFunc_t endfunc, void *efarg, int *result)
{
	int ret;
	static mcCallbackData_t data;
	
	// check mc lib is inited
	if(!mclibInited)
		return -1;
	// check nothing else is processing
	if(lastCmd != 0)
		return lastCmd;
	
	// set global variables
	mcCmd.port		= port;
	mcCmd.slot		= slot;
	mcCmd.flags		= mode;
	strncpy(mcCmd.name, name, 1023);
	mcCmd.name[1023] = 0;
	
	// send sif command
	data.endfunc = endfunc;
	data.efarg = efarg;
	data.result = result;
	lastCmd = MC_FUNC_OPEN;
	if((ret = SifCallRpc(&cdata, mcRpcCmd[mcType][MC_RPCCMD_OPEN], 1, &mcCmd, 0x414, rdata, 4, mcCallback, &data)) != 0) {
		lastCmd = 0;
		return ret;
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
int mcClose(int fd, SifRpcEndFunc_t endfunc, void *efarg, int *result)
{
	int ret;
	static mcCallbackData_t data;
	
	// check mc lib is inited
	if(!mclibInited)
		return -1;
	// check nothing else is processing
	if(lastCmd != 0)
		return lastCmd;
	
	// set global variables
	mcFileCmd.fd	= fd;
	
	// send sif command
	data.endfunc = endfunc;
	data.efarg = efarg;
	data.result = result;
	lastCmd = MC_FUNC_CLOSE;
	if((ret = SifCallRpc(&cdata, mcRpcCmd[mcType][MC_RPCCMD_CLOSE], 1, &mcFileCmd, 48, rdata, 4, mcCallback, &data)) != 0) {
		lastCmd = 0;
		return ret;
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
int mcSeek(int fd, int offset, int origin, SifRpcEndFunc_t endfunc, void *efarg, int *result)
{
	int ret;
	static mcCallbackData_t data;
	
	// check mc lib is inited
	if(!mclibInited)
		return -1;
	// check nothing else is processing
	if(lastCmd != 0)
		return lastCmd;
	
	// set global variables
	mcFileCmd.fd		= fd;
	mcFileCmd.offset	= offset;
	mcFileCmd.origin	= origin;
	
	// send sif command
	data.endfunc = endfunc;
	data.efarg = efarg;
	data.result = result;
	lastCmd = MC_FUNC_SEEK;
	if((ret = SifCallRpc(&cdata, mcRpcCmd[mcType][MC_RPCCMD_SEEK], 1, &mcFileCmd, 48, rdata, 4, mcCallback, &data)) != 0) {
		lastCmd = 0;
		return ret;
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
int mcRead(int fd, void *buffer, int size, SifRpcEndFunc_t endfunc, void *efarg, int *result)
{
	int ret;
	static mcCallbackData_t data;
	
	// check mc lib is inited
	if(!mclibInited)
		return -1;
	// check nothing else is processing
	if(lastCmd != 0)
		return lastCmd;
	
	// set global variables
	mcFileCmd.fd	= fd;
	mcFileCmd.size	= size;
	mcFileCmd.buffer= (int)buffer;
	mcFileCmd.param	= (int)endParameter;
	SifWriteBackDCache(buffer,size);
	SifWriteBackDCache(endParameter,192);
	
	// send sif command
	data.endfunc = endfunc;
	data.efarg = efarg;
	data.result = result;
	data.optArg = endParameter;

	lastCmd = MC_FUNC_READ;
	if((ret = SifCallRpc(&cdata, mcRpcCmd[mcType][MC_RPCCMD_READ], 1, &mcFileCmd, 48, rdata, 4, (SifRpcEndFunc_t)mcReadFixAlign, &data)) != 0) {
		lastCmd = 0;
		return ret;
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
int mcWrite(int fd, const void *buffer, int size, SifRpcEndFunc_t endfunc, void *efarg, int *result)
{
	int i, ret;
	static mcCallbackData_t data;
	
	// check mc lib is inited
	if(!mclibInited)
		return -1;
	// check nothing else is processing
	if(lastCmd != 0)
		return lastCmd;
	
	// set global variables
	mcFileCmd.fd	= fd;
	if(size < 17)
	{
		mcFileCmd.size		= 0;
		mcFileCmd.origin	= size;
		mcFileCmd.buffer	= 0;
	}
	else
	{
		mcFileCmd.size		= size        - ( ((int)(buffer-1) & 0xFFFFFFF0) - (int)(buffer-16) );
		mcFileCmd.origin	=               ( ((int)(buffer-1) & 0xFFFFFFF0) - (int)(buffer-16) );
		mcFileCmd.buffer	= (int)buffer + ( ((int)(buffer-1) & 0xFFFFFFF0) - (int)(buffer-16) );
	}
	for(i=0; i<mcFileCmd.origin; i++)
	{
		mcFileCmd.data[i] = *(char*)(buffer+i);
	}
	FlushCache(0);
	
	// send sif command
	data.endfunc = endfunc;
	data.efarg = efarg;
	data.result = result;
	lastCmd = MC_FUNC_WRITE;
	if((ret = SifCallRpc(&cdata, mcRpcCmd[mcType][MC_RPCCMD_WRITE], 1, &mcFileCmd, 48, rdata, 4, mcCallback, &data)) != 0) {
		lastCmd = 0;
		return ret;
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
int mcFlush(int fd, SifRpcEndFunc_t endfunc, void *efarg, int *result)
{
	int ret;
	static mcCallbackData_t data;
	
	// check mc lib is inited
	if(!mclibInited)
		return -1;
	// check nothing else is processing
	if(lastCmd != 0)
		return lastCmd;
	
	// set global variables
	mcFileCmd.fd	= fd;
	
	// send sif command
	data.endfunc = endfunc;
	data.efarg = efarg;
	data.result = result;
	lastCmd = MC_FUNC_FLUSH;
	if((ret = SifCallRpc(&cdata, mcRpcCmd[mcType][MC_RPCCMD_FLUSH], 1, &mcFileCmd, 48, rdata, 4, mcCallback, &data)) != 0) {
		lastCmd = 0;
		return ret;
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
int mcMkDir(int port, int slot, const char* name, SifRpcEndFunc_t endfunc, void *efarg, int *result)
{
	int ret = mcOpen(port, slot, name, 0x40, endfunc, efarg, result);
	
	if(ret != 0)
		lastCmd = MC_FUNC_MK_DIR;
	return ret;
}

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
	static mcCallbackData_t data;
	
	// check mc lib is inited
	if(!mclibInited)
		return -1;
	// check nothing else is processing
	if(lastCmd != 0)
		return lastCmd;
	
	// set global variables
	mcCmd.port		= port;
	mcCmd.slot		= slot;
	mcCmd.table		= (int)curDir;
	strncpy(mcCmd.name, newDir, 1023);
	mcCmd.name[1023] = 0;
	SifWriteBackDCache(curDir,1024);
	
	// send sif command
	data.endfunc = endfunc;
	data.efarg = efarg;
	data.result = result;
	data.optArg = currentDir;
	lastCmd = MC_FUNC_CH_DIR;
	if((ret = SifCallRpc(&cdata, mcRpcCmd[mcType][MC_RPCCMD_CH_DIR], 1, &mcCmd, 0x414, rdata, 4, (SifRpcEndFunc_t)mcStoreDir, &data)) != 0) {
		lastCmd = 0;
		return ret;
	}
	return ret;
}

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
int mcGetDir(int port, int slot, const char *name, unsigned mode, int maxent, mcTable* table, SifRpcEndFunc_t endfunc, void *efarg, int *result)
{
	int ret;
	static mcCallbackData_t data;
	
	// check mc lib is inited
	if(!mclibInited)
		return -1;
	// check nothing else is processing
	if(lastCmd != 0)
		return lastCmd;
	
	// set global variables
	mcCmd.port		= port;
	mcCmd.slot		= slot;
	mcCmd.flags		= mode;
	mcCmd.maxent	= maxent;
	mcCmd.table		= (int)table;
	strncpy(mcCmd.name, name, 1023);
	mcCmd.name[1023] = 0;
	SifWriteBackDCache(table, maxent * sizeof(mcTable));
	
	// send sif command
	data.endfunc = endfunc;
	data.efarg = efarg;
	lastCmd = MC_FUNC_GET_DIR;
	data.result = result;
	if((ret = SifCallRpc(&cdata, mcRpcCmd[mcType][MC_RPCCMD_GET_DIR], 1, &mcCmd, 0x414, rdata, 4, mcCallback, &data)) != 0) {
		lastCmd = 0;
		return ret;
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
int mcSetFileInfo(int port, int slot, const char* name, const mcTable* info, unsigned flags, SifRpcEndFunc_t endfunc, void *efarg, int *result)
{
	int ret;
	static mcCallbackData_t data;
	
	// check mc lib is inited
	if(!mclibInited)
		return -1;
	// check nothing else is processing
	if(lastCmd != 0)
		return lastCmd;
	
	// set global variables
	mcCmd.port		= port;
	mcCmd.slot		= slot;
	mcCmd.flags		= flags;	// NOTE: this was ANDed with 7 so that u cant turn off copy protect! :)
	mcCmd.table		= (int)buffFileInfo;
	// copy info to buffFileInfo
	// (usually uses ldr/ldl sdr/sdl to copy)
	memcpy(buffFileInfo, info, 64);
	
	strncpy(mcCmd.name, name, 1023);
	mcCmd.name[1023] = 0;
	FlushCache(0);
	
	// send sif command
	data.endfunc = endfunc;
	data.efarg = efarg;
	data.result = result;
	lastCmd = MC_FUNC_SET_INFO;
	if((ret = SifCallRpc(&cdata, mcRpcCmd[mcType][MC_RPCCMD_SET_INFO], 1, &mcCmd, 0x414, rdata, 4, mcCallback, &data)) != 0) {
		lastCmd = 0;
		return ret;
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
int mcDelete(int port, int slot, const char *name, SifRpcEndFunc_t endfunc, void *efarg, int *result)
{
	int ret;
	static mcCallbackData_t data;
	
	// check lib is inited
	if(!mclibInited)
		return -1;
	// check nothing else is processing
	if(lastCmd != 0)
		return lastCmd;
	
	// set global variables
	mcCmd.port = port;
	mcCmd.slot = slot;
	mcCmd.flags = 0;
	strncpy(mcCmd.name, name, 1023);
	mcCmd.name[1023] = 0;
	
	// call delete function
	data.endfunc = endfunc;
	data.efarg = efarg;
	data.result = result;
	lastCmd = MC_FUNC_DELETE;
	if((ret = SifCallRpc(&cdata, mcRpcCmd[mcType][MC_RPCCMD_DELETE], 1, &mcCmd, 0x414, rdata, 4, mcCallback, &data)) != 0) {
		lastCmd = 0;
		return ret;
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
int mcFormat(int port, int slot, SifRpcEndFunc_t endfunc, void *efarg, int *result)
{
	int ret;
	static mcCallbackData_t data;
	
	// check lib is inited
	if(!mclibInited)
		return -1;
	// check nothing else is processing
	if(lastCmd != 0)
		return lastCmd;
	
	// set global variables
	mcCmd.port = port;
	mcCmd.slot = slot;
	
	// call format function
	data.endfunc = endfunc;
	data.efarg = efarg;
	data.result = result;
	lastCmd = MC_FUNC_FORMAT;
	if((ret = SifCallRpc(&cdata, mcRpcCmd[mcType][MC_RPCCMD_FORMAT], 1, &mcCmd, 48, rdata, 4, mcCallback, &data)) != 0) {
		lastCmd = 0;
		return ret;
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
int mcUnformat(int port, int slot, SifRpcEndFunc_t endfunc, void *efarg, int *result)
{
	int ret;
	static mcCallbackData_t data;
	
	// check lib is inited
	if(!mclibInited)
		return -1;
	// check nothing else is processing
	if(lastCmd != 0)
		return lastCmd;
	
	// set global variables
	mcCmd.port = port;
	mcCmd.slot = slot;
	
	// call unformat function
	data.endfunc = endfunc;
	data.efarg = efarg;
	lastCmd = MC_FUNC_UNFORMAT;
	data.result = result;
	if((ret = SifCallRpc(&cdata, mcRpcCmd[mcType][MC_RPCCMD_UNFORMAT], 1, &mcCmd, 48, rdata, 4, mcCallback, &data)) != 0) {
		lastCmd = 0;
		return ret;
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
int mcGetEntSpace(int port, int slot, const char* path, SifRpcEndFunc_t endfunc, void *efarg, int *result)
{
	int ret;
	static mcCallbackData_t data;
	
	// check lib is inited
	if(!mclibInited)
		return -1;
	// check nothing else is processing
	if(lastCmd != 0)
		return lastCmd;
	
	// set global variables
	mcCmd.port = port;
	mcCmd.slot = slot;
	strncpy(mcCmd.name, path, 1023);
	mcCmd.name[1023] = 0;
	
	// call sif function
	data.endfunc = endfunc;
	data.efarg = efarg;
	data.result = result;
	lastCmd = MC_FUNC_GET_ENT;
	if((ret = SifCallRpc(&cdata, mcRpcCmd[mcType][MC_FUNC_GET_ENT], 1, &mcCmd, 0x414, rdata, 4, mcCallback, &data)) != 0) {
		lastCmd = 0;
		return ret;
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
int mcRename(int port, int slot, const char* oldName, const char* newName, SifRpcEndFunc_t endfunc, void *efarg, int *result)
{
	int ret;
	static mcCallbackData_t data;
	
	// check lib is inited
	if(!mclibInited)
		return -1;
	// check nothing else is processing
	if(lastCmd != 0)
		return lastCmd;
	
	// set global variables
	mcCmd.port = port;
	mcCmd.slot = slot;
	mcCmd.flags = 0x10;
	mcCmd.table = (int)buffFileInfo;
	strncpy(mcCmd.name, oldName, 1023);
	mcCmd.name[1023] = 0;
	strncpy(&buffFileInfo[0x20], newName, 31);
	buffFileInfo[63] = 0;
	FlushCache(0);
	
	// call sif function
	data.endfunc = endfunc;
	data.efarg = efarg;
	data.result = result;
	lastCmd = MC_FUNC_RENAME;
	if((ret = SifCallRpc(&cdata, mcRpcCmd[mcType][MC_FUNC_SET_INFO], 1, &mcCmd, 0x414, rdata, 4, mcCallback, &data)) != 0) {
		lastCmd = 0;
		return ret;
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
	
	// check lib is inited
	if(!mclibInited)
		return -1;
	// check nothing else is processing
	if(lastCmd != 0)
		return lastCmd;
	
	// set global variables
	*(u32*)mcCmd.name = level;
	
	// call sif function
	lastCmd = MC_FUNC_CHG_PRITY;
	if((ret = SifCallRpc(&cdata, mcRpcCmd[mcType][MC_FUNC_CHG_PRITY], 1, &mcCmd, 48, rdata, 4, 0, 0)) != 0) {
		lastCmd = 0;
		return ret;
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
	smod_mod_info_t mod;

	/* Detect module loaded on IOP. */
	if (smod_get_mod_by_name("mcserv", &mod) > 0) {
		int type;
		int rv;

		if ((mod.version >> 8) > 1) {
			printf("MCSERV new version 0x%x.\n", mod.version);
			type = MC_TYPE_XMC;
		} else {
			printf("MCSERV old version 0x%x.\n", mod.version);
			type = MC_TYPE_MC;
		}
		rv = mcInit(type, carg->endfunc, carg->efarg, &carg->result);

		printf("mcInit() result %d.\n", rv);
		return rv;
	} else {
		printf("Error: MCSERV not loaded.\n");
		return -1;
	}
}

int sbcall_mcgetinfo(tge_sbcall_rpc_arg_t *carg)
{
	tge_sbcall_mcgetinfo_arg_t *arg = carg->sbarg;
	return mcGetInfo(arg->port, arg->slot, arg->type, arg->free, arg->format, carg->endfunc, carg->efarg, &carg->result);
}

int sbcall_mcgetdir(tge_sbcall_rpc_arg_t *carg)
{
	tge_sbcall_mcgetdir_arg_t *arg = carg->sbarg;

	return mcGetDir(arg->port, arg->slot, arg->name, arg->mode, arg->max_entries, arg->dirents, carg->endfunc, carg->efarg, &carg->result);
}

int sbcall_mcopen(tge_sbcall_rpc_arg_t *carg)
{
	tge_sbcall_mcopen_arg_t *arg = carg->sbarg;
	return mcOpen(arg->port, arg->slot, arg->name, arg->mode, carg->endfunc, carg->efarg, &carg->result);
}

int sbcall_mcclose(tge_sbcall_rpc_arg_t *carg)
{
	tge_sbcall_mcclose_arg_t *arg = carg->sbarg;
	return mcClose(arg->fd, carg->endfunc, carg->efarg, &carg->result);
}

int sbcall_mcseek(tge_sbcall_rpc_arg_t *carg)
{
	tge_sbcall_mcseek_arg_t *arg = carg->sbarg;
	return mcSeek(arg->fd, arg->offset, arg->whence, carg->endfunc, carg->efarg, &carg->result);
}

int sbcall_mcread(tge_sbcall_rpc_arg_t *carg)
{
	tge_sbcall_mcread_arg_t *arg = carg->sbarg;
	return mcRead(arg->fd, arg->buf, arg->size, carg->endfunc, carg->efarg, &carg->result);
}

int sbcall_mcwrite(tge_sbcall_rpc_arg_t *carg)
{
	tge_sbcall_mcwrite_arg_t *arg = carg->sbarg;
	return mcWrite(arg->fd, arg->buf, arg->size, carg->endfunc, carg->efarg, &carg->result);
}

int sbcall_mcflush(tge_sbcall_rpc_arg_t *carg)
{
	tge_sbcall_mcflush_arg_t *arg = carg->sbarg;
	return mcFlush(arg->fd, carg->endfunc, carg->efarg, &carg->result);
}

int sbcall_mcmkdir(tge_sbcall_rpc_arg_t *carg)
{
	tge_sbcall_mcmkdir_arg_t *arg = carg->sbarg;
	return mcMkDir(arg->port, arg->slot, arg->name, carg->endfunc, carg->efarg, &carg->result);
}

int sbcall_mcsetfileinfo(tge_sbcall_rpc_arg_t *carg)
{
	tge_sbcall_mcsetfileinfo_arg_t *arg = carg->sbarg;
	return mcSetFileInfo(arg->port, arg->slot, arg->name, arg->dirent, arg->modes, carg->endfunc, carg->efarg, &carg->result);
}

int sbcall_mcdelete(tge_sbcall_rpc_arg_t *carg)
{
	tge_sbcall_mcdelete_arg_t *arg = carg->sbarg;
	return mcDelete(arg->port, arg->slot, arg->name, carg->endfunc, carg->efarg, &carg->result);
}

int sbcall_mcformat(tge_sbcall_rpc_arg_t *carg)
{
	tge_sbcall_mcformat_arg_t *arg = carg->sbarg;
	return mcFormat(arg->port, arg->slot, carg->endfunc, carg->efarg, &carg->result);
}

int sbcall_mcunformat(tge_sbcall_rpc_arg_t *carg)
{
	tge_sbcall_mcunformat_arg_t *arg = carg->sbarg;
	return mcUnformat(arg->port, arg->slot, carg->endfunc, carg->efarg, &carg->result);
}

int sbcall_mcgetentspace(tge_sbcall_rpc_arg_t *carg)
{
	tge_sbcall_mcgetentspace_arg_t *arg = carg->sbarg;
	return mcGetEntSpace(arg->port, arg->slot, arg->name, carg->endfunc, carg->efarg, &carg->result);
}

int sbcall_mcrename(tge_sbcall_rpc_arg_t *carg)
{
	tge_sbcall_mcrename_arg_t *arg = carg->sbarg;
	return mcRename(arg->port, arg->slot, arg->origname, arg->newname, carg->endfunc, carg->efarg, &carg->result);
}

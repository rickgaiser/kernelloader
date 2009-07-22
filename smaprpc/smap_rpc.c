#include <tamtypes.h>
#include <thbase.h>
#include <thsemap.h>
#include <sifrpc.h>
#include <ioman.h>
#include <sysclib.h>
#include <sifman.h>

#include "stddef.h"
#include "stdio.h"
#include "smap.h"
#include "smap_rpc.h"
#include "main.h"
#include "pbuf.h"

#define NUMBER_OF_BUFFERS 20
/** Aligned frame size */
#define FRAME_SIZE 1536

static struct pbuf buffers[NUMBER_OF_BUFFERS];
static unsigned char payloadBuffer[FRAME_SIZE * NUMBER_OF_BUFFERS];

static SifRpcDataQueue_t rpc_queue __attribute__((aligned(64)));
static SifRpcServerData_t rpc_server __attribute((aligned(64)));
static int _rpc_buffer[2048] __attribute((aligned(64)));

void pbuf_check_transfers(void)
{
	int i;

	for (i = 0; i < NUMBER_OF_BUFFERS; i++) {
		if (buffers[i].ref != 0) {
			if (buffers[i].id != 0) {
				if (SifDmaStat(buffers[i].id) < 0) {
					/* DMA transfer is complete. */
					pbuf_free(&buffers[i]);
				}
			}
		}
	}
}

u8 pbuf_free(struct pbuf *p)
{
	p->id = 0;
	p->ref = 0;
	return 1;
}

struct pbuf *pbuf_alloc(pbuf_layer l, u16 size, pbuf_flag flag)
{
	int i;

	pbuf_check_transfers();
	for (i = 0; i < NUMBER_OF_BUFFERS; i++) {
		if (buffers[i].ref == 0) {
			buffers[i].ref++;
			buffers[i].payload = &payloadBuffer[FRAME_SIZE * i];
			buffers[i].next = NULL;
			buffers[i].len = buffers[i].tot_len = size;
			buffers[i].id = 0;
			return &buffers[i];
		}
	}
	return NULL;
}

void smap_send(u8 *buffer, unsigned int size)
{
	struct pbuf *pBuf;

	pBuf=(struct pbuf*)pbuf_alloc(PBUF_RAW, size, PBUF_POOL);
	if (pBuf != NULL) {
		unsigned char *data;
		data = (unsigned char *) pBuf->payload;

		memcpy(data, buffer, size);

		SMapLowLevelOutput(pBuf);
	} else {
		printf("Failed to allocate PBuf.\n");
	}
}

void *rpcCommandHandler(u32 command, void *buffer, int size)

{
	u32 *buf = (u32 *) buffer;
	int ret = 0;

	switch (command) {
		case SMAP_CMD_SEND:
			smap_send(buffer, size);
			break;

		case SMAP_CMD_SET_BUFFER: {
			unsigned int offset;

			ee_buffer_size = 0; /* Disable use of ee_buffer in case an interrupt occurs. */
			ee_buffer = buf[0];
			if (buf[1] > 0) {
				/* Align memory offset. */
				offset = DMA_TRANSFER_SIZE - (ee_buffer & (DMA_TRANSFER_SIZE - 1));
				if (offset == DMA_TRANSFER_SIZE) {
					offset = 0;
				}
				ee_buffer = ee_buffer + offset;
				ee_buffer_pos = 0;
				ee_buffer_size = buf[1] - offset;
			}
			break;
		}

		case SMAP_CMD_GET_MAC_ADDR: {
			memcpy(&buf[1], SMap_GetMACAddress(), 6);
			break;
		}

		default:
			printf("Received unknown cmd %lu\n", command);
			ret = -1;
			break;
	}
	buf[0] = ret; //store return code
	return buffer;
}

void rpcMainThread(void* param)
{
	int tid;

	SifInitRpc(0);

	printf("smap rpc thread\n");

	tid = GetThreadId();

	SifSetRpcQueue(&rpc_queue, tid);
	SifRegisterRpc(&rpc_server, SMAP_BIND_RPC_ID, (void *) rpcCommandHandler, (u8 *) &_rpc_buffer, 0, 0, &rpc_queue);

	SifRpcLoop(&rpc_queue);
}

int rpc_start(void)
{
	iop_thread_t param;
	int th;

	/*create thread*/
	param.attr         = TH_C;
	param.thread     = rpcMainThread;
	param.priority = 40;
	param.stacksize    = 0x8000;
	param.option      = 0;


	th = CreateThread(&param);
	if (th > 0) {
		StartThread(th,0);
		return 0;
	} else  {
		printf("Failed to create rpx thread.\n");
		return 1;
	}
}

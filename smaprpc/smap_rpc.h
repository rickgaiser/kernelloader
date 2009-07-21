#ifndef _SMAP_RPC_H_
#define _SMAP_RPC_H_

#define SMAP_BIND_RPC_ID 0x0815e000

#define SMAP_CMD_SEND 1
#define SMAP_CMD_SET_BUFFER 2
#define SMAP_CMD_GET_MAC_ADDR 3

#define SIF_SMAP_RECEIVE 0x07


typedef struct smap_message_s {
	SifCmdHeader_t header;
	u32 payload;
	u32 size;
	u32 padding[10];
} smap_message_t;

int rpc_start(void);

#endif

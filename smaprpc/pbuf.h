#ifndef _PBUF_H_
#define _PBUF_H_

typedef struct pbuf		PBuf;

struct pbuf {
  /** next pbuf in singly linked pbuf chain */
  struct pbuf *next;

  /** pointer to the actual data in the buffer */
  void *payload;
  
  /**
   * total length of this buffer and all next buffers in chain
   * invariant: p->tot_len == p->len + (p->next? p->next->tot_len: 0)
   */
  u16 tot_len;

  /* length of this buffer */
  u16 len;  

#if 0  
  /* flags telling the type of pbuf */
  u16 flags;
#endif
  
  /**
   * the reference count always equals the number of pointers
   * that refer to this pbuf. This can be pointers from an application,
   * the stack itself, or pbuf->next pointers from a chain.
   */
  u16 ref;

  /** Transfer id of received frame which is sent to the EE. */
  int id;  
};

typedef int err_t;

typedef enum {
  PBUF_TRANSPORT,
  PBUF_IP,
  PBUF_LINK,
  PBUF_RAW
} pbuf_layer;

typedef enum {
  PBUF_RAM,
  PBUF_ROM,
  PBUF_REF,
  PBUF_POOL
} pbuf_flag;

u8 pbuf_free(struct pbuf *p);
struct pbuf *pbuf_alloc(pbuf_layer l, u16 size, pbuf_flag flag);
void pbuf_check_transfers(void);

#endif

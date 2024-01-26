#ifndef NOUVEAU_WINSYS_H
#define NOUVEAU_WINSYS_H

#include <stdint.h>
#include <inttypes.h>

#include "pipe/p_defines.h"

#include "drm-uapi/drm.h"
#include <nouveau.h>

#ifndef NV04_PFIFO_MAX_PACKET_LEN
#define NV04_PFIFO_MAX_PACKET_LEN 2047
#endif

#define NOUVEAU_MIN_BUFFER_MAP_ALIGN      64
#define NOUVEAU_MIN_BUFFER_MAP_ALIGN_MASK (NOUVEAU_MIN_BUFFER_MAP_ALIGN - 1)

static inline uint32_t
PUSH_AVAIL(struct nouveau_pushbuf *push)
{
   return push->end - push->cur;
}

static inline bool
PUSH_SPACE(struct nouveau_pushbuf *push, uint32_t size)
{
   /* Provide a buffer so that fences always have room to be emitted */
   size += 8;
   if (PUSH_AVAIL(push) < size)
      return nouveau_pushbuf_space(push, size, 0, 0) == 0;
   return true;
}

static inline void
PUSH_DATA(struct nouveau_pushbuf *push, uint32_t data)
{
   *push->cur++ = data;
}

static inline void
PUSH_DATAp(struct nouveau_pushbuf *push, const void *data, uint32_t size)
{
   memcpy(push->cur, data, size * 4);
   push->cur += size;
}

static inline void
PUSH_DATAb(struct nouveau_pushbuf *push, const void *data, uint32_t size)
{
   memcpy(push->cur, data, size);
   push->cur += DIV_ROUND_UP(size, 4);
}

static inline void
PUSH_DATAf(struct nouveau_pushbuf *push, float f)
{
   union { float f; uint32_t i; } u;
   u.f = f;
   PUSH_DATA(push, u.i);
}

static inline void
PUSH_KICK(struct nouveau_pushbuf *push)
{
   nouveau_pushbuf_kick(push, push->channel);
}


#define NOUVEAU_RESOURCE_FLAG_LINEAR   (PIPE_RESOURCE_FLAG_DRV_PRIV << 0)
#define NOUVEAU_RESOURCE_FLAG_DRV_PRIV (PIPE_RESOURCE_FLAG_DRV_PRIV << 1)

static inline uint32_t
nouveau_screen_transfer_flags(unsigned pipe)
{
   uint32_t flags = 0;

   if (!(pipe & PIPE_MAP_UNSYNCHRONIZED)) {
      if (pipe & PIPE_MAP_READ)
         flags |= NOUVEAU_BO_RD;
      if (pipe & PIPE_MAP_WRITE)
         flags |= NOUVEAU_BO_WR;
      if (pipe & PIPE_MAP_DONTBLOCK)
         flags |= NOUVEAU_BO_NOBLOCK;
   }

   return flags;
}

extern struct nouveau_screen *
nv30_screen_create(struct nouveau_device *);

extern struct nouveau_screen *
nv50_screen_create(struct nouveau_device *);

extern struct nouveau_screen *
nvc0_screen_create(struct nouveau_device *);

#endif

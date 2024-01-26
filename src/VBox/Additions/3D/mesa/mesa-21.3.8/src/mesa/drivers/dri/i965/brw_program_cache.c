/*
 Copyright (C) Intel Corp.  2006.  All Rights Reserved.
 Intel funded Tungsten Graphics to
 develop this 3D driver.

 Permission is hereby granted, free of charge, to any person obtaining
 a copy of this software and associated documentation files (the
 "Software"), to deal in the Software without restriction, including
 without limitation the rights to use, copy, modify, merge, publish,
 distribute, sublicense, and/or sell copies of the Software, and to
 permit persons to whom the Software is furnished to do so, subject to
 the following conditions:

 The above copyright notice and this permission notice (including the
 next paragraph) shall be included in all copies or substantial
 portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
 LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 **********************************************************************/
 /*
  * Authors:
  *   Keith Whitwell <keithw@vmware.com>
  */

/** @file brw_program_cache.c
 *
 * This file implements a simple program cache for 965.  The consumers can
 *  query the hash table of programs using a cache_id and program key, and
 * receive the corresponding program buffer object (plus associated auxiliary
 *  data) in return.  Objects in the cache may not have relocations
 * (pointers to other BOs) in them.
 *
 * The inner workings are a simple hash table based on a FNV-1a of the
 * key data.
 *
 * Replacement is not implemented.  Instead, when the cache gets too
 * big we throw out all of the cache data and let it get regenerated.
 */

#include "main/streaming-load-memcpy.h"
#include "x86/common_x86_asm.h"
#include "brw_batch.h"
#include "brw_state.h"
#include "brw_wm.h"
#include "brw_gs.h"
#include "brw_cs.h"
#include "brw_program.h"
#include "compiler/brw_eu.h"
#include "util/u_memory.h"
#define XXH_INLINE_ALL
#include "util/xxhash.h"

#define FILE_DEBUG_FLAG DEBUG_STATE

struct brw_cache_item {
   /**
    * Effectively part of the key, cache_id identifies what kind of state
    * buffer is involved, and also which dirty flag should set.
    */
   enum brw_cache_id cache_id;

   /** 32-bit hash of the key data */
   GLuint hash;

   /** for variable-sized keys */
   GLuint key_size;
   GLuint prog_data_size;
   const struct brw_base_prog_key *key;

   uint32_t offset;
   uint32_t size;

   struct brw_cache_item *next;
};

enum brw_cache_id
brw_stage_cache_id(gl_shader_stage stage)
{
   static const enum brw_cache_id stage_ids[] = {
      BRW_CACHE_VS_PROG,
      BRW_CACHE_TCS_PROG,
      BRW_CACHE_TES_PROG,
      BRW_CACHE_GS_PROG,
      BRW_CACHE_FS_PROG,
      BRW_CACHE_CS_PROG,
   };
   assert((int)stage >= 0 && stage < ARRAY_SIZE(stage_ids));
   return stage_ids[stage];
}

static GLuint
hash_key(struct brw_cache_item *item)
{
    uint32_t hash = 0;
    hash = XXH32(&item->cache_id, sizeof(item->cache_id), hash);
    hash = XXH32(item->key, item->key_size, hash);

   return hash;
}

static int
brw_cache_item_equals(const struct brw_cache_item *a,
                      const struct brw_cache_item *b)
{
   return a->cache_id == b->cache_id &&
      a->hash == b->hash &&
      a->key_size == b->key_size &&
      (memcmp(a->key, b->key, a->key_size) == 0);
}

static struct brw_cache_item *
search_cache(struct brw_cache *cache, GLuint hash,
             struct brw_cache_item *lookup)
{
   struct brw_cache_item *c;

#if 0
   int bucketcount = 0;

   for (c = cache->items[hash % cache->size]; c; c = c->next)
      bucketcount++;

   fprintf(stderr, "bucket %d/%d = %d/%d items\n", hash % cache->size,
           cache->size, bucketcount, cache->n_items);
#endif

   for (c = cache->items[hash % cache->size]; c; c = c->next) {
      if (brw_cache_item_equals(lookup, c))
         return c;
   }

   return NULL;
}


static void
rehash(struct brw_cache *cache)
{
   struct brw_cache_item **items;
   struct brw_cache_item *c, *next;
   GLuint size, i;

   size = cache->size * 3;
   items = calloc(size, sizeof(*items));

   for (i = 0; i < cache->size; i++)
      for (c = cache->items[i]; c; c = next) {
         next = c->next;
         c->next = items[c->hash % size];
         items[c->hash % size] = c;
      }

   free(cache->items);
   cache->items = items;
   cache->size = size;
}


/**
 * Returns the buffer object matching cache_id and key, or NULL.
 */
bool
brw_search_cache(struct brw_cache *cache, enum brw_cache_id cache_id,
                 const void *key, GLuint key_size, uint32_t *inout_offset,
                 void *inout_prog_data, bool flag_state)
{
   struct brw_cache_item *item;
   struct brw_cache_item lookup;
   GLuint hash;

   lookup.cache_id = cache_id;
   lookup.key = key;
   lookup.key_size = key_size;
   hash = hash_key(&lookup);
   lookup.hash = hash;

   item = search_cache(cache, hash, &lookup);

   if (item == NULL)
      return false;

   void *prog_data = ((char *) item->key) + item->key_size;

   if (item->offset != *inout_offset ||
       prog_data != *((void **) inout_prog_data)) {
      if (likely(flag_state))
         cache->brw->ctx.NewDriverState |= (1 << cache_id);
      *inout_offset = item->offset;
      *((void **) inout_prog_data) = prog_data;
   }

   return true;
}

static void
brw_cache_new_bo(struct brw_cache *cache, uint32_t new_size)
{
   struct brw_context *brw = cache->brw;
   struct brw_bo *new_bo;

   perf_debug("Copying to larger program cache: %u kB -> %u kB\n",
              (unsigned) cache->bo->size / 1024, new_size / 1024);

   new_bo = brw_bo_alloc(brw->bufmgr, "program cache", new_size,
                         BRW_MEMZONE_SHADER);
   if (can_do_exec_capture(brw->screen))
      new_bo->kflags |= EXEC_OBJECT_CAPTURE;

   void *map = brw_bo_map(brw, new_bo, MAP_READ | MAP_WRITE |
                                       MAP_ASYNC | MAP_PERSISTENT);

   /* Copy any existing data that needs to be saved. */
   if (cache->next_offset != 0) {
#ifdef USE_SSE41
      if (!cache->bo->cache_coherent && cpu_has_sse4_1)
         _mesa_streaming_load_memcpy(map, cache->map, cache->next_offset);
      else
#endif
         memcpy(map, cache->map, cache->next_offset);
   }

   brw_bo_unmap(cache->bo);
   brw_bo_unreference(cache->bo);
   cache->bo = new_bo;
   cache->map = map;

   /* Since we have a new BO in place, we need to signal the units
    * that depend on it (state base address on gfx5+, or unit state before).
    */
   brw->ctx.NewDriverState |= BRW_NEW_PROGRAM_CACHE;
   brw->batch.state_base_address_emitted = false;
}

/**
 * Attempts to find an item in the cache with identical data.
 */
static const struct brw_cache_item *
brw_lookup_prog(const struct brw_cache *cache,
                enum brw_cache_id cache_id,
                const void *data, unsigned data_size)
{
   unsigned i;
   const struct brw_cache_item *item;

   for (i = 0; i < cache->size; i++) {
      for (item = cache->items[i]; item; item = item->next) {
         if (item->cache_id != cache_id || item->size != data_size ||
             memcmp(cache->map + item->offset, data, item->size) != 0)
            continue;

         return item;
      }
   }

   return NULL;
}

static uint32_t
brw_alloc_item_data(struct brw_cache *cache, uint32_t size)
{
   uint32_t offset;

   /* Allocate space in the cache BO for our new program. */
   if (cache->next_offset + size > cache->bo->size) {
      uint32_t new_size = cache->bo->size * 2;

      while (cache->next_offset + size > new_size)
         new_size *= 2;

      brw_cache_new_bo(cache, new_size);
   }

   offset = cache->next_offset;

   /* Programs are always 64-byte aligned, so set up the next one now */
   cache->next_offset = ALIGN(offset + size, 64);

   return offset;
}

const void *
brw_find_previous_compile(struct brw_cache *cache,
                          enum brw_cache_id cache_id,
                          unsigned program_string_id)
{
   for (unsigned i = 0; i < cache->size; i++) {
      for (struct brw_cache_item *c = cache->items[i]; c; c = c->next) {
         if (c->cache_id == cache_id &&
             c->key->program_string_id == program_string_id) {
            return c->key;
         }
      }
   }

   return NULL;
}

void
brw_upload_cache(struct brw_cache *cache,
                 enum brw_cache_id cache_id,
                 const void *key,
                 GLuint key_size,
                 const void *data,
                 GLuint data_size,
                 const void *prog_data,
                 GLuint prog_data_size,
                 uint32_t *out_offset,
                 void *out_prog_data)
{
   struct brw_cache_item *item = CALLOC_STRUCT(brw_cache_item);
   const struct brw_cache_item *matching_data =
      brw_lookup_prog(cache, cache_id, data, data_size);
   GLuint hash;
   void *tmp;

   item->cache_id = cache_id;
   item->size = data_size;
   item->key = key;
   item->key_size = key_size;
   item->prog_data_size = prog_data_size;
   hash = hash_key(item);
   item->hash = hash;

   /* If we can find a matching prog in the cache already, then reuse the
    * existing stuff without creating new copy into the underlying buffer
    * object. This is notably useful for programs generating shaders at
    * runtime, where multiple shaders may compile to the same thing in our
    * backend.
    */
   if (matching_data) {
      item->offset = matching_data->offset;
   } else {
      item->offset = brw_alloc_item_data(cache, data_size);

      /* Copy data to the buffer */
      memcpy(cache->map + item->offset, data, data_size);
   }

   /* Set up the memory containing the key and prog_data */
   tmp = malloc(key_size + prog_data_size);

   memcpy(tmp, key, key_size);
   memcpy(tmp + key_size, prog_data, prog_data_size);

   item->key = tmp;

   if (cache->n_items > cache->size * 1.5f)
      rehash(cache);

   hash %= cache->size;
   item->next = cache->items[hash];
   cache->items[hash] = item;
   cache->n_items++;

   *out_offset = item->offset;
   *(void **)out_prog_data = (void *)((char *)item->key + item->key_size);
   cache->brw->ctx.NewDriverState |= 1 << cache_id;
}

void
brw_init_caches(struct brw_context *brw)
{
   struct brw_cache *cache = &brw->cache;

   cache->brw = brw;

   cache->size = 7;
   cache->n_items = 0;
   cache->items =
      calloc(cache->size, sizeof(struct brw_cache_item *));

   cache->bo = brw_bo_alloc(brw->bufmgr, "program cache", 16384,
                            BRW_MEMZONE_SHADER);
   if (can_do_exec_capture(brw->screen))
      cache->bo->kflags |= EXEC_OBJECT_CAPTURE;

   cache->map = brw_bo_map(brw, cache->bo, MAP_READ | MAP_WRITE |
                                           MAP_ASYNC | MAP_PERSISTENT);
}

static void
brw_clear_cache(struct brw_context *brw, struct brw_cache *cache)
{
   struct brw_cache_item *c, *next;
   GLuint i;

   DBG("%s\n", __func__);

   for (i = 0; i < cache->size; i++) {
      for (c = cache->items[i]; c; c = next) {
         next = c->next;
         if (c->cache_id == BRW_CACHE_VS_PROG ||
             c->cache_id == BRW_CACHE_TCS_PROG ||
             c->cache_id == BRW_CACHE_TES_PROG ||
             c->cache_id == BRW_CACHE_GS_PROG ||
             c->cache_id == BRW_CACHE_FS_PROG ||
             c->cache_id == BRW_CACHE_CS_PROG) {
            const void *item_prog_data = ((char *)c->key) + c->key_size;
            brw_stage_prog_data_free(item_prog_data);
         }
         free((void *)c->key);
         free(c);
      }
      cache->items[i] = NULL;
   }

   cache->n_items = 0;

   /* Start putting programs into the start of the BO again, since
    * we'll never find the old results.
    */
   cache->next_offset = 0;

   /* We need to make sure that the programs get regenerated, since
    * any offsets leftover in brw_context will no longer be valid.
    */
   brw->NewGLState = ~0;
   brw->ctx.NewDriverState = ~0ull;
   brw->state.pipelines[BRW_RENDER_PIPELINE].mesa = ~0;
   brw->state.pipelines[BRW_RENDER_PIPELINE].brw = ~0ull;
   brw->state.pipelines[BRW_COMPUTE_PIPELINE].mesa = ~0;
   brw->state.pipelines[BRW_COMPUTE_PIPELINE].brw = ~0ull;

   /* Also, NULL out any stale program pointers. */
   brw->vs.base.prog_data = NULL;
   brw->tcs.base.prog_data = NULL;
   brw->tes.base.prog_data = NULL;
   brw->gs.base.prog_data = NULL;
   brw->wm.base.prog_data = NULL;
   brw->cs.base.prog_data = NULL;

   brw_batch_flush(brw);
}

void
brw_program_cache_check_size(struct brw_context *brw)
{
   /* un-tuned guess.  Each object is generally a page, so 2000 of them is 8 MB of
    * state cache.
    */
   if (brw->cache.n_items > 2000) {
      perf_debug("Exceeded state cache size limit.  Clearing the set "
                 "of compiled programs, which will trigger recompiles\n");
      brw_clear_cache(brw, &brw->cache);
      brw_cache_new_bo(&brw->cache, brw->cache.bo->size);
   }
}


static void
brw_destroy_cache(struct brw_context *brw, struct brw_cache *cache)
{

   DBG("%s\n", __func__);

   /* This can be NULL if context creation failed early on */
   if (cache->bo) {
      brw_bo_unmap(cache->bo);
      brw_bo_unreference(cache->bo);
      cache->bo = NULL;
      cache->map = NULL;
   }
   brw_clear_cache(brw, cache);
   free(cache->items);
   cache->items = NULL;
   cache->size = 0;
}


void
brw_destroy_caches(struct brw_context *brw)
{
   brw_destroy_cache(brw, &brw->cache);
}

static const char *
cache_name(enum brw_cache_id cache_id)
{
   switch (cache_id) {
   case BRW_CACHE_VS_PROG:
      return "VS kernel";
   case BRW_CACHE_TCS_PROG:
      return "TCS kernel";
   case BRW_CACHE_TES_PROG:
      return "TES kernel";
   case BRW_CACHE_FF_GS_PROG:
      return "Fixed-function GS kernel";
   case BRW_CACHE_GS_PROG:
      return "GS kernel";
   case BRW_CACHE_CLIP_PROG:
      return "CLIP kernel";
   case BRW_CACHE_SF_PROG:
      return "SF kernel";
   case BRW_CACHE_FS_PROG:
      return "FS kernel";
   case BRW_CACHE_CS_PROG:
      return "CS kernel";
   default:
      return "unknown";
   }
}

void
brw_print_program_cache(struct brw_context *brw)
{
   const struct brw_cache *cache = &brw->cache;
   struct brw_cache_item *item;

   for (unsigned i = 0; i < cache->size; i++) {
      for (item = cache->items[i]; item; item = item->next) {
         fprintf(stderr, "%s:\n", cache_name(i));
         brw_disassemble_with_labels(&brw->screen->devinfo, cache->map,
                                     item->offset, item->size, stderr);
      }
   }
}

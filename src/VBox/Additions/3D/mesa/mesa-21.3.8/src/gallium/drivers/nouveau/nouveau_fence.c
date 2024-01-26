/*
 * Copyright 2010 Christoph Bumiller
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#include "nouveau_screen.h"
#include "nouveau_winsys.h"
#include "nouveau_fence.h"
#include "util/os_time.h"

#ifdef PIPE_OS_UNIX
#include <sched.h>
#endif

bool
nouveau_fence_new(struct nouveau_screen *screen, struct nouveau_fence **fence)
{
   *fence = CALLOC_STRUCT(nouveau_fence);
   if (!*fence)
      return false;

   (*fence)->screen = screen;
   (*fence)->ref = 1;
   list_inithead(&(*fence)->work);

   return true;
}

static void
nouveau_fence_trigger_work(struct nouveau_fence *fence)
{
   struct nouveau_fence_work *work, *tmp;

   LIST_FOR_EACH_ENTRY_SAFE(work, tmp, &fence->work, list) {
      work->func(work->data);
      list_del(&work->list);
      FREE(work);
   }
}

void
nouveau_fence_emit(struct nouveau_fence *fence)
{
   struct nouveau_screen *screen = fence->screen;

   assert(fence->state == NOUVEAU_FENCE_STATE_AVAILABLE);

   /* set this now, so that if fence.emit triggers a flush we don't recurse */
   fence->state = NOUVEAU_FENCE_STATE_EMITTING;

   ++fence->ref;

   if (screen->fence.tail)
      screen->fence.tail->next = fence;
   else
      screen->fence.head = fence;

   screen->fence.tail = fence;

   screen->fence.emit(&screen->base, &fence->sequence);

   assert(fence->state == NOUVEAU_FENCE_STATE_EMITTING);
   fence->state = NOUVEAU_FENCE_STATE_EMITTED;
}

void
nouveau_fence_del(struct nouveau_fence *fence)
{
   struct nouveau_fence *it;
   struct nouveau_screen *screen = fence->screen;

   if (fence->state == NOUVEAU_FENCE_STATE_EMITTED ||
       fence->state == NOUVEAU_FENCE_STATE_FLUSHED) {
      if (fence == screen->fence.head) {
         screen->fence.head = fence->next;
         if (!screen->fence.head)
            screen->fence.tail = NULL;
      } else {
         for (it = screen->fence.head; it && it->next != fence; it = it->next);
         it->next = fence->next;
         if (screen->fence.tail == fence)
            screen->fence.tail = it;
      }
   }

   if (!list_is_empty(&fence->work)) {
      debug_printf("WARNING: deleting fence with work still pending !\n");
      nouveau_fence_trigger_work(fence);
   }

   FREE(fence);
}

void
nouveau_fence_cleanup(struct nouveau_screen *screen)
{
   if (screen->fence.current) {
      struct nouveau_fence *current = NULL;

      /* nouveau_fence_wait will create a new current fence, so wait on the
       * _current_ one, and remove both.
       */
      nouveau_fence_ref(screen->fence.current, &current);
      nouveau_fence_wait(current, NULL);
      nouveau_fence_ref(NULL, &current);
      nouveau_fence_ref(NULL, &screen->fence.current);
   }
}

void
nouveau_fence_update(struct nouveau_screen *screen, bool flushed)
{
   struct nouveau_fence *fence;
   struct nouveau_fence *next = NULL;
   u32 sequence = screen->fence.update(&screen->base);

   if (screen->fence.sequence_ack == sequence)
      return;
   screen->fence.sequence_ack = sequence;

   for (fence = screen->fence.head; fence; fence = next) {
      next = fence->next;
      sequence = fence->sequence;

      fence->state = NOUVEAU_FENCE_STATE_SIGNALLED;

      nouveau_fence_trigger_work(fence);
      nouveau_fence_ref(NULL, &fence);

      if (sequence == screen->fence.sequence_ack)
         break;
   }
   screen->fence.head = next;
   if (!next)
      screen->fence.tail = NULL;

   if (flushed) {
      for (fence = next; fence; fence = fence->next)
         if (fence->state == NOUVEAU_FENCE_STATE_EMITTED)
            fence->state = NOUVEAU_FENCE_STATE_FLUSHED;
   }
}

#define NOUVEAU_FENCE_MAX_SPINS (1 << 31)

bool
nouveau_fence_signalled(struct nouveau_fence *fence)
{
   struct nouveau_screen *screen = fence->screen;

   if (fence->state == NOUVEAU_FENCE_STATE_SIGNALLED)
      return true;

   if (fence->state >= NOUVEAU_FENCE_STATE_EMITTED)
      nouveau_fence_update(screen, false);

   return fence->state == NOUVEAU_FENCE_STATE_SIGNALLED;
}

static bool
nouveau_fence_kick(struct nouveau_fence *fence)
{
   struct nouveau_screen *screen = fence->screen;

   /* wtf, someone is waiting on a fence in flush_notify handler? */
   assert(fence->state != NOUVEAU_FENCE_STATE_EMITTING);

   if (fence->state < NOUVEAU_FENCE_STATE_EMITTED) {
      PUSH_SPACE(screen->pushbuf, 8);
      /* The space allocation might trigger a flush, which could emit the
       * current fence. So check again.
       */
      if (fence->state < NOUVEAU_FENCE_STATE_EMITTED)
         nouveau_fence_emit(fence);
   }

   if (fence->state < NOUVEAU_FENCE_STATE_FLUSHED)
      if (nouveau_pushbuf_kick(screen->pushbuf, screen->pushbuf->channel))
         return false;

   if (fence == screen->fence.current)
      nouveau_fence_next(screen);

   nouveau_fence_update(screen, false);

   return true;
}

bool
nouveau_fence_wait(struct nouveau_fence *fence, struct pipe_debug_callback *debug)
{
   struct nouveau_screen *screen = fence->screen;
   uint32_t spins = 0;
   int64_t start = 0;

   if (debug && debug->debug_message)
      start = os_time_get_nano();

   if (!nouveau_fence_kick(fence))
      return false;

   do {
      if (fence->state == NOUVEAU_FENCE_STATE_SIGNALLED) {
         if (debug && debug->debug_message)
            pipe_debug_message(debug, PERF_INFO,
                               "stalled %.3f ms waiting for fence",
                               (os_time_get_nano() - start) / 1000000.f);
         return true;
      }
      if (!spins)
         NOUVEAU_DRV_STAT(screen, any_non_kernel_fence_sync_count, 1);
      spins++;
#ifdef PIPE_OS_UNIX
      if (!(spins % 8)) /* donate a few cycles */
         sched_yield();
#endif

      nouveau_fence_update(screen, false);
   } while (spins < NOUVEAU_FENCE_MAX_SPINS);

   debug_printf("Wait on fence %u (ack = %u, next = %u) timed out !\n",
                fence->sequence,
                screen->fence.sequence_ack, screen->fence.sequence);

   return false;
}

void
nouveau_fence_next(struct nouveau_screen *screen)
{
   if (screen->fence.current->state < NOUVEAU_FENCE_STATE_EMITTING) {
      if (screen->fence.current->ref > 1)
         nouveau_fence_emit(screen->fence.current);
      else
         return;
   }

   nouveau_fence_ref(NULL, &screen->fence.current);

   nouveau_fence_new(screen, &screen->fence.current);
}

void
nouveau_fence_unref_bo(void *data)
{
   struct nouveau_bo *bo = data;

   nouveau_bo_ref(NULL, &bo);
}

bool
nouveau_fence_work(struct nouveau_fence *fence,
                   void (*func)(void *), void *data)
{
   struct nouveau_fence_work *work;

   if (!fence || fence->state == NOUVEAU_FENCE_STATE_SIGNALLED) {
      func(data);
      return true;
   }

   work = CALLOC_STRUCT(nouveau_fence_work);
   if (!work)
      return false;
   work->func = func;
   work->data = data;
   list_add(&work->list, &fence->work);
   p_atomic_inc(&fence->work_count);
   if (fence->work_count > 64)
      nouveau_fence_kick(fence);
   return true;
}

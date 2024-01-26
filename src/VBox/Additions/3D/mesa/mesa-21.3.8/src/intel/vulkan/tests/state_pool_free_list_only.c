/*
 * Copyright © 2015 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include <pthread.h>

#include "anv_private.h"
#include "test_common.h"

#define NUM_THREADS 8
#define STATES_PER_THREAD_LOG2 12
#define STATES_PER_THREAD (1 << STATES_PER_THREAD_LOG2)

#include "state_pool_test_helper.h"

int main(void)
{
   struct anv_physical_device physical_device = { };
   struct anv_device device = {
      .physical = &physical_device,
   };
   struct anv_state_pool state_pool;

   pthread_mutex_init(&device.mutex, NULL);
   anv_bo_cache_init(&device.bo_cache, &device);
   anv_state_pool_init(&state_pool, &device, "test", 4096, 0, 4096);

   /* Grab one so a zero offset is impossible */
   anv_state_pool_alloc(&state_pool, 16, 16);

   /* Grab and return enough states that the state pool test below won't
    * actually ever resize anything.
    */
   {
      struct anv_state states[NUM_THREADS * STATES_PER_THREAD];
      for (unsigned i = 0; i < NUM_THREADS * STATES_PER_THREAD; i++) {
         states[i] = anv_state_pool_alloc(&state_pool, 16, 16);
         ASSERT(states[i].offset != 0);
      }

      for (unsigned i = 0; i < NUM_THREADS * STATES_PER_THREAD; i++)
         anv_state_pool_free(&state_pool, states[i]);
   }

   run_state_pool_test(&state_pool);

   anv_state_pool_finish(&state_pool);
   pthread_mutex_destroy(&device.mutex);
}

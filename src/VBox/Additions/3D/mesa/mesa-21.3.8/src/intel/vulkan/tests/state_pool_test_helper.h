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

struct job {
   struct anv_state_pool *pool;
   unsigned id;
   pthread_t thread;
} jobs[NUM_THREADS];

pthread_barrier_t barrier;

static void *alloc_states(void *void_job)
{
   struct job *job = void_job;

   const unsigned chunk_size = 1 << (job->id % STATES_PER_THREAD_LOG2);
   const unsigned num_chunks = STATES_PER_THREAD / chunk_size;

   struct anv_state states[chunk_size];

   pthread_barrier_wait(&barrier);

   for (unsigned c = 0; c < num_chunks; c++) {
      for (unsigned i = 0; i < chunk_size; i++) {
         states[i] = anv_state_pool_alloc(job->pool, 16, 16);
         memset(states[i].map, 139, 16);
         ASSERT(states[i].offset != 0);
      }

      for (unsigned i = 0; i < chunk_size; i++)
         anv_state_pool_free(job->pool, states[i]);
   }

   return NULL;
}

static void run_state_pool_test(struct anv_state_pool *state_pool)
{
   pthread_barrier_init(&barrier, NULL, NUM_THREADS);

   for (unsigned i = 0; i < NUM_THREADS; i++) {
      jobs[i].pool = state_pool;
      jobs[i].id = i;
      pthread_create(&jobs[i].thread, NULL, alloc_states, &jobs[i]);
   }

   for (unsigned i = 0; i < NUM_THREADS; i++)
      pthread_join(jobs[i].thread, NULL);
}

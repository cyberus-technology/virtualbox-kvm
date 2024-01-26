/*
 * Copyright © 2017 Intel Corporation
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

#ifndef INTEL_CLFLUSH_H
#define INTEL_CLFLUSH_H

#define CACHELINE_SIZE 64
#define CACHELINE_MASK 63

static inline void
intel_clflush_range(void *start, size_t size)
{
   void *p = (void *) (((uintptr_t) start) & ~CACHELINE_MASK);
   void *end = start + size;

   while (p < end) {
      __builtin_ia32_clflush(p);
      p += CACHELINE_SIZE;
   }
}

static inline void
intel_flush_range(void *start, size_t size)
{
   __builtin_ia32_mfence();
   intel_clflush_range(start, size);
}

static inline void
intel_invalidate_range(void *start, size_t size)
{
   intel_clflush_range(start, size);

   /* Modern Atom CPUs (Baytrail+) have issues with clflush serialization,
    * where mfence is not a sufficient synchronization barrier.  We must
    * double clflush the last cacheline.  This guarantees it will be ordered
    * after the preceding clflushes, and then the mfence guards against
    * prefetches crossing the clflush boundary.
    *
    * See kernel commit 396f5d62d1a5fd99421855a08ffdef8edb43c76e
    * ("drm: Restore double clflush on the last partial cacheline")
    * and https://bugs.freedesktop.org/show_bug.cgi?id=92845.
    */
   __builtin_ia32_clflush(start + size - 1);
   __builtin_ia32_mfence();
}

#endif

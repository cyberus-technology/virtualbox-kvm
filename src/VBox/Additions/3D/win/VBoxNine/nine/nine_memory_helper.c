/*
 * Copyright (C) 2016-2023 Oracle and/or its affiliates.
 *
 * This file is part of VirtualBox base platform packages, as
 * available from https://www.virtualbox.org.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, in version 3 of the
 * License.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <https://www.gnu.org/licenses>.
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */

/*
 * Copyright 2020 Axel Davy <davyaxel0@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * on the rights to use, copy, modify, merge, publish, distribute, sub
 * license, and/or sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHOR(S) AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE. */

#include "util/list.h"
#include "util/u_memory.h"
#include "util/slab.h"

#include "nine_debug.h"
#include "nine_memory_helper.h"
#include "nine_state.h"

#include <iprt/win/windows.h>

#define DIVUP(a,b) (((a)+(b)-1)/(b))

/* Required alignment for allocations */
#define NINE_ALLOCATION_ALIGNMENT 32

#define DBG_CHANNEL (DBG_BASETEXTURE|DBG_SURFACE|DBG_VOLUME|DBG_TEXTURE|DBG_CUBETEXTURE)

struct nine_allocation {
    unsigned is_external;
    void *external;
};

struct nine_allocator {
    struct slab_mempool external_allocation_pool;
    CRITICAL_SECTION mutex_slab;
};

struct nine_allocation *
nine_allocate(struct nine_allocator *allocator, unsigned size)
{
    struct nine_allocation *allocation;
    (void)allocator;
    assert(sizeof(struct nine_allocation) <= NINE_ALLOCATION_ALIGNMENT);
    allocation = align_calloc(size + NINE_ALLOCATION_ALIGNMENT, NINE_ALLOCATION_ALIGNMENT);
    allocation->is_external = false;
    return allocation;
}


void nine_free(struct nine_allocator *allocator, struct nine_allocation *allocation)
{
    if (allocation->is_external) {
        EnterCriticalSection(&allocator->mutex_slab);
        slab_free_st(&allocator->external_allocation_pool, allocation);
        LeaveCriticalSection(&allocator->mutex_slab);
    } else
        align_free(allocation);
}

void nine_free_worker(struct nine_allocator *allocator, struct nine_allocation *allocation)
{
    nine_free(allocator, allocation);
}

void *nine_get_pointer(struct nine_allocator *allocator, struct nine_allocation *allocation)
{
    (void)allocator;
    if (allocation->is_external)
        return allocation->external;
    return (uint8_t *)allocation + NINE_ALLOCATION_ALIGNMENT;
}

void nine_pointer_weakrelease(struct nine_allocator *allocator, struct nine_allocation *allocation)
{
    (void)allocator;
    (void)allocation;
}

void nine_pointer_strongrelease(struct nine_allocator *allocator, struct nine_allocation *allocation)
{
    (void)allocator;
    (void)allocation;
}

void nine_pointer_delayedstrongrelease(struct nine_allocator *allocator,
                                       struct nine_allocation *allocation,
                                       unsigned *counter)
{
    (void)allocator;
    (void)allocation;
    (void)counter;
}

struct nine_allocation *
nine_suballocate(struct nine_allocator* allocator, struct nine_allocation *allocation, int offset)
{
    struct nine_allocation *new_allocation;
    EnterCriticalSection(&allocator->mutex_slab);
    new_allocation = slab_alloc_st(&allocator->external_allocation_pool);
    LeaveCriticalSection(&allocator->mutex_slab);
    new_allocation->is_external = true;
    if (allocation->is_external)
       new_allocation->external = (uint8_t *)allocation->external + offset;
    else
       new_allocation->external = (uint8_t *)allocation + NINE_ALLOCATION_ALIGNMENT + offset;
    return new_allocation;
}

struct nine_allocation *
nine_wrap_external_pointer(struct nine_allocator* allocator, void* data)
{
    struct nine_allocation *new_allocation;
    EnterCriticalSection(&allocator->mutex_slab);
    new_allocation = slab_alloc_st(&allocator->external_allocation_pool);
    LeaveCriticalSection(&allocator->mutex_slab);
    new_allocation->is_external = true;
    new_allocation->external = data;
    return new_allocation;
}

struct nine_allocator *
nine_allocator_create(struct NineDevice9 *device, int memfd_virtualsizelimit)
{
    struct nine_allocator* allocator = MALLOC(sizeof(struct nine_allocator));
    (void)device;
    (void)memfd_virtualsizelimit;

    if (!allocator)
        return NULL;

    slab_create(&allocator->external_allocation_pool, sizeof(struct nine_allocation), 4096);
    InitializeCriticalSection(&allocator->mutex_slab);

    return allocator;
}

void
nine_allocator_destroy(struct nine_allocator *allocator)
{
    slab_destroy(&allocator->external_allocation_pool);
    DeleteCriticalSection(&allocator->mutex_slab);
}

/*
 * state block implementation
 *
 * Copyright 2002 Raphael Junqueira
 * Copyright 2004 Jason Edmeades
 * Copyright 2005 Oliver Stieber
 * Copyright 2007 Stefan Dösinger for CodeWeavers
 * Copyright 2009 Henri Verbeet for CodeWeavers
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

/*
 * Oracle LGPL Disclaimer: For the avoidance of doubt, except that if any license choice
 * other than GPL or LGPL is available it will apply instead, Oracle elects to use only
 * the Lesser General Public License version 2.1 (LGPLv2) at this time for any software where
 * a choice of LGPL license versions is made available with the language indicating
 * that LGPLv2 or any later version may be used, or where a choice of which version
 * of the LGPL is applied is otherwise unspecified.
 */

#include "config.h"
#include "wined3d_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d);

/* Allocates the correct amount of space for pixel and vertex shader constants,
 * along with their set/changed flags on the given stateblock object
 */
static HRESULT stateblock_allocate_shader_constants(IWineD3DStateBlockImpl *object)
{
    IWineD3DDeviceImpl *device = object->device;

    /* Allocate space for floating point constants */
    object->pixelShaderConstantF = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
            sizeof(float) * device->d3d_pshader_constantF * 4);
    if (!object->pixelShaderConstantF) goto fail;

    object->changed.pixelShaderConstantsF = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
            sizeof(BOOL) * device->d3d_pshader_constantF);
    if (!object->changed.pixelShaderConstantsF) goto fail;

    object->vertexShaderConstantF = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
            sizeof(float) * device->d3d_vshader_constantF * 4);
    if (!object->vertexShaderConstantF) goto fail;

    object->changed.vertexShaderConstantsF = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
            sizeof(BOOL) * device->d3d_vshader_constantF);
    if (!object->changed.vertexShaderConstantsF) goto fail;

    object->contained_vs_consts_f = HeapAlloc(GetProcessHeap(), 0,
            sizeof(DWORD) * device->d3d_vshader_constantF);
    if (!object->contained_vs_consts_f) goto fail;

    object->contained_ps_consts_f = HeapAlloc(GetProcessHeap(), 0,
            sizeof(DWORD) * device->d3d_pshader_constantF);
    if (!object->contained_ps_consts_f) goto fail;

    return WINED3D_OK;

fail:
    ERR("Failed to allocate memory\n");
    HeapFree(GetProcessHeap(), 0, object->pixelShaderConstantF);
    HeapFree(GetProcessHeap(), 0, object->changed.pixelShaderConstantsF);
    HeapFree(GetProcessHeap(), 0, object->vertexShaderConstantF);
    HeapFree(GetProcessHeap(), 0, object->changed.vertexShaderConstantsF);
    HeapFree(GetProcessHeap(), 0, object->contained_vs_consts_f);
    HeapFree(GetProcessHeap(), 0, object->contained_ps_consts_f);
    return E_OUTOFMEMORY;
}

static inline void stateblock_set_bits(DWORD *map, UINT map_size)
{
    DWORD mask = (1 << (map_size & 0x1f)) - 1;
    memset(map, 0xff, (map_size >> 5) * sizeof(*map));
    if (mask) map[map_size >> 5] = mask;
}


HRESULT stateblock_init(IWineD3DStateBlockImpl *stateblock, IWineD3DDeviceImpl *device, WINED3DSTATEBLOCKTYPE type)
{
    stateblock->ref = 1;
    stateblock->device = device;
    stateblock->blockType = type;

    return stateblock_allocate_shader_constants(stateblock);
}

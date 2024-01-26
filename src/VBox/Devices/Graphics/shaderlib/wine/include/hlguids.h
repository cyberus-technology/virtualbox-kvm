/*
 * Implementation of hyperlinking (hlink.dll)
 *
 * Copyright 2005 Aric Stewart for CodeWeavers
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

#ifndef __WINE_HLGUIDS_H__
#define __WINE_HLGUIDS_H__

DEFINE_GUID(CLSID_StdHlink,
    0x79eac9d0, 0xbaf9, 0x11ce, 0x8c, 0x82, 0x00, 0xaa,0x00,0x4b,0xa9,0x0b);
DEFINE_GUID(CLSID_StdHlinkBrowseContext,
    0x79eac9d1, 0xbaf9, 0x11ce, 0x8c, 0x82, 0x00, 0xaa,0x00,0x4b,0xa9,0x0b);
DEFINE_GUID(CLSID_IID_IExtensionServices,
    0x79eac9cb, 0xbaf9, 0x11ce, 0x8c, 0x82, 0x00, 0xaa,0x00,0x4b,0xa9,0x0b);

#endif

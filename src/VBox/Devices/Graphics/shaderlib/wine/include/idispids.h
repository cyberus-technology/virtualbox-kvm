/*
 * Copyright (C) 2005 Mike McCormack
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

#ifndef __WINE_IDISPIDS_H__
#define __WINE_IDISPIDS_H__

#define DISPID_READYSTATE (-525)
#define DISPID_READYSTATECHANGE (-609)
#define DISPID_AMBIENT_TRANSFERPRIORITY (-728)
#define DISPID_AMBIENT_OFFLINEIFNOTCONNECTED (-5501)
#define DISPID_AMBIENT_SILENT (-5502)

#ifndef DISPID_AMBIENT_CODEPAGE
#define DISPID_AMBIENT_CODEPAGE (-725)
#define DISPID_AMBIENT_CHARSET (-727)
#endif /* DISPID_AMBIENT_CODEPAGE */

#endif /* __WINE_IDISPIDS_H__ */

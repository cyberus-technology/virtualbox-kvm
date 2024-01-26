/*
 * Copyright (C) 2006 Dmitry Timoshkov
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

#ifndef __WINE_NTDSAPI_H
#define __WINE_NTDSAPI_H

/* FIXME: #include <schedule.h> */

#ifdef __cplusplus
extern "C" {
#endif

DWORD WINAPI DsMakeSpnA(LPCSTR, LPCSTR, LPCSTR, USHORT, LPCSTR, DWORD*, LPSTR);
DWORD WINAPI DsMakeSpnW(LPCWSTR, LPCWSTR, LPCWSTR, USHORT, LPCWSTR, DWORD*, LPWSTR);
#define DsMakeSpn WINELIB_NAME_AW(DsMakeSpn)

typedef enum
{
    DS_SPN_DNS_HOST    = 0,
    DS_SPN_DN_HOST     = 1,
    DS_SPN_NB_HOST     = 2,
    DS_SPN_DOMAIN      = 3,
    DS_SPN_NB_DOMAIN   = 4,
    DS_SPN_SERVICE     = 5
} DS_SPN_NAME_TYPE;

typedef enum
{
    DS_SPN_ADD_SPN_OP     = 0,
    DS_SPN_REPLACE_SPN_OP = 1,
    DS_SPN_DELETE_SPN_OP  = 2
} DS_SPN_WRITE_OP;

DWORD WINAPI DsServerRegisterSpnA(DS_SPN_WRITE_OP operation, LPCSTR ServiceClass, LPCSTR UserObjectDN);
DWORD WINAPI DsServerRegisterSpnW(DS_SPN_WRITE_OP operation, LPCWSTR ServiceClass, LPCWSTR UserObjectDN);
#define DsServerRegisterSpn WINELIB_NAME_AW(DsServerRegisterSpn)

#ifdef __cplusplus
}
#endif

#endif /* __WINE_NTDSAPI_H */

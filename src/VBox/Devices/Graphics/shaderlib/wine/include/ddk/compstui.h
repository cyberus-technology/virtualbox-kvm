/*
 * Definitions for Common Property Sheet User Interface
 *
 * Copyright 2006 Detlef Riekenberg
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
 *
 */

/*
 * Oracle LGPL Disclaimer: For the avoidance of doubt, except that if any license choice
 * other than GPL or LGPL is available it will apply instead, Oracle elects to use only
 * the Lesser General Public License version 2.1 (LGPLv2) at this time for any software where
 * a choice of LGPL license versions is made available with the language indicating
 * that LGPLv2 or any later version may be used, or where a choice of which version
 * of the LGPL is applied is otherwise unspecified.
 */

#ifndef _COMPSTUI_
#define _COMPSTUI_

#ifdef __cplusplus
extern "C" {
#endif

/* DEFINES */

#define PROPSHEETUI_INFO_VERSION    0x0100

#define PSUIINFO_UNICODE    1

/* return-values for CommonPropertySheetUI on success */
#define CPSUI_CANCEL            0
#define CPSUI_OK                1
#define CPSUI_RESTARTWINDOWS    2
#define CPSUI_REBOOTSYSTEM      3


/* TYPES */
typedef DWORD (CALLBACK *PFNCOMPROPSHEET)(HANDLE, UINT, LPARAM, LPARAM);

typedef struct _PROPSHEETUI_INFO {
    WORD            cbSize;
    WORD            Version;
    WORD            Flags;              /* set PSUIINFO_UNICODE for UNICODE */
    WORD            Reason;
    HANDLE          hComPropSheet;
    PFNCOMPROPSHEET pfnComPropSheet;
    LPARAM          lParamInit;
    DWORD           UserData;
    DWORD           Result;
 } PROPSHEETUI_INFO, *PPROPSHEETUI_INFO;

typedef LONG  (CALLBACK *PFNPROPSHEETUI)(PROPSHEETUI_INFO, LPARAM);

/* FUNCTIONS */
LONG WINAPI CommonPropertySheetUIA(HWND, PFNPROPSHEETUI, LPARAM, LPDWORD);
LONG WINAPI CommonPropertySheetUIW(HWND, PFNPROPSHEETUI, LPARAM, LPDWORD);
#define CommonPropertySheetUI WINELIB_NAME_AW(CommonPropertySheetUI)

ULONG_PTR WINAPI GetPSTUIUserData(HWND);
BOOL      WINAPI SetPSTUIUserData(HWND, ULONG_PTR);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif  /* _COMPSTUI_ */

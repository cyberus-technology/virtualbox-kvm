/*
 * errorrep.h - error reporting APIs (implemented in faultrep.dll)
 *
 * Copyright 2007 Google (Mikolaj Zalewski)
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

#ifndef __WINE_ERRORREP_H
#define __WINE_ERRORREP_H

typedef enum tagEFaultRepRetVal
{
    frrvOk,
    frrvOkManifest,
    frrvOkQueued,
    frrvOkErr,
    frrvErrNoDW,
    frrvErrTimeout,
    frrvLaunchDebugger,
    frrvOkHeadless,
    frrvErrAnotherInstance
} EFaultRepRetVal;

EFaultRepRetVal WINAPI ReportFault(LPEXCEPTION_POINTERS, DWORD);

BOOL WINAPI AddERExcludedApplicationA(LPCSTR);
BOOL WINAPI AddERExcludedApplicationW(LPCWSTR);
#define     AddERExcludedApplication WINELIB_NAME_AW(AddERExcludedApplication)

#endif  /* __WINE_ERORREP_H */

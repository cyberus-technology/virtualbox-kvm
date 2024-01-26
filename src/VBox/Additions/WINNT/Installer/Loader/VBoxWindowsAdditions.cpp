/* $Id: VBoxWindowsAdditions.cpp $ */
/** @file
 * VBoxWindowsAdditions - The Windows Guest Additions Loader.
 *
 * This is STUB which select whether to install 32-bit or 64-bit additions.
 */

/*
 * Copyright (C) 2006-2023 Oracle and/or its affiliates.
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


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#include <iprt/cdefs.h>
#include <iprt/win/windows.h>
#ifndef ERROR_ELEVATION_REQUIRED    /* Windows Vista and later. */
# define ERROR_ELEVATION_REQUIRED  740
#endif

#include <iprt/string.h>
#include <iprt/utf16.h>

#include "NoCrtOutput.h"


static BOOL IsWow64(void)
{
    BOOL fIsWow64 = FALSE;
    typedef BOOL (WINAPI *LPFN_ISWOW64PROCESS)(HANDLE, PBOOL);
    LPFN_ISWOW64PROCESS fnIsWow64Process = (LPFN_ISWOW64PROCESS)GetProcAddress(GetModuleHandleW(L"kernel32"), "IsWow64Process");
    if (fnIsWow64Process != NULL)
    {
        if (!fnIsWow64Process(GetCurrentProcess(), &fIsWow64))
        {
            ErrorMsgLastErr("Unable to determine the process type!");

            /* Error in retrieving process type - assume that we're running on 32bit. */
            fIsWow64 = FALSE;
        }
    }
    return fIsWow64;
}

static int WaitForProcess2(HANDLE hProcess)
{
    /*
     * Wait for the process, make sure the deal with messages.
     */
    for (;;)
    {
        DWORD dwRc = MsgWaitForMultipleObjects(1, &hProcess, FALSE, 5000/*ms*/, QS_ALLEVENTS);

        MSG Msg;
        while (PeekMessageW(&Msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&Msg);
            DispatchMessageW(&Msg);
        }

        if (dwRc == WAIT_OBJECT_0)
            break;
        if (   dwRc != WAIT_TIMEOUT
            && dwRc != WAIT_OBJECT_0 + 1)
        {
            ErrorMsgLastErrSUR("MsgWaitForMultipleObjects failed: ", dwRc);
            break;
        }
    }

    /*
     * Collect the process info.
     */
    DWORD dwExitCode;
    if (GetExitCodeProcess(hProcess, &dwExitCode))
        return (int)dwExitCode;
    return ErrorMsgRcLastErr(16, "GetExitCodeProcess failed");
}

static int WaitForProcess(HANDLE hProcess)
{
    DWORD WaitRc = WaitForSingleObjectEx(hProcess, INFINITE, TRUE);
    while (   WaitRc == WAIT_IO_COMPLETION
           || WaitRc == WAIT_TIMEOUT)
        WaitRc = WaitForSingleObjectEx(hProcess, INFINITE, TRUE);
    if (WaitRc == WAIT_OBJECT_0)
    {
        DWORD dwExitCode;
        if (GetExitCodeProcess(hProcess, &dwExitCode))
            return (int)dwExitCode;
        return ErrorMsgRcLastErr(16, "GetExitCodeProcess failed");
    }
    return ErrorMsgRcLastErrSUR(16, "MsgWaitForMultipleObjects failed: ", WaitRc);
}

#ifndef IPRT_NO_CRT
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
#else
int main()
#endif
{
#ifndef IPRT_NO_CRT
    RT_NOREF(hInstance, hPrevInstance, lpCmdLine, nCmdShow);
#endif

    /*
     * Gather the parameters of the real installer program.
     */
    SetLastError(NO_ERROR);
    WCHAR wszCurDir[MAX_PATH] = { 0 };
    DWORD cwcCurDir = GetCurrentDirectoryW(sizeof(wszCurDir), wszCurDir);
    if (cwcCurDir == 0 || cwcCurDir >= sizeof(wszCurDir))
        return ErrorMsgRcLastErrSUR(12, "GetCurrentDirectoryW failed: ", cwcCurDir);

    SetLastError(NO_ERROR);
    WCHAR wszExePath[MAX_PATH] = { 0 };
    DWORD cwcExePath = GetModuleFileNameW(NULL, wszExePath, sizeof(wszExePath));
    if (cwcExePath == 0 || cwcExePath >= sizeof(wszExePath))
        return ErrorMsgRcLastErrSUR(13, "GetModuleFileNameW failed: ", cwcExePath);

    /*
    * Strip the extension off the module name and construct the arch specific
    * one of the real installer program.
    */
    DWORD off = cwcExePath - 1;
    while (   off > 0
           && (   wszExePath[off] != '/'
               && wszExePath[off] != '\\'
               && wszExePath[off] != ':'))
    {
        if (wszExePath[off] == '.')
        {
            wszExePath[off] = '\0';
            cwcExePath = off;
            break;
        }
        off--;
    }

    WCHAR const  *pwszSuff = IsWow64() ? L"-amd64.exe" : L"-x86.exe";
    int rc = RTUtf16Copy(&wszExePath[cwcExePath], RT_ELEMENTS(wszExePath) - cwcExePath, pwszSuff);
    if (RT_FAILURE(rc))
        return ErrorMsgRc(14, "Real installer name is too long!");
    cwcExePath += RTUtf16Len(&wszExePath[cwcExePath]);

    /*
     * Replace the first argument of the argument list.
     */
    PWCHAR  pwszNewCmdLine = NULL;
    LPCWSTR pwszOrgCmdLine = GetCommandLineW();
    if (pwszOrgCmdLine) /* Dunno if this can be NULL, but whatever. */
    {
        /* Skip the first argument in the original. */
        /** @todo Is there some ISBLANK or ISSPACE macro/function in Win32 that we could
         *        use here, if it's correct wrt. command line conventions? */
        WCHAR wch;
        while ((wch = *pwszOrgCmdLine) == L' ' || wch == L'\t')
            pwszOrgCmdLine++;
        if (wch == L'"')
        {
            pwszOrgCmdLine++;
            while ((wch = *pwszOrgCmdLine) != L'\0')
            {
                pwszOrgCmdLine++;
                if (wch == L'"')
                    break;
            }
        }
        else
        {
            while ((wch = *pwszOrgCmdLine) != L'\0')
            {
                pwszOrgCmdLine++;
                if (wch == L' ' || wch == L'\t')
                    break;
            }
        }
        while ((wch = *pwszOrgCmdLine) == L' ' || wch == L'\t')
            pwszOrgCmdLine++;

        /* Join up "wszExePath" with the remainder of the original command line. */
        size_t cwcOrgCmdLine = RTUtf16Len(pwszOrgCmdLine);
        size_t cwcNewCmdLine = 1 + cwcExePath + 1 + 1 + cwcOrgCmdLine + 1;
        PWCHAR pwsz = pwszNewCmdLine = (PWCHAR)LocalAlloc(LPTR, cwcNewCmdLine * sizeof(WCHAR));
        if (!pwsz)
            return ErrorMsgRcSUS(15, "Out of memory (", cwcNewCmdLine * sizeof(WCHAR), " bytes)");
        *pwsz++ = L'"';
        memcpy(pwsz, wszExePath, cwcExePath * sizeof(pwsz[0]));
        pwsz += cwcExePath;
        *pwsz++ = L'"';
        if (cwcOrgCmdLine)
        {
            *pwsz++ = L' ';
            memcpy(pwsz, pwszOrgCmdLine, cwcOrgCmdLine * sizeof(pwsz[0]));
        }
        else
        {
            *pwsz = L'\0';
            pwszOrgCmdLine = NULL;
        }
    }

    /*
     * Start the process.
     */
    int                 rcExit      = 0;
    STARTUPINFOW        StartupInfo = { sizeof(StartupInfo), 0 };
    PROCESS_INFORMATION ProcInfo    = { 0 };
    SetLastError(740);
    BOOL fOk = CreateProcessW(wszExePath,
                              pwszNewCmdLine,
                              NULL /*pProcessAttributes*/,
                              NULL /*pThreadAttributes*/,
                              TRUE /*fInheritHandles*/,
                              0    /*dwCreationFlags*/,
                              NULL /*pEnvironment*/,
                              NULL /*pCurrentDirectory*/,
                              &StartupInfo,
                              &ProcInfo);
    if (fOk)
    {
        /* Wait for the process to finish. */
        CloseHandle(ProcInfo.hThread);
        rcExit = WaitForProcess(ProcInfo.hProcess);
        CloseHandle(ProcInfo.hProcess);
    }
    else if (GetLastError() == ERROR_ELEVATION_REQUIRED)
    {
        /*
         * Elevation is required. That can be accomplished via ShellExecuteEx
         * and the runas atom.
         */
        MSG Msg;
        PeekMessage(&Msg, NULL, 0, 0, PM_NOREMOVE);
        CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

        SHELLEXECUTEINFOW ShExecInfo = { 0 };
        ShExecInfo.cbSize       = sizeof(SHELLEXECUTEINFOW);
        ShExecInfo.fMask        = SEE_MASK_NOCLOSEPROCESS;
        ShExecInfo.hwnd         = NULL;
        ShExecInfo.lpVerb       = L"runas" ;
        ShExecInfo.lpFile       = wszExePath;
        ShExecInfo.lpParameters = pwszOrgCmdLine; /* pass only args here!!! */
        ShExecInfo.lpDirectory  = wszCurDir;
        ShExecInfo.nShow        = SW_NORMAL;
        ShExecInfo.hProcess     = INVALID_HANDLE_VALUE;
        if (ShellExecuteExW(&ShExecInfo))
        {
            if (ShExecInfo.hProcess != INVALID_HANDLE_VALUE)
            {
                rcExit = WaitForProcess2(ShExecInfo.hProcess);
                CloseHandle(ShExecInfo.hProcess);
            }
            else
                rcExit = ErrorMsgRc(1, "ShellExecuteExW did not return a valid process handle!");
        }
        else
            rcExit = ErrorMsgRcLastErrSWSR(9, "Failed to execute '", wszExePath, "' via ShellExecuteExW!");
    }
    else
        rcExit = ErrorMsgRcLastErrSWSR(8, "Failed to execute '", wszExePath, "' via CreateProcessW!");

    if (pwszNewCmdLine)
        LocalFree(pwszNewCmdLine);

    return rcExit;
}


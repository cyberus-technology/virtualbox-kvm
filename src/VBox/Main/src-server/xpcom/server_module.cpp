/* $Id: server_module.cpp $ */
/** @file
 * XPCOM server process helper module implementation functions
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

#define LOG_GROUP LOG_GROUP_MAIN_VBOXSVC
#ifdef RT_OS_OS2
# include <prproces.h>
#endif

#include <nsMemory.h>
#include <nsString.h>
#include <nsCOMPtr.h>
#include <nsIFile.h>
#include <nsIGenericFactory.h>
#include <nsIServiceManagerUtils.h>
#include <nsICategoryManager.h>
#include <nsDirectoryServiceDefs.h>

#include <ipcIService.h>
#include <ipcIDConnectService.h>
#include <ipcCID.h>
#include <ipcdclient.h>

#include "prio.h"
#include "prproces.h"

// official XPCOM headers don't define it yet
#define IPC_DCONNECTSERVICE_CONTRACTID \
    "@mozilla.org/ipc/dconnect-service;1"

// generated file
#include <VBox/com/VirtualBox.h>

#include "server.h"
#include "LoggingNew.h"

#include <iprt/errcore.h>

#include <iprt/assert.h>
#include <iprt/param.h>
#include <iprt/path.h>
#include <iprt/process.h>
#include <iprt/env.h>
#include <iprt/string.h>
#include <iprt/thread.h>

#if defined(RT_OS_SOLARIS)
# include <sys/systeminfo.h>
#endif

/// @todo move this to RT headers (and use them in MachineImpl.cpp as well)
#if defined(RT_OS_WINDOWS) || defined(RT_OS_OS2)
#define HOSTSUFF_EXE ".exe"
#else /* !RT_OS_WINDOWS */
#define HOSTSUFF_EXE ""
#endif /* !RT_OS_WINDOWS */


/** Name of the server executable. */
const char VBoxSVC_exe[] = RTPATH_SLASH_STR "VBoxSVC" HOSTSUFF_EXE;

enum
{
    /** Amount of time to wait for the server to establish a connection, ms */
    VBoxSVC_Timeout = 30000,
    /** How often to perform a connection check, ms */
    VBoxSVC_WaitSlice = 100
};

/**
 *  Full path to the VBoxSVC executable.
 */
static char VBoxSVCPath[RTPATH_MAX];
static bool IsVBoxSVCPathSet = false;

/*
 *  The following macros define the method necessary to provide a list of
 *  interfaces implemented by the VirtualBox component. Note that this must be
 *  in sync with macros used for VirtualBox in server.cpp for the same purpose.
 */

NS_DECL_CLASSINFO(VirtualBoxWrap)
NS_IMPL_CI_INTERFACE_GETTER1(VirtualBoxWrap, IVirtualBox)

static nsresult vboxsvcSpawnDaemon(void)
{
    PRFileDesc *readable = nsnull, *writable = nsnull;
    PRProcessAttr *attr = nsnull;
    nsresult rv = NS_ERROR_FAILURE;
    PRFileDesc *devNull;
    // The ugly casts are necessary because the PR_CreateProcessDetached has
    // a const array of writable strings as a parameter. It won't write. */
    char * const args[] = { (char *)VBoxSVCPath, (char *)"--auto-shutdown", 0 };

    // Use a pipe to determine when the daemon process is in the position
    // to actually process requests. The daemon will write "READY" to the pipe.
    if (PR_CreatePipe(&readable, &writable) != PR_SUCCESS)
        goto end;
    PR_SetFDInheritable(writable, PR_TRUE);

    attr = PR_NewProcessAttr();
    if (!attr)
        goto end;

    if (PR_ProcessAttrSetInheritableFD(attr, writable, VBOXSVC_STARTUP_PIPE_NAME) != PR_SUCCESS)
        goto end;

    devNull = PR_Open("/dev/null", PR_RDWR, 0);
    if (!devNull)
        goto end;

    PR_ProcessAttrSetStdioRedirect(attr, PR_StandardInput, devNull);
    PR_ProcessAttrSetStdioRedirect(attr, PR_StandardOutput, devNull);
    PR_ProcessAttrSetStdioRedirect(attr, PR_StandardError, devNull);

    if (PR_CreateProcessDetached(VBoxSVCPath, args, nsnull, attr) != PR_SUCCESS)
        goto end;

    // Close /dev/null
    PR_Close(devNull);
    // Close the child end of the pipe to make it the only owner of the
    // file descriptor, so that unexpected closing can be detected.
    PR_Close(writable);
    writable = nsnull;

    char msg[10];
    RT_ZERO(msg);
    if (   PR_Read(readable, msg, sizeof(msg)-1) != 5
        || strcmp(msg, "READY"))
    {
        /* If several clients start VBoxSVC simultaneously only one can
         * succeed. So treat this as success as well. */
        rv = NS_OK;
        goto end;
    }

    rv = NS_OK;

end:
    if (readable)
        PR_Close(readable);
    if (writable)
        PR_Close(writable);
    if (attr)
        PR_DestroyProcessAttr(attr);
    return rv;
}


/**
 *  VirtualBox component constructor.
 *
 *  This constructor is responsible for starting the VirtualBox server
 *  process, connecting to it, and redirecting the constructor request to the
 *  VirtualBox component defined on the server.
 */
static NS_IMETHODIMP
VirtualBoxConstructor(nsISupports *aOuter, REFNSIID aIID,
                      void **aResult)
{
    LogFlowFuncEnter();

    nsresult rc = NS_OK;

    do
    {
        *aResult = NULL;
        if (NULL != aOuter)
        {
            rc = NS_ERROR_NO_AGGREGATION;
            break;
        }

        if (!IsVBoxSVCPathSet)
        {
            /* Get the directory containing XPCOM components -- the VBoxSVC
             * executable is expected in the parent directory. */
            nsCOMPtr<nsIProperties> dirServ = do_GetService(NS_DIRECTORY_SERVICE_CONTRACTID, &rc);
            if (NS_SUCCEEDED(rc))
            {
                nsCOMPtr<nsIFile> componentDir;
                rc = dirServ->Get(NS_XPCOM_COMPONENT_DIR,
                                  NS_GET_IID(nsIFile), getter_AddRefs(componentDir));

                if (NS_SUCCEEDED(rc))
                {
                    nsCAutoString path;
                    componentDir->GetNativePath(path);

                    LogFlowFunc(("component directory = \"%s\"\n", path.get()));
                    AssertBreakStmt(path.Length() + strlen(VBoxSVC_exe) < RTPATH_MAX,
                                    rc = NS_ERROR_FAILURE);

#if defined(RT_OS_SOLARIS) && defined(VBOX_WITH_HARDENING)
                    char achKernArch[128];
                    int cbKernArch = sysinfo(SI_ARCHITECTURE_K, achKernArch, sizeof(achKernArch));
                    if (cbKernArch > 0)
                    {
                        sprintf(VBoxSVCPath, "/opt/VirtualBox/%s%s", achKernArch, VBoxSVC_exe);
                        IsVBoxSVCPathSet = true;
                    }
                    else
                        rc = NS_ERROR_UNEXPECTED;
#else
                    strcpy(VBoxSVCPath, path.get());
                    RTPathStripFilename(VBoxSVCPath);
                    strcat(VBoxSVCPath, VBoxSVC_exe);

                    IsVBoxSVCPathSet = true;
#endif
                }
            }
            if (NS_FAILED(rc))
                break;
        }

        nsCOMPtr<ipcIService> ipcServ = do_GetService(IPC_SERVICE_CONTRACTID, &rc);
        if (NS_FAILED(rc))
            break;

        /* connect to the VBoxSVC server process */

        bool startedOnce = false;
        unsigned timeLeft = VBoxSVC_Timeout;

        do
        {
            LogFlowFunc(("Resolving server name \"%s\"...\n", VBOXSVC_IPC_NAME));

            PRUint32 serverID = 0;
            rc = ipcServ->ResolveClientName(VBOXSVC_IPC_NAME, &serverID);
            if (NS_FAILED(rc))
            {
                LogFlowFunc(("Starting server \"%s\"...\n", VBoxSVCPath));

                startedOnce = true;

                rc = vboxsvcSpawnDaemon();
                if (NS_FAILED(rc))
                    break;

                /* wait for the server process to establish a connection */
                do
                {
                    RTThreadSleep(VBoxSVC_WaitSlice);
                    rc = ipcServ->ResolveClientName(VBOXSVC_IPC_NAME, &serverID);
                    if (NS_SUCCEEDED(rc))
                        break;
                    if (timeLeft <= VBoxSVC_WaitSlice)
                    {
                        timeLeft = 0;
                        break;
                    }
                    timeLeft -= VBoxSVC_WaitSlice;
                }
                while (1);

                if (!timeLeft)
                {
                    rc = IPC_ERROR_WOULD_BLOCK;
                    break;
                }
            }

            LogFlowFunc(("Connecting to server (ID=%d)...\n", serverID));

            nsCOMPtr<ipcIDConnectService> dconServ =
                do_GetService(IPC_DCONNECTSERVICE_CONTRACTID, &rc);
            if (NS_FAILED(rc))
                break;

            rc = dconServ->CreateInstance(serverID,
                                          CLSID_VirtualBox,
                                          aIID, aResult);
            if (NS_SUCCEEDED(rc))
                break;

            LogFlowFunc(("Failed to connect (rc=%Rhrc (%#08x))\n", rc, rc));

            /* It's possible that the server gets shut down after we
             * successfully resolve the server name but before it
             * receives our CreateInstance() request. So, check for the
             * name again, and restart the cycle if it fails. */
            if (!startedOnce)
            {
                nsresult rc2 =
                    ipcServ->ResolveClientName(VBOXSVC_IPC_NAME, &serverID);
                if (NS_SUCCEEDED(rc2))
                    break;

                LogFlowFunc(("Server seems to have terminated before receiving our request. Will try again.\n"));
            }
            else
                break;
        }
        while (1);
    }
    while (0);

    LogFlowFunc(("rc=%Rhrc (%#08x)\n", rc, rc));
    LogFlowFuncLeave();

    return rc;
}

#if 0
/// @todo not really necessary for the moment
/**
 *
 * @param aCompMgr
 * @param aPath
 * @param aLoaderStr
 * @param aType
 * @param aInfo
 *
 * @return
 */
static NS_IMETHODIMP
VirtualBoxRegistration(nsIComponentManager *aCompMgr,
                       nsIFile *aPath,
                       const char *aLoaderStr,
                       const char *aType,
                       const nsModuleComponentInfo *aInfo)
{
    nsCAutoString modulePath;
    aPath->GetNativePath(modulePath);
    nsCAutoString moduleTarget;
    aPath->GetNativeTarget(moduleTarget);

    LogFlowFunc(("aPath=%s, aTarget=%s, aLoaderStr=%s, aType=%s\n",
                 modulePath.get(), moduleTarget.get(), aLoaderStr, aType));

    nsresult rc = NS_OK;

    return rc;
}
#endif

/**
 *  Component definition table.
 *  Lists all components defined in this module.
 */
static const nsModuleComponentInfo components[] =
{
    {
        "VirtualBox component", // description
        NS_VIRTUALBOX_CID, NS_VIRTUALBOX_CONTRACTID, // CID/ContractID
        VirtualBoxConstructor, // constructor function
        NULL, /* VirtualBoxRegistration, */ // registration function
        NULL, // deregistration function
        NULL, // destructor function
        /// @todo
        NS_CI_INTERFACE_GETTER_NAME(VirtualBoxWrap), // interfaces function
        NULL, // language helper
        /// @todo
        &NS_CLASSINFO_NAME(VirtualBoxWrap) // global class info & flags
    }
};

NS_IMPL_NSGETMODULE(VirtualBox_Server_Module, components)

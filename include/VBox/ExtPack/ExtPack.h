/** @file
 * VirtualBox - Extension Pack Interface.
 */

/*
 * Copyright (C) 2010-2023 Oracle and/or its affiliates.
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
 * The contents of this file may alternatively be used under the terms
 * of the Common Development and Distribution License Version 1.0
 * (CDDL), a copy of it is provided in the "COPYING.CDDL" file included
 * in the VirtualBox distribution, in which case the provisions of the
 * CDDL are applicable instead of those of the GPL.
 *
 * You may elect to license modified versions of this file under the
 * terms and conditions of either the GPL or the CDDL or both.
 *
 * SPDX-License-Identifier: GPL-3.0-only OR CDDL-1.0
 */

#ifndef VBOX_INCLUDED_ExtPack_ExtPack_h
#define VBOX_INCLUDED_ExtPack_ExtPack_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <VBox/types.h>

/** @def VBOXEXTPACK_IF_CS
 * Selects 'class' on 'struct' for interface references.
 * @param I         The interface name
 */
#if defined(__cplusplus) && !defined(RT_OS_WINDOWS)
# define VBOXEXTPACK_IF_CS(I)   class I
#else
# define VBOXEXTPACK_IF_CS(I)   struct I
#endif

VBOXEXTPACK_IF_CS(IUnknown);
VBOXEXTPACK_IF_CS(IConsole);
VBOXEXTPACK_IF_CS(IMachine);
VBOXEXTPACK_IF_CS(IVirtualBox);
VBOXEXTPACK_IF_CS(IProgress);
VBOXEXTPACK_IF_CS(IEvent);
VBOXEXTPACK_IF_CS(IVetoEvent);
VBOXEXTPACK_IF_CS(IEventSource);

/**
 * Module kind for use with VBOXEXTPACKHLP::pfnFindModule.
 */
typedef enum VBOXEXTPACKMODKIND
{
    /** Zero is invalid as always. */
    VBOXEXTPACKMODKIND_INVALID = 0,
    /** Raw-mode context module. */
    VBOXEXTPACKMODKIND_RC,
    /** Ring-0 context module. */
    VBOXEXTPACKMODKIND_R0,
    /** Ring-3 context module. */
    VBOXEXTPACKMODKIND_R3,
    /** End of the valid values (exclusive). */
    VBOXEXTPACKMODKIND_END,
    /** The usual 32-bit type hack. */
    VBOXEXTPACKMODKIND_32BIT_HACK = 0x7fffffff
} VBOXEXTPACKMODKIND;

/**
 * Contexts returned by VBOXEXTPACKHLP::pfnGetContext.
 */
typedef enum VBOXEXTPACKCTX
{
    /** Zero is invalid as always. */
    VBOXEXTPACKCTX_INVALID = 0,
    /** The per-user daemon process (VBoxSVC). */
    VBOXEXTPACKCTX_PER_USER_DAEMON,
    /** A VM process. */
    VBOXEXTPACKCTX_VM_PROCESS,
    /** An API client process.
     * @remarks This will not be returned by VirtualBox yet. */
    VBOXEXTPACKCTX_CLIENT_PROCESS,
    /** End of the valid values (exclusive). */
    VBOXEXTPACKCTX_END,
    /** The usual 32-bit type hack. */
    VBOXEXTPACKCTX_32BIT_HACK = 0x7fffffff
} VBOXEXTPACKCTX;


/** Pointer to const helpers passed to the VBoxExtPackRegister() call. */
typedef const struct VBOXEXTPACKHLP *PCVBOXEXTPACKHLP;
/**
 * Extension pack helpers passed to VBoxExtPackRegister().
 *
 * This will be valid until the module is unloaded.
 */
typedef struct VBOXEXTPACKHLP
{
    /** Interface version.
     * This is set to VBOXEXTPACKHLP_VERSION. */
    uint32_t                    u32Version;

    /** The VirtualBox full version (see VBOX_FULL_VERSION).  */
    uint32_t                    uVBoxFullVersion;
    /** The VirtualBox subversion tree revision.  */
    uint32_t                    uVBoxInternalRevision;
    /** Explicit alignment padding, must be zero. */
    uint32_t                    u32Padding;
    /** Pointer to the version string (read-only). */
    const char                 *pszVBoxVersion;

    /**
     * Finds a module belonging to this extension pack.
     *
     * @returns VBox status code.
     * @param   pHlp            Pointer to this helper structure.
     * @param   pszName         The module base name.
     * @param   pszExt          The extension. If NULL the default ring-3
     *                          library extension will be used.
     * @param   enmKind         The kind of module to locate.
     * @param   pszFound        Where to return the path to the module on
     *                          success.
     * @param   cbFound         The size of the buffer @a pszFound points to.
     * @param   pfNative        Where to return the native/agnostic indicator.
     */
    DECLR3CALLBACKMEMBER(int, pfnFindModule,(PCVBOXEXTPACKHLP pHlp, const char *pszName, const char *pszExt,
                                             VBOXEXTPACKMODKIND enmKind,
                                             char *pszFound, size_t cbFound, bool *pfNative));

    /**
     * Gets the path to a file belonging to this extension pack.
     *
     * @returns VBox status code.
     * @retval  VERR_INVALID_POINTER if any of the pointers are invalid.
     * @retval  VERR_BUFFER_OVERFLOW if the buffer is too small.  The buffer
     *          will contain nothing.
     *
     * @param   pHlp            Pointer to this helper structure.
     * @param   pszFilename     The filename.
     * @param   pszPath         Where to return the path to the file on
     *                          success.
     * @param   cbPath          The size of the buffer @a pszPath.
     */
    DECLR3CALLBACKMEMBER(int, pfnGetFilePath,(PCVBOXEXTPACKHLP pHlp, const char *pszFilename, char *pszPath, size_t cbPath));

    /**
     * Gets the context the extension pack is operating in.
     *
     * @returns The context.
     * @retval  VBOXEXTPACKCTX_INVALID if @a pHlp is invalid.
     *
     * @param   pHlp            Pointer to this helper structure.
     */
    DECLR3CALLBACKMEMBER(VBOXEXTPACKCTX, pfnGetContext,(PCVBOXEXTPACKHLP pHlp));

    /**
     * Loads a HGCM service provided by an extension pack.
     *
     * @returns VBox status code.
     * @param   pHlp            Pointer to this helper structure.
     * @param   pConsole        Pointer to the VM's console object.
     * @param   pszServiceLibrary Name of the library file containing the
     *                          service implementation, without extension.
     * @param   pszServiceName  Name of HGCM service.
     */
    DECLR3CALLBACKMEMBER(int, pfnLoadHGCMService,(PCVBOXEXTPACKHLP pHlp, VBOXEXTPACK_IF_CS(IConsole) *pConsole,
                                                  const char *pszServiceLibrary, const char *pszServiceName));

    /**
     * Loads a VD plugin provided by an extension pack.
     *
     * This makes sense only in the context of the per-user service (VBoxSVC).
     *
     * @returns VBox status code.
     * @param   pHlp            Pointer to this helper structure.
     * @param   pVirtualBox     Pointer to the VirtualBox object.
     * @param   pszPluginLibrary Name of the library file containing the plugin
     *                          implementation, without extension.
     */
    DECLR3CALLBACKMEMBER(int, pfnLoadVDPlugin,(PCVBOXEXTPACKHLP pHlp, VBOXEXTPACK_IF_CS(IVirtualBox) *pVirtualBox,
                                               const char *pszPluginLibrary));

    /**
     * Unloads a VD plugin provided by an extension pack.
     *
     * This makes sense only in the context of the per-user service (VBoxSVC).
     *
     * @returns VBox status code.
     * @param   pHlp            Pointer to this helper structure.
     * @param   pVirtualBox     Pointer to the VirtualBox object.
     * @param   pszPluginLibrary Name of the library file containing the plugin
     *                          implementation, without extension.
     */
    DECLR3CALLBACKMEMBER(int, pfnUnloadVDPlugin,(PCVBOXEXTPACKHLP pHlp, VBOXEXTPACK_IF_CS(IVirtualBox) *pVirtualBox,
                                                 const char *pszPluginLibrary));

    /**
     * Creates an IProgress object instance for a long running extension
     * pack provided API operation which is executed asynchronously.
     *
     * This implicitly creates a cancellable progress object, since anything
     * else is user unfriendly. You need to design your code to handle
     * cancellation with reasonable response time.
     *
     * @returns COM status code.
     * @param   pHlp            Pointer to this helper structure.
     * @param   pInitiator      Pointer to the initiating object.
     * @param   pcszDescription Description of the overall task.
     * @param   cOperations     Number of operations for this task.
     * @param   uTotalOperationsWeight        Overall weight for the entire task.
     * @param   pcszFirstOperationDescription Description of the first operation.
     * @param   uFirstOperationWeight         Weight for the first operation.
     * @param   ppProgressOut   Output parameter for the IProgress object reference.
     */
    DECLR3CALLBACKMEMBER(uint32_t, pfnCreateProgress,(PCVBOXEXTPACKHLP pHlp, VBOXEXTPACK_IF_CS(IUnknown) *pInitiator,
                                                      const char *pcszDescription, uint32_t cOperations,
                                                      uint32_t uTotalOperationsWeight, const char *pcszFirstOperationDescription,
                                                      uint32_t uFirstOperationWeight, VBOXEXTPACK_IF_CS(IProgress) **ppProgressOut));

    /**
     * Checks if the Progress object is marked as canceled.
     *
     * @returns COM status code.
     * @param   pHlp            Pointer to this helper structure.
     * @param   pProgress       Pointer to the IProgress object reference returned
     *                          by pfnCreateProgress.
     * @param   pfCanceled      @c true if canceled, @c false otherwise.
     */
    DECLR3CALLBACKMEMBER(uint32_t, pfnGetCanceledProgress,(PCVBOXEXTPACKHLP pHlp, VBOXEXTPACK_IF_CS(IProgress) *pProgress,
                                                           bool *pfCanceled));

    /**
     * Updates the percentage value of the current operation of the
     * Progress object.
     *
     * @returns COM status code.
     * @param   pHlp            Pointer to this helper structure.
     * @param   pProgress       Pointer to the IProgress object reference returned
     *                          by pfnCreateProgress.
     * @param   uPercent        Result of the overall task.
     */
    DECLR3CALLBACKMEMBER(uint32_t, pfnUpdateProgress,(PCVBOXEXTPACKHLP pHlp, VBOXEXTPACK_IF_CS(IProgress) *pProgress,
                                                      uint32_t uPercent));

    /**
     * Signals that the current operation is successfully completed and
     * advances to the next operation. The operation percentage is reset
     * to 0.
     *
     * If the operation count is exceeded this returns an error.
     *
     * @returns COM status code.
     * @param   pHlp            Pointer to this helper structure.
     * @param   pProgress       Pointer to the IProgress object reference returned
     *                          by pfnCreateProgress.
     * @param   pcszNextOperationDescription Description of the next operation.
     * @param   uNextOperationWeight         Weight for the next operation.
     */
    DECLR3CALLBACKMEMBER(uint32_t, pfnNextOperationProgress,(PCVBOXEXTPACKHLP pHlp, VBOXEXTPACK_IF_CS(IProgress) *pProgress,
                                                             const char *pcszNextOperationDescription,
                                                             uint32_t uNextOperationWeight));

    /**
     * Waits until the other task is completed (including all sub-operations)
     * and forward all changes from the other progress to this progress. This
     * means sub-operation number, description, percent and so on.
     *
     * The caller is responsible for having at least the same count of
     * sub-operations in this progress object as there are in the other
     * progress object.
     *
     * If the other progress object supports cancel and this object gets any
     * cancel request (when here enabled as well), it will be forwarded to
     * the other progress object.
     *
     * Error information is automatically preserved (by transferring it to
     * the current thread's error information). If the caller wants to set it
     * as the completion state of this progress it needs to be done separately.
     *
     * @returns COM status code.
     * @param   pHlp            Pointer to this helper structure.
     * @param   pProgress       Pointer to the IProgress object reference returned
     *                          by pfnCreateProgress.
     * @param   pProgressOther  Pointer to an IProgress object reference, the one
     *                          to be waited for.
     * @param   cTimeoutMS      Timeout in milliseconds, 0 for indefinite wait.
     */
    DECLR3CALLBACKMEMBER(uint32_t, pfnWaitOtherProgress,(PCVBOXEXTPACKHLP pHlp, VBOXEXTPACK_IF_CS(IProgress) *pProgress,
                                                         VBOXEXTPACK_IF_CS(IProgress) *pProgressOther,
                                                         uint32_t cTimeoutMS));

    /**
     * Marks the whole task as complete and sets the result code.
     *
     * If the result code indicates a failure then this method will store
     * the currently set COM error info from the current thread in the
     * the errorInfo attribute of this Progress object instance. If there
     * is no error information available then an error is returned.
     *
     * If the result code indicates success then the task is terminated,
     * without paying attention to the current operation being the last.
     *
     * Note that this must be called only once for the given Progress
     * object. Subsequent calls will return errors.
     *
     * @returns COM status code.
     * @param   pHlp            Pointer to this helper structure.
     * @param   pProgress       Pointer to the IProgress object reference returned
     *                          by pfnCreateProgress.
     * @param   uResultCode     Result of the overall task.
     */
    DECLR3CALLBACKMEMBER(uint32_t, pfnCompleteProgress,(PCVBOXEXTPACKHLP pHlp, VBOXEXTPACK_IF_CS(IProgress) *pProgress,
                                                        uint32_t uResultCode));


    DECLR3CALLBACKMEMBER(uint32_t, pfnCreateEvent,(PCVBOXEXTPACKHLP pHlp,
                                                   VBOXEXTPACK_IF_CS(IEventSource) *aSource,
                                                   /* VBoxEventType_T */ uint32_t aType, bool aWaitable,
                                                   VBOXEXTPACK_IF_CS(IEvent) **ppEventOut));

    DECLR3CALLBACKMEMBER(uint32_t, pfnCreateVetoEvent,(PCVBOXEXTPACKHLP pHlp,
                                                       VBOXEXTPACK_IF_CS(IEventSource) *aSource,
                                                       /* VBoxEventType_T */ uint32_t aType,
                                                       VBOXEXTPACK_IF_CS(IVetoEvent) **ppEventOut));

    /**
     * Translate the string using registered translation files.
     *
     * Translation files are excluded from translation engine. Although
     * the already loaded translation remains in the translation cache the new
     * translation will not be loaded after returning from the function if the
     * user changes the language.
     *
     * @returns Translated string on success the pszSourceText otherwise.
     * @param   pHlp                      Pointer to this helper structure.
     * @param   aComponent                Translation context e.g. class name
     * @param   pszSourceText             String to translate
     * @param   pszComment                Comment to the string to resolve possible ambiguities
     *                                    (NULL means no comment).
     * @param   aNum                      Number used to define plural form of the translation
     */
    DECLR3CALLBACKMEMBER(const char *, pfnTranslate,(PCVBOXEXTPACKHLP pHlp,
                                                     const char  *pszComponent,
                                                     const char  *pszSourceText,
                                                     const char  *pszComment,
                                                     const size_t aNum));

    DECLR3CALLBACKMEMBER(int, pfnReserved1,(PCVBOXEXTPACKHLP pHlp)); /**< Reserved for minor structure revisions. */
    DECLR3CALLBACKMEMBER(int, pfnReserved2,(PCVBOXEXTPACKHLP pHlp)); /**< Reserved for minor structure revisions. */
    DECLR3CALLBACKMEMBER(int, pfnReserved3,(PCVBOXEXTPACKHLP pHlp)); /**< Reserved for minor structure revisions. */
    DECLR3CALLBACKMEMBER(int, pfnReserved4,(PCVBOXEXTPACKHLP pHlp)); /**< Reserved for minor structure revisions. */
    DECLR3CALLBACKMEMBER(int, pfnReserved5,(PCVBOXEXTPACKHLP pHlp)); /**< Reserved for minor structure revisions. */
    DECLR3CALLBACKMEMBER(int, pfnReserved6,(PCVBOXEXTPACKHLP pHlp)); /**< Reserved for minor structure revisions. */

    /** Reserved for minor structure revisions. */
    uint32_t                    uReserved7;

    /** End of structure marker (VBOXEXTPACKHLP_VERSION). */
    uint32_t                    u32EndMarker;
} VBOXEXTPACKHLP;
/** Current version of the VBOXEXTPACKHLP structure.  */
#define VBOXEXTPACKHLP_VERSION          RT_MAKE_U32(0, 5)


/** Pointer to the extension pack callback table. */
typedef struct VBOXEXTPACKREG const *PCVBOXEXTPACKREG;
/**
 * Callback table returned by VBoxExtPackRegister.
 *
 * All the callbacks are called the context of the per-user service (VBoxSVC).
 *
 * This must be valid until the extension pack main module is unloaded.
 */
typedef struct VBOXEXTPACKREG
{
    /** Interface version.
     * This is set to VBOXEXTPACKREG_VERSION. */
    uint32_t                    u32Version;
    /** The VirtualBox version this extension pack was built against.  */
    uint32_t                    uVBoxVersion;
    /** Translation files base name. Set to NULL if no translation files. */
    const char                 *pszNlsBaseName;

    /**
     * Hook for doing setups after the extension pack was installed.
     *
     * @returns VBox status code.
     * @retval  VERR_EXTPACK_UNSUPPORTED_HOST_UNINSTALL if the extension pack
     *          requires some different host version or a prerequisite is
     *          missing from the host.  Automatic uninstall will be attempted.
     *          Must set error info.
     *
     * @param   pThis       Pointer to this structure.
     * @param   pVirtualBox The VirtualBox interface.
     * @param   pErrInfo    Where to return extended error information.
     */
    DECLCALLBACKMEMBER(int, pfnInstalled,(PCVBOXEXTPACKREG pThis, VBOXEXTPACK_IF_CS(IVirtualBox) *pVirtualBox,
                                          PRTERRINFO pErrInfo));

    /**
     * Hook for cleaning up before the extension pack is uninstalled.
     *
     * @returns VBox status code.
     * @param   pThis       Pointer to this structure.
     * @param   pVirtualBox The VirtualBox interface.
     *
     * @todo    This is currently called holding locks making pVirtualBox
     *          relatively unusable.
     */
    DECLCALLBACKMEMBER(int, pfnUninstall,(PCVBOXEXTPACKREG pThis, VBOXEXTPACK_IF_CS(IVirtualBox) *pVirtualBox));

    /**
     * Hook for doing work after the VirtualBox object is ready.
     *
     * @param   pThis       Pointer to this structure.
     * @param   pVirtualBox The VirtualBox interface.
     */
    DECLCALLBACKMEMBER(void, pfnVirtualBoxReady,(PCVBOXEXTPACKREG pThis, VBOXEXTPACK_IF_CS(IVirtualBox) *pVirtualBox));

    /**
     * Hook for doing work before unloading.
     *
     * @param   pThis       Pointer to this structure.
     *
     * @remarks The helpers are not available at this point in time.
     * @remarks This is not called on uninstall, then pfnUninstall will be the
     *          last callback.
     */
    DECLCALLBACKMEMBER(void, pfnUnload,(PCVBOXEXTPACKREG pThis));

    /**
     * Hook for changing the default VM configuration upon creation.
     *
     * @returns VBox status code.
     * @param   pThis       Pointer to this structure.
     * @param   pVirtualBox The VirtualBox interface.
     * @param   pMachine    The machine interface.
     */
    DECLCALLBACKMEMBER(int, pfnVMCreated,(PCVBOXEXTPACKREG pThis, VBOXEXTPACK_IF_CS(IVirtualBox) *pVirtualBox,
                                          VBOXEXTPACK_IF_CS(IMachine) *pMachine));

    /**
     * Query the IUnknown interface to an object in the main module.
     *
     * @returns IUnknown pointer (referenced) on success, NULL on failure.
     * @param   pThis       Pointer to this structure.
     * @param   pObjectId   Pointer to the object ID (UUID).
     */
    DECLCALLBACKMEMBER(void *, pfnQueryObject,(PCVBOXEXTPACKREG pThis, PCRTUUID pObjectId));

    DECLR3CALLBACKMEMBER(int, pfnReserved1,(PCVBOXEXTPACKREG pThis)); /**< Reserved for minor structure revisions. */
    DECLR3CALLBACKMEMBER(int, pfnReserved2,(PCVBOXEXTPACKREG pThis)); /**< Reserved for minor structure revisions. */
    DECLR3CALLBACKMEMBER(int, pfnReserved3,(PCVBOXEXTPACKREG pThis)); /**< Reserved for minor structure revisions. */
    DECLR3CALLBACKMEMBER(int, pfnReserved4,(PCVBOXEXTPACKREG pThis)); /**< Reserved for minor structure revisions. */
    DECLR3CALLBACKMEMBER(int, pfnReserved5,(PCVBOXEXTPACKREG pThis)); /**< Reserved for minor structure revisions. */
    DECLR3CALLBACKMEMBER(int, pfnReserved6,(PCVBOXEXTPACKREG pThis)); /**< Reserved for minor structure revisions. */

    /** Reserved for minor structure revisions. */
    uint32_t                    uReserved7;

    /** End of structure marker (VBOXEXTPACKREG_VERSION). */
    uint32_t                    u32EndMarker;
} VBOXEXTPACKREG;
/** Current version of the VBOXEXTPACKREG structure.  */
#define VBOXEXTPACKREG_VERSION        RT_MAKE_U32(0, 3)


/**
 * The VBoxExtPackRegister callback function.
 *
 * The Main API (as in VBoxSVC) will invoke this function after loading an
 * extension pack Main module. Its job is to do version compatibility checking
 * and returning the extension pack registration structure.
 *
 * @returns VBox status code.
 * @param   pHlp            Pointer to the extension pack helper function
 *                          table.  This is valid until the module is unloaded.
 * @param   ppReg           Where to return the pointer to the registration
 *                          structure containing all the hooks.  This structure
 *                          be valid and unchanged until the module is unloaded
 *                          (i.e. use some static const data for it).
 * @param   pErrInfo        Where to return extended error information.
 */
typedef DECLCALLBACKTYPE(int, FNVBOXEXTPACKREGISTER,(PCVBOXEXTPACKHLP pHlp, PCVBOXEXTPACKREG *ppReg, PRTERRINFO pErrInfo));
/** Pointer to a FNVBOXEXTPACKREGISTER. */
typedef FNVBOXEXTPACKREGISTER *PFNVBOXEXTPACKREGISTER;

/** The name of the main module entry point. */
#define VBOX_EXTPACK_MAIN_MOD_ENTRY_POINT   "VBoxExtPackRegister"


/** Pointer to the extension pack VM callback table. */
typedef struct VBOXEXTPACKVMREG const *PCVBOXEXTPACKVMREG;
/**
 * Callback table returned by VBoxExtPackVMRegister.
 *
 * All the callbacks are called the context of a VM process.
 *
 * This must be valid until the extension pack main VM module is unloaded.
 */
typedef struct VBOXEXTPACKVMREG
{
    /** Interface version.
     * This is set to VBOXEXTPACKVMREG_VERSION. */
    uint32_t                    u32Version;
    /** The VirtualBox version this extension pack was built against.  */
    uint32_t                    uVBoxVersion;
    /** Translation files base name.  Set to NULL if no translation files.  */
    const char                 *pszNlsBaseName;

    /**
     * Hook for doing work after the Console object is ready.
     *
     * @param   pThis       Pointer to this structure.
     * @param   pConsole    The Console interface.
     */
    DECLCALLBACKMEMBER(void, pfnConsoleReady,(PCVBOXEXTPACKVMREG pThis, VBOXEXTPACK_IF_CS(IConsole) *pConsole));

    /**
     * Hook for doing work before unloading.
     *
     * @param   pThis       Pointer to this structure.
     *
     * @remarks The helpers are not available at this point in time.
     */
    DECLCALLBACKMEMBER(void, pfnUnload,(PCVBOXEXTPACKVMREG pThis));

    /**
     * Hook for configuring the VMM for a VM.
     *
     * @returns VBox status code.
     * @param   pThis       Pointer to this structure.
     * @param   pConsole    The console interface.
     * @param   pVM         The cross context VM structure.
     * @param   pVMM        The VMM function table.
     */
    DECLCALLBACKMEMBER(int, pfnVMConfigureVMM,(PCVBOXEXTPACKVMREG pThis, VBOXEXTPACK_IF_CS(IConsole) *pConsole,
                                               PVM pVM, PCVMMR3VTABLE pVMM));

    /**
     * Hook for doing work right before powering on the VM.
     *
     * @returns VBox status code.
     * @param   pThis       Pointer to this structure.
     * @param   pConsole    The console interface.
     * @param   pVM         The cross context VM structure.
     * @param   pVMM        The VMM function table.
     */
    DECLCALLBACKMEMBER(int, pfnVMPowerOn,(PCVBOXEXTPACKVMREG pThis, VBOXEXTPACK_IF_CS(IConsole) *pConsole,
                                          PVM pVM, PCVMMR3VTABLE pVMM));

    /**
     * Hook for doing work after powering off the VM.
     *
     * @param   pThis       Pointer to this structure.
     * @param   pConsole    The console interface.
     * @param   pVM         The cross context VM structure. Can be NULL.
     * @param   pVMM        The VMM function table.
     */
    DECLCALLBACKMEMBER(void, pfnVMPowerOff,(PCVBOXEXTPACKVMREG pThis, VBOXEXTPACK_IF_CS(IConsole) *pConsole,
                                            PVM pVM, PCVMMR3VTABLE pVMM));

    /**
     * Query the IUnknown interface to an object in the main VM module.
     *
     * @returns IUnknown pointer (referenced) on success, NULL on failure.
     * @param   pThis       Pointer to this structure.
     * @param   pObjectId   Pointer to the object ID (UUID).
     */
    DECLCALLBACKMEMBER(void *, pfnQueryObject,(PCVBOXEXTPACKVMREG pThis, PCRTUUID pObjectId));

    DECLR3CALLBACKMEMBER(int, pfnReserved1,(PCVBOXEXTPACKVMREG pThis)); /**< Reserved for minor structure revisions. */
    DECLR3CALLBACKMEMBER(int, pfnReserved2,(PCVBOXEXTPACKVMREG pThis)); /**< Reserved for minor structure revisions. */
    DECLR3CALLBACKMEMBER(int, pfnReserved3,(PCVBOXEXTPACKVMREG pThis)); /**< Reserved for minor structure revisions. */
    DECLR3CALLBACKMEMBER(int, pfnReserved4,(PCVBOXEXTPACKVMREG pThis)); /**< Reserved for minor structure revisions. */
    DECLR3CALLBACKMEMBER(int, pfnReserved5,(PCVBOXEXTPACKVMREG pThis)); /**< Reserved for minor structure revisions. */
    DECLR3CALLBACKMEMBER(int, pfnReserved6,(PCVBOXEXTPACKVMREG pThis)); /**< Reserved for minor structure revisions. */

    /** Reserved for minor structure revisions. */
    uint32_t                    uReserved7;

    /** End of structure marker (VBOXEXTPACKVMREG_VERSION). */
    uint32_t                    u32EndMarker;
} VBOXEXTPACKVMREG;
/** Current version of the VBOXEXTPACKVMREG structure.  */
#define VBOXEXTPACKVMREG_VERSION      RT_MAKE_U32(1, 0)


/**
 * The VBoxExtPackVMRegister callback function.
 *
 * The Main API (in a VM process) will invoke this function after loading an
 * extension pack VM module. Its job is to do version compatibility checking
 * and returning the extension pack registration structure for a VM.
 *
 * @returns VBox status code.
 * @param   pHlp            Pointer to the extension pack helper function
 *                          table.  This is valid until the module is unloaded.
 * @param   ppReg           Where to return the pointer to the registration
 *                          structure containing all the hooks.  This structure
 *                          be valid and unchanged until the module is unloaded
 *                          (i.e. use some static const data for it).
 * @param   pErrInfo        Where to return extended error information.
 */
typedef DECLCALLBACKTYPE(int, FNVBOXEXTPACKVMREGISTER,(PCVBOXEXTPACKHLP pHlp, PCVBOXEXTPACKVMREG *ppReg, PRTERRINFO pErrInfo));
/** Pointer to a FNVBOXEXTPACKVMREGISTER. */
typedef FNVBOXEXTPACKVMREGISTER *PFNVBOXEXTPACKVMREGISTER;

/** The name of the main VM module entry point. */
#define VBOX_EXTPACK_MAIN_VM_MOD_ENTRY_POINT   "VBoxExtPackVMRegister"


/**
 * Checks if extension pack interface version is compatible.
 *
 * @returns true if the do, false if they don't.
 * @param   u32Provider     The provider version.
 * @param   u32User         The user version.
 */
#define VBOXEXTPACK_IS_VER_COMPAT(u32Provider, u32User) \
    (    VBOXEXTPACK_IS_MAJOR_VER_EQUAL(u32Provider, u32User) \
      && (int32_t)RT_LOWORD(u32Provider) >= (int32_t)RT_LOWORD(u32User) ) /* stupid casts to shut up gcc */

/**
 * Check if two extension pack interface versions has the same major version.
 *
 * @returns true if the do, false if they don't.
 * @param   u32Ver1         The first version number.
 * @param   u32Ver2         The second version number.
 */
#define VBOXEXTPACK_IS_MAJOR_VER_EQUAL(u32Ver1, u32Ver2)  (RT_HIWORD(u32Ver1) == RT_HIWORD(u32Ver2))

#endif /* !VBOX_INCLUDED_ExtPack_ExtPack_h */


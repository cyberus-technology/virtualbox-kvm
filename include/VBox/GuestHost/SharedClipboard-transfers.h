/* $Id: SharedClipboard-transfers.h $ */
/** @file
 * Shared Clipboard - Shared transfer functions between host and guest.
 */

/*
 * Copyright (C) 2019-2023 Oracle and/or its affiliates.
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

#ifndef VBOX_INCLUDED_GuestHost_SharedClipboard_transfers_h
#define VBOX_INCLUDED_GuestHost_SharedClipboard_transfers_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <map>

#include <iprt/assert.h>
#include <iprt/critsect.h>
#include <iprt/fs.h>
#ifdef VBOX_WITH_SHARED_CLIPBOARD_TRANSFERS_HTTP
# include <iprt/http-server.h>
#endif
#include <iprt/list.h>

#include <iprt/cpp/list.h>
#include <iprt/cpp/ministring.h>

#include <VBox/GuestHost/SharedClipboard.h>
#include <VBox/HostServices/VBoxClipboardSvc.h>


struct SHCLTRANSFER;
/** Pointer to a single shared clipboard transfer   */
typedef struct SHCLTRANSFER *PSHCLTRANSFER;


/** @name Shared Clipboard transfer definitions.
 *  @{
 */

/** Defines the maximum length (in chars) a Shared Clipboard transfer path can have. */
#define SHCL_TRANSFER_PATH_MAX          RTPATH_MAX

/**
 * Defines the transfer status codes.
 */
typedef enum
{
    /** No status set. */
    SHCLTRANSFERSTATUS_NONE = 0,
    /** The transfer has been initialized but is not running yet. */
    SHCLTRANSFERSTATUS_INITIALIZED,
    /** The transfer is active and running. */
    SHCLTRANSFERSTATUS_STARTED,
    /** The transfer has been stopped. */
    SHCLTRANSFERSTATUS_STOPPED,
    /** The transfer has been canceled. */
    SHCLTRANSFERSTATUS_CANCELED,
    /** The transfer has been killed. */
    SHCLTRANSFERSTATUS_KILLED,
    /** The transfer ran into an unrecoverable error. */
    SHCLTRANSFERSTATUS_ERROR,
    /** The usual 32-bit hack. */
    SHCLTRANSFERSTATUS_32BIT_SIZE_HACK = 0x7fffffff
} SHCLTRANSFERSTATUSENUM;

/** Defines a transfer status. */
typedef uint32_t SHCLTRANSFERSTATUS;

/** @} */

/** @name Shared Clipboard handles.
 *  @{
 */

/** A Shared Clipboard list handle. */
typedef uint64_t SHCLLISTHANDLE;
/** Pointer to a Shared Clipboard list handle. */
typedef SHCLLISTHANDLE *PSHCLLISTHANDLE;
/** Specifies an invalid Shared Clipboard list handle.
 * @todo r=bird: The convention is NIL_SHCLLISTHANDLE. */
#define SHCLLISTHANDLE_INVALID        ((SHCLLISTHANDLE)UINT64_MAX)

/** A Shared Clipboard object handle. */
typedef uint64_t SHCLOBJHANDLE;
/** Pointer to a Shared Clipboard object handle. */
typedef SHCLOBJHANDLE *PSHCLOBJHANDLE;
/** Specifies an invalid Shared Clipboard object handle.
 * @todo r=bird: The convention is NIL_SHCLOBJHANDLE. */
#define SHCLOBJHANDLE_INVALID         ((SHCLOBJHANDLE)UINT64_MAX)

/** @} */

/** @name Shared Clipboard open/create flags.
 *  @{
 */
/** No flags. Initialization value. */
#define SHCL_OBJ_CF_NONE                    UINT32_C(0x00000000)

#if 0 /* These probably won't be needed either */
/** Lookup only the object, do not return a handle. All other flags are ignored. */
#define SHCL_OBJ_CF_LOOKUP                  UINT32_C(0x00000001)
/** Create/open a directory. */
#define SHCL_OBJ_CF_DIRECTORY               UINT32_C(0x00000004)
#endif

/** Read/write requested access for the object. */
#define SHCL_OBJ_CF_ACCESS_MASK_RW          UINT32_C(0x00001000)
/** No access requested. */
#define SHCL_OBJ_CF_ACCESS_NONE             UINT32_C(0x00000000)
/** Read access requested. */
#define SHCL_OBJ_CF_ACCESS_READ             UINT32_C(0x00001000)

/** Requested share access for the object. */
#define SHCL_OBJ_CF_ACCESS_MASK_DENY        UINT32_C(0x00008000)
/** Allow any access. */
#define SHCL_OBJ_CF_ACCESS_DENYNONE         UINT32_C(0x00000000)
/** Do not allow write. */
#define SHCL_OBJ_CF_ACCESS_DENYWRITE        UINT32_C(0x00008000)

/** Requested access to attributes of the object. */
#define SHCL_OBJ_CF_ACCESS_MASK_ATTR        UINT32_C(0x00010000)
/** No access requested. */
#define SHCL_OBJ_CF_ACCESS_ATTR_NONE        UINT32_C(0x00000000)
/** Read access requested. */
#define SHCL_OBJ_CF_ACCESS_ATTR_READ        UINT32_C(0x00010000)

/** Valid bits. */
#define SHCL_OBJ_CF_VALID_MASK              UINT32_C(0x00019000)
/** @} */

/**
 * The available additional information in a SHCLFSOBJATTR object.
 * @sa RTFSOBJATTRADD
 */
typedef enum _SHCLFSOBJATTRADD
{
    /** No additional information is available / requested. */
    SHCLFSOBJATTRADD_NOTHING = 1,
    /** The additional unix attributes (SHCLFSOBJATTR::u::Unix) are
     *  available / requested. */
    SHCLFSOBJATTRADD_UNIX,
    /** The additional extended attribute size (SHCLFSOBJATTR::u::EASize) is
     *  available / requested. */
    SHCLFSOBJATTRADD_EASIZE,
    /** The last valid item (inclusive).
     * The valid range is SHCLFSOBJATTRADD_NOTHING thru
     * SHCLFSOBJATTRADD_LAST. */
    SHCLFSOBJATTRADD_LAST = SHCLFSOBJATTRADD_EASIZE,
    /** The usual 32-bit hack. */
    SHCLFSOBJATTRADD_32BIT_SIZE_HACK = 0x7fffffff
} SHCLFSOBJATTRADD;


/* Assert sizes of the IRPT types we're using below. */
AssertCompileSize(RTFMODE,      4);
AssertCompileSize(RTFOFF,       8);
AssertCompileSize(RTINODE,      8);
AssertCompileSize(RTTIMESPEC,   8);
AssertCompileSize(RTDEV,        4);
AssertCompileSize(RTUID,        4);

/**
 * Shared Clipboard filesystem object attributes.
 *
 * @sa RTFSOBJATTR
 */
typedef struct _SHCLFSOBJATTR
{
    /** Mode flags (st_mode). RTFS_UNIX_*, RTFS_TYPE_*, and RTFS_DOS_*.
     * @remarks We depend on a number of RTFS_ defines to remain unchanged.
     *          Fortuntately, these are depending on windows, dos and unix
     *          standard values, so this shouldn't be much of a pain. */
    RTFMODE          fMode;

    /** The additional attributes available. */
    SHCLFSOBJATTRADD enmAdditional;

    /**
     * Additional attributes.
     *
     * Unless explicitly specified to an API, the API can provide additional
     * data as it is provided by the underlying OS.
     */
    union SHCLFSOBJATTRUNION
    {
        /** Additional Unix Attributes
         * These are available when SHCLFSOBJATTRADD is set in fUnix.
         */
         struct SHCLFSOBJATTRUNIX
         {
            /** The user owning the filesystem object (st_uid).
             * This field is ~0U if not supported. */
            RTUID           uid;

            /** The group the filesystem object is assigned (st_gid).
             * This field is ~0U if not supported. */
            RTGID           gid;

            /** Number of hard links to this filesystem object (st_nlink).
             * This field is 1 if the filesystem doesn't support hardlinking or
             * the information isn't available.
             */
            uint32_t        cHardlinks;

            /** The device number of the device which this filesystem object resides on (st_dev).
             * This field is 0 if this information is not available. */
            RTDEV           INodeIdDevice;

            /** The unique identifier (within the filesystem) of this filesystem object (st_ino).
             * Together with INodeIdDevice, this field can be used as a OS wide unique id
             * when both their values are not 0.
             * This field is 0 if the information is not available. */
            RTINODE         INodeId;

            /** User flags (st_flags).
             * This field is 0 if this information is not available. */
            uint32_t        fFlags;

            /** The current generation number (st_gen).
             * This field is 0 if this information is not available. */
            uint32_t        GenerationId;

            /** The device number of a character or block device type object (st_rdev).
             * This field is 0 if the file isn't of a character or block device type and
             * when the OS doesn't subscribe to the major+minor device idenfication scheme. */
            RTDEV           Device;
        } Unix;

        /**
         * Extended attribute size.
         */
        struct SHCLFSOBJATTREASIZE
        {
            /** Size of EAs. */
            RTFOFF          cb;
        } EASize;

        /** Padding the structure to a multiple of 8 bytes. */
        uint64_t au64Padding[5];
    } u;
} SHCLFSOBJATTR;
AssertCompileSize(SHCLFSOBJATTR, 48);
/** Pointer to a Shared Clipboard filesystem object attributes structure. */
typedef SHCLFSOBJATTR *PSHCLFSOBJATTR;
/** Pointer to a const Shared Clipboard filesystem object attributes structure. */
typedef const SHCLFSOBJATTR *PCSHCLFSOBJATTR;

/**
 * Shared Clipboard file system object information structure.
 *
 * @sa RTFSOBJINFO
 */
typedef struct _SHCLFSOBJINFO
{
   /** Logical size (st_size).
    * For normal files this is the size of the file.
    * For symbolic links, this is the length of the path name contained
    * in the symbolic link.
    * For other objects this fields needs to be specified.
    */
   RTFOFF       cbObject;

   /** Disk allocation size (st_blocks * DEV_BSIZE). */
   RTFOFF       cbAllocated;

   /** Time of last access (st_atime).
    * @remarks  Here (and other places) we depend on the IPRT timespec to
    *           remain unchanged. */
   RTTIMESPEC   AccessTime;

   /** Time of last data modification (st_mtime). */
   RTTIMESPEC   ModificationTime;

   /** Time of last status change (st_ctime).
    * If not available this is set to ModificationTime.
    */
   RTTIMESPEC   ChangeTime;

   /** Time of file birth (st_birthtime).
    * If not available this is set to ChangeTime.
    */
   RTTIMESPEC   BirthTime;

   /** Attributes. */
   SHCLFSOBJATTR Attr;

} SHCLFSOBJINFO;
AssertCompileSize(SHCLFSOBJINFO, 96);
/** Pointer to a Shared Clipboard filesystem object information structure. */
typedef SHCLFSOBJINFO *PSHCLFSOBJINFO;
/** Pointer to a const Shared Clipboard filesystem object information
 *  structure. */
typedef const SHCLFSOBJINFO *PCSHCLFSOBJINFO;

/**
 * Structure for keeping object open/create parameters.
 */
typedef struct _SHCLOBJOPENCREATEPARMS
{
    /** Path to object to open / create. */
    char                       *pszPath;
    /** Size (in bytes) of path to to object. */
    uint32_t                    cbPath;
    /** SHCL_OBJ_CF_* */
    uint32_t                    fCreate;
    /**
     * Attributes of object to open/create and
     * returned actual attributes of opened/created object.
     */
    SHCLFSOBJINFO    ObjInfo;
} SHCLOBJOPENCREATEPARMS, *PSHCLOBJOPENCREATEPARMS;

/**
 * Structure for keeping a reply message.
 */
typedef struct _SHCLREPLY
{
    /** Message type of type VBOX_SHCL_REPLYMSGTYPE_XXX. */
    uint32_t uType;
    /** IPRT result of overall operation. Note: int vs. uint32! */
    uint32_t rc;
    union
    {
        struct
        {
            SHCLTRANSFERSTATUS uStatus;
        } TransferStatus;
        struct
        {
            SHCLLISTHANDLE uHandle;
        } ListOpen;
        struct
        {
            SHCLLISTHANDLE uHandle;
        } ListClose;
        struct
        {
            SHCLOBJHANDLE uHandle;
        } ObjOpen;
        struct
        {
            SHCLOBJHANDLE uHandle;
        } ObjClose;
    } u;
    /** Pointer to optional payload. */
    void    *pvPayload;
    /** Payload size (in bytes). */
    uint32_t cbPayload;
} SHCLREPLY, *PSHCLREPLY;

struct _SHCLLISTENTRY;
typedef _SHCLLISTENTRY SHCLLISTENTRY;

/** Defines a single root list entry. Currently the same as a regular list entry. */
typedef SHCLLISTENTRY SHCLROOTLISTENTRY;
/** Defines a pointer to a single root list entry. Currently the same as a regular list entry pointer. */
typedef SHCLROOTLISTENTRY *PSHCLROOTLISTENTRY;

/**
 * Structure for keeping Shared Clipboard root list headers.
 */
typedef struct _SHCLROOTLISTHDR
{
    /** Roots listing flags; unused at the moment. */
    uint32_t                fRoots;
    /** Number of root list entries. */
    uint32_t                cRoots;
} SHCLROOTLISTHDR, *PSHCLROOTLISTHDR;

/**
 * Structure for maintaining a Shared Clipboard root list.
 */
typedef struct _SHCLROOTLIST
{
    /** Root list header. */
    SHCLROOTLISTHDR    Hdr;
    /** Root list entries. */
    SHCLROOTLISTENTRY *paEntries;
} SHCLROOTLIST, *PSHCLROOTLIST;

/**
 * Structure for maintaining Shared Clipboard list open paramters.
 */
typedef struct _SHCLLISTOPENPARMS
{
    /** Listing flags (see VBOX_SHCL_LIST_FLAG_XXX). */
    uint32_t fList;
    /** Size (in bytes) of the filter string. */
    uint32_t cbFilter;
    /** Filter string. DOS wilcard-style. */
    char    *pszFilter;
    /** Size (in bytes) of the listing path. */
    uint32_t cbPath;
    /** Listing path (absolute). If empty or NULL the listing's root path will be opened. */
    char    *pszPath;
} SHCLLISTOPENPARMS, *PSHCLLISTOPENPARMS;

/**
 * Structure for keeping a Shared Clipboard list header.
 */
typedef struct _SHCLLISTHDR
{
    /** Feature flag(s). Not being used atm. */
    uint32_t fFeatures;
    /** Total objects returned. */
    uint64_t cTotalObjects;
    /** Total size (in bytes) returned. */
    uint64_t cbTotalSize;
} SHCLLISTHDR, *PSHCLLISTHDR;

/**
 * Structure for a Shared Clipboard list entry.
 */
typedef struct _SHCLLISTENTRY
{
    /** Entry name. */
    char    *pszName;
    /** Size (in bytes) of entry name. */
    uint32_t cbName;
    /** Information flag(s). */
    uint32_t fInfo;
    /** Size (in bytes) of the actual list entry. */
    uint32_t cbInfo;
    /** Data of the actual list entry. */
    void    *pvInfo;
} SHCLLISTENTRY, *PSHCLLISTENTRY;

/** Maximum length (in UTF-8 characters) of a list entry name. */
#define SHCLLISTENTRY_MAX_NAME     RTPATH_MAX /** @todo Improve this to be more dynamic. */

/**
 * Structure for maintaining a Shared Clipboard list.
 */
typedef struct _SHCLLIST
{
    /** List header. */
    SHCLLISTHDR        Hdr;
    /** List entries. */
    SHCLROOTLISTENTRY *paEntries;
} SHCLLIST, *PSHCLLIST;

/**
 * Structure for keeping a Shared Clipboard object data chunk.
 */
typedef struct _SHCLOBJDATACHUNK
{
    /** Handle of object this data chunk is related to. */
    uint64_t  uHandle;
    /** Pointer to actual data chunk. */
    void     *pvData;
    /** Size (in bytes) of data chunk. */
    uint32_t  cbData;
} SHCLOBJDATACHUNK, *PSHCLOBJDATACHUNK;

/**
 * Structure for handling a single transfer object context.
 */
typedef struct _SHCLCLIENTTRANSFEROBJCTX
{
    SHCLTRANSFER *pTransfer;
    SHCLOBJHANDLE uHandle;
} SHCLCLIENTTRANSFEROBJCTX, *PSHCLCLIENTTRANSFEROBJCTX;

typedef struct _SHCLTRANSFEROBJSTATE
{
    /** How many bytes were processed (read / write) so far. */
    uint64_t cbProcessed;
} SHCLTRANSFEROBJSTATE, *PSHCLTRANSFEROBJSTATE;

typedef struct _SHCLTRANSFEROBJ
{
    SHCLOBJHANDLE        uHandle;
    char                *pszPathAbs;
    SHCLFSOBJINFO        objInfo;
    SHCLSOURCE           enmSource;
    SHCLTRANSFEROBJSTATE State;
} SHCLTRANSFEROBJ, *PSHCLTRANSFEROBJ;

/**
 * Enumeration for specifying a Shared Clipboard object type.
 */
typedef enum _SHCLOBJTYPE
{
    /** Invalid object type. */
    SHCLOBJTYPE_INVALID = 0,
    /** Object is a directory. */
    SHCLOBJTYPE_DIRECTORY,
    /** Object is a file. */
    SHCLOBJTYPE_FILE,
    /** Object is a symbolic link. */
    SHCLOBJTYPE_SYMLINK,
    /** The usual 32-bit hack. */
    SHCLOBJTYPE_32BIT_SIZE_HACK = 0x7fffffff
} SHCLOBJTYPE;

/**
 * Structure for keeping transfer list handle information.
 * This is using to map own (local) handles to the underlying file system.
 */
typedef struct _SHCLLISTHANDLEINFO
{
    /** The list node. */
    RTLISTNODE      Node;
    /** The list's handle. */
    SHCLLISTHANDLE  hList;
    /** Type of list handle. */
    SHCLOBJTYPE     enmType;
    /** Absolute local path of the list object. */
    char           *pszPathLocalAbs;
    union
    {
        /** Local data, based on enmType. */
        struct
        {
            union
            {
                RTDIR  hDir;
                RTFILE hFile;
            };
        } Local;
    } u;
} SHCLLISTHANDLEINFO, *PSHCLLISTHANDLEINFO;

/**
 * Structure for keeping transfer object handle information.
 * This is using to map own (local) handles to the underlying file system.
 */
typedef struct _SHCLOBJHANDLEINFO
{
    /** The list node. */
    RTLISTNODE     Node;
    /** The object's handle. */
    SHCLOBJHANDLE  hObj;
    /** Type of object handle. */
    SHCLOBJTYPE    enmType;
    /** Absolute local path of the object. */
    char          *pszPathLocalAbs;
    union
    {
        /** Local data, based on enmType. */
        struct
        {
            union
            {
                RTDIR  hDir;
                RTFILE hFile;
            };
        } Local;
    } u;
} SHCLOBJHANDLEINFO, *PSHCLOBJHANDLEINFO;

/**
 * Structure for keeping a single root list entry.
 */
typedef struct _SHCLLISTROOT
{
    /** The list node. */
    RTLISTNODE          Node;
    /** Absolute path of entry. */
    char               *pszPathAbs;
} SHCLLISTROOT, *PSHCLLISTROOT;

/**
 * Structure for maintaining an Shared Clipboard transfer state.
 * Everything in here will be part of a saved state (later).
 */
typedef struct _SHCLTRANSFERSTATE
{
    /** The transfer's (local) ID. */
    SHCLTRANSFERID     uID;
    /** The transfer's current status. */
    SHCLTRANSFERSTATUS enmStatus;
    /** The transfer's direction, seen from the perspective who created the transfer. */
    SHCLTRANSFERDIR    enmDir;
    /** The transfer's source, seen from the perspective who created the transfer. */
    SHCLSOURCE         enmSource;
} SHCLTRANSFERSTATE, *PSHCLTRANSFERSTATE;

/**
 * Structure maintaining clipboard transfer provider context data.
 * This is handed in to the provider interface implementations.
 */
typedef struct _SHCLTXPROVIDERCTX
{
    /** Pointer to the related Shared Clipboard transfer. */
    PSHCLTRANSFER pTransfer;
    /** User-defined data pointer. Can be NULL if not needed. */
    void         *pvUser;
    /** Size (in bytes) of data at user pointer. */
    size_t        cbUser;
} SHCLTXPROVIDERCTX, *PSHCLTXPROVIDERCTX;

struct SHCLTRANSFERCTX;
typedef struct SHCLTRANSFERCTX *PSHCLTRANSFERCTX;

/**
 * Shared Clipboard transfer provider interface table.
 *
 * A transfer provider inteface implementation realizes all low level functions
 * needed for making a Shared Clipboard transfer happen.
 */
typedef struct _SHCLTXPROVIDERIFACE
{
    DECLCALLBACKMEMBER(int, pfnRootsGet,(PSHCLTXPROVIDERCTX pCtx, PSHCLROOTLIST *ppRootList));
    DECLCALLBACKMEMBER(int, pfnListOpen,(PSHCLTXPROVIDERCTX pCtx, PSHCLLISTOPENPARMS pOpenParms, PSHCLLISTHANDLE phList));
    DECLCALLBACKMEMBER(int, pfnListClose,(PSHCLTXPROVIDERCTX pCtx, SHCLLISTHANDLE hList));
    DECLCALLBACKMEMBER(int, pfnListHdrRead,(PSHCLTXPROVIDERCTX pCtx, SHCLLISTHANDLE hList, PSHCLLISTHDR pListHdr));
    DECLCALLBACKMEMBER(int, pfnListHdrWrite,(PSHCLTXPROVIDERCTX pCtx, SHCLLISTHANDLE hList, PSHCLLISTHDR pListHdr));
    DECLCALLBACKMEMBER(int, pfnListEntryRead,(PSHCLTXPROVIDERCTX pCtx, SHCLLISTHANDLE hList, PSHCLLISTENTRY pEntry));
    DECLCALLBACKMEMBER(int, pfnListEntryWrite,(PSHCLTXPROVIDERCTX pCtx, SHCLLISTHANDLE hList, PSHCLLISTENTRY pEntry));
    DECLCALLBACKMEMBER(int, pfnObjOpen,(PSHCLTXPROVIDERCTX pCtx, PSHCLOBJOPENCREATEPARMS pCreateParms, PSHCLOBJHANDLE phObj));
    DECLCALLBACKMEMBER(int, pfnObjClose,(PSHCLTXPROVIDERCTX pCtx, SHCLOBJHANDLE hObj));
    DECLCALLBACKMEMBER(int, pfnObjRead,(PSHCLTXPROVIDERCTX pCtx, SHCLOBJHANDLE hObj, void *pvData, uint32_t cbData,
                                        uint32_t fFlags, uint32_t *pcbRead));
    DECLCALLBACKMEMBER(int, pfnObjWrite,(PSHCLTXPROVIDERCTX pCtx, SHCLOBJHANDLE hObj, void *pvData, uint32_t cbData,
                                         uint32_t fFlags, uint32_t *pcbWritten));
} SHCLTXPROVIDERIFACE, *PSHCLTXPROVIDERIFACE;

/**
 * Structure for the Shared Clipboard transfer provider creation context.
 */
typedef struct _SHCLTXPROVIDERCREATIONCTX
{
    /** Specifies what the source of the provider is. */
    SHCLSOURCE           enmSource;
    /** The provider interface table. */
    SHCLTXPROVIDERIFACE  Interface;
    /** User-provided callback data. */
    void                *pvUser;
    /** Size (in bytes) of data at user pointer. */
    size_t               cbUser;
} SHCLTXPROVIDERCREATIONCTX, *PSHCLTXPROVIDERCREATIONCTX;

/**
 * Structure maintaining clipboard transfer callback context data.
 */
typedef struct _SHCLTRANSFERCALLBACKCTX
{
    /** Pointer to the related Shared Clipboard transfer. */
    PSHCLTRANSFER pTransfer;
    /** User-defined data pointer. Can be NULL if not needed. */
    void         *pvUser;
    /** Size (in bytes) of data at user pointer. */
    size_t        cbUser;
} SHCLTRANSFERCALLBACKCTX, *PSHCLTRANSFERCALLBACKCTX;

/**
 * Shared Clipboard transfer callback table.
 *
 * All callbacks are optional and can provide additional information / feedback to a frontend.
 */
typedef struct _SHCLTRANSFERCALLBACKTABLE
{
    /**
     * Called when the transfer gets initialized.
     *
     * @param   pCbCtx              Pointer to callback context to use.
     */
    DECLCALLBACKMEMBER(int,  pfnOnInitialize,(PSHCLTRANSFERCALLBACKCTX pCbCtx));
    /**
     * Called before the transfer will be started.
     *
     * @param   pCbCtx              Pointer to callback context to use.
     */
    DECLCALLBACKMEMBER(int,  pfnOnStart,(PSHCLTRANSFERCALLBACKCTX pCbCtx));
    /**
     * Called when the transfer has been complete.
     *
     * @param   pCbCtx              Pointer to callback context to use.
     * @param   rcCompletion        Completion result.
     *                              VERR_CANCELED if transfer has been canceled.
     */
    DECLCALLBACKMEMBER(void, pfnOnCompleted,(PSHCLTRANSFERCALLBACKCTX pCbCtx, int rcCompletion));
    /**
     * Called when transfer resulted in an unrecoverable error.
     *
     * @param   pCbCtx              Pointer to callback context to use.
     * @param   rcError             Error reason, IPRT-style.
     */
    DECLCALLBACKMEMBER(void, pfnOnError,(PSHCLTRANSFERCALLBACKCTX pCbCtx, int rcError));
    /**
     * Called when transfer got registered to a transfer context.
     *
     * @param   pCbCtx              Pointer to callback context to use.
     * @param   pTransferCtx        Transfer context transfer was registered to.
     */
    DECLCALLBACKMEMBER(void, pfnOnRegistered,(PSHCLTRANSFERCALLBACKCTX pCbCtx, PSHCLTRANSFERCTX pTransferCtx));
    /**
     * Called when transfer got unregistered from a transfer context.
     *
     * @param   pCbCtx              Pointer to callback context to use.
     * @param   pTransferCtx        Transfer context transfer was unregistered from.
     */
    DECLCALLBACKMEMBER(void, pfnOnUnregistered,(PSHCLTRANSFERCALLBACKCTX pCbCtx, PSHCLTRANSFERCTX pTransferCtx));

    /** User-provided callback data. Can be NULL if not used. */
    void  *pvUser;
    /** Size (in bytes) of data pointer at \a pvUser. */
    size_t cbUser;
} SHCLTRANSFERCALLBACKTABLE, *PSHCLTRANSFERCALLBACKTABLE;

/**
 * Structure for thread-related members for a single Shared Clipboard transfer.
 */
typedef struct _SHCLTRANSFERTHREAD
{
    /** Thread handle for the reading / writing thread.
     *  Can be NIL_RTTHREAD if not being used. */
    RTTHREAD                    hThread;
    /** Thread started indicator. */
    volatile bool               fStarted;
    /** Thread stop flag. */
    volatile bool               fStop;
    /** Thread cancelled flag / indicator. */
    volatile bool               fCancelled;
} SHCLTRANSFERTHREAD, *PSHCLTRANSFERTHREAD;

/**
 * A single Shared Clipboard transfer.
 *
 ** @todo Not yet thread safe.
 */
typedef struct SHCLTRANSFER
{
    /** The node member for using this struct in a RTList. */
    RTLISTNODE                Node;
    /** The transfer's state (for SSM, later). */
    SHCLTRANSFERSTATE         State;
    /** Absolute path to root entries. */
    char                     *pszPathRootAbs;
    /** Timeout (in ms) for waiting of events. Default is 30s. */
    RTMSINTERVAL              uTimeoutMs;
    /** Maximum data chunk size (in bytes) to transfer. Default is 64K. */
    uint32_t                  cbMaxChunkSize;
    /** The transfer's own event source. */
    SHCLEVENTSOURCE           Events;
    /** Current number of concurrent list handles. */
    uint32_t                  cListHandles;
    /** Maximum number of concurrent list handles. */
    uint32_t                  cMaxListHandles;
    /** Next upcoming list handle. */
    SHCLLISTHANDLE            uListHandleNext;
    /** List of all list handles elated to this transfer. */
    RTLISTANCHOR              lstList;
    /** Number of root entries in list. */
    uint64_t                  cRoots;
    /** List of root entries of this transfer. */
    RTLISTANCHOR              lstRoots;
    /** Current number of concurrent object handles. */
    uint32_t                  cObjHandles;
    /** Maximum number of concurrent object handles. */
    uint32_t                  cMaxObjHandles;
    /** Next upcoming object handle. */
    SHCLOBJHANDLE             uObjHandleNext;
    /** Map of all objects handles related to this transfer. */
    RTLISTANCHOR              lstObj;
    /** The transfer's own provider context. */
    SHCLTXPROVIDERCTX         ProviderCtx;
    /** The transfer's provider interface. */
    SHCLTXPROVIDERIFACE       ProviderIface;
    /** The transfer's callback context. */
    SHCLTRANSFERCALLBACKCTX   CallbackCtx;
    /** The transfer's callback table. */
    SHCLTRANSFERCALLBACKTABLE Callbacks;
    /** Opaque pointer to implementation-specific parameters. */
    void                     *pvUser;
    /** Size (in bytes) of implementation-specific parameters. */
    size_t                    cbUser;
    /** Contains thread-related attributes. */
    SHCLTRANSFERTHREAD        Thread;
    /** Critical section for serializing access. */
    RTCRITSECT                CritSect;
} SHCLTRANSFER, *PSHCLTRANSFER;

/**
 * Structure for keeping an Shared Clipboard transfer status report.
 */
typedef struct _SHCLTRANSFERREPORT
{
    /** Actual status to report. */
    SHCLTRANSFERSTATUS    uStatus;
    /** Result code (rc) to report; might be unused / invalid, based on enmStatus. */
    int                   rc;
    /** Reporting flags. Currently unused and must be 0. */
    uint32_t              fFlags;
} SHCLTRANSFERREPORT, *PSHCLTRANSFERREPORT;

#ifdef VBOX_WITH_SHARED_CLIPBOARD_TRANSFERS_HTTP
typedef struct _SHCLHTTPSERVER
{
    /** Critical section for serializing access. */
    RTCRITSECT          CritSect;
    /** Handle of the HTTP server instance. */
    RTHTTPSERVER        hHTTPServer;
    /** Port number the HTTP server is running on. 0 if not running. */
    uint16_t            uPort;
    /** List of registered HTTP transfers. */
    RTLISTANCHOR        lstTransfers;
    /** Number of registered HTTP transfers. */
    uint32_t            cTransfers;
    /** Cached response data. */
    RTHTTPSERVERRESP    Resp;
} SHCLHTTPSERVER;
typedef SHCLHTTPSERVER *PSHCLHTTPSERVER;

typedef struct _SHCLHTTPCONTEXT
{
    /** HTTP server instance data. */
    SHCLHTTPSERVER      HttpServer;
} SHCLHTTPCONTEXT;
typedef SHCLHTTPCONTEXT *PSHCLHTTPCONTEXT;

#endif /* VBOX_WITH_SHARED_CLIPBOARD_TRANSFERS_HTTP */

/**
 * Structure for keeping Shared Clipboard transfer context around.
 */
struct SHCLTRANSFERCTX
{
    /** Critical section for serializing access. */
    RTCRITSECT                  CritSect;
    /** List of transfers. */
    RTLISTANCHOR                List;
    /** Transfer ID allocation bitmap; clear bits are free, set bits are busy. */
    uint64_t                    bmTransferIds[VBOX_SHCL_MAX_TRANSFERS / sizeof(uint64_t) / 8];
    /** Number of running (concurrent) transfers. */
    uint16_t                    cRunning;
    /** Maximum Number of running (concurrent) transfers. */
    uint16_t                    cMaxRunning;
    /** Number of total transfers (in list). */
    uint16_t                    cTransfers;
#ifdef VBOX_WITH_SHARED_CLIPBOARD_TRANSFERS_HTTP
    /** HTTP server instance for this transfer context. */
    SHCLHTTPSERVER              HttpServer;
#endif /* VBOX_WITH_SHARED_CLIPBOARD_TRANSFERS_HTTP */
};

int ShClTransferObjCtxInit(PSHCLCLIENTTRANSFEROBJCTX pObjCtx);
void ShClTransferObjCtxDestroy(PSHCLCLIENTTRANSFEROBJCTX pObjCtx);
bool ShClTransferObjCtxIsValid(PSHCLCLIENTTRANSFEROBJCTX pObjCtx);

int ShClTransferObjHandleInfoInit(PSHCLOBJHANDLEINFO pInfo);
void ShClTransferObjHandleInfoDestroy(PSHCLOBJHANDLEINFO pInfo);

int ShClTransferObjOpenParmsInit(PSHCLOBJOPENCREATEPARMS pParms);
int ShClTransferObjOpenParmsCopy(PSHCLOBJOPENCREATEPARMS pParmsDst, PSHCLOBJOPENCREATEPARMS pParmsSrc);
void ShClTransferObjOpenParmsDestroy(PSHCLOBJOPENCREATEPARMS pParms);

int ShClTransferObjOpen(PSHCLTRANSFER pTransfer, PSHCLOBJOPENCREATEPARMS pOpenCreateParms, PSHCLOBJHANDLE phObj);
int ShClTransferObjClose(PSHCLTRANSFER pTransfer, SHCLOBJHANDLE hObj);
int ShClTransferObjRead(PSHCLTRANSFER pTransfer, SHCLOBJHANDLE hObj, void *pvBuf, uint32_t cbBuf, uint32_t fFlags, uint32_t *pcbRead);
int ShClTransferObjWrite(PSHCLTRANSFER pTransfer, SHCLOBJHANDLE hObj, void *pvBuf, uint32_t cbBuf, uint32_t fFlags, uint32_t *pcbWritten);

PSHCLOBJDATACHUNK ShClTransferObjDataChunkDup(PSHCLOBJDATACHUNK pDataChunk);
void ShClTransferObjDataChunkDestroy(PSHCLOBJDATACHUNK pDataChunk);
void ShClTransferObjDataChunkFree(PSHCLOBJDATACHUNK pDataChunk);

int ShClTransferCreate(PSHCLTRANSFER *ppTransfer);
int ShClTransferInit(PSHCLTRANSFER pTransfer, SHCLTRANSFERDIR enmDir, SHCLSOURCE enmSource);
int ShClTransferDestroy(PSHCLTRANSFER pTransfer);

int ShClTransferListOpen(PSHCLTRANSFER pTransfer, PSHCLLISTOPENPARMS pOpenParms, PSHCLLISTHANDLE phList);
int ShClTransferListClose(PSHCLTRANSFER pTransfer, SHCLLISTHANDLE hList);
int ShClTransferListGetHeader(PSHCLTRANSFER pTransfer, SHCLLISTHANDLE hList, PSHCLLISTHDR pHdr);
PSHCLTRANSFEROBJ ShClTransferListGetObj(PSHCLTRANSFER pTransfer, SHCLLISTHANDLE hList, uint64_t uIdx);
int ShClTransferListRead(PSHCLTRANSFER pTransfer, SHCLLISTHANDLE hList, PSHCLLISTENTRY pEntry);
int ShClTransferListWrite(PSHCLTRANSFER pTransfer, SHCLLISTHANDLE hList, PSHCLLISTENTRY pEntry);
bool ShClTransferListHandleIsValid(PSHCLTRANSFER pTransfer, SHCLLISTHANDLE hList);

int ShClPathSanitizeFilename(char *pszPath, size_t cbPath);
int ShClPathSanitize(char *pszPath, size_t cbPath);

PSHCLROOTLIST ShClTransferRootListAlloc(void);
void ShClTransferRootListFree(PSHCLROOTLIST pRootList);

PSHCLROOTLISTHDR ShClTransferRootListHdrDup(PSHCLROOTLISTHDR pRoots);
int ShClTransferRootListHdrInit(PSHCLROOTLISTHDR pRoots);
void ShClTransferRootListHdrDestroy(PSHCLROOTLISTHDR pRoots);

int ShClTransferRootListEntryCopy(PSHCLROOTLISTENTRY pDst, PSHCLROOTLISTENTRY pSrc);
int ShClTransferRootListEntryInit(PSHCLROOTLISTENTRY pRootListEntry);
void ShClTransferRootListEntryDestroy(PSHCLROOTLISTENTRY pRootListEntry);
PSHCLROOTLISTENTRY ShClTransferRootListEntryDup(PSHCLROOTLISTENTRY pRootListEntry);

int ShClTransferListHandleInfoInit(PSHCLLISTHANDLEINFO pInfo);
void ShClTransferListHandleInfoDestroy(PSHCLLISTHANDLEINFO pInfo);

int ShClTransferListHdrAlloc(PSHCLLISTHDR *ppListHdr);
void ShClTransferListHdrFree(PSHCLLISTHDR pListHdr);
PSHCLLISTHDR ShClTransferListHdrDup(PSHCLLISTHDR pListHdr);
int ShClTransferListHdrInit(PSHCLLISTHDR pListHdr);
void ShClTransferListHdrDestroy(PSHCLLISTHDR pListHdr);
void ShClTransferListHdrReset(PSHCLLISTHDR pListHdr);
bool ShClTransferListHdrIsValid(PSHCLLISTHDR pListHdr);

int ShClTransferListOpenParmsCopy(PSHCLLISTOPENPARMS pDst, PSHCLLISTOPENPARMS pSrc);
PSHCLLISTOPENPARMS ShClTransferListOpenParmsDup(PSHCLLISTOPENPARMS pParms);
int ShClTransferListOpenParmsInit(PSHCLLISTOPENPARMS pParms);
void ShClTransferListOpenParmsDestroy(PSHCLLISTOPENPARMS pParms);

int ShClTransferListEntryAlloc(PSHCLLISTENTRY *ppListEntry);
void ShClTransferListEntryFree(PSHCLLISTENTRY pListEntry);
int ShClTransferListEntryCopy(PSHCLLISTENTRY pDst, PSHCLLISTENTRY pSrc);
PSHCLLISTENTRY ShClTransferListEntryDup(PSHCLLISTENTRY pListEntry);
int ShClTransferListEntryInit(PSHCLLISTENTRY pListEntry);
void ShClTransferListEntryDestroy(PSHCLLISTENTRY pListEntry);
bool ShClTransferListEntryIsValid(PSHCLLISTENTRY pListEntry);

void ShClTransferCopyCallbacks(PSHCLTRANSFERCALLBACKTABLE pCallbacksDst, PSHCLTRANSFERCALLBACKTABLE pCallbacksSrc);
void ShClTransferSetCallbacks(PSHCLTRANSFER pTransfer, PSHCLTRANSFERCALLBACKTABLE pCallbacks);
int ShClTransferSetProviderIface(PSHCLTRANSFER pTransfer, PSHCLTXPROVIDERCREATIONCTX pCreationCtx);
int ShClTransferRootsSet(PSHCLTRANSFER pTransfer, const char *pszRoots, size_t cbRoots);
void ShClTransferReset(PSHCLTRANSFER pTransfer);

uint32_t ShClTransferRootsCount(PSHCLTRANSFER pTransfer);
int ShClTransferRootsEntry(PSHCLTRANSFER pTransfer, uint64_t uIndex, PSHCLROOTLISTENTRY pEntry);
int ShClTransferRootsGet(PSHCLTRANSFER pTransfer, PSHCLROOTLIST *ppRootList);

SHCLTRANSFERID ShClTransferGetID(PSHCLTRANSFER pTransfer);
SHCLTRANSFERDIR ShClTransferGetDir(PSHCLTRANSFER pTransfer);
SHCLSOURCE ShClTransferGetSource(PSHCLTRANSFER pTransfer);
SHCLTRANSFERSTATUS ShClTransferGetStatus(PSHCLTRANSFER pTransfer);
int ShClTransferRun(PSHCLTRANSFER pTransfer, PFNRTTHREAD pfnThreadFunc, void *pvUser);
int ShClTransferStart(PSHCLTRANSFER pTransfer);

int ShClTransferCtxInit(PSHCLTRANSFERCTX pTransferCtx);
void ShClTransferCtxDestroy(PSHCLTRANSFERCTX pTransferCtx);
void ShClTransferCtxReset(PSHCLTRANSFERCTX pTransferCtx);
PSHCLTRANSFER ShClTransferCtxGetTransferById(PSHCLTRANSFERCTX pTransferCtx, uint32_t uID);
PSHCLTRANSFER ShClTransferCtxGetTransferByIndex(PSHCLTRANSFERCTX pTransferCtx, uint32_t uIdx);
uint32_t ShClTransferCtxGetRunningTransfers(PSHCLTRANSFERCTX pTransferCtx);
uint32_t ShClTransferCtxGetTotalTransfers(PSHCLTRANSFERCTX pTransferCtx);
void ShClTransferCtxCleanup(PSHCLTRANSFERCTX pTransferCtx);
bool ShClTransferCtxTransfersMaximumReached(PSHCLTRANSFERCTX pTransferCtx);
int ShClTransferCtxTransferRegister(PSHCLTRANSFERCTX pTransferCtx, PSHCLTRANSFER pTransfer, SHCLTRANSFERID *pidTransfer);
int ShClTransferCtxTransferRegisterById(PSHCLTRANSFERCTX pTransferCtx, PSHCLTRANSFER pTransfer, SHCLTRANSFERID idTransfer);
int ShClTransferCtxTransferUnregister(PSHCLTRANSFERCTX pTransferCtx, SHCLTRANSFERID idTransfer);

#ifdef VBOX_WITH_SHARED_CLIPBOARD_TRANSFERS_HTTP
int ShClHttpTransferRegister(PSHCLHTTPCONTEXT pCtx, PSHCLTRANSFER pTransfer);
int ShClHttpTransferUnregister(PSHCLHTTPCONTEXT pCtx, PSHCLTRANSFER pTransfer);

int ShClTransferHttpServerCreate(PSHCLHTTPSERVER pSrv, uint16_t *puPort);
int ShClTransferHttpServerCreateEx(PSHCLHTTPSERVER pSrv, uint16_t uPort);
int ShClTransferHttpServerDestroy(PSHCLHTTPSERVER pSrv);
void ShClTransferHttpServerInit(PSHCLHTTPSERVER pSrv);
int ShClTransferHttpServerRegisterTransfer(PSHCLHTTPSERVER pSrv, PSHCLTRANSFER pTransfer);
int ShClTransferHttpServerUnregisterTransfer(PSHCLHTTPSERVER pSrv, PSHCLTRANSFER pTransfer);
bool ShClTransferHttpServerHasTransfer(PSHCLHTTPSERVER pSrv, SHCLTRANSFERID idTransfer);
uint16_t ShClTransferHttpServerGetPort(PSHCLHTTPSERVER pSrv);
uint32_t ShClTransferHttpServerGetTransferCount(PSHCLHTTPSERVER pSrv);
char *ShClTransferHttpServerGetAddressA(PSHCLHTTPSERVER pSrv);
char *ShClTransferHttpServerGetUrlA(PSHCLHTTPSERVER pSrv, SHCLTRANSFERID idTransfer);
bool ShClTransferHttpServerIsRunning(PSHCLHTTPSERVER pSrv);
#endif /* VBOX_WITH_SHARED_CLIPBOARD_TRANSFERS_HTTP */

void ShClFsObjFromIPRT(PSHCLFSOBJINFO pDst, PCRTFSOBJINFO pSrc);

bool ShClMIMEHasFileURLs(const char *pcszFormat, size_t cchFormatMax);
bool ShClMIMENeedsCache(const char *pcszFormat, size_t cchFormatMax);

const char *ShClTransferStatusToStr(SHCLTRANSFERSTATUS enmStatus);

#endif /* !VBOX_INCLUDED_GuestHost_SharedClipboard_transfers_h */

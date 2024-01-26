/** @file
 * IPRT - Virtual Filesystem.
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

#ifndef IPRT_INCLUDED_vfslowlevel_h
#define IPRT_INCLUDED_vfslowlevel_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <iprt/vfs.h>
#include <iprt/errcore.h>
#include <iprt/list.h>
#include <iprt/param.h>


RT_C_DECLS_BEGIN

/** @defgroup grp_rt_vfs_lowlevel   RTVfs - Low-level Interface.
 * @ingroup grp_rt_vfs
 * @{
 */


/** @name VFS Lock Abstraction
 * @todo This should be moved somewhere else as it is of general use.
 * @{ */

/**
 * VFS lock types.
 */
typedef enum RTVFSLOCKTYPE
{
    /** Invalid lock type. */
    RTVFSLOCKTYPE_INVALID = 0,
    /** Read write semaphore. */
    RTVFSLOCKTYPE_RW,
    /** Fast mutex semaphore (critical section in ring-3). */
    RTVFSLOCKTYPE_FASTMUTEX,
    /** Full fledged mutex semaphore. */
    RTVFSLOCKTYPE_MUTEX,
    /** The end of valid lock types. */
    RTVFSLOCKTYPE_END,
    /** The customary 32-bit type hack. */
    RTVFSLOCKTYPE_32BIT_HACK = 0x7fffffff
} RTVFSLOCKTYPE;

/** VFS lock handle. */
typedef struct RTVFSLOCKINTERNAL   *RTVFSLOCK;
/** Pointer to a VFS lock handle. */
typedef RTVFSLOCK                  *PRTVFSLOCK;
/** Nil VFS lock handle. */
#define NIL_RTVFSLOCK               ((RTVFSLOCK)~(uintptr_t)0)

/** Special handle value for creating a new read/write semaphore based lock. */
#define RTVFSLOCK_CREATE_RW         ((RTVFSLOCK)~(uintptr_t)1)
/** Special handle value for creating a new fast mutex semaphore based lock. */
#define RTVFSLOCK_CREATE_FASTMUTEX  ((RTVFSLOCK)~(uintptr_t)2)
/** Special handle value for creating a new mutex semaphore based lock. */
#define RTVFSLOCK_CREATE_MUTEX      ((RTVFSLOCK)~(uintptr_t)3)

/**
 * Retains a reference to the VFS lock handle.
 *
 * @returns New reference count on success, UINT32_MAX on failure.
 * @param   hLock           The VFS lock handle.
 */
RTDECL(uint32_t) RTVfsLockRetain(RTVFSLOCK hLock);

/**
 * Releases a reference to the VFS lock handle.
 *
 * @returns New reference count on success (0 if closed), UINT32_MAX on failure.
 * @param   hLock           The VFS lock handle.
 */
RTDECL(uint32_t) RTVfsLockRelease(RTVFSLOCK hLock);

/**
 * Gets the lock type.
 *
 * @returns The lock type on success, RTVFSLOCKTYPE_INVALID if the handle is
 *          not valid.
 * @param   hLock               The lock handle.
 */
RTDECL(RTVFSLOCKTYPE) RTVfsLockGetType(RTVFSLOCK hLock);



RTDECL(void) RTVfsLockAcquireReadSlow(RTVFSLOCK hLock);
RTDECL(void) RTVfsLockReleaseReadSlow(RTVFSLOCK hLock);
RTDECL(void) RTVfsLockAcquireWriteSlow(RTVFSLOCK hLock);
RTDECL(void) RTVfsLockReleaseWriteSlow(RTVFSLOCK hLock);

/**
 * Acquire a read lock.
 *
 * @param   hLock               The lock handle, can be NIL.
 */
DECLINLINE(void) RTVfsLockAcquireRead(RTVFSLOCK hLock)
{
    if (hLock != NIL_RTVFSLOCK)
        RTVfsLockAcquireReadSlow(hLock);
}


/**
 * Release a read lock.
 *
 * @param   hLock               The lock handle, can be NIL.
 */
DECLINLINE(void) RTVfsLockReleaseRead(RTVFSLOCK hLock)
{
    if (hLock != NIL_RTVFSLOCK)
        RTVfsLockReleaseReadSlow(hLock);
}


/**
 * Acquire a write lock.
 *
 * @param   hLock               The lock handle, can be NIL.
 */
DECLINLINE(void) RTVfsLockAcquireWrite(RTVFSLOCK hLock)
{
    if (hLock != NIL_RTVFSLOCK)
        RTVfsLockAcquireWriteSlow(hLock);
}


/**
 * Release a write lock.
 *
 * @param   hLock               The lock handle, can be NIL.
 */
DECLINLINE(void) RTVfsLockReleaseWrite(RTVFSLOCK hLock)
{
    if (hLock != NIL_RTVFSLOCK)
        RTVfsLockReleaseWriteSlow(hLock);
}

/** @}  */

/**
 * Info queried via RTVFSOBJOPS::pfnQueryInfoEx, ++.
 */
typedef enum RTVFSQIEX
{
    /** Invalid zero value. */
    RTVFSQIEX_INVALID = 0,
    /** Volume label.
     * Returns a UTF-8 string. */
    RTVFSQIEX_VOL_LABEL,
    /** Alternative volume label, the primary one for ISOs, otherwise treated same
     * as RTVFSQIEX_VOL_LABEL. */
    RTVFSQIEX_VOL_LABEL_ALT,
    /** Volume serial number.
     * Returns a uint32_t, uint64_t or RTUUID. */
    RTVFSQIEX_VOL_SERIAL,
    /** End of valid queries. */
    RTVFSQIEX_END,

    /** The usual 32-bit hack. */
    RTVFSQIEX_32BIT_SIZE_HACK = 0x7fffffff
} RTVFSQIEX;


/**
 * The basis for all virtual file system objects.
 */
typedef struct RTVFSOBJOPS
{
    /** The structure version (RTVFSOBJOPS_VERSION). */
    uint32_t                uVersion;
    /** The object type for type introspection. */
    RTVFSOBJTYPE            enmType;
    /** The name of the operations. */
    const char             *pszName;

    /**
     * Close the object.
     *
     * @returns IPRT status code.
     * @param   pvThis      The implementation specific file data.
     */
    DECLCALLBACKMEMBER(int, pfnClose,(void *pvThis));

    /**
     * Get information about the file.
     *
     * @returns IPRT status code. See RTVfsObjQueryInfo.
     * @retval  VERR_WRONG_TYPE if file system or file system stream.
     *
     * @param   pvThis      The implementation specific file data.
     * @param   pObjInfo    Where to return the object info on success.
     * @param   enmAddAttr  Which set of additional attributes to request.
     * @sa      RTVfsObjQueryInfo, RTFileQueryInfo, RTPathQueryInfo
     */
    DECLCALLBACKMEMBER(int, pfnQueryInfo,(void *pvThis, PRTFSOBJINFO pObjInfo, RTFSOBJATTRADD enmAddAttr));

    /**
     * Query arbritray information about the file, volume, or whatever.
     *
     * @returns IPRT status code.
     * @retval  VERR_BUFFER_OVERFLOW sets pcbRet.
     *
     * @param   pvThis      The implementation specific file data.
     * @param   enmInfo     The information being queried.
     * @param   pvInfo      Where to return the info.
     * @param   cbInfo      The size of the @a pvInfo buffer.
     * @param   pcbRet      The size of the returned data.  In case of
     *                      VERR_BUFFER_OVERFLOW this will be set to the required
     *                      buffer size.
     */
    DECLCALLBACKMEMBER(int, pfnQueryInfoEx,(void *pvThis, RTVFSQIEX enmInfo, void *pvInfo, size_t cbInfo, size_t *pcbRet));

    /** Marks the end of the structure (RTVFSOBJOPS_VERSION). */
    uintptr_t               uEndMarker;
} RTVFSOBJOPS;
/** Pointer to constant VFS object operations. */
typedef RTVFSOBJOPS const *PCRTVFSOBJOPS;

/** The RTVFSOBJOPS structure version. */
#define RTVFSOBJOPS_VERSION         RT_MAKE_U32_FROM_U8(0xff,0x1f,2,0)


/**
 * The VFS operations.
 */
typedef struct RTVFSOPS
{
    /** The basic object operation.  */
    RTVFSOBJOPS             Obj;
    /** The structure version (RTVFSOPS_VERSION). */
    uint32_t                uVersion;
    /** The virtual file system feature mask.  */
    uint32_t                fFeatures;

    /**
     * Opens the root directory.
     *
     * @returns IPRT status code.
     * @param   pvThis      The implementation specific data.
     * @param   phVfsDir    Where to return the handle to the root directory.
     */
    DECLCALLBACKMEMBER(int, pfnOpenRoot,(void *pvThis, PRTVFSDIR phVfsDir));

    /**
     * Query the status of the given storage range (optional).
     *
     * This can be used by the image compaction utilites to evict non-zero blocks
     * that aren't currently being used by the file system.
     *
     * @returns IPRT status code.
     * @param   pvThis      The implementation specific data.
     * @param   off         Start offset to check.
     * @param   cb          Number of bytes to check.
     * @param   pfUsed      Where to store whether the given range is in use.
     */
    DECLCALLBACKMEMBER(int, pfnQueryRangeState,(void *pvThis, uint64_t off, size_t cb, bool *pfUsed));

    /** @todo There will be more methods here to optimize opening and
     *        querying. */

#if 0
    /**
     * Optional entry point for optimizing path traversal within the file system.
     *
     * @returns IPRT status code.
     * @param   pvThis      The implementation specific data.
     * @param   pszPath     The path to resolve.
     * @param   poffPath    The current path offset on input, what we've
     *                      traversed to on successful return.
     * @param   phVfs???    Return handle to what we've traversed.
     * @param   p???        Return other stuff...
     */
    DECLCALLBACKMEMBER(int, pfnTraverse,(void *pvThis, const char *pszPath, size_t *poffPath, PRTVFS??? phVfs?, ???* p???));
#endif

    /** @todo need rename API */

    /** Marks the end of the structure (RTVFSOPS_VERSION). */
    uintptr_t               uEndMarker;
} RTVFSOPS;
/** Pointer to constant VFS operations. */
typedef RTVFSOPS const *PCRTVFSOPS;

/** The RTVFSOPS structure version. */
#define RTVFSOPS_VERSION            RT_MAKE_U32_FROM_U8(0xff,0x0f,1,0)

/** @name RTVFSOPS::fFeatures
 * @{ */
/** The VFS supports attaching other systems. */
#define RTVFSOPS_FEAT_ATTACH        RT_BIT_32(0)
/** @}  */

/**
 * Creates a new VFS handle.
 *
 * @returns IPRT status code
 * @param   pVfsOps             The VFS operations.
 * @param   cbInstance          The size of the instance data.
 * @param   hVfs                The VFS handle to associate this VFS with.
 *                              NIL_VFS is ok.
 * @param   hLock               Handle to a custom lock to be used with the new
 *                              object.  The reference is consumed.  NIL and
 *                              special lock handles are fine.
 * @param   phVfs               Where to return the new handle.
 * @param   ppvInstance         Where to return the pointer to the instance data
 *                              (size is @a cbInstance).
 */
RTDECL(int) RTVfsNew(PCRTVFSOPS pVfsOps, size_t cbInstance, RTVFS hVfs, RTVFSLOCK hLock,
                     PRTVFS phVfs, void **ppvInstance);


/**
 * Creates a new VFS base object handle.
 *
 * @returns IPRT status code
 * @param   pObjOps             The base object operations.
 * @param   cbInstance          The size of the instance data.
 * @param   hVfs                The VFS handle to associate this base object
 *                              with.  NIL_VFS is ok.
 * @param   hLock               Handle to a custom lock to be used with the new
 *                              object.  The reference is consumed.  NIL and
 *                              special lock handles are fine.
 * @param   phVfsObj            Where to return the new handle.
 * @param   ppvInstance         Where to return the pointer to the instance data
 *                              (size is @a cbInstance).
 */
RTDECL(int) RTVfsNewBaseObj(PCRTVFSOBJOPS pObjOps, size_t cbInstance, RTVFS hVfs, RTVFSLOCK hLock,
                            PRTVFSOBJ phVfsObj, void **ppvInstance);


/**
 * Gets the private data of a base object.
 *
 * @returns Pointer to the private data.  NULL if the handle is invalid in some
 *          way.
 * @param   hVfsObj             The I/O base object handle.
 * @param   pObjOps             The base object operations.  This servers as a
 *                              sort of password.
 */
RTDECL(void *) RTVfsObjToPrivate(RTVFSOBJ hVfsObj, PCRTVFSOBJOPS pObjOps);

/**
 * Additional operations for setting object attributes.
 */
typedef struct RTVFSOBJSETOPS
{
    /** The structure version (RTVFSOBJSETOPS_VERSION). */
    uint32_t                uVersion;
    /** The offset back to the RTVFSOBJOPS structure. */
    uint32_t                offObjOps;

    /**
     * Set the unix style owner and group.
     *
     * @returns IPRT status code.
     * @param   pvThis              The implementation specific file data.
     * @param   fMode               The new mode bits.
     * @param   fMask               The mask indicating which bits we are
     *                              changing.
     * @note    Optional, failing with VERR_WRITE_PROTECT if NULL.
     * @sa      RTFileSetMode
     */
    DECLCALLBACKMEMBER(int, pfnSetMode,(void *pvThis, RTFMODE fMode, RTFMODE fMask));

    /**
     * Set the timestamps associated with the object.
     *
     * @returns IPRT status code.
     * @param   pvThis              The implementation specific file data.
     * @param   pAccessTime         Pointer to the new access time. NULL if not
     *                              to be changed.
     * @param   pModificationTime   Pointer to the new modifcation time. NULL if
     *                              not to be changed.
     * @param   pChangeTime         Pointer to the new change time. NULL if not
     *                              to be changed.
     * @param   pBirthTime          Pointer to the new time of birth. NULL if
     *                              not to be changed.
     * @remarks See RTFileSetTimes for restrictions and behavior imposed by the
     *          host OS or underlying VFS provider.
     * @note    Optional, failing with VERR_WRITE_PROTECT if NULL.
     * @sa      RTFileSetTimes
     */
    DECLCALLBACKMEMBER(int, pfnSetTimes,(void *pvThis, PCRTTIMESPEC pAccessTime, PCRTTIMESPEC pModificationTime,
                                         PCRTTIMESPEC pChangeTime, PCRTTIMESPEC pBirthTime));

    /**
     * Set the unix style owner and group.
     *
     * @returns IPRT status code.
     * @param   pvThis      The implementation specific file data.
     * @param   uid         The user ID of the new owner.  NIL_RTUID if
     *                      unchanged.
     * @param   gid         The group ID of the new owner group.  NIL_RTGID if
     *                      unchanged.
     * @note    Optional, failing with VERR_WRITE_PROTECT if NULL.
     * @sa      RTFileSetOwner
     */
    DECLCALLBACKMEMBER(int, pfnSetOwner,(void *pvThis, RTUID uid, RTGID gid));

    /** Marks the end of the structure (RTVFSOBJSETOPS_VERSION). */
    uintptr_t               uEndMarker;
} RTVFSOBJSETOPS;
/** Pointer to const object attribute setter operations. */
typedef RTVFSOBJSETOPS const *PCRTVFSOBJSETOPS;

/** The RTVFSOBJSETOPS structure version. */
#define RTVFSOBJSETOPS_VERSION      RT_MAKE_U32_FROM_U8(0xff,0x2f,1,0)


/**
 * The filesystem stream operations.
 *
 * @extends RTVFSOBJOPS
 */
typedef struct RTVFSFSSTREAMOPS
{
    /** The basic object operation.  */
    RTVFSOBJOPS             Obj;
    /** The structure version (RTVFSFSSTREAMOPS_VERSION). */
    uint32_t                uVersion;
    /** Reserved field, MBZ. */
    uint32_t                fReserved;

    /**
     * Gets the next object in the stream.
     *
     * Readable streams only.
     *
     * @returns IPRT status code.
     * @retval  VINF_SUCCESS if a new object was retrieved.
     * @retval  VERR_EOF when there are no more objects.
     * @param   pvThis      The implementation specific directory data.
     * @param   ppszName    Where to return the object name.  Must be freed by
     *                      calling RTStrFree.
     * @param   penmType    Where to return the object type.
     * @param   phVfsObj    Where to return the object handle (referenced). This
     *                      must be cast to the desired type before use.
     * @sa      RTVfsFsStrmNext
     *
     * @note    Setting this member to NULL is okay for write-only streams.
     */
    DECLCALLBACKMEMBER(int, pfnNext,(void *pvThis, char **ppszName, RTVFSOBJTYPE *penmType, PRTVFSOBJ phVfsObj));

    /**
     * Adds another object into the stream.
     *
     * Writable streams only.
     *
     * @returns IPRT status code.
     * @param   pvThis      The implementation specific directory data.
     * @param   pszPath     The path to the object.
     * @param   hVfsObj     The object to add.
     * @param   fFlags      Reserved for the future, MBZ.
     * @sa      RTVfsFsStrmAdd
     *
     * @note    Setting this member to NULL is okay for read-only streams.
     */
    DECLCALLBACKMEMBER(int, pfnAdd,(void *pvThis, const char *pszPath, RTVFSOBJ hVfsObj, uint32_t fFlags));

    /**
     * Pushes an byte stream onto the stream (optional).
     *
     * Writable streams only.
     *
     * This differs from RTVFSFSSTREAMOPS::pfnAdd() in that it will create a regular
     * file in the output file system stream and provide the actual content bytes
     * via the returned I/O stream object.
     *
     * @returns IPRT status code.
     * @param   pvThis      The implementation specific directory data.
     * @param   pszPath     The path to the file.
     * @param   cbFile      The file size.  This can also be set to UINT64_MAX if
     *                      the file system stream is backed by a file.
     * @param   paObjInfo   Array of zero or more RTFSOBJINFO structures containing
     *                      different pieces of information about the file.  If any
     *                      provided, the first one should be a RTFSOBJATTRADD_UNIX
     *                      one, additional can be supplied if wanted.  What exactly
     *                      is needed depends on the underlying FS stream
     *                      implementation.
     * @param   cObjInfo    Number of items in the array @a paObjInfo points at.
     * @param   fFlags      RTVFSFSSTRM_PUSH_F_XXX.
     * @param   phVfsIos    Where to return the I/O stream to feed the file content
     *                      to.  If the FS stream is backed by a file, the returned
     *                      handle can be cast to a file if necessary.
     */
    DECLCALLBACKMEMBER(int, pfnPushFile,(void *pvThis, const char *pszPath, uint64_t cbFile,
                                         PCRTFSOBJINFO paObjInfo, uint32_t cObjInfo, uint32_t fFlags, PRTVFSIOSTREAM phVfsIos));

    /**
     * Marks the end of the stream.
     *
     * Writable streams only.
     *
     * @returns IPRT status code.
     * @param   pvThis      The implementation specific directory data.
     * @sa      RTVfsFsStrmEnd
     *
     * @note    Setting this member to NULL is okay for read-only streams.
     */
    DECLCALLBACKMEMBER(int, pfnEnd,(void *pvThis));

    /** Marks the end of the structure (RTVFSFSSTREAMOPS_VERSION). */
    uintptr_t               uEndMarker;
} RTVFSFSSTREAMOPS;
/** Pointer to const object attribute setter operations. */
typedef RTVFSFSSTREAMOPS const *PCRTVFSFSSTREAMOPS;

/** The RTVFSFSSTREAMOPS structure version. */
#define RTVFSFSSTREAMOPS_VERSION    RT_MAKE_U32_FROM_U8(0xff,0x3f,2,0)


/**
 * Creates a new VFS filesystem stream handle.
 *
 * @returns IPRT status code
 * @param   pFsStreamOps        The filesystem stream operations.
 * @param   cbInstance          The size of the instance data.
 * @param   hVfs                The VFS handle to associate this filesystem
 *                              stream with.  NIL_VFS is ok.
 * @param   hLock               Handle to a custom lock to be used with the new
 *                              object.  The reference is consumed.  NIL and
 *                              special lock handles are fine.
 * @param   fAccess             RTFILE_O_READ and/or RTFILE_O_WRITE.
 * @param   phVfsFss            Where to return the new handle.
 * @param   ppvInstance         Where to return the pointer to the instance data
 *                              (size is @a cbInstance).
 */
RTDECL(int) RTVfsNewFsStream(PCRTVFSFSSTREAMOPS pFsStreamOps, size_t cbInstance, RTVFS hVfs, RTVFSLOCK hLock, uint32_t fAccess,
                             PRTVFSFSSTREAM phVfsFss, void **ppvInstance);

/**
 * Gets the private data of an filesystem stream.
 *
 * @returns Pointer to the private data.  NULL if the handle is invalid in some
 *          way.
 * @param   hVfsFss             The FS stream handle.
 * @param   pFsStreamOps        The FS stream operations.  This servers as a
 *                              sort of password.
 */
RTDECL(void *) RTVfsFsStreamToPrivate(RTVFSFSSTREAM hVfsFss, PCRTVFSFSSTREAMOPS pFsStreamOps);


/**
 * The directory operations.
 *
 * @extends RTVFSOBJOPS
 * @extends RTVFSOBJSETOPS
 */
typedef struct RTVFSDIROPS
{
    /** The basic object operation.  */
    RTVFSOBJOPS             Obj;
    /** The structure version (RTVFSDIROPS_VERSION). */
    uint32_t                uVersion;
    /** Reserved field, MBZ. */
    uint32_t                fReserved;
    /** The object setter operations. */
    RTVFSOBJSETOPS          ObjSet;

    /**
     * Generic method for opening any kind of file system object.
     *
     * Can also create files and directories.  Symbolic links, devices and such
     * needs to be created using special methods or this would end up being way more
     * complicated than it already is.
     *
     * There are optional specializations available.
     *
     * @returns IPRT status code.
     * @retval  VERR_PATH_NOT_FOUND or VERR_FILE_NOT_FOUND if @a pszEntry was not
     *          found.
     * @retval  VERR_IS_A_FILE if @a pszEntry is a file or similar but @a fFlags
     *          indicates that the type of object should not be opened.
     * @retval  VERR_IS_A_DIRECTORY if @a pszEntry is a directory but @a fFlags
     *          indicates that directories should not be opened.
     * @retval  VERR_IS_A_SYMLINK if @a pszEntry is a symbolic link but @a fFlags
     *          indicates that symbolic links should not be opened (or followed).
     * @retval  VERR_IS_A_FIFO if @a pszEntry is a FIFO but @a fFlags indicates that
     *          FIFOs should not be opened.
     * @retval  VERR_IS_A_SOCKET if @a pszEntry is a socket but @a fFlags indicates
     *          that sockets should not be opened.
     * @retval  VERR_IS_A_BLOCK_DEVICE if @a pszEntry is a block device but
     *          @a fFlags indicates that block devices should not be opened, or vice
     *          versa.
     *
     * @param   pvThis      The implementation specific directory data.
     * @param   pszEntry    The name of the immediate file to open or create.
     * @param   fOpenFile   RTFILE_O_XXX combination.
     * @param   fObjFlags   More flags: RTVFSOBJ_F_XXX, RTPATH_F_XXX.
     *                      The meaning of RTPATH_F_FOLLOW_LINK differs here, if
     *                      @a pszEntry is a symlink it should be opened for
     *                      traversal rather than according to @a fOpenFile.
     * @param   phVfsObj    Where to return the handle to the opened object.
     * @sa      RTFileOpen, RTDirOpen
     */
    DECLCALLBACKMEMBER(int, pfnOpen,(void *pvThis, const char *pszEntry, uint64_t fOpenFile,
                                     uint32_t fObjFlags, PRTVFSOBJ phVfsObj));

    /**
     * Optional method for symbolic link handling in the vfsstddir.cpp.
     *
     * This is really just a hack to make symbolic link handling work when working
     * with directory objects that doesn't have an associated VFS.  It also helps
     * deal with drive letters in symbolic links on Windows and OS/2.
     *
     * @returns IPRT status code.
     * @retval  VERR_PATH_IS_RELATIVE if @a pszPath isn't absolute and should be
     *          handled using pfnOpen().
     *
     * @param   pvThis      The implementation specific directory data.
     * @param   pszRoot     Path to the alleged root.
     * @param   phVfsDir    Where to return the handle to the specified root
     *                      directory (or may current dir on a drive letter).
     */
    DECLCALLBACKMEMBER(int, pfnFollowAbsoluteSymlink,(void *pvThis, const char *pszRoot, PRTVFSDIR phVfsDir));

    /**
     * Open or create a file.
     *
     * @returns IPRT status code.
     * @param   pvThis      The implementation specific directory data.
     * @param   pszFilename The name of the immediate file to open or create.
     * @param   fOpen       The open flags (RTFILE_O_XXX).
     * @param   phVfsFile   Where to return the handle to the opened file.
     * @note    Optional.  RTVFSDIROPS::pfnOpenObj will be used if NULL.
     * @sa      RTFileOpen.
     */
    DECLCALLBACKMEMBER(int, pfnOpenFile,(void *pvThis, const char *pszFilename, uint64_t fOpen, PRTVFSFILE phVfsFile));

    /**
     * Open an existing subdirectory.
     *
     * @returns IPRT status code.
     * @retval  VERR_IS_A_SYMLINK if @a pszSubDir is a symbolic link.
     * @retval  VERR_NOT_A_DIRECTORY is okay for symbolic links too.
     *
     * @param   pvThis      The implementation specific directory data.
     * @param   pszSubDir   The name of the immediate subdirectory to open.
     * @param   fFlags      RTDIR_F_XXX.
     * @param   phVfsDir    Where to return the handle to the opened directory.
     *                      Optional.
     * @note    Optional.  RTVFSDIROPS::pfnOpenObj will be used if NULL.
     * @sa      RTDirOpen.
     */
    DECLCALLBACKMEMBER(int, pfnOpenDir,(void *pvThis, const char *pszSubDir, uint32_t fFlags, PRTVFSDIR phVfsDir));

    /**
     * Creates a new subdirectory.
     *
     * @returns IPRT status code.
     * @param   pvThis      The implementation specific directory data.
     * @param   pszSubDir   The name of the immediate subdirectory to create.
     * @param   fMode       The mode mask of the new directory.
     * @param   phVfsDir    Where to optionally return the handle to the newly
     *                      create directory.
     * @note    Optional.  RTVFSDIROPS::pfnOpenObj will be used if NULL.
     * @sa      RTDirCreate.
     */
    DECLCALLBACKMEMBER(int, pfnCreateDir,(void *pvThis, const char *pszSubDir, RTFMODE fMode, PRTVFSDIR phVfsDir));

    /**
     * Opens an existing symbolic link.
     *
     * @returns IPRT status code.
     * @param   pvThis      The implementation specific directory data.
     * @param   pszSymlink  The name of the immediate symbolic link to open.
     * @param   phVfsSymlink    Where to optionally return the handle to the
     *                      newly create symbolic link.
     * @note    Optional.  RTVFSDIROPS::pfnOpenObj will be used if NULL.
     * @sa      RTSymlinkCreate.
     */
    DECLCALLBACKMEMBER(int, pfnOpenSymlink,(void *pvThis, const char *pszSymlink, PRTVFSSYMLINK phVfsSymlink));

    /**
     * Creates a new symbolic link.
     *
     * @returns IPRT status code.
     * @param   pvThis      The implementation specific directory data.
     * @param   pszSymlink  The name of the immediate symbolic link to create.
     * @param   pszTarget   The symbolic link target.
     * @param   enmType     The symbolic link type.
     * @param   phVfsSymlink    Where to optionally return the handle to the
     *                      newly create symbolic link.
     * @sa      RTSymlinkCreate.
     */
    DECLCALLBACKMEMBER(int, pfnCreateSymlink,(void *pvThis, const char *pszSymlink, const char *pszTarget,
                                              RTSYMLINKTYPE enmType, PRTVFSSYMLINK phVfsSymlink));

    /**
     * Query information about an entry.
     *
     * @returns IPRT status code.
     * @param   pvThis      The implementation specific directory data.
     * @param   pszEntry    The name of the directory entry to remove.
     * @param   pObjInfo    Where to return the info on success.
     * @param   enmAddAttr  Which set of additional attributes to request.
     * @note    Optional.  RTVFSDIROPS::pfnOpenObj and RTVFSOBJOPS::pfnQueryInfo
     *          will be used if NULL.
     * @sa      RTPathQueryInfo, RTVFSOBJOPS::pfnQueryInfo
     */
    DECLCALLBACKMEMBER(int, pfnQueryEntryInfo,(void *pvThis, const char *pszEntry,
                                               PRTFSOBJINFO pObjInfo, RTFSOBJATTRADD enmAddAttr));

    /**
     * Removes a directory entry.
     *
     * @returns IPRT status code.
     * @param   pvThis      The implementation specific directory data.
     * @param   pszEntry    The name of the directory entry to remove.
     * @param   fType       If non-zero, this restricts the type of the entry to
     *                      the object type indicated by the mask
     *                      (RTFS_TYPE_XXX).
     * @sa      RTFileRemove, RTDirRemove, RTSymlinkRemove.
     */
    DECLCALLBACKMEMBER(int, pfnUnlinkEntry,(void *pvThis, const char *pszEntry, RTFMODE fType));

    /**
     * Renames a directory entry.
     *
     * @returns IPRT status code.
     * @param   pvThis      The implementation specific directory data.
     * @param   pszEntry    The name of the directory entry to rename.
     * @param   fType       If non-zero, this restricts the type of the entry to
     *                      the object type indicated by the mask
     *                      (RTFS_TYPE_XXX).
     * @param   pszNewName  The new entry name.
     * @sa      RTPathRename
     *
     * @todo    This API is not flexible enough, must be able to rename between
     *          directories within a file system.
     */
    DECLCALLBACKMEMBER(int, pfnRenameEntry,(void *pvThis, const char *pszEntry, RTFMODE fType, const char *pszNewName));

    /**
     * Rewind the directory stream so that the next read returns the first
     * entry.
     *
     * @returns IPRT status code.
     * @param   pvThis      The implementation specific directory data.
     */
    DECLCALLBACKMEMBER(int, pfnRewindDir,(void *pvThis));

    /**
     * Rewind the directory stream so that the next read returns the first
     * entry.
     *
     * @returns IPRT status code.
     * @param   pvThis      The implementation specific directory data.
     * @param   pDirEntry   Output buffer.
     * @param   pcbDirEntry Complicated, see RTDirReadEx.
     * @param   enmAddAttr  Which set of additional attributes to request.
     * @sa      RTDirReadEx
     */
    DECLCALLBACKMEMBER(int, pfnReadDir,(void *pvThis, PRTDIRENTRYEX pDirEntry, size_t *pcbDirEntry, RTFSOBJATTRADD enmAddAttr));

    /** Marks the end of the structure (RTVFSDIROPS_VERSION). */
    uintptr_t               uEndMarker;
} RTVFSDIROPS;
/** Pointer to const directory operations. */
typedef RTVFSDIROPS const *PCRTVFSDIROPS;
/** The RTVFSDIROPS structure version. */
#define RTVFSDIROPS_VERSION         RT_MAKE_U32_FROM_U8(0xff,0x4f,1,0)


/**
 * Creates a new VFS directory handle.
 *
 * @returns IPRT status code
 * @param   pDirOps             The directory operations.
 * @param   cbInstance          The size of the instance data.
 * @param   fFlags              RTVFSDIR_F_XXX
 * @param   hVfs                The VFS handle to associate this directory with.
 *                              NIL_VFS is ok.
 * @param   hLock               Handle to a custom lock to be used with the new
 *                              object.  The reference is consumed.  NIL and
 *                              special lock handles are fine.
 * @param   phVfsDir            Where to return the new handle.
 * @param   ppvInstance         Where to return the pointer to the instance data
 *                              (size is @a cbInstance).
 */
RTDECL(int) RTVfsNewDir(PCRTVFSDIROPS pDirOps, size_t cbInstance, uint32_t fFlags, RTVFS hVfs, RTVFSLOCK hLock,
                        PRTVFSDIR phVfsDir, void **ppvInstance);

/** @name RTVFSDIR_F_XXX
 * @{ */
/** Don't reference the @a hVfs parameter passed to RTVfsNewDir.
 * This is a permanent root directory hack. */
#define RTVFSDIR_F_NO_VFS_REF   RT_BIT_32(0)
/** @} */

/**
 * Gets the private data of a directory.
 *
 * @returns Pointer to the private data.  NULL if the handle is invalid in some
 *          way.
 * @param   hVfsDir             The directory handle.
 * @param   pDirOps             The directory operations.  This servers as a
 *                              sort of password.
 */
RTDECL(void *) RTVfsDirToPrivate(RTVFSDIR hVfsDir, PCRTVFSDIROPS pDirOps);


/**
 * The symbolic link operations.
 *
 * @extends RTVFSOBJOPS
 * @extends RTVFSOBJSETOPS
 */
typedef struct RTVFSSYMLINKOPS
{
    /** The basic object operation.  */
    RTVFSOBJOPS             Obj;
    /** The structure version (RTVFSSYMLINKOPS_VERSION). */
    uint32_t                uVersion;
    /** Reserved field, MBZ. */
    uint32_t                fReserved;
    /** The object setter operations. */
    RTVFSOBJSETOPS          ObjSet;

    /**
     * Read the symbolic link target.
     *
     * @returns IPRT status code.
     * @param   pvThis      The implementation specific symbolic link data.
     * @param   pszTarget   The target buffer.
     * @param   cbTarget    The size of the target buffer.
     * @sa      RTSymlinkRead
     */
    DECLCALLBACKMEMBER(int, pfnRead,(void *pvThis, char *pszTarget, size_t cbTarget));

    /** Marks the end of the structure (RTVFSSYMLINKOPS_VERSION). */
    uintptr_t               uEndMarker;
} RTVFSSYMLINKOPS;
/** Pointer to const symbolic link operations. */
typedef RTVFSSYMLINKOPS const *PCRTVFSSYMLINKOPS;
/** The RTVFSSYMLINKOPS structure version. */
#define RTVFSSYMLINKOPS_VERSION     RT_MAKE_U32_FROM_U8(0xff,0x5f,1,0)


/**
 * Creates a new VFS symlink handle.
 *
 * @returns IPRT status code
 * @param   pSymlinkOps         The symlink operations.
 * @param   cbInstance          The size of the instance data.
 * @param   hVfs                The VFS handle to associate this symlink object
 *                              with.  NIL_VFS is ok.
 * @param   hLock               Handle to a custom lock to be used with the new
 *                              object.  The reference is consumed.  NIL and
 *                              special lock handles are fine.
 * @param   phVfsSym            Where to return the new handle.
 * @param   ppvInstance         Where to return the pointer to the instance data
 *                              (size is @a cbInstance).
 */
RTDECL(int) RTVfsNewSymlink(PCRTVFSSYMLINKOPS pSymlinkOps, size_t cbInstance, RTVFS hVfs, RTVFSLOCK hLock,
                            PRTVFSSYMLINK phVfsSym, void **ppvInstance);


/**
 * Gets the private data of a symbolic link.
 *
 * @returns Pointer to the private data.  NULL if the handle is invalid in some
 *          way.
 * @param   hVfsSym             The symlink handle.
 * @param   pSymlinkOps         The symlink operations.  This servers as a sort
 *                              of password.
 */
RTDECL(void *) RTVfsSymlinkToPrivate(RTVFSSYMLINK hVfsSym, PCRTVFSSYMLINKOPS pSymlinkOps);

/**
 * The basis for all I/O objects (files, pipes, sockets, devices, ++).
 *
 * @extends RTVFSOBJOPS
 */
typedef struct RTVFSIOSTREAMOPS
{
    /** The basic object operation.  */
    RTVFSOBJOPS             Obj;
    /** The structure version (RTVFSIOSTREAMOPS_VERSION). */
    uint32_t                uVersion;
    /** Feature field. */
    uint32_t                fFeatures;

    /**
     * Reads from the file/stream.
     *
     * @returns IPRT status code. See RTVfsIoStrmRead.
     * @param   pvThis      The implementation specific file data.
     * @param   off         Where to read at, -1 for the current position.
     * @param   pSgBuf      Gather buffer describing the bytes that are to be
     *                      written.
     * @param   fBlocking   If @c true, the call is blocking, if @c false it
     *                      should not block.
     * @param   pcbRead     Where return the number of bytes actually read.
     *                      This is set it 0 by the caller.  If NULL, try read
     *                      all and fail if incomplete.
     * @sa      RTVfsIoStrmRead, RTVfsIoStrmSgRead, RTVfsFileRead,
     *          RTVfsFileReadAt, RTFileRead, RTFileReadAt.
     */
    DECLCALLBACKMEMBER(int, pfnRead,(void *pvThis, RTFOFF off, PCRTSGBUF pSgBuf, bool fBlocking, size_t *pcbRead));

    /**
     * Writes to the file/stream.
     *
     * @returns IPRT status code.
     * @param   pvThis      The implementation specific file data.
     * @param   off         Where to start wrinting, -1 for the current
     *                      position.
     * @param   pSgBuf      Gather buffers describing the bytes that are to be
     *                      written.
     * @param   fBlocking   If @c true, the call is blocking, if @c false it
     *                      should not block.
     * @param   pcbWritten  Where to return the number of bytes actually
     *                      written.  This is set it 0 by the caller.  If
     *                      NULL, try write it all and fail if incomplete.
     * @note    Optional, failing with VERR_WRITE_PROTECT if NULL.
     * @sa      RTFileWrite, RTFileWriteAt.
     */
    DECLCALLBACKMEMBER(int, pfnWrite,(void *pvThis, RTFOFF off, PCRTSGBUF pSgBuf, bool fBlocking, size_t *pcbWritten));

    /**
     * Flushes any pending data writes to the stream.
     *
     * @returns IPRT status code.
     * @param   pvThis      The implementation specific file data.
     * @sa      RTFileFlush.
     */
    DECLCALLBACKMEMBER(int, pfnFlush,(void *pvThis));

    /**
     * Poll for events.
     *
     * @returns IPRT status code.
     * @param   pvThis      The implementation specific file data.
     * @param   fEvents     The events to poll for (RTPOLL_EVT_XXX).
     * @param   cMillies    How long to wait for event to eventuate.
     * @param   fIntr       Whether the wait is interruptible and can return
     *                      VERR_INTERRUPTED (@c true) or if this condition
     *                      should be hidden from the caller (@c false).
     * @param   pfRetEvents Where to return the event mask.
     * @note    Optional.  If NULL, immediately return all requested non-error
     *          events, waiting for errors works like sleep.
     * @sa      RTPollSetAdd, RTPoll, RTPollNoResume.
     */
    DECLCALLBACKMEMBER(int, pfnPollOne,(void *pvThis, uint32_t fEvents, RTMSINTERVAL cMillies, bool fIntr,
                                        uint32_t *pfRetEvents));

    /**
     * Tells the current file/stream position.
     *
     * @returns IPRT status code.
     * @param   pvThis      The implementation specific file data.
     * @param   poffActual  Where to return the actual offset.
     * @sa      RTFileTell
     */
    DECLCALLBACKMEMBER(int, pfnTell,(void *pvThis, PRTFOFF poffActual));

    /**
     * Skips @a cb ahead in the stream.
     *
     * @returns IPRT status code.
     * @param   pvThis      The implementation specific file data.
     * @param   cb          The number bytes to skip.
     * @remarks This is optional and can be NULL.
     */
    DECLCALLBACKMEMBER(int, pfnSkip,(void *pvThis, RTFOFF cb));

    /**
     * Fills the stream with @a cb zeros.
     *
     * @returns IPRT status code.
     * @param   pvThis      The implementation specific file data.
     * @param   cb          The number of zero bytes to insert.
     * @remarks This is optional and can be NULL.
     */
    DECLCALLBACKMEMBER(int, pfnZeroFill,(void *pvThis, RTFOFF cb));

    /** Marks the end of the structure (RTVFSIOSTREAMOPS_VERSION). */
    uintptr_t               uEndMarker;
} RTVFSIOSTREAMOPS;
/** Pointer to const I/O stream operations. */
typedef RTVFSIOSTREAMOPS const *PCRTVFSIOSTREAMOPS;

/** The RTVFSIOSTREAMOPS structure version. */
#define RTVFSIOSTREAMOPS_VERSION    RT_MAKE_U32_FROM_U8(0xff,0x6f,1,0)

/** @name RTVFSIOSTREAMOPS::fFeatures
 * @{ */
/** No scatter gather lists, thank you. */
#define RTVFSIOSTREAMOPS_FEAT_NO_SG         RT_BIT_32(0)
/** Mask of the valid I/O stream feature flags. */
#define RTVFSIOSTREAMOPS_FEAT_VALID_MASK    UINT32_C(0x00000001)
/** @}  */


/**
 * Creates a new VFS I/O stream handle.
 *
 * @returns IPRT status code
 * @param   pIoStreamOps        The I/O stream operations.
 * @param   cbInstance          The size of the instance data.
 * @param   fOpen               The open flags.  The minimum is the access mask.
 * @param   hVfs                The VFS handle to associate this I/O stream
 *                              with.  NIL_VFS is ok.
 * @param   hLock               Handle to a custom lock to be used with the new
 *                              object.  The reference is consumed.  NIL and
 *                              special lock handles are fine.
 * @param   phVfsIos            Where to return the new handle.
 * @param   ppvInstance         Where to return the pointer to the instance data
 *                              (size is @a cbInstance).
 */
RTDECL(int) RTVfsNewIoStream(PCRTVFSIOSTREAMOPS pIoStreamOps, size_t cbInstance, uint32_t fOpen, RTVFS hVfs, RTVFSLOCK hLock,
                             PRTVFSIOSTREAM phVfsIos, void **ppvInstance);


/**
 * Gets the private data of an I/O stream.
 *
 * @returns Pointer to the private data.  NULL if the handle is invalid in some
 *          way.
 * @param   hVfsIos             The I/O stream handle.
 * @param   pIoStreamOps        The I/O stream operations.  This servers as a
 *                              sort of password.
 */
RTDECL(void *) RTVfsIoStreamToPrivate(RTVFSIOSTREAM hVfsIos, PCRTVFSIOSTREAMOPS pIoStreamOps);


/**
 * The file operations.
 *
 * @extends RTVFSIOSTREAMOPS
 * @extends RTVFSOBJSETOPS
 */
typedef struct RTVFSFILEOPS
{
    /** The I/O stream and basis object operations. */
    RTVFSIOSTREAMOPS        Stream;
    /** The structure version (RTVFSFILEOPS_VERSION). */
    uint32_t                uVersion;
    /** Reserved field, MBZ. */
    uint32_t                fReserved;
    /** The object setter operations. */
    RTVFSOBJSETOPS          ObjSet;

    /**
     * Changes the current file position.
     *
     * @returns IPRT status code.
     * @param   pvThis      The implementation specific file data.
     * @param   offSeek     The offset to seek.
     * @param   uMethod     The seek method, i.e. what the seek is relative to.
     * @param   poffActual  Where to return the actual offset.
     * @sa      RTFileSeek
     */
    DECLCALLBACKMEMBER(int, pfnSeek,(void *pvThis, RTFOFF offSeek, unsigned uMethod, PRTFOFF poffActual));

    /**
     * Get the current file size.
     *
     * @returns IPRT status code.
     * @param   pvThis      The implementation specific file data.
     * @param   pcbFile     Where to store the current file size.
     * @sa      RTFileQuerySize
     */
    DECLCALLBACKMEMBER(int, pfnQuerySize,(void *pvThis, uint64_t *pcbFile));

    /**
     * Change the file size.
     *
     * @returns IPRT status code.
     * @retval  VERR_ACCESS_DENIED if handle isn't writable.
     * @retval  VERR_WRITE_PROTECT if read-only file system.
     * @retval  VERR_FILE_TOO_BIG if cbSize is larger than what the file system can
     *          theoretically deal with.
     * @retval  VERR_DISK_FULL if the file system if full.
     * @retval  VERR_NOT_SUPPORTED if fFlags indicates some operation that's not
     *          supported by the file system / host operating system.
     *
     * @param   pvThis      The implementation specific file data.
     * @param   pcbFile     Where to store the current file size.
     * @param   fFlags      RTVFSFILE_SET_SIZE_F_XXX.
     * @note    Optional.  If NULL, VERR_WRITE_PROTECT will be returned.
     * @sa      RTFileSetSize, RTFileSetAllocationSize
     */
    DECLCALLBACKMEMBER(int, pfnSetSize,(void *pvThis, uint64_t cbFile, uint32_t fFlags));

    /**
     * Determine the maximum file size.
     *
     * This won't take amount of freespace into account, just the limitations of the
     * underlying file system / host operating system.
     *
     * @returns IPRT status code.
     * @param   pvThis      The implementation specific file data.
     * @param   pcbMax      Where to return the max file size.
     * @note    Optional.  If NULL, VERR_NOT_IMPLEMENTED will be returned.
     * @sa      RTFileQueryMaxSizeEx
     */
    DECLCALLBACKMEMBER(int, pfnQueryMaxSize,(void *pvThis, uint64_t *pcbMax));

    /** @todo There will be more methods here. */

    /** Marks the end of the structure (RTVFSFILEOPS_VERSION). */
    uintptr_t               uEndMarker;
} RTVFSFILEOPS;
/** Pointer to const file operations. */
typedef RTVFSFILEOPS const *PCRTVFSFILEOPS;

/** The RTVFSFILEOPS structure version. */
#define RTVFSFILEOPS_VERSION        RT_MAKE_U32_FROM_U8(0xff,0x7f,2,0)

/**
 * Creates a new VFS file handle.
 *
 * @returns IPRT status code
 * @param   pFileOps            The file operations.
 * @param   cbInstance          The size of the instance data.
 * @param   fOpen               The open flags.  The minimum is the access mask.
 * @param   hVfs                The VFS handle to associate this file with.
 *                              NIL_VFS is ok.
 * @param   hLock               Handle to a custom lock to be used with the new
 *                              object.  The reference is consumed.  NIL and
 *                              special lock handles are fine.
 * @param   phVfsFile           Where to return the new handle.
 * @param   ppvInstance         Where to return the pointer to the instance data
 *                              (size is @a cbInstance).
 */
RTDECL(int) RTVfsNewFile(PCRTVFSFILEOPS pFileOps, size_t cbInstance, uint32_t fOpen, RTVFS hVfs, RTVFSLOCK hLock,
                         PRTVFSFILE phVfsFile, void **ppvInstance);


/** @defgroup grp_rt_vfs_ll_util        VFS Utility APIs
 * @{ */

/**
 * Parsed path.
 */
typedef struct RTVFSPARSEDPATH
{
    /** The length of the path in szCopy. */
    uint16_t        cch;
    /** The number of path components. */
    uint16_t        cComponents;
    /** Set if the path ends with slash, indicating that it's a directory
     * reference and not a file reference.  The slash has been removed from
     * the copy. */
    bool            fDirSlash;
    /** Set if absolute. */
    bool            fAbsolute;
    /** The offset where each path component starts, i.e. the char after the
     * slash.  The array has cComponents + 1 entries, where the final one is
     * cch + 1 so that one can always terminate the current component by
     * szPath[aoffComponent[i] - 1] = '\0'. */
    uint16_t        aoffComponents[RTPATH_MAX / 2 + 1];
    /** A normalized copy of the path.
     * Reserve some extra space so we can be more relaxed about overflow
     * checks and terminator paddings, especially when recursing. */
    char            szPath[RTPATH_MAX];
} RTVFSPARSEDPATH;
/** Pointer to a parsed path. */
typedef RTVFSPARSEDPATH *PRTVFSPARSEDPATH;

/** The max accepted path length.
 * This must be a few chars shorter than RTVFSPARSEDPATH::szPath because we
 * use two terminators and wish be a little bit lazy with checking. */
#define RTVFSPARSEDPATH_MAX     (RTPATH_MAX - 4)

/**
 * Appends @a pszPath (relative) to the already parsed path @a pPath.
 *
 * @retval  VINF_SUCCESS
 * @retval  VERR_FILENAME_TOO_LONG
 * @retval  VERR_INTERNAL_ERROR_4
 * @param   pPath               The parsed path to append @a pszPath onto.
 *                              This is both input and output.
 * @param   pszPath             The path to append.  This must be relative.
 * @param   piRestartComp       The component to restart parsing at.  This is
 *                              input/output.  The input does not have to be
 *                              within the valid range.  Optional.
 */
RTDECL(int) RTVfsParsePathAppend(PRTVFSPARSEDPATH pPath, const char *pszPath, uint16_t *piRestartComp);

/**
 * Parses a path.
 *
 * @retval  VINF_SUCCESS
 * @retval  VERR_FILENAME_TOO_LONG
 * @param   pPath               Where to store the parsed path.
 * @param   pszPath             The path to parse.  Absolute or relative to @a
 *                              pszCwd.
 * @param   pszCwd              The current working directory.  Must be
 *                              absolute.
 */
RTDECL(int) RTVfsParsePath(PRTVFSPARSEDPATH pPath, const char *pszPath, const char *pszCwd);

/**
 * Same as RTVfsParsePath except that it allocates a temporary buffer.
 *
 * @retval  VINF_SUCCESS
 * @retval  VERR_NO_TMP_MEMORY
 * @retval  VERR_FILENAME_TOO_LONG
 * @param   pszPath             The path to parse.  Absolute or relative to @a
 *                              pszCwd.
 * @param   pszCwd              The current working directory.  Must be
 *                              absolute.
 * @param   ppPath              Where to store the pointer to the allocated
 *                              buffer containing the parsed path.  This must
 *                              be freed by calling RTVfsParsePathFree.  NULL
 *                              will be stored on failured.
 */
RTDECL(int) RTVfsParsePathA(const char *pszPath, const char *pszCwd, PRTVFSPARSEDPATH *ppPath);

/**
 * Frees a buffer returned by RTVfsParsePathA.
 *
 * @param   pPath               The parsed path buffer to free.  NULL is fine.
 */
RTDECL(void) RTVfsParsePathFree(PRTVFSPARSEDPATH pPath);

/**
 * Dummy implementation of RTVFSIOSTREAMOPS::pfnPollOne.
 *
 * This handles the case where there is no chance any events my be raised and
 * all that is required is to wait according to the parameters.
 *
 * @returns IPRT status code.
 * @param   fEvents     The events to poll for (RTPOLL_EVT_XXX).
 * @param   cMillies    How long to wait for event to eventuate.
 * @param   fIntr       Whether the wait is interruptible and can return
 *                      VERR_INTERRUPTED (@c true) or if this condition
 *                      should be hidden from the caller (@c false).
 * @param   pfRetEvents Where to return the event mask.
 * @sa      RTVFSIOSTREAMOPS::pfnPollOne, RTPollSetAdd, RTPoll, RTPollNoResume.
 */
RTDECL(int) RTVfsUtilDummyPollOne(uint32_t fEvents, RTMSINTERVAL cMillies, bool fIntr, uint32_t *pfRetEvents);

/** @}  */


/** @defgroup grp_rt_vfs_lowlevel_chain     VFS Chains (Low Level)
 * @ref grp_rt_vfs_chain
 * @{
 */

/** Pointer to a VFS chain element registration record. */
typedef struct RTVFSCHAINELEMENTREG *PRTVFSCHAINELEMENTREG;
/** Pointer to a const VFS chain element registration record. */
typedef struct RTVFSCHAINELEMENTREG const *PCRTVFSCHAINELEMENTREG;

/**
 * VFS chain element argument.
 */
typedef struct RTVFSCHAINELEMENTARG
{
    /** The string argument value. */
    char                   *psz;
    /** The specification offset of this argument. */
    uint16_t                offSpec;
    /** Provider specific value. */
    uint64_t                uProvider;
} RTVFSCHAINELEMENTARG;
/** Pointer to a VFS chain element argument. */
typedef RTVFSCHAINELEMENTARG *PRTVFSCHAINELEMENTARG;


/**
 * VFS chain element specification.
 */
typedef struct RTVFSCHAINELEMSPEC
{
    /** The provider name.
     * This can be NULL if this is the final component and it's just a path. */
    char                   *pszProvider;
    /** The input type, RTVFSOBJTYPE_INVALID if first. */
    RTVFSOBJTYPE            enmTypeIn;
    /** The element type.
     *  RTVFSOBJTYPE_END if this is the final component and it's just a path. */
    RTVFSOBJTYPE            enmType;
    /** The input spec offset of this element. */
    uint16_t                offSpec;
    /** The length of the input spec. */
    uint16_t                cchSpec;
    /** The number of arguments. */
    uint32_t                cArgs;
    /** Arguments. */
    PRTVFSCHAINELEMENTARG   paArgs;

    /** The provider. */
    PCRTVFSCHAINELEMENTREG  pProvider;
    /** Provider specific value. */
    uint64_t                uProvider;
    /** The object (with reference). */
    RTVFSOBJ                hVfsObj;
} RTVFSCHAINELEMSPEC;
/** Pointer to a chain element specification. */
typedef RTVFSCHAINELEMSPEC *PRTVFSCHAINELEMSPEC;
/** Pointer to a const chain element specification. */
typedef RTVFSCHAINELEMSPEC const *PCRTVFSCHAINELEMSPEC;


/**
 * Parsed VFS chain specification.
 */
typedef struct RTVFSCHAINSPEC
{
    /** Open directory flags (RTFILE_O_XXX). */
    uint64_t                fOpenFile;
    /** To be defined. */
    uint32_t                fOpenDir;
    /** The type desired by the caller. */
    RTVFSOBJTYPE            enmDesiredType;
    /** The number of elements. */
    uint32_t                cElements;
    /** The elements. */
    PRTVFSCHAINELEMSPEC     paElements;
} RTVFSCHAINSPEC;
/** Pointer to a parsed VFS chain specification. */
typedef RTVFSCHAINSPEC *PRTVFSCHAINSPEC;
/** Pointer to a const, parsed VFS chain specification. */
typedef RTVFSCHAINSPEC const *PCRTVFSCHAINSPEC;


/**
 * A chain element provider registration record.
 */
typedef struct RTVFSCHAINELEMENTREG
{
    /** The version (RTVFSCHAINELEMENTREG_VERSION). */
    uint32_t                uVersion;
    /** Reserved, MBZ. */
    uint32_t                fReserved;
    /** The provider name (unique). */
    const char             *pszName;
    /** For chaining the providers. */
    RTLISTNODE              ListEntry;
    /** Help text. */
    const char             *pszHelp;

    /**
     * Checks the element specification.
     *
     * This is allowed to parse arguments and use pSpec->uProvider and
     * pElement->paArgs[].uProvider to store information that pfnInstantiate and
     * pfnCanReuseElement may use later on, thus avoiding duplicating work/code.
     *
     * @returns IPRT status code.
     * @param   pProviderReg    Pointer to the element provider registration.
     * @param   pSpec           The chain specification.
     * @param   pElement        The chain element specification to validate.
     * @param   poffError       Where to return error offset on failure.  This is
     *                          set to the pElement->offSpec on input, so it only
     *                          needs to be adjusted if an argument is at fault.
     * @param   pErrInfo        Where to return additional error information, if
     *                          available.  Optional.
     */
    DECLCALLBACKMEMBER(int, pfnValidate,(PCRTVFSCHAINELEMENTREG pProviderReg, PRTVFSCHAINSPEC pSpec,
                                         PRTVFSCHAINELEMSPEC pElement, uint32_t *poffError, PRTERRINFO pErrInfo));

    /**
     * Create a VFS object according to the element specification.
     *
     * @returns IPRT status code.
     * @param   pProviderReg    Pointer to the element provider registration.
     * @param   pSpec           The chain specification.
     * @param   pElement        The chain element specification to instantiate.
     * @param   hPrevVfsObj     Handle to the previous VFS object, NIL_RTVFSOBJ if
     *                          first.
     * @param   phVfsObj        Where to return the VFS object handle.
     * @param   poffError       Where to return error offset on failure.  This is
     *                          set to the pElement->offSpec on input, so it only
     *                          needs to be adjusted if an argument is at fault.
     * @param   pErrInfo        Where to return additional error information, if
     *                          available.  Optional.
     */
    DECLCALLBACKMEMBER(int, pfnInstantiate,(PCRTVFSCHAINELEMENTREG pProviderReg, PCRTVFSCHAINSPEC pSpec,
                                            PCRTVFSCHAINELEMSPEC pElement, RTVFSOBJ hPrevVfsObj,
                                            PRTVFSOBJ phVfsObj, uint32_t *poffError, PRTERRINFO pErrInfo));

    /**
     * Determins whether the element can be reused.
     *
     * This is for handling situations accessing the same file system twice, like
     * for both the source and destiation of a copy operation.  This allows not only
     * sharing resources and avoid doing things twice, but also helps avoid file
     * sharing violations and inconsistencies araising from the image being updated
     * and read independently.
     *
     * @returns true if the element from @a pReuseSpec an be reused, false if not.
     * @param   pProviderReg    Pointer to the element provider registration.
     * @param   pSpec           The chain specification.
     * @param   pElement        The chain element specification.
     * @param   pReuseSpec      The chain specification of the existing chain.
     * @param   pReuseElement   The chain element specification of the existing
     *                          element that is being considered for reuse.
     */
    DECLCALLBACKMEMBER(bool, pfnCanReuseElement,(PCRTVFSCHAINELEMENTREG pProviderReg,
                                                 PCRTVFSCHAINSPEC pSpec, PCRTVFSCHAINELEMSPEC pElement,
                                                 PCRTVFSCHAINSPEC pReuseSpec, PCRTVFSCHAINELEMSPEC pReuseElement));

    /** End marker (RTVFSCHAINELEMENTREG_VERSION). */
    uintptr_t               uEndMarker;
} RTVFSCHAINELEMENTREG;

/** The VFS chain element registration record version number. */
#define RTVFSCHAINELEMENTREG_VERSION        RT_MAKE_U32_FROM_U8(0xff, 0x7f, 1, 0)


/**
 * Parses the specification.
 *
 * @returns IPRT status code.
 * @param   pszSpec         The specification string to parse.
 * @param   fFlags          Flags, see RTVFSCHAIN_PF_XXX.
 * @param   enmDesiredType  The object type the caller wants to interface with.
 * @param   ppSpec          Where to return the pointer to the parsed
 *                          specification.  This must be freed by calling
 *                          RTVfsChainSpecFree.  Will always be set (unless
 *                          invalid parameters.)
 * @param   poffError       Where to return the offset into the input
 *                          specification of what's causing trouble.  Always
 *                          set, unless this argument causes an invalid pointer
 *                          error.
 */
RTDECL(int) RTVfsChainSpecParse(const char *pszSpec, uint32_t fFlags, RTVFSOBJTYPE enmDesiredType,
                                PRTVFSCHAINSPEC *ppSpec, uint32_t *poffError);

/** @name RTVfsChainSpecParse
 * @{ */
/** Mask of valid flags. */
#define RTVFSCHAIN_PF_VALID_MASK                UINT32_C(0x00000000)
/** @} */

/**
 * Checks and setups the chain.
 *
 * @returns IPRT status code.
 * @param   pSpec           The parsed specification.
 * @param   pReuseSpec      Spec to reuse if applicable. Optional.
 * @param   phVfsObj        Where to return the VFS object.
 * @param   ppszFinalPath   Where to return the pointer to the final path if
 *                          applicable.  The caller needs to check whether this
 *                          is NULL or a path, in the former case nothing more
 *                          needs doing, whereas in the latter the caller must
 *                          perform the desired operation(s) on *phVfsObj using
 *                          the final path.
 * @param   poffError       Where to return the offset into the input
 *                          specification of what's causing trouble.  Always
 *                          set, unless this argument causes an invalid pointer
 *                          error.
 * @param   pErrInfo        Where to return additional error information, if
 *                          available.  Optional.
 */
RTDECL(int) RTVfsChainSpecCheckAndSetup(PRTVFSCHAINSPEC pSpec, PCRTVFSCHAINSPEC pReuseSpec,
                                        PRTVFSOBJ phVfsObj, const char **ppszFinalPath, uint32_t *poffError, PRTERRINFO pErrInfo);

/**
 * Frees a parsed chain specification.
 *
 * @param   pSpec               What RTVfsChainSpecParse returned.  NULL is
 *                              quietly ignored.
 */
RTDECL(void) RTVfsChainSpecFree(PRTVFSCHAINSPEC pSpec);

/**
 * Registers a chain element provider.
 *
 * @returns IPRT status code
 * @param   pRegRec             The registration record.
 * @param   fFromCtor           Indicates where we're called from.
 */
RTDECL(int) RTVfsChainElementRegisterProvider(PRTVFSCHAINELEMENTREG pRegRec, bool fFromCtor);

/**
 * Deregisters a chain element provider.
 *
 * @returns IPRT status code
 * @param   pRegRec             The registration record.
 * @param   fFromDtor           Indicates where we're called from.
 */
RTDECL(int) RTVfsChainElementDeregisterProvider(PRTVFSCHAINELEMENTREG pRegRec, bool fFromDtor);


/** @def RTVFSCHAIN_AUTO_REGISTER_ELEMENT_PROVIDER
 * Automatically registers a chain element provider using a global constructor
 * and destructor hack.
 *
 * @param   pRegRec     Pointer to the registration record.
 * @param   name        Some unique variable name prefix.
 */

#ifdef __cplusplus
/**
 * Class used for registering a VFS chain element provider.
 */
class RTVfsChainElementAutoRegisterHack
{
private:
    /** The registration record, NULL if registration failed.  */
    PRTVFSCHAINELEMENTREG m_pRegRec;

public:
    RTVfsChainElementAutoRegisterHack(PRTVFSCHAINELEMENTREG a_pRegRec)
        : m_pRegRec(a_pRegRec)
    {
        int rc = RTVfsChainElementRegisterProvider(m_pRegRec, true);
        if (RT_FAILURE(rc))
            m_pRegRec = NULL;
    }

    ~RTVfsChainElementAutoRegisterHack()
    {
        RTVfsChainElementDeregisterProvider(m_pRegRec, true);
        m_pRegRec = NULL;
    }
};

# define RTVFSCHAIN_AUTO_REGISTER_ELEMENT_PROVIDER(pRegRec, name) \
    static RTVfsChainElementAutoRegisterHack name ## AutoRegistrationHack(pRegRec)

#else
# define RTVFSCHAIN_AUTO_REGISTER_ELEMENT_PROVIDER(pRegRec, name) \
    extern void *name ## AutoRegistrationHack = \
        &Sorry_but_RTVFSCHAIN_AUTO_REGISTER_ELEMENT_PROVIDER_does_not_work_in_c_source_files
#endif


/**
 * Common worker for the 'stdfile' and 'open' providers for implementing
 * RTVFSCHAINELEMENTREG::pfnValidate.
 *
 * Stores the RTFILE_O_XXX flags in pSpec->uProvider.
 *
 * @returns IPRT status code.
 * @param   pSpec       The chain specification.
 * @param   pElement    The chain element specification to validate.
 * @param   poffError   Where to return error offset on failure.  This is set to
 *                      the pElement->offSpec on input, so it only needs to be
 *                      adjusted if an argument is at fault.
 * @param   pErrInfo    Where to return additional error information, if
 *                      available.  Optional.
 */
RTDECL(int) RTVfsChainValidateOpenFileOrIoStream(PRTVFSCHAINSPEC pSpec, PRTVFSCHAINELEMSPEC pElement,
                                                 uint32_t *poffError, PRTERRINFO pErrInfo);


/** @}  */


/** @} */

RT_C_DECLS_END

#endif /* !IPRT_INCLUDED_vfslowlevel_h */


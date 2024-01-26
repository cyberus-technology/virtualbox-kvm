/** @file
 * IPRT - File I/O.
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

#ifndef IPRT_INCLUDED_file_h
#define IPRT_INCLUDED_file_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <iprt/cdefs.h>
#include <iprt/types.h>
#include <iprt/stdarg.h>
#include <iprt/fs.h>
#include <iprt/sg.h>

RT_C_DECLS_BEGIN

/** @defgroup grp_rt_fileio     RTFile - File I/O
 * @ingroup grp_rt
 * @{
 */

/** Platform specific text line break.
 * @deprecated Use text I/O streams and '\\n'. See iprt/stream.h. */
#if defined(RT_OS_OS2) || defined(RT_OS_WINDOWS)
# define RTFILE_LINEFEED    "\r\n"
#else
# define RTFILE_LINEFEED    "\n"
#endif

/** Platform specific native standard input "handle". */
#ifdef RT_OS_WINDOWS
# define RTFILE_NATIVE_STDIN ((uint32_t)-10)
#else
# define RTFILE_NATIVE_STDIN 0
#endif

/** Platform specific native standard out "handle". */
#ifdef RT_OS_WINDOWS
# define RTFILE_NATIVE_STDOUT ((uint32_t)-11)
#else
# define RTFILE_NATIVE_STDOUT 1
#endif

/** Platform specific native standard error "handle". */
#ifdef RT_OS_WINDOWS
# define RTFILE_NATIVE_STDERR ((uint32_t)-12)
#else
# define RTFILE_NATIVE_STDERR 2
#endif


/**
 * Checks if the specified file name exists and is a regular file.
 *
 * Symbolic links will be resolved.
 *
 * @returns true if it's a regular file, false if it isn't.
 * @param   pszPath         The path to the file.
 *
 * @sa      RTDirExists, RTPathExists, RTSymlinkExists.
 */
RTDECL(bool) RTFileExists(const char *pszPath);

/**
 * Queries the size of a file, given the path to it.
 *
 * Symbolic links will be resolved.
 *
 * @returns IPRT status code.
 * @param   pszPath         The path to the file.
 * @param   pcbFile         Where to return the file size (bytes).
 *
 * @sa      RTFileQuerySize, RTPathQueryInfoEx.
 */
RTDECL(int) RTFileQuerySizeByPath(const char *pszPath, uint64_t *pcbFile);


/** @name Open flags
 * @{ */
/** Attribute access only.
 * @remarks Only accepted on windows, requires RTFILE_O_ACCESS_ATTR_MASK
 *          to yield a non-zero result.  Otherwise, this is invalid. */
#define RTFILE_O_ATTR_ONLY              UINT32_C(0x00000000)
/** Open the file with read access. */
#define RTFILE_O_READ                   UINT32_C(0x00000001)
/** Open the file with write access. */
#define RTFILE_O_WRITE                  UINT32_C(0x00000002)
/** Open the file with read & write access. */
#define RTFILE_O_READWRITE              UINT32_C(0x00000003)
/** The file access mask.
 * @remarks The value 0 is invalid, except for windows special case. */
#define RTFILE_O_ACCESS_MASK            UINT32_C(0x00000003)

/** Open file in APPEND mode, so all writes to the file handle will
 * append data at the end of the file.
 * @remarks It is ignored if write access is not requested, that is
 *          RTFILE_O_WRITE is not set.
 * @note    Behaviour of functions differ between hosts: See RTFileWriteAt, as
 *          well as ticketref:19003 (RTFileSetSize). */
#define RTFILE_O_APPEND                 UINT32_C(0x00000004)
                                     /* UINT32_C(0x00000008) is unused atm. */

/** Sharing mode: deny none. */
#define RTFILE_O_DENY_NONE              UINT32_C(0x00000080)
/** Sharing mode: deny read. */
#define RTFILE_O_DENY_READ              UINT32_C(0x00000010)
/** Sharing mode: deny write. */
#define RTFILE_O_DENY_WRITE             UINT32_C(0x00000020)
/** Sharing mode: deny read and write. */
#define RTFILE_O_DENY_READWRITE         UINT32_C(0x00000030)
/** Sharing mode: deny all. */
#define RTFILE_O_DENY_ALL               RTFILE_O_DENY_READWRITE
/** Sharing mode: do NOT deny delete (NT).
 * @remarks This might not be implemented on all platforms, and will be
 *          defaulted & ignored on those.
 */
#define RTFILE_O_DENY_NOT_DELETE        UINT32_C(0x00000040)
/** Sharing mode mask. */
#define RTFILE_O_DENY_MASK              UINT32_C(0x000000f0)

/** Action: Open an existing file. */
#define RTFILE_O_OPEN                   UINT32_C(0x00000700)
/** Action: Create a new file or open an existing one. */
#define RTFILE_O_OPEN_CREATE            UINT32_C(0x00000100)
/** Action: Create a new a file. */
#define RTFILE_O_CREATE                 UINT32_C(0x00000200)
/** Action: Create a new file or replace an existing one. */
#define RTFILE_O_CREATE_REPLACE         UINT32_C(0x00000300)
/** Action mask. */
#define RTFILE_O_ACTION_MASK            UINT32_C(0x00000700)

/** Turns off indexing of files on Windows hosts, *CREATE* only.
 * @remarks Window only. */
#define RTFILE_O_NOT_CONTENT_INDEXED    UINT32_C(0x00000800)
/** Truncate the file.
 * @remarks This will not truncate files opened for read-only.
 * @remarks The truncation doesn't have to be atomically, so anyone else opening
 *          the file may be racing us. The caller is responsible for not causing
 *          this race. */
#define RTFILE_O_TRUNCATE               UINT32_C(0x00001000)
/** Make the handle inheritable on RTProcessCreate(/exec). */
#define RTFILE_O_INHERIT                UINT32_C(0x00002000)
/** Open file in non-blocking mode - non-portable.
 * @remarks This flag may not be supported on all platforms, in which case it's
 *          considered an invalid parameter. */
#define RTFILE_O_NON_BLOCK              UINT32_C(0x00004000)
/** Write through directly to disk. Workaround to avoid iSCSI
 * initiator deadlocks on Windows hosts.
 * @remarks This might not be implemented on all platforms, and will be ignored
 *          on those. */
#define RTFILE_O_WRITE_THROUGH          UINT32_C(0x00008000)

/** Attribute access: Attributes can be read if the file is being opened with
 * read access, and can be written with write access. */
#define RTFILE_O_ACCESS_ATTR_DEFAULT    UINT32_C(0x00000000)
/** Attribute access: Attributes can be read.
 * @remarks Windows only.  */
#define RTFILE_O_ACCESS_ATTR_READ       UINT32_C(0x00010000)
/** Attribute access: Attributes can be written.
 * @remarks Windows only.  */
#define RTFILE_O_ACCESS_ATTR_WRITE      UINT32_C(0x00020000)
/** Attribute access: Attributes can be both read & written.
 * @remarks Windows only.  */
#define RTFILE_O_ACCESS_ATTR_READWRITE  UINT32_C(0x00030000)
/** Attribute access: The file attributes access mask.
 * @remarks Windows only.  */
#define RTFILE_O_ACCESS_ATTR_MASK       UINT32_C(0x00030000)

/** Open file for async I/O
 * @remarks This flag may not be needed on all platforms, and will be ignored on
 *          those. */
#define RTFILE_O_ASYNC_IO               UINT32_C(0x00040000)

/** Disables caching.
 *
 * Useful when using very big files which might bring the host I/O scheduler to
 * its knees during high I/O load.
 *
 * @remarks This flag might impose restrictions
 *          on the buffer alignment, start offset and/or transfer size.
 *
 *          On Linux the buffer needs to be aligned to the 512 sector
 *          boundary.
 *
 *          On Windows the FILE_FLAG_NO_BUFFERING is used (see
 *          http://msdn.microsoft.com/en-us/library/cc644950(VS.85).aspx ).
 *          The buffer address, the transfer size and offset needs to be aligned
 *          to the sector size of the volume.  Furthermore FILE_APPEND_DATA is
 *          disabled.  To write beyond the size of file use RTFileSetSize prior
 *          writing the data to the file.
 *
 *          This flag does not work on Solaris if the target filesystem is ZFS.
 *          RTFileOpen will return an error with that configuration.  When used
 *          with UFS the same alginment restrictions apply like Linux and
 *          Windows.
 *
 * @remarks This might not be implemented on all platforms, and will be ignored
 *          on those.
 */
#define RTFILE_O_NO_CACHE               UINT32_C(0x00080000)

/** Don't allow symbolic links as part of the path.
 * @remarks this flag is currently not implemented and will be ignored. */
#define RTFILE_O_NO_SYMLINKS            UINT32_C(0x20000000)

/** Unix file mode mask for use when creating files. */
#define RTFILE_O_CREATE_MODE_MASK       UINT32_C(0x1ff00000)
/** The number of bits to shift to get the file mode mask.
 * To extract it: (fFlags & RTFILE_O_CREATE_MODE_MASK) >> RTFILE_O_CREATE_MODE_SHIFT.
 */
#define RTFILE_O_CREATE_MODE_SHIFT      20

/** Temporary file that should be automatically deleted when closed.
 * If not supported by the OS, the open call will fail with VERR_NOT_SUPPORTED
 * to prevent leaving undeleted files behind.
 * @note On unix the file wont be visible and cannot be accessed by it's path.
 *       On Windows it will be visible but only accessible of deletion is
 *       shared.  Not implemented on OS/2. */
#define RTFILE_O_TEMP_AUTO_DELETE       UINT32_C(0x40000000)

                                      /* UINT32_C(0x80000000) is unused atm. */

/** Mask of all valid flags.
 * @remark  This doesn't validate the access mode properly.
 */
#define RTFILE_O_VALID_MASK             UINT32_C(0x7ffffff7)

/** @} */


/** Action taken by RTFileOpenEx. */
typedef enum RTFILEACTION
{
    /** Invalid zero value.   */
    RTFILEACTION_INVALID = 0,
    /** Existing file was opened (returned by RTFILE_O_OPEN and
     * RTFILE_O_OPEN_CREATE). */
    RTFILEACTION_OPENED,
    /** New file was created (returned by RTFILE_O_CREATE and
     * RTFILE_O_OPEN_CREATE). */
    RTFILEACTION_CREATED,
    /** Existing file was replaced (returned by RTFILE_O_CREATE_REPLACE). */
    RTFILEACTION_REPLACED,
    /** Existing file was truncated (returned if RTFILE_O_TRUNCATE take effect). */
    RTFILEACTION_TRUNCATED,
    /** The file already exists (returned by RTFILE_O_CREATE on failure). */
    RTFILEACTION_ALREADY_EXISTS,
    /** End of valid values. */
    RTFILEACTION_END,
    /** Type size hack.   */
    RTFILEACTION_32BIT_HACK = 0x7fffffff
} RTFILEACTION;
/** Pointer to action taken value (RTFileOpenEx).    */
typedef RTFILEACTION *PRTFILEACTION;


#ifdef IN_RING3
/**
 * Force the use of open flags for all files opened after the setting is
 * changed. The caller is responsible for not causing races with RTFileOpen().
 *
 * @returns iprt status code.
 * @param   fOpenForAccess  Access mode to which the set/mask settings apply.
 * @param   fSet            Open flags to be forced set.
 * @param   fMask           Open flags to be masked out.
 */
RTR3DECL(int)  RTFileSetForceFlags(unsigned fOpenForAccess, unsigned fSet, unsigned fMask);
#endif /* IN_RING3 */

/**
 * Open a file.
 *
 * @returns iprt status code.
 * @param   pFile           Where to store the handle to the opened file.
 * @param   pszFilename     Path to the file which is to be opened. (UTF-8)
 * @param   fOpen           Open flags, i.e a combination of the RTFILE_O_* defines.
 *                          The ACCESS, ACTION and DENY flags are mandatory!
 */
RTDECL(int)  RTFileOpen(PRTFILE pFile, const char *pszFilename, uint64_t fOpen);

/**
 * Open a file given as a format string.
 *
 * @returns iprt status code.
 * @param   pFile           Where to store the handle to the opened file.
 * @param   fOpen           Open flags, i.e a combination of the RTFILE_O_* defines.
 *                          The ACCESS, ACTION and DENY flags are mandatory!
 * @param   pszFilenameFmt  Format string givin the path to the file which is to
 *                          be opened. (UTF-8)
 * @param   ...             Arguments to the format string.
 */
RTDECL(int)  RTFileOpenF(PRTFILE pFile, uint64_t fOpen, const char *pszFilenameFmt, ...) RT_IPRT_FORMAT_ATTR(3, 4);

/**
 * Open a file given as a format string.
 *
 * @returns iprt status code.
 * @param   pFile           Where to store the handle to the opened file.
 * @param   fOpen           Open flags, i.e a combination of the RTFILE_O_* defines.
 *                          The ACCESS, ACTION and DENY flags are mandatory!
 * @param   pszFilenameFmt  Format string givin the path to the file which is to
 *                          be opened. (UTF-8)
 * @param   va              Arguments to the format string.
 */
RTDECL(int)  RTFileOpenV(PRTFILE pFile, uint64_t fOpen, const char *pszFilenameFmt, va_list va) RT_IPRT_FORMAT_ATTR(3, 0);

/**
 * Open a file, extended version.
 *
 * @returns iprt status code.
 * @param   pszFilename     Path to the file which is to be opened. (UTF-8)
 * @param   fOpen           Open flags, i.e a combination of the RTFILE_O_* defines.
 *                          The ACCESS, ACTION and DENY flags are mandatory!
 * @param   phFile          Where to store the handle to the opened file.
 * @param   penmActionTaken Where to return an indicator of which action was
 *                          taken.  This is optional and it is recommended to
 *                          pass NULL when not strictly needed as it adds
 *                          complexity (slower) on posix systems.
 */
RTDECL(int)  RTFileOpenEx(const char *pszFilename, uint64_t fOpen, PRTFILE phFile, PRTFILEACTION penmActionTaken);

/**
 * Open the bit bucket (aka /dev/null or nul).
 *
 * @returns IPRT status code.
 * @param   phFile          Where to store the handle to the opened file.
 * @param   fAccess         The desired access only, i.e. read, write or both.
 */
RTDECL(int)  RTFileOpenBitBucket(PRTFILE phFile, uint64_t fAccess);

/**
 * Duplicates a file handle.
 *
 * @returns IPRT status code.
 * @param   hFileSrc        The handle to duplicate.
 * @param   fFlags          RTFILE_O_INHERIT or zero.
 * @param   phFileNew       Where to return the new file handle
 */
RTDECL(int)  RTFileDup(RTFILE hFileSrc, uint64_t fFlags, PRTFILE phFileNew);

/**
 * Close a file opened by RTFileOpen().
 *
 * @returns iprt status code.
 * @param   File            The file handle to close.
 */
RTDECL(int)  RTFileClose(RTFILE File);

/**
 * Creates an IPRT file handle from a native one.
 *
 * @returns IPRT status code.
 * @param   pFile           Where to store the IPRT file handle.
 * @param   uNative         The native handle.
 */
RTDECL(int) RTFileFromNative(PRTFILE pFile, RTHCINTPTR uNative);

/**
 * Gets the native handle for an IPRT file handle.
 *
 * @return  The native handle.
 * @param   File            The IPRT file handle.
 */
RTDECL(RTHCINTPTR) RTFileToNative(RTFILE File);

/**
 * Delete a file.
 *
 * @returns iprt status code.
 * @param   pszFilename     Path to the file which is to be deleted. (UTF-8)
 * @todo    This is a RTPath api!
 */
RTDECL(int)  RTFileDelete(const char *pszFilename);

/** @name Seek flags.
 * @{ */
/** Seek from the start of the file. */
#define RTFILE_SEEK_BEGIN     0x00
/** Seek from the current file position. */
#define RTFILE_SEEK_CURRENT   0x01
/** Seek from the end of the file. */
#define RTFILE_SEEK_END       0x02
/** @internal */
#define RTFILE_SEEK_FIRST     RTFILE_SEEK_BEGIN
/** @internal */
#define RTFILE_SEEK_LAST      RTFILE_SEEK_END
/** @} */


/**
 * Changes the read & write position in a file.
 *
 * @returns iprt status code.
 * @param   File        Handle to the file.
 * @param   offSeek     Offset to seek.
 * @param   uMethod     Seek method, i.e. one of the RTFILE_SEEK_* defines.
 * @param   poffActual  Where to store the new file position.
 *                      NULL is allowed.
 */
RTDECL(int)  RTFileSeek(RTFILE File, int64_t offSeek, unsigned uMethod, uint64_t *poffActual);

/**
 * Read bytes from a file.
 *
 * @returns iprt status code.
 * @param   File        Handle to the file.
 * @param   pvBuf       Where to put the bytes we read.
 * @param   cbToRead    How much to read.
 * @param   pcbRead     How much we actually read .
 *                      If NULL an error will be returned for a partial read.
 */
RTDECL(int)  RTFileRead(RTFILE File, void *pvBuf, size_t cbToRead, size_t *pcbRead);

/**
 * Read bytes from a file at a given offset.
 *
 * @returns iprt status code.
 * @param   File        Handle to the file.
 * @param   off         Where to read.
 * @param   pvBuf       Where to put the bytes we read.
 * @param   cbToRead    How much to read.
 * @param   pcbRead     How much we actually read .
 *                      If NULL an error will be returned for a partial read.
 *
 * @note    OS/2 requires separate seek and write calls.
 *
 * @note    Whether the file position is modified or not is host specific.
 */
RTDECL(int)  RTFileReadAt(RTFILE File, RTFOFF off, void *pvBuf, size_t cbToRead, size_t *pcbRead);

/**
 * Read bytes from a file at a given offset into a S/G buffer.
 *
 * @returns iprt status code.
 * @param   hFile       Handle to the file.
 * @param   pSgBuf      Pointer to the S/G buffer to read into.
 * @param   cbToRead    How much to read.
 * @param   pcbRead     How much we actually read .
 *                      If NULL an error will be returned for a partial read.
 *
 * @note    It is not possible to guarantee atomicity on all platforms, so
 *          caller must take care wrt concurrent access to @a hFile.
 */
RTDECL(int)  RTFileSgRead(RTFILE hFile, PRTSGBUF pSgBuf, size_t cbToRead, size_t *pcbRead);

/**
 * Read bytes from a file at a given offset into a S/G buffer.
 *
 * @returns iprt status code.
 * @param   hFile       Handle to the file.
 * @param   off         Where to read.
 * @param   pSgBuf      Pointer to the S/G buffer to read into.
 * @param   cbToRead    How much to read.
 * @param   pcbRead     How much we actually read .
 *                      If NULL an error will be returned for a partial read.
 *
 * @note    Whether the file position is modified or not is host specific.
 *
 * @note    It is not possible to guarantee atomicity on all platforms, so
 *          caller must take care wrt concurrent access to @a hFile.
 */
RTDECL(int)  RTFileSgReadAt(RTFILE hFile, RTFOFF off, PRTSGBUF pSgBuf, size_t cbToRead, size_t *pcbRead);

/**
 * Write bytes to a file.
 *
 * @returns iprt status code.
 * @param   File        Handle to the file.
 * @param   pvBuf       What to write.
 * @param   cbToWrite   How much to write.
 * @param   pcbWritten  How much we actually wrote.
 *                      If NULL an error will be returned for a partial write.
 */
RTDECL(int)  RTFileWrite(RTFILE File, const void *pvBuf, size_t cbToWrite, size_t *pcbWritten);

/**
 * Write bytes to a file at a given offset.
 *
 * @returns iprt status code.
 * @param   hFile       Handle to the file.
 * @param   off         Where to write.
 * @param   pvBuf       What to write.
 * @param   cbToWrite   How much to write.
 * @param   pcbWritten  How much we actually wrote.
 *                      If NULL an error will be returned for a partial write.
 *
 * @note    OS/2 requires separate seek and write calls.
 *
 * @note    Whether the file position is modified or not is host specific.
 *
 * @note    Whether @a off is used when @a hFile was opened with RTFILE_O_APPEND
 *          is also host specific.  Currently Linux is the the only one
 *          documented to ignore @a off.
 */
RTDECL(int)  RTFileWriteAt(RTFILE hFile, RTFOFF off, const void *pvBuf, size_t cbToWrite, size_t *pcbWritten);

/**
 * Write bytes from a S/G buffer to a file.
 *
 * @returns iprt status code.
 * @param   hFile       Handle to the file.
 * @param   pSgBuf      What to write.
 * @param   cbToWrite   How much to write.
 * @param   pcbWritten  How much we actually wrote.
 *                      If NULL an error will be returned for a partial write.
 *
 * @note    It is not possible to guarantee atomicity on all platforms, so
 *          caller must take care wrt concurrent access to @a hFile.
 */
RTDECL(int)  RTFileSgWrite(RTFILE hFile, PRTSGBUF pSgBuf, size_t cbToWrite, size_t *pcbWritten);

/**
 * Write bytes from a S/G buffer to a file at a given offset.
 *
 * @returns iprt status code.
 * @param   hFile       Handle to the file.
 * @param   off         Where to write.
 * @param   pSgBuf      What to write.
 * @param   cbToWrite   How much to write.
 * @param   pcbWritten  How much we actually wrote.
 *                      If NULL an error will be returned for a partial write.
 *
 * @note    It is not possible to guarantee atomicity on all platforms, so
 *          caller must take care wrt concurrent access to @a hFile.
 *
 * @note    Whether the file position is modified or not is host specific.
 *
 * @note    Whether @a off is used when @a hFile was opened with RTFILE_O_APPEND
 *          is also host specific.  Currently Linux is the the only one
 *          documented to ignore @a off.
 */
RTDECL(int)  RTFileSgWriteAt(RTFILE hFile, RTFOFF off, PRTSGBUF pSgBuf, size_t cbToWrite, size_t *pcbWritten);

/**
 * Flushes the buffers for the specified file.
 *
 * @returns iprt status code.
 * @retval  VINF_NOT_SUPPORTED if it is a special file that does not support
 *          flushing.  This is reported as a informational status since in most
 *          cases this is entirely harmless (e.g. tty) and simplifies the usage.
 * @param   File        Handle to the file.
 */
RTDECL(int)  RTFileFlush(RTFILE File);

/**
 * Set the size of the file.
 *
 * @returns iprt status code.
 * @param   File        Handle to the file.
 * @param   cbSize      The new file size.
 */
RTDECL(int)  RTFileSetSize(RTFILE File, uint64_t cbSize);

/**
 * Query the size of the file.
 *
 * @returns iprt status code.
 * @param   File        Handle to the file.
 * @param   pcbSize     Where to store the filesize.
 */
RTDECL(int)  RTFileQuerySize(RTFILE File, uint64_t *pcbSize);

/**
 * Determine the maximum file size.
 *
 * @returns The max size of the file.
 *          -1 on failure, the file position is undefined.
 * @param   File        Handle to the file.
 * @see     RTFileQueryMaxSizeEx.
 */
RTDECL(RTFOFF) RTFileGetMaxSize(RTFILE File);

/**
 * Determine the maximum file size.
 *
 * @returns IPRT status code.
 * @param   File        Handle to the file.
 * @param   pcbMax      Where to store the max file size.
 * @see     RTFileGetMaxSize.
 */
RTDECL(int) RTFileQueryMaxSizeEx(RTFILE File, PRTFOFF pcbMax);

/**
 * Queries the sector size (/ logical block size) for a disk or similar.
 *
 * @returns IPRT status code.
 * @retval  VERR_INVALID_FUNCTION if not a disk/similar.  Could also be returned
 *          if not really implemented.
 * @param   hFile       Handle to the disk.  This must typically be a device
 *                      rather than a file or directory, though this may vary
 *                      from OS to OS.
 * @param   pcbSector   Where to store the sector size.
 */
RTDECL(int) RTFileQuerySectorSize(RTFILE hFile, uint32_t *pcbSector);

/**
 * Gets the current file position.
 *
 * @returns File offset.
 * @returns ~0UUL on failure.
 * @param   File        Handle to the file.
 */
RTDECL(uint64_t)  RTFileTell(RTFILE File);

/**
 * Checks if the supplied handle is valid.
 *
 * @returns true if valid.
 * @returns false if invalid.
 * @param   File        The file handle
 */
RTDECL(bool) RTFileIsValid(RTFILE File);

/**
 * Copies a file.
 *
 * @returns IPRT status code
 * @retval VERR_ALREADY_EXISTS if the destination file exists.
 *
 * @param   pszSrc      The path to the source file.
 * @param   pszDst      The path to the destination file.
 *                      This file will be created.
 */
RTDECL(int) RTFileCopy(const char *pszSrc, const char *pszDst);

/**
 * Copies a file given the handles to both files.
 *
 * @returns IPRT status code
 *
 * @param   FileSrc     The source file. The file position is unaltered.
 * @param   FileDst     The destination file.
 *                      On successful returns the file position is at the end of the file.
 *                      On failures the file position and size is undefined.
 */
RTDECL(int) RTFileCopyByHandles(RTFILE FileSrc, RTFILE FileDst);

/** Flags for RTFileCopyEx().
 * @{ */
/** Do not use RTFILE_O_DENY_WRITE on the source file to allow for copying files opened for writing. */
#define RTFILECOPY_FLAGS_NO_SRC_DENY_WRITE  RT_BIT(0)
/** Do not use RTFILE_O_DENY_WRITE on the target file. */
#define RTFILECOPY_FLAGS_NO_DST_DENY_WRITE  RT_BIT(1)
/** Do not use RTFILE_O_DENY_WRITE on either of the two files. */
#define RTFILECOPY_FLAGS_NO_DENY_WRITE      ( RTFILECOPY_FLAGS_NO_SRC_DENY_WRITE | RTFILECOPY_FLAGS_NO_DST_DENY_WRITE )
/** */
#define RTFILECOPY_FLAGS_MASK               UINT32_C(0x00000003)
/** @} */

/**
 * Copies a file.
 *
 * @returns IPRT status code
 * @retval  VERR_ALREADY_EXISTS if the destination file exists.
 *
 * @param   pszSrc      The path to the source file.
 * @param   pszDst      The path to the destination file.
 *                      This file will be created.
 * @param   fFlags      Flags (RTFILECOPY_*).
 * @param   pfnProgress Pointer to callback function for reporting progress.
 * @param   pvUser      User argument to pass to pfnProgress along with the completion percentage.
 */
RTDECL(int) RTFileCopyEx(const char *pszSrc, const char *pszDst, uint32_t fFlags, PFNRTPROGRESS pfnProgress, void *pvUser);

/**
 * Copies a file given the handles to both files and
 * provide progress callbacks.
 *
 * @returns IPRT status code.
 *
 * @param   FileSrc     The source file. The file position is unaltered.
 * @param   FileDst     The destination file.
 *                      On successful returns the file position is at the end of the file.
 *                      On failures the file position and size is undefined.
 * @param   pfnProgress Pointer to callback function for reporting progress.
 * @param   pvUser      User argument to pass to pfnProgress along with the completion percentage.
 */
RTDECL(int) RTFileCopyByHandlesEx(RTFILE FileSrc, RTFILE FileDst, PFNRTPROGRESS pfnProgress, void *pvUser);

/**
 * Copies a part of a file to another one.
 *
 * @returns IPRT status code.
 * @retval  VERR_EOF if @a pcbCopied is NULL and the end-of-file is reached
 *          before @a cbToCopy bytes have been copied.
 *
 * @param   hFileSrc    Handle to the source file.  Must be readable.
 * @param   offSrc      The source file offset.
 * @param   hFileDst    Handle to the destination file.  Must be writable and
 *                      RTFILE_O_APPEND must be be in effect.
 * @param   offDst      The destination file offset.
 * @param   cbToCopy    How many bytes to copy.
 * @param   fFlags      Reserved for the future, must be zero.
 * @param   pcbCopied   Where to return the exact number of bytes copied.
 *                      Optional.
 *
 * @note    The file positions of @a hFileSrc and @a hFileDst are undefined
 *          upon return of this function.
 *
 * @sa      RTFileCopyPartEx.
 */
RTDECL(int) RTFileCopyPart(RTFILE hFileSrc, RTFOFF offSrc, RTFILE hFileDst, RTFOFF offDst, uint64_t cbToCopy,
                           uint32_t fFlags, uint64_t *pcbCopied);


/** Copy buffer state for RTFileCopyPartEx.
 * @note The fields are considered internal!
 */
typedef struct RTFILECOPYPARTBUFSTATE
{
    /** Magic value (RTFILECOPYPARTBUFSTATE_MAGIC).
     * @internal */
    uint32_t    uMagic;
    /** Allocation type (internal).
     * @internal */
    int32_t     iAllocType;
    /** Buffer pointer.
     * @internal */
    uint8_t    *pbBuf;
    /** Buffer size.
     * @internal */
    size_t      cbBuf;
    /** Reserved.
     * @internal */
    void       *papReserved[3];
} RTFILECOPYPARTBUFSTATE;
/** Pointer to copy buffer state for RTFileCopyPartEx(). */
typedef RTFILECOPYPARTBUFSTATE *PRTFILECOPYPARTBUFSTATE;
/** Magic value for the RTFileCopyPartEx() buffer state structure (Stephen John Fry). */
#define RTFILECOPYPARTBUFSTATE_MAGIC   UINT32_C(0x19570857)

/**
 * Prepares buffer state for one or more RTFileCopyPartEx() calls.
 *
 * Caller must call RTFileCopyPartCleanup() after the final RTFileCopyPartEx()
 * call.
 *
 * @returns IPRT status code.
 * @param   pBufState   The buffer state to prepare.
 * @param   cbToCopy    The number of bytes we typically to copy in one
 *                      RTFileCopyPartEx call.
 */
RTDECL(int) RTFileCopyPartPrep(PRTFILECOPYPARTBUFSTATE pBufState, uint64_t cbToCopy);

/**
 * Cleans up after RTFileCopyPartPrep() once the final RTFileCopyPartEx()
 * call has been made.
 *
 * @param   pBufState   The buffer state to clean up.
 */
RTDECL(void) RTFileCopyPartCleanup(PRTFILECOPYPARTBUFSTATE pBufState);

/**
 * Copies a part of a file to another one, extended version.
 *
 * @returns IPRT status code.
 * @retval  VERR_EOF if @a pcbCopied is NULL and the end-of-file is reached
 *          before @a cbToCopy bytes have been copied.
 *
 * @param   hFileSrc    Handle to the source file.  Must be readable.
 * @param   offSrc      The source file offset.
 * @param   hFileDst    Handle to the destination file.  Must be writable and
 *                      RTFILE_O_APPEND must be be in effect.
 * @param   offDst      The destination file offset.
 * @param   cbToCopy    How many bytes to copy.
 * @param   fFlags      Reserved for the future, must be zero.
 * @param   pBufState   Copy buffer state prepared by RTFileCopyPartPrep().
 * @param   pcbCopied   Where to return the exact number of bytes copied.
 *                      Optional.
 *
 * @note    The file positions of @a hFileSrc and @a hFileDst are undefined
 *          upon return of this function.
 *
 * @sa      RTFileCopyPart.
 */
RTDECL(int) RTFileCopyPartEx(RTFILE hFileSrc, RTFOFF offSrc, RTFILE hFileDst, RTFOFF offDst, uint64_t cbToCopy,
                             uint32_t fFlags, PRTFILECOPYPARTBUFSTATE pBufState, uint64_t *pcbCopied);

/**
 * Copy file attributes from @a hFileSrc to @a hFileDst.
 *
 * @returns IPRT status code.
 * @param   hFileSrc    Handle to the source file.
 * @param   hFileDst    Handle to the destination file.
 * @param   fFlags      Reserved, pass zero.
 */
RTDECL(int) RTFileCopyAttributes(RTFILE hFileSrc, RTFILE hFileDst, uint32_t fFlags);

/**
 * Compares two file given the paths to both files.
 *
 * @returns IPRT status code.
 * @retval  VINF_SUCCESS if equal.
 * @retval  VERR_NOT_EQUAL if not equal.
 *
 * @param   pszFile1    The path to the first file.
 * @param   pszFile2    The path to the second file.
 */
RTDECL(int) RTFileCompare(const char *pszFile1, const char *pszFile2);

/**
 * Compares two file given the handles to both files.
 *
 * @returns IPRT status code.
 * @retval  VINF_SUCCESS if equal.
 * @retval  VERR_NOT_EQUAL if not equal.
 *
 * @param   hFile1      The first file.  Undefined return position.
 * @param   hFile2      The second file.  Undefined return position.
 */
RTDECL(int) RTFileCompareByHandles(RTFILE hFile1, RTFILE hFile2);

/** Flags for RTFileCompareEx().
 * @{ */
/** Do not use RTFILE_O_DENY_WRITE on the first file. */
#define RTFILECOMP_FLAGS_NO_DENY_WRITE_FILE1  RT_BIT(0)
/** Do not use RTFILE_O_DENY_WRITE on the second file. */
#define RTFILECOMP_FLAGS_NO_DENY_WRITE_FILE2  RT_BIT(1)
/** Do not use RTFILE_O_DENY_WRITE on either of the two files. */
#define RTFILECOMP_FLAGS_NO_DENY_WRITE      ( RTFILECOMP_FLAGS_NO_DENY_WRITE_FILE1 | RTFILECOMP_FLAGS_NO_DENY_WRITE_FILE2 )
/** */
#define RTFILECOMP_FLAGS_MASK               UINT32_C(0x00000003)
/** @} */

/**
 * Compares two files, extended version with progress callback.
 *
 * @returns IPRT status code.
 * @retval  VINF_SUCCESS if equal.
 * @retval  VERR_NOT_EQUAL if not equal.
 *
 * @param   pszFile1    The path to the source file.
 * @param   pszFile2    The path to the destination file. This file will be
 *                      created.
 * @param   fFlags      Flags, any of the RTFILECOMP_FLAGS_ \#defines.
 * @param   pfnProgress Pointer to callback function for reporting progress.
 * @param   pvUser      User argument to pass to pfnProgress along with the completion percentage.
 */
RTDECL(int) RTFileCompareEx(const char *pszFile1, const char *pszFile2, uint32_t fFlags, PFNRTPROGRESS pfnProgress, void *pvUser);

/**
 * Compares two files given their handles, extended version with progress
 * callback.
 *
 * @returns IPRT status code.
 * @retval  VINF_SUCCESS if equal.
 * @retval  VERR_NOT_EQUAL if not equal.
 *
 * @param   hFile1      The first file.  Undefined return position.
 * @param   hFile2      The second file.  Undefined return position.
 *
 * @param   fFlags      Flags, any of the RTFILECOMP_FLAGS_ \#defines, flags
 *                      related to opening of the files will be ignored.
 * @param   pfnProgress Pointer to callback function for reporting progress.
 * @param   pvUser      User argument to pass to pfnProgress along with the completion percentage.
 */
RTDECL(int) RTFileCompareByHandlesEx(RTFILE hFile1, RTFILE hFile2, uint32_t fFlags, PFNRTPROGRESS pfnProgress, void *pvUser);

/**
 * Renames a file.
 *
 * Identical to RTPathRename except that it will ensure that the source is not a directory.
 *
 * @returns IPRT status code.
 * @returns VERR_ALREADY_EXISTS if the destination file exists.
 *
 * @param   pszSrc      The path to the source file.
 * @param   pszDst      The path to the destination file.
 *                      This file will be created.
 * @param   fRename     See RTPathRename.
 */
RTDECL(int) RTFileRename(const char *pszSrc, const char *pszDst, unsigned fRename);


/** @name RTFileMove flags (bit masks).
 * @{ */
/** Replace destination file if present. */
#define RTFILEMOVE_FLAGS_REPLACE      0x1
/** Don't allow symbolic links as part of the path.
 * @remarks this flag is currently not implemented and will be ignored. */
#define RTFILEMOVE_FLAGS_NO_SYMLINKS  0x2
/** @} */

/**
 * Converts file opening modes (used by fopen, for example) to IPRT
 * compatible flags, which then can be used with RTFileOpen* APIs.
 *
 * @note    Handling sharing modes is not supported yet, so RTFILE_O_DENY_NONE
 *          will always be used.
 *
 * @return  IPRT status code.
 * @param   pszMode                 Mode string to convert.
 * @param   pfMode                  Where to store the converted mode flags on
 *                                  success.
 */
RTDECL(int) RTFileModeToFlags(const char *pszMode, uint64_t *pfMode);

/**
 * Converts file opening modes along with a separate disposition command
 * to IPRT compatible flags, which then can be used with RTFileOpen* APIs.
 *
 * Access modes:
 *      - "r":  Opens a file for reading.
 *      - "r+": Opens a file for reading and writing.
 *      - "w":  Opens a file for writing.
 *      - "w+": Opens a file for writing and reading.
 *
 * Disposition modes:
 *      - "oe", "open": Opens an existing file or fail if it does not exist.
 *      - "oc", "open-create": Opens an existing file or create it if it does
 *        not exist.
 *      - "oa", "open-append": Opens an existing file and places the file
 *        pointer at the end of the file, if opened with write access. Create
 *        the file if it does not exist.
 *      - "ot", "open-truncate": Opens and truncate an existing file or fail if
 *        it does not exist.
 *      - "ce", "create": Creates a new file if it does not exist. Fail if
 *        exist.
 *      - "ca", "create-replace": Creates a new file, always. Overwrites an
 *        existing file.
 *
 * Sharing mode:
 *      - "nr":     Deny read.
 *      - "nw":     Deny write.
 *      - "nrw":    Deny both read and write.
 *      - "d":      Allow delete.
 *      - "", NULL: Deny none, except delete.
 *
 * @return  IPRT status code.
 * @param   pszAccess       Access mode string to convert.
 * @param   pszDisposition  Disposition mode string to convert.
 * @param   pszSharing      Sharing mode string to convert.
 * @param   pfMode          Where to store the converted mode flags on success.
 */
RTDECL(int) RTFileModeToFlagsEx(const char *pszAccess, const char *pszDisposition, const char *pszSharing, uint64_t *pfMode);

/**
 * Moves a file.
 *
 * RTFileMove differs from RTFileRename in that it works across volumes.
 *
 * @returns IPRT status code.
 * @returns VERR_ALREADY_EXISTS if the destination file exists.
 *
 * @param   pszSrc      The path to the source file.
 * @param   pszDst      The path to the destination file.
 *                      This file will be created.
 * @param   fMove       A combination of the RTFILEMOVE_* flags.
 */
RTDECL(int) RTFileMove(const char *pszSrc, const char *pszDst, unsigned fMove);


/**
 * Creates a new file with a unique name using the given template, returning a
 * handle to it.
 *
 * One or more trailing X'es in the template will be replaced by random alpha
 * numeric characters until a RTFileOpen with RTFILE_O_CREATE succeeds or we
 * run out of patience.
 * For instance:
 *          "/tmp/myprog-XXXXXX"
 *
 * As an alternative to trailing X'es, it is possible to put 3 or more X'es
 * somewhere inside the file name. In the following string only the last
 * bunch of X'es will be modified:
 *          "/tmp/myprog-XXX-XXX.tmp"
 *
 * @returns IPRT status code.
 * @param   phFile          Where to return the file handle on success.  Set to
 *                          NIL on failure.
 * @param   pszTemplate     The file name template on input. The actual file
 *                          name on success. Empty string on failure.
 * @param   fOpen           The RTFILE_O_XXX flags to open the file with.
 *                          RTFILE_O_CREATE is mandatory.
 * @see     RTFileCreateTemp
 */
RTDECL(int) RTFileCreateUnique(PRTFILE phFile, char *pszTemplate, uint64_t fOpen);

/**
 * Creates a new file with a unique name using the given template.
 *
 * One or more trailing X'es in the template will be replaced by random alpha
 * numeric characters until a RTFileOpen with RTFILE_O_CREATE succeeds or we
 * run out of patience.
 * For instance:
 *          "/tmp/myprog-XXXXXX"
 *
 * As an alternative to trailing X'es, it is possible to put 3 or more X'es
 * somewhere inside the file name. In the following string only the last
 * bunch of X'es will be modified:
 *          "/tmp/myprog-XXX-XXX.tmp"
 *
 * @returns iprt status code.
 * @param   pszTemplate     The file name template on input. The actual file
 *                          name on success. Empty string on failure.
 * @param   fMode           The mode to create the file with.  Use 0600 unless
 *                          you have reason not to.
 * @see     RTFileCreateUnique
 */
RTDECL(int) RTFileCreateTemp(char *pszTemplate, RTFMODE fMode);

/**
 * Secure version of @a RTFileCreateTemp with a fixed mode of 0600.
 *
 * This function behaves in the same way as @a RTFileCreateTemp with two
 * additional points.  Firstly the mode is fixed to 0600.  Secondly it will
 * fail if it is not possible to perform the operation securely.  Possible
 * reasons include that the file could be removed by another unprivileged
 * user before it is used (e.g. if is created in a non-sticky /tmp directory)
 * or that the path contains symbolic links which another unprivileged user
 * could manipulate; however the exact criteria will be specified on a
 * platform-by-platform basis as platform support is added.
 * @see RTPathIsSecure for the current list of criteria.
 *
 * @returns iprt status code.
 * @returns VERR_NOT_SUPPORTED if the interface can not be supported on the
 *                             current platform at this time.
 * @returns VERR_INSECURE      if the file could not be created securely.
 * @param   pszTemplate        The file name template on input. The actual
 *                             file name on success. Empty string on failure.
 * @see     RTFileCreateUnique
 */
RTDECL(int) RTFileCreateTempSecure(char *pszTemplate);

/**
 * Opens a new file with a unique name in the temp directory.
 *
 * Unlike the other temp file creation APIs, this does not allow you any control
 * over the name.  Nor do you have to figure out where the temporary directory
 * is.
 *
 * @returns iprt status code.
 * @param   phFile          Where to return the handle to the file.
 * @param   pszFilename     Where to return the name (+path) of the file .
 * @param   cbFilename      The size of the buffer @a pszFilename points to.
 * @param   fOpen           The RTFILE_O_XXX flags to open the file with.
 *
 * @remarks If actual control over the filename or location is required, we'll
 *          create an extended edition of this API.
 */
RTDECL(int) RTFileOpenTemp(PRTFILE phFile, char *pszFilename, size_t cbFilename, uint64_t fOpen);


/** @defgroup   grp_rt_fileio_locking   RT File locking API
 *
 * File locking general rules:
 *
 * Region to lock or unlock can be located beyond the end of file, this can be used for
 * growing files.
 * Read (or Shared) locks can be acquired held by an unlimited number of processes at the
 * same time, but a Write (or Exclusive) lock can only be acquired by one process, and
 * cannot coexist with a Shared lock. To acquire a Read lock, a process must wait until
 * there are no processes holding any Write locks. To acquire a Write lock, a process must
 * wait until there are no processes holding either kind of lock.
 * By default, RTFileLock and RTFileChangeLock calls returns error immediately if the lock
 * can't be acquired due to conflict with other locks, however they can be called in wait mode.
 *
 * Differences in implementation:
 *
 * Win32, OS/2: Locking is mandatory, since locks are enforced by the operating system.
 * I.e. when file region is locked in Read mode, any write in it will fail; in case of Write
 * lock - region can be read and writed only by lock's owner.
 *
 * Win32: File size change (RTFileSetSize) is not controlled by locking at all (!) in the
 * operation system. Also see comments to RTFileChangeLock API call.
 *
 * Linux/Posix: By default locks in Unixes are advisory. This means that cooperating processes
 * may use locks to coordinate access to a file between themselves, but programs are also free
 * to ignore locks and access the file in any way they choose to.
 *
 * Additional reading:
 *     - http://en.wikipedia.org/wiki/File_locking
 *     - http://unixhelp.ed.ac.uk/CGI/man-cgi?fcntl+2
 *     - http://msdn.microsoft.com/library/default.asp?url=/library/en-us/fileio/fs/lockfileex.asp
 *
 * @{
 */

/** @name Lock flags (bit masks).
 * @{ */
/** Read access, can be shared with others. */
#define RTFILE_LOCK_READ            0x00
/** Write access, one at a time. */
#define RTFILE_LOCK_WRITE           0x01
/** Don't wait for other locks to be released. */
#define RTFILE_LOCK_IMMEDIATELY     0x00
/** Wait till conflicting locks have been released. */
#define RTFILE_LOCK_WAIT            0x02
/** Valid flags mask */
#define RTFILE_LOCK_MASK            0x03
/** @} */


/**
 * Locks a region of file for read (shared) or write (exclusive) access.
 *
 * @returns iprt status code.
 * @returns VERR_FILE_LOCK_VIOLATION if lock can't be acquired.
 * @param   File        Handle to the file.
 * @param   fLock       Lock method and flags, see RTFILE_LOCK_* defines.
 * @param   offLock     Offset of lock start.
 * @param   cbLock      Length of region to lock, may overlap the end of file.
 */
RTDECL(int)  RTFileLock(RTFILE File, unsigned fLock, int64_t offLock, uint64_t cbLock);

/**
 * Changes a lock type from read to write or from write to read.
 *
 * The region to type change must correspond exactly to an existing locked region.
 * If change can't be done due to locking conflict and non-blocking mode is used, error is
 * returned and lock keeps its state (see next warning).
 *
 * WARNING: win32 implementation of this call is not atomic, it transforms to a pair of
 * calls RTFileUnlock and RTFileLock. Potentially the previously acquired lock can be
 * lost, i.e. function is called in non-blocking mode, previous lock is freed, new lock can't
 * be acquired, and old lock (previous state) can't be acquired back too. This situation
 * may occurs _only_ if the other process is acquiring a _write_ lock in blocking mode or
 * in race condition with the current call.
 * In this very bad case special error code VERR_FILE_LOCK_LOST will be returned.
 *
 * @returns iprt status code.
 * @returns VERR_FILE_NOT_LOCKED if region was not locked.
 * @returns VERR_FILE_LOCK_VIOLATION if lock type can't be changed, lock remains its type.
 * @returns VERR_FILE_LOCK_LOST if lock was lost, we haven't this lock anymore :(
 * @param   File        Handle to the file.
 * @param   fLock       Lock method and flags, see RTFILE_LOCK_* defines.
 * @param   offLock     Offset of lock start.
 * @param   cbLock      Length of region to lock, may overlap the end of file.
 */
RTDECL(int)  RTFileChangeLock(RTFILE File, unsigned fLock, int64_t offLock, uint64_t cbLock);

/**
 * Unlocks previously locked region of file.
 * The region to unlock must correspond exactly to an existing locked region.
 *
 * @returns iprt status code.
 * @returns VERR_FILE_NOT_LOCKED if region was not locked.
 * @param   File        Handle to the file.
 * @param   offLock     Offset of lock start.
 * @param   cbLock      Length of region to unlock, may overlap the end of file.
 */
RTDECL(int)  RTFileUnlock(RTFILE File, int64_t offLock, uint64_t cbLock);

/** @} */


/**
 * Query information about an open file.
 *
 * @returns iprt status code.
 *
 * @param   File                    Handle to the file.
 * @param   pObjInfo                Object information structure to be filled on successful return.
 * @param   enmAdditionalAttribs    Which set of additional attributes to request.
 *                                  Use RTFSOBJATTRADD_NOTHING if this doesn't matter.
 */
RTDECL(int) RTFileQueryInfo(RTFILE File, PRTFSOBJINFO pObjInfo, RTFSOBJATTRADD enmAdditionalAttribs);

/**
 * Changes one or more of the timestamps associated of file system object.
 *
 * @returns iprt status code.
 * @retval  VERR_NOT_SUPPORTED is returned if the operation isn't supported by
 *          the OS.
 *
 * @param   File                Handle to the file.
 * @param   pAccessTime         Pointer to the new access time. NULL if not to be changed.
 * @param   pModificationTime   Pointer to the new modifcation time. NULL if not to be changed.
 * @param   pChangeTime         Pointer to the new change time. NULL if not to be changed.
 * @param   pBirthTime          Pointer to the new time of birth. NULL if not to be changed.
 *
 * @remark  The file system might not implement all these time attributes,
 *          the API will ignore the ones which aren't supported.
 *
 * @remark  The file system might not implement the time resolution
 *          employed by this interface, the time will be chopped to fit.
 *
 * @remark  The file system may update the change time even if it's
 *          not specified.
 *
 * @remark  POSIX can only set Access & Modification and will always set both.
 */
RTDECL(int) RTFileSetTimes(RTFILE File, PCRTTIMESPEC pAccessTime, PCRTTIMESPEC pModificationTime,
                           PCRTTIMESPEC pChangeTime, PCRTTIMESPEC pBirthTime);

/**
 * Gets one or more of the timestamps associated of file system object.
 *
 * @returns iprt status code.
 * @param   File                Handle to the file.
 * @param   pAccessTime         Where to store the access time. NULL is ok.
 * @param   pModificationTime   Where to store the modifcation time. NULL is ok.
 * @param   pChangeTime         Where to store the change time. NULL is ok.
 * @param   pBirthTime          Where to store the time of birth. NULL is ok.
 *
 * @remark  This is wrapper around RTFileQueryInfo() and exists to complement RTFileSetTimes().
 */
RTDECL(int) RTFileGetTimes(RTFILE File, PRTTIMESPEC pAccessTime, PRTTIMESPEC pModificationTime,
                           PRTTIMESPEC pChangeTime, PRTTIMESPEC pBirthTime);

/**
 * Changes the mode flags of an open file.
 *
 * The API requires at least one of the mode flag sets (Unix/Dos) to
 * be set. The type is ignored.
 *
 * @returns iprt status code.
 * @param   File        Handle to the file.
 * @param   fMode       The new file mode, see @ref grp_rt_fs for details.
 */
RTDECL(int) RTFileSetMode(RTFILE File, RTFMODE fMode);

/**
 * Gets the mode flags of an open file.
 *
 * @returns iprt status code.
 * @param   File        Handle to the file.
 * @param   pfMode      Where to store the file mode, see @ref grp_rt_fs for details.
 *
 * @remark  This is wrapper around RTFileQueryInfo()
 *          and exists to complement RTFileSetMode().
 */
RTDECL(int) RTFileGetMode(RTFILE File, uint32_t *pfMode);

/**
 * Changes the owner and/or group of an open file.
 *
 * @returns iprt status code.
 * @param   File        Handle to the file.
 * @param   uid         The new file owner user id.  Pass NIL_RTUID to leave
 *                      this unchanged.
 * @param   gid         The new group id.  Pass NIL_RTGID to leave this
 *                      unchanged.
 */
RTDECL(int) RTFileSetOwner(RTFILE File, uint32_t uid, uint32_t gid);

/**
 * Gets the owner and/or group of an open file.
 *
 * @returns iprt status code.
 * @param   File        Handle to the file.
 * @param   pUid        Where to store the owner user id. NULL is ok.
 * @param   pGid        Where to store the group id. NULL is ok.
 *
 * @remark  This is wrapper around RTFileQueryInfo() and exists to complement RTFileGetOwner().
 */
RTDECL(int) RTFileGetOwner(RTFILE File, uint32_t *pUid, uint32_t *pGid);

/**
 * Executes an IOCTL on a file descriptor.
 *
 * This function is currently only available in L4 and posix environments.
 * Attemps at calling it from code shared with any other platforms will break things!
 *
 * The rational for defining this API is to simplify L4 porting of audio drivers,
 * and to remove some of the assumptions on RTFILE being a file descriptor on
 * platforms using the posix file implementation.
 *
 * @returns iprt status code.
 * @param   File        Handle to the file.
 * @param   ulRequest   IOCTL request to carry out.
 * @param   pvData      IOCTL data.
 * @param   cbData      Size of the IOCTL data.
 * @param   piRet       Return value of the IOCTL request.
 */
RTDECL(int) RTFileIoCtl(RTFILE File, unsigned long ulRequest, void *pvData, unsigned cbData, int *piRet);

/**
 * Query the sizes of a filesystem.
 *
 * @returns iprt status code.
 * @retval  VERR_NOT_SUPPORTED is returned if the operation isn't supported by
 *          the OS.
 *
 * @param   hFile       The file handle.
 * @param   pcbTotal    Where to store the total filesystem space. (Optional)
 * @param   pcbFree     Where to store the remaining free space in the filesystem. (Optional)
 * @param   pcbBlock    Where to store the block size. (Optional)
 * @param   pcbSector   Where to store the sector size. (Optional)
 *
 * @sa      RTFsQuerySizes
 */
RTDECL(int) RTFileQueryFsSizes(RTFILE hFile, PRTFOFF pcbTotal, RTFOFF *pcbFree,
                               uint32_t *pcbBlock, uint32_t *pcbSector);

/**
 * Reads the file into memory.
 *
 * The caller must free the memory using RTFileReadAllFree().
 *
 * @returns IPRT status code.
 * @param   pszFilename     The name of the file.
 * @param   ppvFile         Where to store the pointer to the memory on successful return.
 * @param   pcbFile         Where to store the size of the returned memory.
 *
 * @remarks Note that this function may be implemented using memory mapping, which means
 *          that the file may remain open until RTFileReadAllFree() is called. It also
 *          means that the return memory may reflect the state of the file when it's
 *          accessed instead of when this call was done. So, in short, don't use this
 *          API for volatile files, then rather use the extended variant with a
 *          yet-to-be-defined flag.
 */
RTDECL(int) RTFileReadAll(const char *pszFilename, void **ppvFile, size_t *pcbFile);

/**
 * Reads the file into memory.
 *
 * The caller must free the memory using RTFileReadAllFree().
 *
 * @returns IPRT status code.
 * @param   pszFilename     The name of the file.
 * @param   off             The offset to start reading at.
 * @param   cbMax           The maximum number of bytes to read into memory. Specify RTFOFF_MAX
 *                          to read to the end of the file.
 * @param   fFlags          See RTFILE_RDALL_*.
 * @param   ppvFile         Where to store the pointer to the memory on successful return.
 * @param   pcbFile         Where to store the size of the returned memory.
 *
 * @remarks See the remarks for RTFileReadAll.
 */
RTDECL(int) RTFileReadAllEx(const char *pszFilename, RTFOFF off, RTFOFF cbMax, uint32_t fFlags, void **ppvFile, size_t *pcbFile);

/**
 * Reads the file into memory.
 *
 * The caller must free the memory using RTFileReadAllFree().
 *
 * @returns IPRT status code.
 * @param   File            The handle to the file.
 * @param   ppvFile         Where to store the pointer to the memory on successful return.
 * @param   pcbFile         Where to store the size of the returned memory.
 *
 * @remarks See the remarks for RTFileReadAll.
 */
RTDECL(int) RTFileReadAllByHandle(RTFILE File, void **ppvFile, size_t *pcbFile);

/**
 * Reads the file into memory.
 *
 * The caller must free the memory using RTFileReadAllFree().
 *
 * @returns IPRT status code.
 * @param   File            The handle to the file.
 * @param   off             The offset to start reading at.
 * @param   cbMax           The maximum number of bytes to read into memory. Specify RTFOFF_MAX
 *                          to read to the end of the file.
 * @param   fFlags          See RTFILE_RDALL_*.
 * @param   ppvFile         Where to store the pointer to the memory on successful return.
 * @param   pcbFile         Where to store the size of the returned memory.
 *
 * @remarks See the remarks for RTFileReadAll.
 */
RTDECL(int) RTFileReadAllByHandleEx(RTFILE File, RTFOFF off, RTFOFF cbMax, uint32_t fFlags, void **ppvFile, size_t *pcbFile);

/**
 * Frees the memory returned by one of the RTFileReadAll(), RTFileReadAllEx(),
 * RTFileReadAllByHandle() and RTFileReadAllByHandleEx() functions.
 *
 * @param   pvFile          Pointer to the memory.
 * @param   cbFile          The size of the memory.
 */
RTDECL(void) RTFileReadAllFree(void *pvFile, size_t cbFile);

/** @name RTFileReadAllEx and RTFileReadAllHandleEx flags
 * The open flags are ignored by RTFileReadAllHandleEx.
 * @{ */
#define RTFILE_RDALL_O_DENY_NONE            RTFILE_O_DENY_NONE
#define RTFILE_RDALL_O_DENY_READ            RTFILE_O_DENY_READ
#define RTFILE_RDALL_O_DENY_WRITE           RTFILE_O_DENY_WRITE
#define RTFILE_RDALL_O_DENY_READWRITE       RTFILE_O_DENY_READWRITE
#define RTFILE_RDALL_O_DENY_ALL             RTFILE_O_DENY_ALL
#define RTFILE_RDALL_O_DENY_NOT_DELETE      RTFILE_O_DENY_NOT_DELETE
#define RTFILE_RDALL_O_DENY_MASK            RTFILE_O_DENY_MASK
/** Fail with VERR_OUT_OF_RANGE if the file size exceeds the specified maximum
 * size.  The default behavior is to cap the size at cbMax. */
#define RTFILE_RDALL_F_FAIL_ON_MAX_SIZE     RT_BIT_32(30)
/** Add a trailing zero byte to facilitate reading text files. */
#define RTFILE_RDALL_F_TRAILING_ZERO_BYTE   RT_BIT_32(31)
/** Mask of valid flags. */
#define RTFILE_RDALL_VALID_MASK             (RTFILE_RDALL_O_DENY_MASK | UINT32_C(0xc0000000))
/** @} */

/**
 * Sets the current size of the file ensuring that all required blocks
 * are allocated on the underlying medium.
 *
 * @returns IPRT status code.
 * @retval  VERR_NOT_SUPPORTED if either this operation is not supported on the
 *          current host in an efficient manner or the given combination of
 *          flags is not supported.
 * @param   hFile           The handle to the file.
 * @param   cbSize          The new size of the file to allocate.
 * @param   fFlags          Combination of RTFILE_ALLOC_SIZE_F_*
 */
RTDECL(int) RTFileSetAllocationSize(RTFILE hFile, uint64_t cbSize, uint32_t fFlags);

/** @name RTFILE_ALLOC_SIZE_F_XXX - RTFileSetAllocationSize flags
 * @{ */
/** Default flags. */
#define RTFILE_ALLOC_SIZE_F_DEFAULT         0
/** Do not change the size of the file if the given size is bigger than the
 * current file size.
 *
 * Useful to preallocate blocks beyond the current size for appending data in an
 * efficient manner. Might not be supported on all hosts and will return
 * VERR_NOT_SUPPORTED in that case. */
#define RTFILE_ALLOC_SIZE_F_KEEP_SIZE       RT_BIT(0)
/** Mask of valid flags. */
#define RTFILE_ALLOC_SIZE_F_VALID           (RTFILE_ALLOC_SIZE_F_KEEP_SIZE)
/** @} */


#ifdef IN_RING3

/** @defgroup grp_rt_fileio_async   RT File Async I/O API
 *
 * File operations are usually blocking the calling thread until they completed
 * making it impossible to let the thread do anything else in-between. The RT
 * File async I/O API provides an easy and efficient way to access files
 * asynchronously using the native facilities provided by each operating system.
 *
 * @section sec_rt_asyncio_objects       Objects
 *
 * There are two objects used in this API.
 *
 * The first object is the request. A request contains every information needed
 * two complete the file operation successfully like the start offset and
 * pointer to the source or destination buffer. Requests are created with
 * RTFileAioReqCreate() and destroyed with RTFileAioReqDestroy(). Because
 * creating a request may require allocating various operating system dependent
 * resources and may be quite expensive it is possible to use a request more
 * than once to save CPU cycles. A request is constructed with either
 * RTFileAioReqPrepareRead() which will set up a request to read from the given
 * file or RTFileAioReqPrepareWrite() which will write to a given file.
 *
 * The second object is the context. A file is associated with a context and
 * requests for this file may complete only on the context the file was
 * associated with and not on the context given in RTFileAioCtxSubmit() (see
 * below for further information). RTFileAioCtxWait() is used to wait for
 * completion of requests which were associated with the context. While waiting
 * for requests the thread can not respond to global state changes. That's why
 * the API provides a way to let RTFileAioCtxWait() return immediately no matter
 * how many requests have finished through RTFileAioCtxWakeup(). The return code
 * is VERR_INTERRUPTED to let the thread know that he got interrupted.
 *
 * @section sec_rt_asyncio_request_states  Request states
 *
 * @b Created:
 * After a request was created with RTFileAioReqCreate() it is in the same state
 * like it just completed successfully. RTFileAioReqGetRC() will return
 * VINF_SUCCESS and a transfer size of 0. RTFileAioReqGetUser() will return
 * NULL. The request can be destroyed RTFileAioReqDestroy(). It is also allowed
 * to prepare a the request for a data transfer with the RTFileAioReqPrepare*
 * methods. Calling any other method like RTFileAioCtxSubmit() will return
 * VERR_FILE_AIO_NOT_PREPARED and RTFileAioReqCancel() returns
 * VERR_FILE_AIO_NOT_SUBMITTED.
 *
 * @b Prepared:
 * A request will enter this state if one of the RTFileAioReqPrepare* methods is
 * called. In this state you can still destroy and retrieve the user data
 * associated with the request but trying to cancel the request or getting the
 * result of the operation will return VERR_FILE_AIO_NOT_SUBMITTED.
 *
 * @b Submitted:
 * A prepared request can be submitted with RTFileAioCtxSubmit(). If the
 * operation succeeds it is not allowed to touch the request or free any
 * resources until it completed through RTFileAioCtxWait(). The only allowed
 * method is RTFileAioReqCancel() which tries to cancel the request. The request
 * will go into the completed state and RTFileAioReqGetRC() will return
 * VERR_FILE_AIO_CANCELED. If the request completes not matter if successfully
 * or with an error it will switch into the completed state. RTFileReqDestroy()
 * fails if the given request is in this state.
 *
 * @b Completed:
 * The request will be in this state after it completed and returned through
 * RTFileAioCtxWait(). RTFileAioReqGetRC() returns the final result code and the
 * number of bytes transferred. The request can be used for new data transfers.
 *
 * @section sec_rt_asyncio_threading       Threading
 *
 * The API is a thin wrapper around the specific host OS APIs and therefore
 * relies on the thread safety of the underlying API. The interesting functions
 * with regards to thread safety are RTFileAioCtxSubmit() and
 * RTFileAioCtxWait(). RTFileAioCtxWait() must not be called from different
 * threads at the same time with the same context handle. The same applies to
 * RTFileAioCtxSubmit(). However it is possible to submit new requests from a
 * different thread while waiting for completed requests on another thread with
 * RTFileAioCtxWait().
 *
 * @section sec_rt_asyncio_implementations  Differences in implementation
 *
 * Because the host APIs are quite different on every OS and every API has other
 * limitations there are some things to consider to make the code as portable as
 * possible.
 *
 * The first restriction at the moment is that every buffer has to be aligned to
 * a 512 byte boundary. This limitation comes from the Linux io_* interface. To
 * use the interface the file must be opened with O_DIRECT. This flag disables
 * the kernel cache too which may degrade performance but is unfortunately the
 * only way to make asynchronous I/O work till today (if O_DIRECT is omitted
 * io_submit will revert to sychronous behavior and will return when the
 * requests finished and when they are queued). It is mostly used by DBMS which
 * do theire own caching. Furthermore there is no filesystem independent way to
 * discover the restrictions at least for the 2.4 kernel series. Since 2.6 the
 * 512 byte boundary seems to be used by all file systems. So Linus comment
 * about this flag is comprehensible but Linux lacks an alternative at the
 * moment.
 *
 * The next limitation applies only to Windows. Requests are not associated with
 * the I/O context they are associated with but with the file the request is
 * for. The file needs to be associated with exactly one I/O completion port and
 * requests for this file will only arrive at that context after they completed
 * and not on the context the request was submitted. To associate a file with a
 * specific context RTFileAioCtxAssociateWithFile() is used. It is only
 * implemented on Windows and does nothing on the other platforms. If the file
 * needs to be associated with different context for some reason the file must
 * be closed first. After it was opened again the new context can be associated
 * with the other context. This can't be done by the API because there is no way
 * to retrieve the flags the file was opened with.
 *
 * @{
 */

/**
 * Global limits for the AIO API.
 */
typedef struct RTFILEAIOLIMITS
{
    /** Global number of simultaneous outstanding requests allowed.
     *  RTFILEAIO_UNLIMITED_REQS means no limit. */
    uint32_t cReqsOutstandingMax;
    /** The alignment data buffers need to have.
     * 0 means no alignment restrictions. */
    uint32_t cbBufferAlignment;
} RTFILEAIOLIMITS;
/** A pointer to a AIO limits structure. */
typedef RTFILEAIOLIMITS *PRTFILEAIOLIMITS;

/**
 * Returns the global limits for the AIO API.
 *
 * @returns IPRT status code.
 * @retval  VERR_NOT_SUPPORTED if the host does not support the async I/O API.
 *
 * @param   pAioLimits      Where to store the global limit information.
 */
RTDECL(int) RTFileAioGetLimits(PRTFILEAIOLIMITS pAioLimits);

/**
 * Creates an async I/O request handle.
 *
 * @returns IPRT status code.
 * @param   phReq           Where to store the request handle.
 */
RTDECL(int) RTFileAioReqCreate(PRTFILEAIOREQ phReq);

/**
 * Destroys an async I/O request handle.
 *
 * @returns IPRT status code.
 * @retval  VERR_FILE_AIO_IN_PROGRESS if the request is still in progress.
 *
 * @param   hReq            The request handle.
 */
RTDECL(int) RTFileAioReqDestroy(RTFILEAIOREQ hReq);

/**
 * Prepares an async read request.
 *
 * @returns IPRT status code.
 * @retval  VERR_FILE_AIO_IN_PROGRESS if the request is still in progress.
 *
 * @param   hReq            The request handle.
 * @param   hFile           The file to read from.
 * @param   off             The offset to start reading at.
 * @param   pvBuf           Where to store the read bits.
 * @param   cbRead          Number of bytes to read.
 * @param   pvUser          Opaque user data associated with this request which
 *                          can be retrieved with RTFileAioReqGetUser().
 */
RTDECL(int) RTFileAioReqPrepareRead(RTFILEAIOREQ hReq, RTFILE hFile, RTFOFF off,
                                    void *pvBuf, size_t cbRead, void *pvUser);

/**
 * Prepares an async write request.
 *
 * @returns IPRT status code.
 * @retval  VERR_FILE_AIO_IN_PROGRESS if the request is still in progress.
 *
 * @param   hReq            The request handle.
 * @param   hFile           The file to write to.
 * @param   off             The offset to start writing at.
 * @param   pvBuf           The bits to write.
 * @param   cbWrite         Number of bytes to write.
 * @param   pvUser          Opaque user data associated with this request which
 *                          can be retrieved with RTFileAioReqGetUser().
 */
RTDECL(int) RTFileAioReqPrepareWrite(RTFILEAIOREQ hReq, RTFILE hFile, RTFOFF off,
                                     void const *pvBuf, size_t cbWrite, void *pvUser);

/**
 * Prepares an async flush of all cached data associated with a file handle.
 *
 * @returns IPRT status code.
 * @retval  VERR_FILE_AIO_IN_PROGRESS if the request is still in progress.
 *
 * @param   hReq            The request handle.
 * @param   hFile           The file to flush.
 * @param   pvUser          Opaque user data associated with this request which
 *                          can be retrieved with RTFileAioReqGetUser().
 *
 * @remarks May also flush other caches on some platforms.
 */
RTDECL(int) RTFileAioReqPrepareFlush(RTFILEAIOREQ hReq, RTFILE hFile, void *pvUser);

/**
 * Gets the opaque user data associated with the given request.
 *
 * @returns Opaque user data.
 * @retval  NULL if the request hasn't been prepared yet.
 *
 * @param   hReq            The request handle.
 */
RTDECL(void *) RTFileAioReqGetUser(RTFILEAIOREQ hReq);

/**
 * Cancels a pending request.
 *
 * @returns IPRT status code.
 * @retval  VINF_SUCCESS              If the request was canceled.
 * @retval  VERR_FILE_AIO_NOT_SUBMITTED If the request wasn't submitted yet.
 * @retval  VERR_FILE_AIO_IN_PROGRESS If the request could not be canceled because it is already processed.
 * @retval  VERR_FILE_AIO_COMPLETED   If the request could not be canceled because it already completed.
 *
 * @param   hReq            The request to cancel.
 */
RTDECL(int) RTFileAioReqCancel(RTFILEAIOREQ hReq);

/**
 * Gets the status of a completed request.
 *
 * @returns The IPRT status code of the given request.
 * @retval  VERR_FILE_AIO_NOT_SUBMITTED if the request wasn't submitted yet.
 * @retval  VERR_FILE_AIO_CANCELED if the request was canceled.
 * @retval  VERR_FILE_AIO_IN_PROGRESS if the request isn't yet completed.
 *
 * @param   hReq            The request handle.
 * @param   pcbTransferred  Where to store the number of bytes transferred.
 *                          Optional since it is not relevant for all kinds of
 *                          requests.
 */
RTDECL(int) RTFileAioReqGetRC(RTFILEAIOREQ hReq, size_t *pcbTransferred);



/**
 * Creates an async I/O context.
 *
 * @todo briefly explain what an async context is here or in the page
 *       above.
 *
 * @returns IPRT status code.
 * @param   phAioCtx        Where to store the async I/O context handle.
 * @param   cAioReqsMax     How many async I/O requests the context should be capable
 *                          to handle. Pass RTFILEAIO_UNLIMITED_REQS if the
 *                          context should support an unlimited number of
 *                          requests.
 * @param   fFlags          Combination of RTFILEAIOCTX_FLAGS_*.
 */
RTDECL(int) RTFileAioCtxCreate(PRTFILEAIOCTX phAioCtx, uint32_t cAioReqsMax,
                               uint32_t fFlags);

/** Unlimited number of requests.
 * Used with RTFileAioCtxCreate and RTFileAioCtxGetMaxReqCount. */
#define RTFILEAIO_UNLIMITED_REQS    UINT32_MAX

/** When set RTFileAioCtxWait() will always wait for completing requests,
 * even when there is none waiting currently, instead of returning
 * VERR_FILE_AIO_NO_REQUEST. */
#define RTFILEAIOCTX_FLAGS_WAIT_WITHOUT_PENDING_REQUESTS RT_BIT_32(0)
/** mask of valid flags. */
#define RTFILEAIOCTX_FLAGS_VALID_MASK (RTFILEAIOCTX_FLAGS_WAIT_WITHOUT_PENDING_REQUESTS)

/**
 * Destroys an async I/O context.
 *
 * @returns IPRT status code.
 * @param   hAioCtx         The async I/O context handle.
 */
RTDECL(int) RTFileAioCtxDestroy(RTFILEAIOCTX hAioCtx);

/**
 * Get the maximum number of requests one aio context can handle.
 *
 * @returns Maximum number of tasks the context can handle.
 *          RTFILEAIO_UNLIMITED_REQS if there is no limit.
 *
 * @param   hAioCtx         The async I/O context handle.
 *                          If NIL_RTAIOCONTEXT is passed the maximum value
 *                          which can be passed to RTFileAioCtxCreate()
 *                          is returned.
 */
RTDECL(uint32_t) RTFileAioCtxGetMaxReqCount(RTFILEAIOCTX hAioCtx);

/**
 * Associates a file with an async I/O context.
 * Requests for this file will arrive at the completion port
 * associated with the file.
 *
 * @returns IPRT status code.
 *
 * @param   hAioCtx        The async I/O context handle.
 * @param   hFile          The file handle.
 */
RTDECL(int) RTFileAioCtxAssociateWithFile(RTFILEAIOCTX hAioCtx, RTFILE hFile);

/**
 * Submits a set of requests to an async I/O context for processing.
 *
 * @returns IPRT status code.
 * @returns VERR_FILE_AIO_INSUFFICIENT_RESSOURCES if the maximum number of
 *          simultaneous outstanding requests would be exceeded.
 *
 * @param   hAioCtx         The async I/O context handle.
 * @param   pahReqs         Pointer to an array of request handles.
 * @param   cReqs           The number of entries in the array.
 *
 * @remarks It is possible that some requests could be submitted successfully
 *          even if the method returns an error code. In that case RTFileAioReqGetRC()
 *          can be used to determine the status of a request.
 *          If it returns VERR_FILE_AIO_IN_PROGRESS it was submitted successfully.
 *          Any other error code may indicate why the request failed.
 *          VERR_FILE_AIO_NOT_SUBMITTED indicates that a request wasn't submitted
 *          probably because the previous request encountered an error.
 *
 * @remarks @a cReqs uses the type size_t while it really is a uint32_t, this is
 *          to avoid annoying warnings when using RT_ELEMENTS and similar
 *          macros.
 */
RTDECL(int) RTFileAioCtxSubmit(RTFILEAIOCTX hAioCtx, PRTFILEAIOREQ pahReqs, size_t cReqs);

/**
 * Waits for request completion.
 *
 * Only one thread at a time may call this API on a context.
 *
 * @returns IPRT status code.
 * @retval  VERR_INVALID_POINTER     If pcReqs or/and pahReqs are invalid.
 * @retval  VERR_INVALID_HANDLE      If hAioCtx is invalid.
 * @retval  VERR_OUT_OF_RANGE        If cMinReqs is larger than cReqs.
 * @retval  VERR_INVALID_PARAMETER   If cReqs is 0.
 * @retval  VERR_TIMEOUT             If cMinReqs didn't complete before the
 *                                   timeout expired.
 * @retval  VERR_INTERRUPTED         If the completion context was interrupted
 *                                   by RTFileAioCtxWakeup().
 * @retval  VERR_FILE_AIO_NO_REQUEST If there are no pending request.
 *
 * @param   hAioCtx         The async I/O context handle to wait and get
 *                          completed requests from.
 * @param   cMinReqs        The minimum number of requests which have to
 *                          complete before this function returns.
 * @param   cMillies        The number of milliseconds to wait before returning
 *                          VERR_TIMEOUT.  Use RT_INDEFINITE_WAIT to wait
 *                          forever.
 * @param   pahReqs         Pointer to an array where the handles of the
 *                          completed requests will be stored on success.
 * @param   cReqs           The number of entries @a pahReqs can hold.
 * @param   pcReqs          Where to store the number of returned (complete)
 *                          requests. This will always be set.
 *
 * @remarks The wait will be resume if interrupted by a signal. An
 *          RTFileAioCtxWaitNoResume variant can be added later if it becomes
 *          necessary.
 *
 * @remarks @a cMinReqs and @a cReqs use the type size_t while they really are
 *          uint32_t's, this is to avoid annoying warnings when using
 *          RT_ELEMENTS and similar macros.
 */
RTDECL(int) RTFileAioCtxWait(RTFILEAIOCTX hAioCtx, size_t cMinReqs, RTMSINTERVAL cMillies,
                             PRTFILEAIOREQ pahReqs, size_t cReqs, uint32_t *pcReqs);

/**
 * Forces any RTFileAioCtxWait() call on another thread to return immediately.
 *
 * @returns IPRT status code.
 *
 * @param   hAioCtx         The handle of the async I/O context to wakeup.
 */
RTDECL(int) RTFileAioCtxWakeup(RTFILEAIOCTX hAioCtx);

/** @} */

#endif /* IN_RING3 */

/** @} */

RT_C_DECLS_END

#endif /* !IPRT_INCLUDED_file_h */


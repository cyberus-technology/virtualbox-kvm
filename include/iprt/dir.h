/** @file
 * IPRT - Directory Manipulation.
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

#ifndef IPRT_INCLUDED_dir_h
#define IPRT_INCLUDED_dir_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <iprt/cdefs.h>
#include <iprt/types.h>
#include <iprt/fs.h>
#include <iprt/symlink.h>


RT_C_DECLS_BEGIN

/** @defgroup grp_rt_dir    RTDir - Directory Manipulation
 * @ingroup grp_rt
 * @{
 */

/**
 * Check for the existence of a directory.
 *
 * All symbolic links will be attemped resolved.  If that is undesirable, please
 * use RTPathQueryInfo instead.
 *
 * @returns true if exist and is a directory.
 * @returns false if not exists or isn't a directory.
 * @param   pszPath     Path to the directory.
 */
RTDECL(bool) RTDirExists(const char *pszPath);

/** @name RTDirCreate  flags.
 * @{ */
/** Don't allow symbolic links as part of the path.
 * @remarks this flag is currently not implemented and will be ignored. */
#define RTDIRCREATE_FLAGS_NO_SYMLINKS                       RT_BIT(0)
/** Set the not-content-indexed flag (default).  Windows only atm. */
#define RTDIRCREATE_FLAGS_NOT_CONTENT_INDEXED_DONT_SET      RT_BIT(1)
/** Do not set the not-content-indexed flag.  Windows only atm. */
#define RTDIRCREATE_FLAGS_NOT_CONTENT_INDEXED_SET           UINT32_C(0)
/** Ignore errors setting the not-content-indexed flag.  Windows only atm. */
#define RTDIRCREATE_FLAGS_NOT_CONTENT_INDEXED_NOT_CRITICAL  RT_BIT(2)
/** Ignore umask when applying the mode. */
#define RTDIRCREATE_FLAGS_IGNORE_UMASK                      RT_BIT(3)
/** Valid mask. */
#define RTDIRCREATE_FLAGS_VALID_MASK                        UINT32_C(0x0000000f)
/** @} */

/**
 * Creates a directory.
 *
 * @returns iprt status code.
 * @param   pszPath     Path to the directory to create.
 * @param   fMode       The mode of the new directory.
 * @param   fCreate     Create flags, RTDIRCREATE_FLAGS_*.
 */
RTDECL(int) RTDirCreate(const char *pszPath, RTFMODE fMode, uint32_t fCreate);

/**
 * Creates a directory including all non-existing parent directories.
 *
 * @returns iprt status code.
 * @param   pszPath     Path to the directory to create.
 * @param   fMode       The mode of the new directories.
 */
RTDECL(int) RTDirCreateFullPath(const char *pszPath, RTFMODE fMode);

/**
 * Creates a directory including all non-existing parent directories.
 *
 * @returns iprt status code.
 * @param   pszPath     Path to the directory to create.
 * @param   fMode       The mode of the new directories.
 * @param   fFlags      Create flags, RTDIRCREATE_FLAGS_*.
 */
RTDECL(int) RTDirCreateFullPathEx(const char *pszPath, RTFMODE fMode, uint32_t fFlags);

/**
 * Creates a new directory with a unique name using the given template.
 *
 * One or more trailing X'es in the template will be replaced by random alpha
 * numeric characters until a RTDirCreate succeeds or we run out of patience.
 * For instance:
 *          "/tmp/myprog-XXXXXX"
 *
 * As an alternative to trailing X'es, it
 * is possible to put 3 or more X'es somewhere inside the directory name. In
 * the following string only the last bunch of X'es will be modified:
 *          "/tmp/myprog-XXX-XXX.tmp"
 *
 * @returns iprt status code.
 * @param   pszTemplate     The directory name template on input. The actual
 *                          directory name on success. Empty string on failure.
 * @param   fMode           The mode to create the directory with.  Use 0700
 *                          unless you have reason not to.
 */
RTDECL(int) RTDirCreateTemp(char *pszTemplate, RTFMODE fMode);

/**
 * Secure version of @a RTDirCreateTemp with a fixed mode of 0700.
 *
 * This function behaves in the same way as @a RTDirCreateTemp with two
 * additional points.  Firstly the mode is fixed to 0700.  Secondly it will
 * fail if it is not possible to perform the operation securely.  Possible
 * reasons include that the directory could be removed by another unprivileged
 * user before it is used (e.g. if is created in a non-sticky /tmp directory)
 * or that the path contains symbolic links which another unprivileged user
 * could manipulate; however the exact criteria will be specified on a
 * platform-by-platform basis as platform support is added.
 * @see RTPathIsSecure for the current list of criteria.
 * @returns iprt status code.
 * @returns VERR_NOT_SUPPORTED if the interface can not be supported on the
 *                             current platform at this time.
 * @returns VERR_INSECURE      if the directory could not be created securely.
 * @param   pszTemplate        The directory name template on input. The
 *                             actual directory name on success. Empty string
 *                             on failure.
 */
RTDECL(int) RTDirCreateTempSecure(char *pszTemplate);

/**
 * Creates a new directory with a unique name by appending a number.
 *
 * This API differs from RTDirCreateTemp & RTDirCreateTempSecure in that it
 * first tries to create the directory without any random bits, thus the best
 * case result will be prettier.  It also differs in that it does not take a
 * template, but is instead given a template description, and will only use
 * digits for the filling.
 *
 * For sake of convenience and debugging , the current implementation
 * starts at 0 and will increment sequentally for a while before switching to
 * random numbers.
 *
 * On success @a pszPath contains the path created.
 *
 * @returns iprt status code.
 * @param   pszPath     The path to the directory.  On input the base template
 *                      name.  On successful return, the unique directory we
 *                      created.
 * @param   cbSize      The size of the pszPath buffer.  Needs enough space for
 *                      holding the digits and the optional separator.
 * @param   fMode       The mode of the new directory.
 * @param   cchDigits   How many digits should the number have (zero padded).
 * @param   chSep       The separator used between the path and the number. Can
 *                      be zero. (optional)
 */
RTDECL(int) RTDirCreateUniqueNumbered(char *pszPath, size_t cbSize, RTFMODE fMode, size_t cchDigits, char chSep);

/**
 * Removes a directory if empty.
 *
 * @returns iprt status code.
 * @param   pszPath         Path to the directory to remove.
 */
RTDECL(int) RTDirRemove(const char *pszPath);

/**
 * Removes a directory tree recursively.
 *
 * @returns iprt status code.
 * @param   pszPath         Path to the directory to remove recursively.
 * @param   fFlags          Flags, see RTDIRRMREC_F_XXX.
 *
 * @remarks This will not work on a root directory.
 */
RTDECL(int) RTDirRemoveRecursive(const char *pszPath, uint32_t fFlags);

/** @name   RTDirRemoveRecursive flags.
 * @{ */
/** Delete the content of the directory and the directory itself. */
#define RTDIRRMREC_F_CONTENT_AND_DIR    UINT32_C(0)
/** Only delete the content of the directory, omit the directory it self. */
#define RTDIRRMREC_F_CONTENT_ONLY       RT_BIT_32(0)
/** Long path hack: Don't apply RTPathAbs to the path. */
#define RTDIRRMREC_F_NO_ABS_PATH        RT_BIT_32(1)
/** Mask of valid flags. */
#define RTDIRRMREC_F_VALID_MASK         UINT32_C(0x00000003)
/** @} */

/**
 * Flushes the specified directory.
 *
 * This API is not implemented on all systems.  On some systems it may be
 * unnecessary if you've already flushed the file.  If you really care for your
 * data and is entering dangerous territories, it doesn't hurt calling it after
 * flushing and closing the file.
 *
 * @returns IPRT status code.
 * @retval  VERR_NOT_IMPLEMENTED must be expected.
 * @retval  VERR_NOT_SUPPORTED must be expected.
 * @param   pszPath     Path to the directory.
 */
RTDECL(int) RTDirFlush(const char *pszPath);

/**
 * Flushes the parent directory of the specified file.
 *
 * This is just a wrapper around RTDirFlush.
 *
 * @returns IPRT status code, see RTDirFlush for details.
 * @param   pszChild    Path to the file which parent should be flushed.
 */
RTDECL(int) RTDirFlushParent(const char *pszChild);



/**
 * Filter option for RTDirOpenFiltered().
 */
typedef enum RTDIRFILTER
{
    /** The usual invalid 0 entry. */
    RTDIRFILTER_INVALID = 0,
    /** No filter should be applied (and none was specified). */
    RTDIRFILTER_NONE,
    /** The Windows NT filter.
     * The following wildcard chars: *, ?, <, > and "
     * The matching is done on the uppercased strings.  */
    RTDIRFILTER_WINNT,
    /** The UNIX filter.
     * The following wildcard chars: *, ?, [..]
     * The matching is done on exact case. */
    RTDIRFILTER_UNIX,
    /** The UNIX filter, uppercased matching.
     * Same as RTDIRFILTER_UNIX except that the strings are uppercased before comparing. */
    RTDIRFILTER_UNIX_UPCASED,

    /** The usual full 32-bit value. */
    RTDIRFILTER_32BIT_HACK = 0x7fffffff
} RTDIRFILTER;


/**
 * Directory entry type.
 *
 * This is the RTFS_TYPE_MASK stuff shifted down 12 bits and
 * identical to the BSD/LINUX ABI.  See RTFS_TYPE_DIRENTRYTYPE_SHIFT.
 */
typedef enum RTDIRENTRYTYPE
{
    /** Unknown type (DT_UNKNOWN). */
    RTDIRENTRYTYPE_UNKNOWN          = 0,
    /** Named pipe (fifo) (DT_FIFO). */
    RTDIRENTRYTYPE_FIFO             = 001,
    /** Character device (DT_CHR). */
    RTDIRENTRYTYPE_DEV_CHAR         = 002,
    /** Directory (DT_DIR). */
    RTDIRENTRYTYPE_DIRECTORY        = 004,
    /** Block device (DT_BLK). */
    RTDIRENTRYTYPE_DEV_BLOCK        = 006,
    /** Regular file (DT_REG). */
    RTDIRENTRYTYPE_FILE             = 010,
    /** Symbolic link (DT_LNK). */
    RTDIRENTRYTYPE_SYMLINK          = 012,
    /** Socket (DT_SOCK). */
    RTDIRENTRYTYPE_SOCKET           = 014,
    /** Whiteout (DT_WHT). */
    RTDIRENTRYTYPE_WHITEOUT         = 016
} RTDIRENTRYTYPE;


/**
 * Directory entry.
 *
 * This is inspired by the POSIX interfaces.
 */
#pragma pack(1)
typedef struct RTDIRENTRY
{
    /** The unique identifier (within the file system) of this file system object (d_ino).
     *
     * Together with INodeIdDevice, this field can be used as a OS wide unique id
     * when both their values are not 0.  This field is 0 if the information is not
     * available. */
    RTINODE         INodeId;
    /** The entry type. (d_type)
     *
     * @warning RTDIRENTRYTYPE_UNKNOWN is a common return value here since not all
     *          file systems (or Unixes) stores the type of a directory entry and
     *          instead expects the user to use stat() to get it.  So, when you see
     *          this you should use RTDirQueryUnknownType or RTDirQueryUnknownTypeEx
     *          to get the type, or if if you're lazy, use RTDirReadEx.
     */
    RTDIRENTRYTYPE  enmType;
    /** The length of the filename, excluding the terminating nul character. */
    uint16_t        cbName;
    /** The filename. (no path)
     * Using the pcbDirEntry parameter of RTDirRead makes this field variable in size. */
    char            szName[260];
} RTDIRENTRY;
#pragma pack()
/** Pointer to a directory entry. */
typedef RTDIRENTRY *PRTDIRENTRY;
/** Pointer to a const directory entry. */
typedef RTDIRENTRY const *PCRTDIRENTRY;


/**
 * Directory entry with extended information.
 *
 * This is inspired by the PC interfaces.
 */
#pragma pack(1)
typedef struct RTDIRENTRYEX
{
    /** Full information about the object. */
    RTFSOBJINFO     Info;
    /** The length of the short field (number of RTUTF16 entries (not chars)).
     * It is 16-bit for reasons of alignment. */
    uint16_t        cwcShortName;
    /** The short name for 8.3 compatibility.
     * Empty string if not available.
     * Since the length is a bit tricky for a UTF-8 encoded name, and since this
     * is practically speaking only a windows thing, it is encoded as UCS-2. */
    RTUTF16         wszShortName[14];
    /** The length of the filename. */
    uint16_t        cbName;
    /** The filename. (no path)
     * Using the pcbDirEntry parameter of RTDirReadEx makes this field variable in size. */
    char            szName[260];
} RTDIRENTRYEX;
#pragma pack()
/** Pointer to a directory entry. */
typedef RTDIRENTRYEX *PRTDIRENTRYEX;
/** Pointer to a const directory entry. */
typedef RTDIRENTRYEX const *PCRTDIRENTRYEX;


/**
 * Opens a directory.
 *
 * @returns iprt status code.
 * @param   phDir       Where to store the open directory handle.
 * @param   pszPath     Path to the directory to open.
 */
RTDECL(int) RTDirOpen(RTDIR *phDir, const char *pszPath);

/** @name RTDIR_F_XXX - RTDirOpenFiltered flags.
 * @{ */
/** Don't allow symbolic links as part of the path.
 * @remarks this flag is currently not implemented and will be ignored. */
#define RTDIR_F_NO_SYMLINKS     RT_BIT_32(0)
/** Deny relative opening of anything above this directory. */
#define RTDIR_F_DENY_ASCENT     RT_BIT_32(1)
/** Don't follow symbolic links in the final component. */
#define RTDIR_F_NO_FOLLOW       RT_BIT_32(2)
/** Long path hack: Don't apply RTPathAbs to the path. */
#define RTDIR_F_NO_ABS_PATH     RT_BIT_32(3)
/** Valid flag mask.   */
#define RTDIR_F_VALID_MASK      UINT32_C(0x0000000f)
/** @} */

/**
 * Opens a directory with flags and optional filtering.
 *
 * @returns IPRT status code.
 * @retval  VERR_IS_A_SYMLINK if RTDIR_F_NO_FOLLOW is set, @a enmFilter is
 *          RTDIRFILTER_NONE and @a pszPath points to a symbolic link and does
 *          not end with a slash.  Note that on Windows this does not apply to
 *          file symlinks, only directory symlinks, for the file variant
 *          VERR_NOT_A_DIRECTORY will be returned.
 *
 * @param   phDir       Where to store the open directory handle.
 * @param   pszPath     Path to the directory to search, this must include wildcards.
 * @param   enmFilter   The kind of filter to apply. Setting this to RTDIRFILTER_NONE makes
 *                      this function behave like RTDirOpen.
 * @param   fFlags      Open flags, RTDIR_F_XXX.
 *
 */
RTDECL(int) RTDirOpenFiltered(RTDIR *phDir, const char *pszPath, RTDIRFILTER enmFilter, uint32_t fFlags);

/**
 * Closes a directory.
 *
 * @returns iprt status code.
 * @param   hDir        Handle to open directory returned by RTDirOpen() or
 *                      RTDirOpenFiltered().
 */
RTDECL(int) RTDirClose(RTDIR hDir);

/**
 * Checks if the supplied directory handle is valid.
 *
 * @returns true if valid.
 * @returns false if invalid.
 * @param   hDir        The directory handle.
 */
RTDECL(bool) RTDirIsValid(RTDIR hDir);

/**
 * Reads the next entry in the directory.
 *
 * @returns VINF_SUCCESS and data in pDirEntry on success.
 * @returns VERR_NO_MORE_FILES when the end of the directory has been reached.
 * @returns VERR_BUFFER_OVERFLOW if the buffer is too small to contain the filename. If
 *          pcbDirEntry is specified it will be updated with the required buffer size.
 * @returns suitable iprt status code on other errors.
 *
 * @param   hDir        Handle to the open directory.
 * @param   pDirEntry   Where to store the information about the next
 *                      directory entry on success.
 * @param   pcbDirEntry Optional parameter used for variable buffer size.
 *
 *                      On input the variable pointed to contains the size of the pDirEntry
 *                      structure. This must be at least OFFSET(RTDIRENTRY, szName[2]) bytes.
 *
 *                      On successful output the field is updated to
 *                      OFFSET(RTDIRENTRY, szName[pDirEntry->cbName + 1]).
 *
 *                      When the data doesn't fit in the buffer and VERR_BUFFER_OVERFLOW is
 *                      returned, this field contains the required buffer size.
 *
 *                      The value is unchanged in all other cases.
 */
RTDECL(int) RTDirRead(RTDIR hDir, PRTDIRENTRY pDirEntry, size_t *pcbDirEntry);

/**
 * Reads the next entry in the directory returning extended information.
 *
 * @returns VINF_SUCCESS and data in pDirEntry on success.
 * @returns VERR_NO_MORE_FILES when the end of the directory has been reached.
 * @returns VERR_BUFFER_OVERFLOW if the buffer is too small to contain the filename. If
 *          pcbDirEntry is specified it will be updated with the required buffer size.
 * @returns suitable iprt status code on other errors.
 *
 * @param   hDir        Handle to the open directory.
 * @param   pDirEntry   Where to store the information about the next
 *                      directory entry on success.
 * @param   pcbDirEntry Optional parameter used for variable buffer size.
 *
 *                      On input the variable pointed to contains the size of the pDirEntry
 *                      structure. This must be at least OFFSET(RTDIRENTRYEX, szName[2]) bytes.
 *
 *                      On successful output the field is updated to
 *                      OFFSET(RTDIRENTRYEX, szName[pDirEntry->cbName + 1]).
 *
 *                      When the data doesn't fit in the buffer and VERR_BUFFER_OVERFLOW is
 *                      returned, this field contains the required buffer size.
 *
 *                      The value is unchanged in all other cases.
 * @param   enmAdditionalAttribs
 *                      Which set of additional attributes to request.
 *                      Use RTFSOBJATTRADD_NOTHING if this doesn't matter.
 * @param   fFlags      RTPATH_F_ON_LINK or RTPATH_F_FOLLOW_LINK.
 */
RTDECL(int) RTDirReadEx(RTDIR hDir, PRTDIRENTRYEX pDirEntry, size_t *pcbDirEntry, RTFSOBJATTRADD enmAdditionalAttribs, uint32_t fFlags);

/**
 * Wrapper around RTDirReadEx that does the directory entry buffer handling.
 *
 * Call RTDirReadExAFree to free the buffers allocated by this function.
 *
 * @returns IPRT status code, see RTDirReadEx() for details.
 *
 * @param   hDir        Handle to the open directory.
 * @param   ppDirEntry  Pointer to the directory entry pointer.  Initialize this
 *                      to NULL before the first call.
 * @param   pcbDirEntry Where the API caches the allocation size.  Set this to
 *                      zero before the first call.
 * @param   enmAddAttr  See RTDirReadEx.
 * @param   fFlags      See RTDirReadEx.
 */
RTDECL(int) RTDirReadExA(RTDIR hDir, PRTDIRENTRYEX *ppDirEntry, size_t *pcbDirEntry, RTFSOBJATTRADD enmAddAttr, uint32_t fFlags);

/**
 * Frees the buffer allocated by RTDirReadExA.
 *
 * @param   ppDirEntry  Pointer to the directory entry pointer.
 * @param   pcbDirEntry Where the API caches the allocation size.
 */
RTDECL(void) RTDirReadExAFree(PRTDIRENTRYEX *ppDirEntry, size_t *pcbDirEntry);

/**
 * Resolves RTDIRENTRYTYPE_UNKNOWN values returned by RTDirRead.
 *
 * @returns IPRT status code (see RTPathQueryInfo).
 * @param   pszComposedName The path to the directory entry. The caller must
 *                          compose this, it's NOT sufficient to pass
 *                          RTDIRENTRY::szName!
 * @param   fFollowSymlinks Whether to follow symbolic links or not.
 * @param   penmType        Pointer to the RTDIRENTRY::enmType member.  If this
 *                          is not RTDIRENTRYTYPE_UNKNOWN and, if
 *                          @a fFollowSymlinks is false, not
 *                          RTDIRENTRYTYPE_SYMLINK, the function will return
 *                          immediately without doing anything.  Otherwise it
 *                          will use RTPathQueryInfo to try figure out the
 *                          correct value.  On failure, this will be unchanged.
 */
RTDECL(int) RTDirQueryUnknownType(const char *pszComposedName, bool fFollowSymlinks, RTDIRENTRYTYPE *penmType);

/**
 * Resolves RTDIRENTRYTYPE_UNKNOWN values returned by RTDirRead, extended
 * version.
 *
 * @returns IPRT status code (see RTPathQueryInfo).
 * @param   pszComposedName The path to the directory entry. The caller must
 *                          compose this, it's NOT sufficient to pass
 *                          RTDIRENTRY::szName!
 * @param   fFollowSymlinks Whether to follow symbolic links or not.
 * @param   penmType        Pointer to the RTDIRENTRY::enmType member or
 *                          similar.  Will NOT be checked on input.
 * @param   pObjInfo        The object info buffer to use with RTPathQueryInfo.
 */
RTDECL(int) RTDirQueryUnknownTypeEx(const char *pszComposedName, bool fFollowSymlinks, RTDIRENTRYTYPE *penmType, PRTFSOBJINFO pObjInfo);

/**
 * Checks if the directory entry returned by RTDirRead is '.', '..' or similar.
 *
 * @returns true / false.
 * @param   pDirEntry       The directory entry to check.
 */
RTDECL(bool) RTDirEntryIsStdDotLink(PRTDIRENTRY pDirEntry);

/**
 * Checks if the directory entry returned by RTDirReadEx is '.', '..' or
 * similar.
 *
 * @returns true / false.
 * @param   pDirEntryEx     The extended directory entry to check.
 */
RTDECL(bool) RTDirEntryExIsStdDotLink(PCRTDIRENTRYEX pDirEntryEx);

/**
 * Rewind and restart the directory reading.
 *
 * @returns IRPT status code.
 * @param   hDir            The directory handle to rewind.
 */
RTDECL(int) RTDirRewind(RTDIR hDir);

/**
 * Renames a file.
 *
 * Identical to RTPathRename except that it will ensure that the source is a directory.
 *
 * @returns IPRT status code.
 * @returns VERR_ALREADY_EXISTS if the destination file exists.
 *
 * @param   pszSrc      The path to the source file.
 * @param   pszDst      The path to the destination file.
 *                      This file will be created.
 * @param   fRename     See RTPathRename.
 */
RTDECL(int) RTDirRename(const char *pszSrc, const char *pszDst, unsigned fRename);


/**
 * Query information about an open directory.
 *
 * @returns iprt status code.
 *
 * @param   hDir                    Handle to the open directory.
 * @param   pObjInfo                Object information structure to be filled on successful return.
 * @param   enmAdditionalAttribs    Which set of additional attributes to request.
 *                                  Use RTFSOBJATTRADD_NOTHING if this doesn't matter.
 */
RTR3DECL(int) RTDirQueryInfo(RTDIR hDir, PRTFSOBJINFO pObjInfo, RTFSOBJATTRADD enmAdditionalAttribs);


/**
 * Changes one or more of the timestamps associated of file system object.
 *
 * @returns iprt status code.
 * @returns VERR_NOT_SUPPORTED is returned if the operation isn't supported by the OS.
 *
 * @param   hDir                Handle to the open directory.
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
RTR3DECL(int) RTDirSetTimes(RTDIR hDir, PCRTTIMESPEC pAccessTime, PCRTTIMESPEC pModificationTime,
                            PCRTTIMESPEC pChangeTime, PCRTTIMESPEC pBirthTime);


/**
 * Changes the mode flags of an open directory.
 *
 * The API requires at least one of the mode flag sets (Unix/Dos) to
 * be set. The type is ignored.
 *
 * @returns iprt status code.
 * @param   hDir                Handle to the open directory.
 * @param   fMode               The new file mode, see @ref grp_rt_fs for details.
 */
RTDECL(int) RTDirSetMode(RTDIR hDir, RTFMODE fMode);


/** @defgroup grp_rt_dir_rel    Directory relative APIs
 *
 * This group of APIs allows working with paths that are relative to an open
 * directory, therebye eliminating some classic path related race conditions on
 * systems with native support for these kinds of operations.
 *
 * On NT (Windows) there is native support for addressing files, directories and
 * stuff _below_ the open directory.  It is not possible to go upwards
 * (hDir:../../grandparent), at least not with NTFS, forcing us to use the
 * directory path as a fallback and opening us to potential races.
 *
 * On most unix-like systems here is now native support for all of this.
 *
 * @{ */

/**
 * Open a file relative to @a hDir.
 *
 * @returns IPRT status code.
 * @param   hDir            The directory to open relative to.
 * @param   pszRelFilename  The relative path to the file.
 * @param   fOpen           Open flags, i.e a combination of the RTFILE_O_XXX
 *                          defines.  The ACCESS, ACTION and DENY flags are
 *                          mandatory!
 * @param   phFile          Where to store the handle to the opened file.
 *
 * @sa      RTFileOpen
 */
RTDECL(int)  RTDirRelFileOpen(RTDIR hDir, const char *pszRelFilename, uint64_t fOpen, PRTFILE phFile);



/**
 * Opens a directory relative to @a hDir.
 *
 * @returns IPRT status code.
 * @param   hDir            The directory to open relative to.
 * @param   pszDir          The relative path to the directory to open.
 * @param   phDir           Where to store the directory handle.
 *
 * @sa      RTDirOpen
 */
RTDECL(int) RTDirRelDirOpen(RTDIR hDir, const char *pszDir, RTDIR *phDir);

/**
 * Opens a directory relative to @a hDir, with flags and optional filtering.
 *
 * @returns IPRT status code.
 * @retval  VERR_IS_A_SYMLINK if RTDIR_F_NO_FOLLOW is set, @a enmFilter is
 *          RTDIRFILTER_NONE and @a pszPath points to a symbolic link and does
 *          not end with a slash.  Note that on Windows this does not apply to
 *          file symlinks, only directory symlinks, for the file variant
 *          VERR_NOT_A_DIRECTORY will be returned.
 *
 * @param   hDir            The directory to open relative to.
 * @param   pszDirAndFilter The relative path to the directory to search, this
 *                          must include wildcards.
 * @param   enmFilter       The kind of filter to apply. Setting this to
 *                          RTDIRFILTER_NONE makes this function behave like
 *                          RTDirOpen.
 * @param   fFlags          Open flags, RTDIR_F_XXX.
 * @param   phDir           Where to store the directory handle.
 *
 * @sa      RTDirOpenFiltered
 */
RTDECL(int) RTDirRelDirOpenFiltered(RTDIR hDir, const char *pszDirAndFilter, RTDIRFILTER enmFilter,
                                    uint32_t fFlags, RTDIR *phDir);

/**
 * Creates a directory relative to @a hDir.
 *
 * @returns IPRT status code.
 * @param   hDir            The directory @a pszRelPath is relative to.
 * @param   pszRelPath      The relative path to the directory to create.
 * @param   fMode           The mode of the new directory.
 * @param   fCreate         Create flags, RTDIRCREATE_FLAGS_XXX.
 * @param   phSubDir        Where to return the handle of the created directory.
 *                          Optional.
 *
 * @sa      RTDirCreate
 */
RTDECL(int) RTDirRelDirCreate(RTDIR hDir, const char *pszRelPath, RTFMODE fMode, uint32_t fCreate, RTDIR *phSubDir);

/**
 * Removes a directory relative to @a hDir if empty.
 *
 * @returns IPRT status code.
 * @param   hDir            The directory @a pszRelPath is relative to.
 * @param   pszRelPath      The relative path to the directory to remove.
 *
 * @sa      RTDirRemove
 */
RTDECL(int) RTDirRelDirRemove(RTDIR hDir, const char *pszRelPath);


/**
 * Query information about a file system object relative to @a hDir.
 *
 * @returns IPRT status code.
 * @retval  VINF_SUCCESS if the object exists, information returned.
 * @retval  VERR_PATH_NOT_FOUND if any but the last component in the specified
 *          path was not found or was not a directory.
 * @retval  VERR_FILE_NOT_FOUND if the object does not exist (but path to the
 *          parent directory exists).
 *
 * @param   hDir            The directory @a pszRelPath is relative to.
 * @param   pszRelPath      The relative path to the file system object.
 * @param   pObjInfo        Object information structure to be filled on successful
 *                          return.
 * @param   enmAddAttr      Which set of additional attributes to request.
 *                          Use RTFSOBJATTRADD_NOTHING if this doesn't matter.
 * @param   fFlags          RTPATH_F_ON_LINK or RTPATH_F_FOLLOW_LINK.
 *
 * @sa      RTPathQueryInfoEx
 */
RTDECL(int) RTDirRelPathQueryInfo(RTDIR hDir, const char *pszRelPath, PRTFSOBJINFO pObjInfo,
                                  RTFSOBJATTRADD enmAddAttr, uint32_t fFlags);

/**
 * Changes the mode flags of a file system object relative to @a hDir.
 *
 * The API requires at least one of the mode flag sets (Unix/Dos) to
 * be set. The type is ignored.
 *
 * @returns IPRT status code.
 * @param   hDir            The directory @a pszRelPath is relative to.
 * @param   pszRelPath      The relative path to the file system object.
 * @param   fMode           The new file mode, see @ref grp_rt_fs for details.
 * @param   fFlags          RTPATH_F_ON_LINK or RTPATH_F_FOLLOW_LINK.
 *
 * @sa      RTPathSetMode
 */
RTDECL(int) RTDirRelPathSetMode(RTDIR hDir, const char *pszRelPath, RTFMODE fMode, uint32_t fFlags);

/**
 * Changes one or more of the timestamps associated of file system object
 * relative to @a hDir.
 *
 * @returns IPRT status code.
 * @param   hDir                The directory @a pszRelPath is relative to.
 * @param   pszRelPath          The relative path to the file system object.
 * @param   pAccessTime         Pointer to the new access time.
 * @param   pModificationTime   Pointer to the new modification time.
 * @param   pChangeTime         Pointer to the new change time. NULL if not to be changed.
 * @param   pBirthTime          Pointer to the new time of birth. NULL if not to be changed.
 * @param   fFlags              RTPATH_F_ON_LINK or RTPATH_F_FOLLOW_LINK.
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
 *
 * @sa      RTPathSetTimesEx
 */
RTDECL(int) RTDirRelPathSetTimes(RTDIR hDir, const char *pszRelPath, PCRTTIMESPEC pAccessTime, PCRTTIMESPEC pModificationTime,
                                 PCRTTIMESPEC pChangeTime, PCRTTIMESPEC pBirthTime, uint32_t fFlags);

/**
 * Changes the owner and/or group of a file system object relative to @a hDir.
 *
 * @returns IPRT status code.
 * @param   hDir            The directory @a pszRelPath is relative to.
 * @param   pszRelPath      The relative path to the file system object.
 * @param   uid             The new file owner user id.  Pass NIL_RTUID to leave
 *                          this unchanged.
 * @param   gid             The new group id.  Pass NIL_RTGID to leave this
 *                          unchanged.
 * @param   fFlags          RTPATH_F_ON_LINK or RTPATH_F_FOLLOW_LINK.
 *
 * @sa      RTPathSetOwnerEx
 */
RTDECL(int) RTDirRelPathSetOwner(RTDIR hDir, const char *pszRelPath, uint32_t uid, uint32_t gid, uint32_t fFlags);

/**
 * Renames a directory relative path within a filesystem.
 *
 * This will rename symbolic links.  If RTPATHRENAME_FLAGS_REPLACE is used and
 * pszDst is a symbolic link, it will be replaced and not its target.
 *
 * @returns IPRT status code.
 * @param   hDirSrc         The directory the source path is relative to.
 * @param   pszSrc          The source path, relative to @a hDirSrc.
 * @param   hDirDst         The directory the destination path is relative to.
 * @param   pszDst          The destination path, relative to @a hDirDst.
 * @param   fRename         Rename flags, RTPATHRENAME_FLAGS_XXX.
 *
 * @sa      RTPathRename
 */
RTDECL(int) RTDirRelPathRename(RTDIR hDirSrc, const char *pszSrc, RTDIR hDirDst, const char *pszDst, unsigned fRename);

/**
 * Removes the last component of the directory relative path.
 *
 * @returns IPRT status code.
 * @param   hDir            The directory @a pszRelPath is relative to.
 * @param   pszRelPath      The relative path to the file system object.
 * @param   fUnlink         Unlink flags, RTPATHUNLINK_FLAGS_XXX.
 *
 * @sa      RTPathUnlink
 */
RTDECL(int) RTDirRelPathUnlink(RTDIR hDir, const char *pszRelPath, uint32_t fUnlink);



/**
 * Creates a symbolic link (@a pszSymlink) relative to @a hDir targeting @a
 * pszTarget.
 *
 * @returns IPRT status code.
 * @param   hDir            The directory @a pszSymlink is relative to.
 * @param   pszSymlink      The relative path of the symbolic link.
 * @param   pszTarget       The path to the symbolic link target.  This is
 *                          relative to @a pszSymlink or an absolute path.
 * @param   enmType         The symbolic link type.  For Windows compatability
 *                          it is very important to set this correctly.  When
 *                          RTSYMLINKTYPE_UNKNOWN is used, the API will try
 *                          make a guess and may attempt query information
 *                          about @a pszTarget in the process.
 * @param   fCreate         Create flags, RTSYMLINKCREATE_FLAGS_XXX.
 *
 * @sa      RTSymlinkCreate
 */
RTDECL(int) RTDirRelSymlinkCreate(RTDIR hDir, const char *pszSymlink, const char *pszTarget,
                                  RTSYMLINKTYPE enmType, uint32_t fCreate);

/**
 * Read the symlink target relative to @a hDir.
 *
 * @returns IPRT status code.
 * @retval  VERR_NOT_SYMLINK if @a pszSymlink does not specify a symbolic link.
 * @retval  VERR_BUFFER_OVERFLOW if the link is larger than @a cbTarget.  The
 *          buffer will contain what all we managed to read, fully terminated
 *          if @a cbTarget > 0.
 *
 * @param   hDir            The directory @a pszSymlink is relative to.
 * @param   pszSymlink      The relative path to the symbolic link that should
 *                          be read.
 * @param   pszTarget       The target buffer.
 * @param   cbTarget        The size of the target buffer.
 * @param   fRead           Read flags, RTSYMLINKREAD_FLAGS_XXX.
 *
 * @sa      RTSymlinkRead
 */
RTDECL(int) RTDirRelSymlinkRead(RTDIR hDir, const char *pszSymlink, char *pszTarget, size_t cbTarget, uint32_t fRead);

/** @} */


/** @} */

RT_C_DECLS_END

#endif /* !IPRT_INCLUDED_dir_h */


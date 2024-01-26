/** @file
 * IPRT - Symbolic Link Manipulation.
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

#ifndef IPRT_INCLUDED_symlink_h
#define IPRT_INCLUDED_symlink_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <iprt/cdefs.h>
#include <iprt/types.h>



RT_C_DECLS_BEGIN

/** @defgroup grp_rt_symlink    RTSymlink - Symbolic Link Manipulation
 * @ingroup grp_rt
 *
 * For querying and changing symlink info (mode, ownership, etc) please refer
 * to the @ref grp_rt_path "RTPath" API: RTPathQueryInfoEx, RTPathSetOwnerEx,
 * RTPathSetModeEx and RTPathSetTimesEx.
 *
 * @{
 */

/**
 * Checks if the specified path exists and is a symlink.
 *
 * @returns true if it's a symlink, false if it isn't.
 * @param   pszSymlink      The path to the symlink.
 *
 * @sa      RTDirExists, RTPathExists, RTSymlinkExists.
 */
RTDECL(bool) RTSymlinkExists(const char *pszSymlink);

/**
 * Checks if this is a dangling link or not.
 *
 * If the target of @a pszSymlink is a symbolic link, this may return false if
 * that or any subsequent links are dangling.
 *
 * @returns true if it's dangling, false if it isn't.
 * @param   pszSymlink      The path to the symlink.
 */
RTDECL(bool) RTSymlinkIsDangling(const char *pszSymlink);

/**
 * RTSymlinkCreate link type argument.
 */
typedef enum RTSYMLINKTYPE
{
    /** Invalid value. */
    RTSYMLINKTYPE_INVALID = 0,
    /** The link targets a directory. */
    RTSYMLINKTYPE_DIR,
    /** The link targets a file (or whatever else). */
    RTSYMLINKTYPE_FILE,
    /** It is not known what is being targeted.
     * @remarks The RTSymlinkCreate API may probe the target to try figure
     *          out what is being targeted. */
    RTSYMLINKTYPE_UNKNOWN,
    /** The end of the valid type values. */
    RTSYMLINKTYPE_END,
    /** Blow the type up to 32-bit. */
    RTSYMLINKTYPE_32BIT_HACK = 0x7fffffff
} RTSYMLINKTYPE;

/** @name RTSymlinkCreate flags.
 * @{ */
/** Don't allow symbolic links as part of the path.
 * @remarks this flag is currently not implemented and will be ignored. */
#define RTSYMLINKCREATE_FLAGS_NO_SYMLINKS  RT_BIT(0)
/** @} */

/**
 * Creates a symbolic link (@a pszSymlink) targeting @a pszTarget.
 *
 * @returns IPRT status code.
 *
 * @param   pszSymlink      The name of the symbolic link.
 * @param   pszTarget       The path to the symbolic link target.  This is
 *                          relative to @a pszSymlink or an absolute path.
 * @param   enmType         The symbolic link type.  For Windows compatability
 *                          it is very important to set this correctly.  When
 *                          RTSYMLINKTYPE_UNKNOWN is used, the API will try
 *                          make a guess and may attempt query information
 *                          about @a pszTarget in the process.
 * @param   fCreate         Create flags, RTSYMLINKCREATE_FLAGS_*.
 */
RTDECL(int) RTSymlinkCreate(const char *pszSymlink, const char *pszTarget,
                            RTSYMLINKTYPE enmType, uint32_t fCreate);

/** @name RTSymlinkDelete flags.
 * @{ */
/** Don't allow symbolic links as part of the path.
 * @remarks this flag is currently not implemented and will be ignored. */
#define RTSYMLINKDELETE_FLAGS_NO_SYMLINKS  RT_BIT(0)
/** @} */

/**
 * Deletes the specified symbolic link.
 *
 * This will try to refuse deleting non-symlinks, however there are usually
 * races in the implementation of this check so no guarantees can be are made.
 *
 * @returns IPRT status code.
 * @retval  VERR_NOT_SYMLINK if @a pszSymlink does not specify a symbolic link.
 *
 * @param   pszSymlink      The symbolic link that should be removed.
 * @param   fDelete         Delete flags, RTSYMLINKDELETE_FLAGS_*.
 */
RTDECL(int) RTSymlinkDelete(const char *pszSymlink, uint32_t fDelete);

/** @name RTSymlinkRead  flags.
 * @{ */
/** Don't allow symbolic links as part of the path.
 * @remarks this flag is currently not implemented and will be ignored. */
#define RTSYMLINKREAD_FLAGS_NO_SYMLINKS  RT_BIT(0)
/** @} */

/**
 * Read the symlink target.
 *
 * @returns IPRT status code.
 * @retval  VERR_NOT_SYMLINK if @a pszSymlink does not specify a symbolic link.
 * @retval  VERR_BUFFER_OVERFLOW if the link is larger than @a cbTarget.  The
 *          buffer will contain what all we managed to read, fully terminated
 *          if @a cbTarget > 0.
 *
 * @param   pszSymlink      The symbolic link that should be read.
 * @param   pszTarget       The target buffer.
 * @param   cbTarget        The size of the target buffer.
 * @param   fRead           Read flags, RTSYMLINKREAD_FLAGS_*.
 */
RTDECL(int) RTSymlinkRead(const char *pszSymlink, char *pszTarget, size_t cbTarget, uint32_t fRead);

/**
 * Read the symlink target into an API allocated buffer.
 *
 * This API eliminates the race involved in determining the right buffer size.
 *
 * @returns IPRT status code.
 * @retval  VERR_NOT_SYMLINK if @a pszSymlink does not specify a symbolic link.
 *
 * @param   pszSymlink      The symbolic link that should be read.
 * @param   ppszTarget      Where to return the target string.  Free the string
 *                          by calling RTStrFree.
 */
RTDECL(int) RTSymlinkReadA(const char *pszSymlink, char **ppszTarget);

/** @} */

RT_C_DECLS_END

#endif /* !IPRT_INCLUDED_symlink_h */


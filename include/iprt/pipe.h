/** @file
 * IPRT - Anonymous Pipes.
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

#ifndef IPRT_INCLUDED_pipe_h
#define IPRT_INCLUDED_pipe_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <iprt/cdefs.h>
#include <iprt/types.h>
#include <iprt/fs.h>

RT_C_DECLS_BEGIN

/** @defgroup grp_rt_pipe      RTPipe - Anonymous Pipes
 * @ingroup grp_rt
 *
 * @note    The current Windows implementation has some peculiarities,
 *          especially with respect to the write side where the it is possible
 *          to write one extra pipe buffer sized block of data when the pipe
 *          buffer is full.
 *
 * @{
 */

/**
 * Create an anonymous pipe.
 *
 * @returns IPRT status code.
 * @param   phPipeRead      Where to return the read end of the pipe.
 * @param   phPipeWrite     Where to return the write end of the pipe.
 * @param   fFlags          A combination of RTPIPE_C_XXX defines.
 */
RTDECL(int)  RTPipeCreate(PRTPIPE phPipeRead, PRTPIPE phPipeWrite, uint32_t fFlags);

/** @name RTPipeCreate flags.
 * @{ */
/** Mark the read end as inheritable. */
#define RTPIPE_C_INHERIT_READ       RT_BIT(0)
/** Mark the write end as inheritable. */
#define RTPIPE_C_INHERIT_WRITE      RT_BIT(1)
/** Mask of valid flags. */
#define RTPIPE_C_VALID_MASK         UINT32_C(0x00000003)
/** @} */

/**
 * Closes one end of a pipe created by RTPipeCreate.
 *
 * @returns IPRT status code.
 * @param   hPipe           The pipe end to close.
 */
RTDECL(int)  RTPipeClose(RTPIPE hPipe);

/**
 * Closes one end of a pipe created by RTPipeCreate, extended version.
 *
 * @returns IPRT status code.
 * @param   hPipe           The pipe end to close.
 * @param   fLeaveOpen      Wheter to leave the underlying native handle open
 *                          (for RTPipeClose() this is @c false).
 */
RTDECL(int)  RTPipeCloseEx(RTPIPE hPipe, bool fLeaveOpen);

/**
 * Creates an IPRT pipe handle from a native one.
 *
 * Do NOT use the native handle after passing it to this function, IPRT owns it
 * and might even have closed in some cases (in order to gain some query
 * information access on Windows).
 *
 * @returns IPRT status code.
 * @param   phPipe          Where to return the pipe handle.
 * @param   hNativePipe     The native pipe handle.
 * @param   fFlags          Pipe flags, RTPIPE_N_XXX.
 */
RTDECL(int)  RTPipeFromNative(PRTPIPE phPipe, RTHCINTPTR hNativePipe, uint32_t fFlags);

/** @name RTPipeFromNative flags.
 * @{ */
/** The read end. */
#define RTPIPE_N_READ               RT_BIT(0)
/** The write end. */
#define RTPIPE_N_WRITE              RT_BIT(1)
/** Make sure the pipe is inheritable if set and not inheritable when clear. */
#define RTPIPE_N_INHERIT            RT_BIT(2)
/** Mask of valid flags for . */
#define RTPIPE_N_VALID_MASK         UINT32_C(0x00000007)
/** RTPipeFromNative: Leave the native pipe handle open on close. */
#define RTPIPE_N_LEAVE_OPEN         RT_BIT(3)
/** Mask of valid flags for RTPipeFromNative(). */
#define RTPIPE_N_VALID_MASK_FN      UINT32_C(0x0000000f)
/** @} */

/**
 * Gets the native handle for an IPRT pipe handle.
 *
 * This is mainly for passing a pipe to a child and then closing the parent
 * handle.  IPRT also uses it internally to implement RTProcCreatEx and
 * RTPollSetAdd on some platforms.  Do NOT expect sane API behavior if used
 * for any other purpose.
 *
 * @returns The native handle. -1 on failure.
 * @param   hPipe           The IPRT pipe handle.
 */
RTDECL(RTHCINTPTR) RTPipeToNative(RTPIPE hPipe);

/**
 * Get the creation inheritability of the pipe.
 *
 * @returns true if inherited by children (when pipe was created), false if not.
 * @param   hPipe           The IPRT pipe handle.
 */
RTDECL(int) RTPipeGetCreationInheritability(RTPIPE hPipe);

/**
 * Read bytes from a pipe, non-blocking.
 *
 * @returns IPRT status code.
 * @retval  VERR_WRONG_ORDER if racing a call to RTPipeReadBlocking.
 * @retval  VERR_BROKEN_PIPE if the remote party has disconnected and we've read
 *          all the buffered data.
 * @retval  VINF_TRY_AGAIN if no data was available.  @a *pcbRead will be set to
 *          0.
 * @retval  VERR_ACCESS_DENIED if it's a write pipe.
 *
 * @param   hPipe           The IPRT pipe handle to read from.
 * @param   pvBuf           Where to put the bytes we read.
 * @param   cbToRead        How much to read.  Must be greater than 0.
 * @param   pcbRead         Where to return the number of bytes that has been
 *                          read (mandatory).  This is 0 if there is no more
 *                          bytes to read.
 * @sa      RTPipeReadBlocking.
 */
RTDECL(int) RTPipeRead(RTPIPE hPipe, void *pvBuf, size_t cbToRead, size_t *pcbRead);

/**
 * Read bytes from a pipe, blocking.
 *
 * @returns IPRT status code.
 * @retval  VERR_WRONG_ORDER if racing a call to RTPipeRead.
 * @retval  VERR_BROKEN_PIPE if the remote party has disconnected and we've read
 *          all the buffered data.
 * @retval  VERR_ACCESS_DENIED if it's a write pipe.
 *
 * @param   hPipe           The IPRT pipe handle to read from.
 * @param   pvBuf           Where to put the bytes we read.
 * @param   cbToRead        How much to read.
 * @param   pcbRead         Where to return the number of bytes that has been
 *                          read. Optional.
 */
RTDECL(int) RTPipeReadBlocking(RTPIPE hPipe, void *pvBuf, size_t cbToRead, size_t *pcbRead);

/**
 * Write bytes to a pipe, non-blocking.
 *
 * @returns IPRT status code.
 * @retval  VERR_WRONG_ORDER if racing a call to RTPipeWriteBlocking.
 * @retval  VERR_BROKEN_PIPE if the remote party has disconnected.  Does not
 *          trigger when @a cbToWrite is 0.
 * @retval  VINF_TRY_AGAIN if no data was written.  @a *pcbWritten will be set
 *          to 0.
 * @retval  VERR_ACCESS_DENIED if it's a read pipe.
 *
 * @param   hPipe           The IPRT pipe handle to write to.
 * @param   pvBuf           What to write.
 * @param   cbToWrite       How much to write.
 * @param   pcbWritten      How many bytes we wrote, mandatory.  The return can
 *                          be 0.
 */
RTDECL(int) RTPipeWrite(RTPIPE hPipe, const void *pvBuf, size_t cbToWrite, size_t *pcbWritten);

/**
 * Write bytes to a pipe, blocking.
 *
 * @returns IPRT status code.
 * @retval  VERR_WRONG_ORDER if racing a call to RTPipeWrite.
 * @retval  VERR_BROKEN_PIPE if the remote party has disconnected.  Does not
 *          trigger when @a cbToWrite is 0.
 * @retval  VERR_ACCESS_DENIED if it's a read pipe.
 *
 * @param   hPipe           The IPRT pipe handle to write to.
 * @param   pvBuf           What to write.
 * @param   cbToWrite       How much to write.
 * @param   pcbWritten      How many bytes we wrote, optional.  If NULL then all
 *                          bytes will be written.
 */
RTDECL(int) RTPipeWriteBlocking(RTPIPE hPipe, const void *pvBuf, size_t cbToWrite, size_t *pcbWritten);

/**
 * Flushes the buffers for the specified pipe and making sure the other party
 * reads them.
 *
 * @returns IPRT status code.
 * @retval  VERR_NOT_SUPPORTED if not supported by the OS.
 * @retval  VERR_BROKEN_PIPE if the remote party has disconnected.
 * @retval  VERR_ACCESS_DENIED if it's a read pipe.
 *
 * @param   hPipe           The IPRT pipe handle to flush.
 */
RTDECL(int) RTPipeFlush(RTPIPE hPipe);

/**
 * Checks if the pipe is ready for reading or writing (depending on the pipe
 * end).
 *
 * @returns IPRT status code.
 * @retval  VERR_TIMEOUT if the timeout was reached before the pipe was ready
 *          for reading/writing.
 * @retval  VERR_NOT_SUPPORTED if not supported by the OS?
 *
 * @param   hPipe           The IPRT pipe handle to select on.
 * @param   cMillies        Number of milliseconds to wait.  Use
 *                          RT_INDEFINITE_WAIT to wait for ever.
 */
RTDECL(int) RTPipeSelectOne(RTPIPE hPipe, RTMSINTERVAL cMillies);

/**
 * Queries the number of bytes immediately available for reading.
 *
 * @returns IPRT status code.
 * @retval  VERR_NOT_SUPPORTED if not supported by the OS.  The caller shall
 *          handle this case.
 *
 * @param   hPipe           The IPRT read pipe handle.
 * @param   pcbReadable     Where to return the number of bytes that is ready
 *                          to be read.
 */
RTDECL(int) RTPipeQueryReadable(RTPIPE hPipe, size_t *pcbReadable);

/**
 * Query information about a pipe (mainly a VFS I/O stream formality).
 *
 * The only thing we guarentee to be returned is RTFSOBJINFO::Attr.fMode being
 * set to FIFO and will reflect the read/write end in the RTFS_DOS_READONLY,
 * RTFS_UNIX_IRUSR and RTFS_UNIX_IWUSR bits.
 *
 * Some implementations sometimes provide the pipe buffer size via
 * RTFSOBJINFO::cbAllocated.
 *
 * Some implementations sometimes provide the available read data or available
 * write space via RTFSOBJINFO::cbObject.
 *
 * Some implementations sometimes provide valid device and/or inode numbers.
 *
 * @returns iprt status code.
 *
 * @param   hPipe       The IPRT read pipe handle.
 * @param   pObjInfo    Object information structure to be filled on successful
 *                      return.
 * @param   enmAddAttr  Which set of additional attributes to request.  Use
 *                      RTFSOBJATTRADD_NOTHING if this doesn't matter.
 */
RTDECL(int) RTPipeQueryInfo(RTPIPE hPipe, PRTFSOBJINFO pObjInfo, RTFSOBJATTRADD enmAddAttr);

/** @} */

RT_C_DECLS_END

#endif /* !IPRT_INCLUDED_pipe_h */


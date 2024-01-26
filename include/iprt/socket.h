/** @file
 * IPRT - Network Sockets.
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

#ifndef IPRT_INCLUDED_socket_h
#define IPRT_INCLUDED_socket_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <iprt/cdefs.h>
#include <iprt/types.h>
#include <iprt/thread.h>
#include <iprt/net.h>
#include <iprt/sg.h>

#ifdef IN_RING0
# error "There are no RTSocket APIs available Ring-0 Host Context!"
#endif


RT_C_DECLS_BEGIN

/** @defgroup grp_rt_socket RTSocket - Network Sockets
 * @ingroup grp_rt
 * @{
 */

/** Use the system default timeout for the connet attempt. */
#define RT_SOCKETCONNECT_DEFAULT_WAIT (RT_INDEFINITE_WAIT - 1)

/**
 * Retains a reference to the socket handle.
 *
 * @returns New reference count, UINT32_MAX on invalid handle (asserted).
 *
 * @param   hSocket         The socket handle.
 */
RTDECL(uint32_t) RTSocketRetain(RTSOCKET hSocket);

/**
 * Release a reference to the socket handle.
 *
 * When the reference count reaches zero, the socket handle is shut down and
 * destroyed.  This will not be graceful shutdown, use the protocol specific
 * close method if this is desired.
 *
 * @returns New reference count, UINT32_MAX on invalid handle (asserted).
 *
 * @param   hSocket         The socket handle.  The NIL handle is quietly
 *                          ignored and 0 is returned.
 */
RTDECL(uint32_t) RTSocketRelease(RTSOCKET hSocket);

/**
 * Shuts down the socket, close it and then release one handle reference.
 *
 * This is slightly different from RTSocketRelease which will first do the
 * shutting down and closing when the reference count reaches zero.
 *
 * @returns IPRT status code.
 * @param   hSocket         The socket handle.  NIL is ignored.
 *
 * @remarks This will not perform a graceful shutdown of the socket, it will
 *          just destroy it.  Use the protocol specific close method if this is
 *          desired.
 */
RTDECL(int) RTSocketClose(RTSOCKET hSocket);

/**
 * Creates an IPRT socket handle from a native one.
 *
 * Do NOT use the native handle after passing it to this function, IPRT owns it
 * and might even have closed upon a successful return.
 *
 * @returns IPRT status code.
 * @param   phSocket        Where to store the IPRT socket handle.
 * @param   uNative         The native handle.
 */
RTDECL(int) RTSocketFromNative(PRTSOCKET phSocket, RTHCINTPTR uNative);

/**
 * Gets the native socket handle.
 *
 * @returns The native socket handle or RTHCUINTPTR_MAX if not invalid.
 * @param   hSocket             The socket handle.
 */
RTDECL(RTHCUINTPTR) RTSocketToNative(RTSOCKET hSocket);

/**
 * Helper that ensures the correct inheritability of a socket.
 *
 * We're currently ignoring failures.
 *
 * @returns IPRT status code
 * @param   hSocket         The socket handle.
 * @param   fInheritable    The desired inheritability state.
 */
RTDECL(int) RTSocketSetInheritance(RTSOCKET hSocket, bool fInheritable);

/**
 * Parse Internet style addresses, getting a generic IPRT network address.
 *
 * @returns IPRT status code
 * @param   pszAddress      Name or IP address.  NULL or empty string (no
 *                          spaces) is taken to mean INADDR_ANY, which is
 *                          meaningful when binding a server socket for
 *                          instance.
 * @param   uPort           Port number (host byte order).
 * @param   pAddr           Where to return the generic IPRT network address.
 */
RTDECL(int) RTSocketParseInetAddress(const char *pszAddress, unsigned uPort, PRTNETADDR pAddr);

/**
 * Try resolve a host name, returning the first matching address.
 *
 * @returns IPRT status code.
 * @param   pszHost         Name or IP address to look up.
 * @param   pszAddress      Where to return the stringified address.
 * @param   pcbAddress      Input: The size of the @a pszResult buffer.
 *                          Output: size of the returned string.  This is set on
 *                          VERR_BUFFER_OVERFLOW and most other error statuses.
 * @param   penmAddrType    Input: Which kind of address to return. Valid values
 *                          are:
 *                              - RTNETADDRTYPE_IPV4 -> lookup AF_INET.
 *                              - RTNETADDRTYPE_IPV6 -> lookup AF_INET6.
 *                              - RTNETADDRTYPE_INVALID/NULL -> lookup anything.
 *                          Output: The type of address that is being returned.
 *                          Not modified on failure.
 */
RTDECL(int) RTSocketQueryAddressStr(const char *pszHost, char *pszAddress, size_t *pcbAddress, PRTNETADDRTYPE penmAddrType);

/**
 * Receive data from a socket.
 *
 * @returns IPRT status code.
 * @param   hSocket         The socket handle.
 * @param   pvBuffer        Where to put the data we read.
 * @param   cbBuffer        Read buffer size.
 * @param   pcbRead         Number of bytes read. If NULL the entire buffer
 *                          will be filled upon successful return. If not NULL a
 *                          partial read can be done successfully.
 */
RTDECL(int) RTSocketRead(RTSOCKET hSocket, void *pvBuffer, size_t cbBuffer, size_t *pcbRead);

/**
 * Receive data from a socket, including sender address. Mainly useful
 * for datagram sockets.
 *
 * @returns IPRT status code.
 * @param   hSocket         The socket handle.
 * @param   pvBuffer        Where to put the data we read.
 * @param   cbBuffer        Read buffer size.
 * @param   pcbRead         Number of bytes read. Must be non-NULL.
 * @param   pSrcAddr        Pointer to sender address buffer. May be NULL.
 */
RTDECL(int) RTSocketReadFrom(RTSOCKET hSocket, void *pvBuffer, size_t cbBuffer, size_t *pcbRead, PRTNETADDR pSrcAddr);

/**
 * Send data to a socket.
 *
 * @returns IPRT status code.
 * @retval  VERR_INTERRUPTED if interrupted before anything was written.
 *
 * @param   hSocket         The socket handle.
 * @param   pvBuffer        Buffer to write data to socket.
 * @param   cbBuffer        How much to write.
 */
RTDECL(int) RTSocketWrite(RTSOCKET hSocket, const void *pvBuffer, size_t cbBuffer);

/**
 * Send data to a socket, including destination address. Mainly useful
 * for datagram sockets.
 *
 * @returns IPRT status code.
 * @retval  VERR_INTERRUPTED if interrupted before anything was written.
 *
 * @param   hSocket         The socket handle.
 * @param   pvBuffer        Buffer to write data to socket.
 * @param   cbBuffer        How much to write.
 * @param   pDstAddr        Pointer to destination address. May be NULL.
 */
RTDECL(int) RTSocketWriteTo(RTSOCKET hSocket, const void *pvBuffer, size_t cbBuffer, PCRTNETADDR pDstAddr);

/**
 * Checks if the socket is ready for reading (for I/O multiplexing).
 *
 * @returns IPRT status code.
 * @param   hSocket         The socket handle.
 * @param   cMillies        Number of milliseconds to wait for the socket.  Use
 *                          RT_INDEFINITE_WAIT to wait for ever.
 */
RTDECL(int) RTSocketSelectOne(RTSOCKET hSocket, RTMSINTERVAL cMillies);

/** @name Select events
 * @{ */
/** Readable without blocking. */
#define RTSOCKET_EVT_READ         RT_BIT_32(0)
/** Writable without blocking. */
#define RTSOCKET_EVT_WRITE        RT_BIT_32(1)
/** Error condition, hangup, exception or similar. */
#define RTSOCKET_EVT_ERROR        RT_BIT_32(2)
/** Mask of the valid bits. */
#define RTSOCKET_EVT_VALID_MASK   UINT32_C(0x00000007)
/** @} */

/**
 * Socket I/O multiplexing
 * Checks if the socket is ready for one of the given events.
 *
 * @returns iprt status code.
 * @param   hSocket         The Socket handle.
 * @param   fEvents         Event mask to wait for.
 * @param   pfEvents        Where to store the event mask on return.
 * @param   cMillies        Number of milliseconds to wait for the socket.  Use
 *                          RT_INDEFINITE_WAIT to wait for ever.
 */
RTR3DECL(int)  RTSocketSelectOneEx(RTSOCKET hSocket, uint32_t fEvents, uint32_t *pfEvents, RTMSINTERVAL cMillies);

/**
 * Shuts down one or both directions of communciation.
 *
 * @returns IPRT status code.
 * @param   hSocket         The socket handle.
 * @param   fRead           Whether to shutdown our read direction.
 * @param   fWrite          Whether to shutdown our write direction.
 */
RTDECL(int) RTSocketShutdown(RTSOCKET hSocket, bool fRead, bool fWrite);

/**
 * Gets the address of the local side.
 *
 * @returns IPRT status code.
 * @param   hSocket         The Socket handle.
 * @param   pAddr           Where to store the local address on success.
 */
RTDECL(int) RTSocketGetLocalAddress(RTSOCKET hSocket, PRTNETADDR pAddr);

/**
 * Gets the address of the other party.
 *
 * @returns IPRT status code.
 * @param   hSocket         The Socket handle.
 * @param   pAddr           Where to store the peer address on success.
 */
RTDECL(int) RTSocketGetPeerAddress(RTSOCKET hSocket, PRTNETADDR pAddr);

/**
 * Send data from a scatter/gather buffer to a socket.
 *
 * @returns IPRT status code.
 * @retval  VERR_INTERRUPTED if interrupted before anything was written.
 *
 * @param   hSocket         The socket handle.
 * @param   pSgBuf          Scatter/gather buffer to write data to socket.
 */
RTDECL(int) RTSocketSgWrite(RTSOCKET hSocket, PCRTSGBUF pSgBuf);

/**
 * Send data from multiple buffers to a socket.
 *
 * This is convenience wrapper around the RTSocketSgWrite and RTSgBufInit calls
 * for lazy coders.  The "L" in the function name is short for "list" just like
 * in the execl libc API.
 *
 * @returns IPRT status code.
 * @retval  VERR_INTERRUPTED if interrupted before anything was written.
 *
 * @param   hSocket         The socket handle.
 * @param   cSegs           The number of data segments in the following
 *                          ellipsis.
 * @param   ...             Pairs of buffer pointers (void const *) and buffer
 *                          sizes (size_t).  Make 101% sure the pointer is
 *                          really size_t.
 */
RTDECL(int) RTSocketSgWriteL(RTSOCKET hSocket, size_t cSegs, ...);

/**
 * Send data from multiple buffers to a socket.
 *
 * This is convenience wrapper around the RTSocketSgWrite and RTSgBufInit calls
 * for lazy coders.  The "L" in the function name is short for "list" just like
 * in the execl libc API.
 *
 * @returns IPRT status code.
 * @retval  VERR_INTERRUPTED if interrupted before anything was written.
 *
 * @param   hSocket         The socket handle.
 * @param   cSegs           The number of data segments in the following
 *                          argument list.
 * @param   va              Pairs of buffer pointers (void const *) and buffer
 *                          sizes (size_t).  Make 101% sure the pointer is
 *                          really size_t.
 */
RTDECL(int) RTSocketSgWriteLV(RTSOCKET hSocket, size_t cSegs, va_list va);

/**
 * Receive data from a socket.
 *
 * This version doesn't block if there is no data on the socket.
 *
 * @returns IPRT status code.
 *
 * @param   hSocket         The socket handle.
 * @param   pvBuffer        Where to put the data we read.
 * @param   cbBuffer        Read buffer size.
 * @param   pcbRead         Number of bytes read.
 */
RTDECL(int) RTSocketReadNB(RTSOCKET hSocket, void *pvBuffer, size_t cbBuffer, size_t *pcbRead);

/**
 * Send data to a socket.
 *
 * This version doesn't block if there is not enough room for the message.
 *
 * @returns IPRT status code.
 *
 * @param   hSocket         The socket handle.
 * @param   pvBuffer        Buffer to write data to socket.
 * @param   cbBuffer        How much to write.
 * @param   pcbWritten      Number of bytes written.
 */
RTDECL(int) RTSocketWriteNB(RTSOCKET hSocket, const void *pvBuffer, size_t cbBuffer, size_t *pcbWritten);

/**
 * Send data to a socket, including destination address. Mainly useful
 * for datagram sockets.
 *
 * This version doesn't block if there is not enough room for the message.
 *
 * @returns IPRT status code.
 * @retval  VERR_INTERRUPTED if interrupted before anything was written.
 *
 * @param   hSocket         The socket handle.
 * @param   pvBuffer        Buffer to write data to socket.
 * @param   cbBuffer        How much to write.
 * @param   pDstAddr        Pointer to destination address. May be NULL.
 */
RTDECL(int) RTSocketWriteToNB(RTSOCKET hSocket, const void *pvBuffer, size_t cbBuffer, PCRTNETADDR pDstAddr);

/**
 * Send data from a scatter/gather buffer to a socket.
 *
 * This version doesn't block if there is not enough room for the message.
 *
 * @returns iprt status code.
 *
 * @param   hSocket     The Socket handle.
 * @param   pSgBuf      Scatter/gather buffer to write data to socket.
 * @param   pcbWritten  Number of bytes written.
 */
RTR3DECL(int)  RTSocketSgWriteNB(RTSOCKET hSocket, PCRTSGBUF pSgBuf, size_t *pcbWritten);


/**
 * Send data from multiple buffers to a socket.
 *
 * This version doesn't block if there is not enough room for the message.
 * This is convenience wrapper around the RTSocketSgWrite and RTSgBufInit calls
 * for lazy coders.  The "L" in the function name is short for "list" just like
 * in the execl libc API.
 *
 * @returns IPRT status code.
 *
 * @param   hSocket         The socket handle.
 * @param   cSegs           The number of data segments in the following
 *                          ellipsis.
 * @param   pcbWritten      Number of bytes written.
 * @param   ...             Pairs of buffer pointers (void const *) and buffer
 *                          sizes (size_t).  Make 101% sure the pointer is
 *                          really size_t.
 */
RTR3DECL(int) RTSocketSgWriteLNB(RTSOCKET hSocket, size_t cSegs, size_t *pcbWritten, ...);

/**
 * Send data from multiple buffers to a socket.
 *
 * This version doesn't block if there is not enough room for the message.
 * This is convenience wrapper around the RTSocketSgWrite and RTSgBufInit calls
 * for lazy coders.  The "L" in the function name is short for "list" just like
 * in the execl libc API.
 *
 * @returns IPRT status code.
 *
 * @param   hSocket         The socket handle.
 * @param   cSegs           The number of data segments in the following
 *                          argument list.
 * @param   pcbWritten      Number of bytes written.
 * @param   va              Pairs of buffer pointers (void const *) and buffer
 *                          sizes (size_t). Make 101% sure the pointer is
 *                          really size_t.
 */
RTR3DECL(int) RTSocketSgWriteLVNB(RTSOCKET hSocket, size_t cSegs, size_t *pcbWritten, va_list va);

/** @} */
RT_C_DECLS_END

#endif /* !IPRT_INCLUDED_socket_h */


/** @file
 * IPRT - TCP/IP.
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

#ifndef IPRT_INCLUDED_tcp_h
#define IPRT_INCLUDED_tcp_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <iprt/cdefs.h>
#include <iprt/types.h>
#include <iprt/thread.h>
#include <iprt/net.h>
#include <iprt/sg.h>
#include <iprt/socket.h>

#ifdef IN_RING0
# error "There are no RTFile APIs available Ring-0 Host Context!"
#endif


RT_C_DECLS_BEGIN

/** @defgroup grp_rt_tcp    RTTcp - TCP/IP
 * @ingroup grp_rt
 * @{
 */


/**
 * Serve a TCP Server connection.
 *
 * @returns iprt status code.
 * @returns VERR_TCP_SERVER_STOP to terminate the server loop forcing
 *          the RTTcpCreateServer() call to return.
 * @param   hSocket     The socket which the client is connected to. The call
 *                      will close this socket.
 * @param   pvUser      User argument.
 */
typedef DECLCALLBACKTYPE(int, FNRTTCPSERVE,(RTSOCKET hSocket, void *pvUser));
/** Pointer to a RTTCPSERVE(). */
typedef FNRTTCPSERVE *PFNRTTCPSERVE;

/**
 * Create single connection at a time TCP Server in a separate thread.
 *
 * The thread will loop accepting connections and call pfnServe for
 * each of the incoming connections in turn. The pfnServe function can
 * return VERR_TCP_SERVER_STOP too terminate this loop. RTTcpServerDestroy()
 * should be used to terminate the server.
 *
 * @returns iprt status code.
 * @param   pszAddress      The address for creating a listening socket.
 *                          If NULL or empty string the server is bound to all interfaces.
 * @param   uPort           The port for creating a listening socket.
 * @param   enmType         The thread type.
 * @param   pszThrdName     The name of the worker thread.
 * @param   pfnServe        The function which will serve a new client connection.
 * @param   pvUser          User argument passed to pfnServe.
 * @param   ppServer        Where to store the serverhandle.
 */
RTR3DECL(int)  RTTcpServerCreate(const char *pszAddress, unsigned uPort, RTTHREADTYPE enmType, const char *pszThrdName,
                                 PFNRTTCPSERVE pfnServe, void *pvUser, PPRTTCPSERVER ppServer);

/**
 * Create single connection at a time TCP Server.
 * The caller must call RTTcpServerListen() to actually start the server.
 *
 * @returns iprt status code.
 * @param   pszAddress      The address for creating a listening socket.
 *                          If NULL the server is bound to all interfaces.
 * @param   uPort           The port for creating a listening socket.
 * @param   ppServer        Where to store the serverhandle.
 */
RTR3DECL(int) RTTcpServerCreateEx(const char *pszAddress, uint32_t uPort, PPRTTCPSERVER ppServer);

/**
 * Closes down and frees a TCP Server.
 * This will also terminate any open connections to the server.
 *
 * @returns iprt status code.
 * @param   pServer         Handle to the server.
 */
RTR3DECL(int) RTTcpServerDestroy(PRTTCPSERVER pServer);

/**
 * Listen for incoming connections.
 *
 * The function will loop accepting connections and call pfnServe for
 * each of the incoming connections in turn. The pfnServe function can
 * return VERR_TCP_SERVER_STOP too terminate this loop. A stopped server
 * can only be destroyed.
 *
 * @returns iprt status code.
 * @param   pServer         The server handle as returned from RTTcpServerCreateEx().
 * @param   pfnServe        The function which will serve a new client connection.
 * @param   pvUser          User argument passed to pfnServe.
 */
RTR3DECL(int) RTTcpServerListen(PRTTCPSERVER pServer, PFNRTTCPSERVE pfnServe, void *pvUser);

/**
 * Listen and accept one incoming connection.
 *
 * This is an alternative to RTTcpServerListen for the use the callbacks are not
 * possible.
 *
 * @returns IPRT status code.
 * @retval  VERR_TCP_SERVER_SHUTDOWN if shut down by RTTcpServerShutdown.
 * @retval  VERR_INTERRUPTED if the listening was interrupted.
 *
 * @param   pServer         The server handle as returned from RTTcpServerCreateEx().
 * @param   phClientSocket  Where to return the socket handle to the client
 *                          connection (on success only).  This must be closed
 *                          by calling RTTcpServerDisconnectClient2().
 */
RTR3DECL(int) RTTcpServerListen2(PRTTCPSERVER pServer, PRTSOCKET phClientSocket);

/**
 * Terminate the open connection to the server.
 *
 * @returns iprt status code.
 * @param   pServer         Handle to the server.
 */
RTR3DECL(int) RTTcpServerDisconnectClient(PRTTCPSERVER pServer);

/**
 * Terminates an open client connect when using RTTcpListen2
 *
 * @returns IPRT status code.
 * @param   hClientSocket   The client socket handle.  This will be invalid upon
 *                          return, whether successful or not.  NIL is quietly
 *                          ignored (VINF_SUCCESS).
 */
RTR3DECL(int) RTTcpServerDisconnectClient2(RTSOCKET hClientSocket);

/**
 * Shuts down the server, leaving client connections open.
 *
 * @returns IPRT status code.
 * @param   pServer         Handle to the server.
 */
RTR3DECL(int) RTTcpServerShutdown(PRTTCPSERVER pServer);

/**
 * Connect (as a client) to a TCP Server.
 *
 * @returns iprt status code.
 * @param   pszAddress      The address to connect to.
 * @param   uPort           The port to connect to.
 * @param   pSock           Where to store the handle to the established connection.
 */
RTR3DECL(int) RTTcpClientConnect(const char *pszAddress, uint32_t uPort, PRTSOCKET pSock);

/** Opaque pointer used by RTTcpClientConnectEx and RTTcpClientCancelConnect. */
typedef struct RTTCPCLIENTCONNECTCANCEL *PRTTCPCLIENTCONNECTCANCEL;

/**
 * Connect (as a client) to a TCP Server, extended version.
 *
 * @returns iprt status code.
 * @param   pszAddress      The address to connect to.
 * @param   uPort           The port to connect to.
 * @param   pSock           Where to store the handle to the established connection.
 * @param   cMillies        Number of milliseconds to wait for the connect attempt to complete.
 *                          Use RT_INDEFINITE_WAIT to wait for ever.
 *                          Use RT_SOCKETCONNECT_DEFAULT_WAIT to wait for the default time
 *                          configured on the running system.
 * @param   ppCancelCookie  Where to store information for canceling the
 *                          operation (from a different thread). Optional.
 *
 *                          The pointer _must_ be initialized to NULL before a
 *                          series of connection attempts begins, i.e. at a time
 *                          where there will be no RTTcpClientCancelConnect
 *                          calls racing access.  RTTcpClientCancelConnect will
 *                          set it to a special non-NULL value that causes the
 *                          current or/and next connect call to fail.
 *
 * @sa      RTTcpClientCancelConnect
 */
RTR3DECL(int) RTTcpClientConnectEx(const char *pszAddress, uint32_t uPort, PRTSOCKET pSock,
                                   RTMSINTERVAL cMillies, PRTTCPCLIENTCONNECTCANCEL volatile *ppCancelCookie);

/**
 * Cancels a RTTcpClientConnectEx call on a different thread.
 *
 * @returns iprt status code.
 * @param   ppCancelCookie  The address of the cookie pointer shared with the
 *                          connect call.
 */
RTR3DECL(int) RTTcpClientCancelConnect(PRTTCPCLIENTCONNECTCANCEL volatile *ppCancelCookie);

/**
 * Close a socket returned by RTTcpClientConnect().
 *
 * @returns iprt status code.
 * @param   hSocket     Socket descriptor.
 */
RTR3DECL(int) RTTcpClientClose(RTSOCKET hSocket);

/**
 * Close a socket returned by RTTcpClientConnect().
 *
 * @returns iprt status code.
 * @param   hSocket             The socket handle.
 * @param   fGracefulShutdown   If true, try do a graceful shutdown of the
 *                              outgoing pipe and draining any lingering input.
 *                              This is sometimes better for the server side.
 *                              If false, just close the connection without
 *                              further ado.
 */
RTR3DECL(int) RTTcpClientCloseEx(RTSOCKET hSocket, bool fGracefulShutdown);

/**
 * Creates connected pair of TCP sockets.
 *
 * @returns IPRT status code.
 * @param   phServer            Where to return the "server" side of the pair.
 * @param   phClient            Where to return the "client" side of the pair.
 * @param   fFlags              Reserved, must be zero.
 *
 * @note    There is no server or client side, but we gotta call it something.
 */
RTR3DECL(int) RTTcpCreatePair(PRTSOCKET phServer, PRTSOCKET phClient, uint32_t fFlags);

/**
 * Receive data from a socket.
 *
 * @returns iprt status code.
 * @param   hSocket     Socket descriptor.
 * @param   pvBuffer    Where to put the data we read.
 * @param   cbBuffer    Read buffer size.
 * @param   pcbRead     Number of bytes read.
 *                      If NULL the entire buffer will be filled upon successful return.
 *                      If not NULL a partial read can be done successfully.
 */
RTR3DECL(int)  RTTcpRead(RTSOCKET hSocket, void *pvBuffer, size_t cbBuffer, size_t *pcbRead);

/**
 * Send data to a socket.
 *
 * @returns iprt status code.
 * @retval  VERR_INTERRUPTED if interrupted before anything was written.
 *
 * @param   hSocket     Socket descriptor.
 * @param   pvBuffer    Buffer to write data to socket.
 * @param   cbBuffer    How much to write.
 */
RTR3DECL(int)  RTTcpWrite(RTSOCKET hSocket, const void *pvBuffer, size_t cbBuffer);

/**
 * Flush socket write buffers.
 *
 * @returns iprt status code.
 * @param   hSocket     Socket descriptor.
 */
RTR3DECL(int)  RTTcpFlush(RTSOCKET hSocket);

/**
 * Enables or disables delaying sends to coalesce packets.
 *
 * The TCP/IP stack usually uses the Nagle algorithm (RFC 896) to implement the
 * coalescing.
 *
 * @returns iprt status code.
 * @param   hSocket     Socket descriptor.
 * @param   fEnable     When set to true enables coalescing.
 */
RTR3DECL(int)  RTTcpSetSendCoalescing(RTSOCKET hSocket, bool fEnable);

/**
 * Sets send and receive buffer sizes.
 *
 * @returns iprt status code.
 * @param   hSocket     Socket descriptor.
 * @param   cbSize      Buffer size in bytes.
 */
RTR3DECL(int)  RTTcpSetBufferSize(RTSOCKET hSocket, uint32_t cbSize);

/**
 * Socket I/O multiplexing.
 * Checks if the socket is ready for reading.
 *
 * @returns iprt status code.
 * @param   hSocket     Socket descriptor.
 * @param   cMillies    Number of milliseconds to wait for the socket.
 *                      Use RT_INDEFINITE_WAIT to wait for ever.
 */
RTR3DECL(int)  RTTcpSelectOne(RTSOCKET hSocket, RTMSINTERVAL cMillies);

/**
 * Socket I/O multiplexing
 * Checks if the socket is ready for one of the given events.
 *
 * @returns iprt status code.
 * @param   hSocket     Socket descriptor.
 * @param   fEvents     Event mask to wait for.
 *                      Use the RTSOCKET_EVT_* defines.
 * @param   pfEvents    Where to store the event mask on return.
 * @param   cMillies    Number of milliseconds to wait for the socket.
 *                      Use RT_INDEFINITE_WAIT to wait for ever.
 */
RTR3DECL(int)  RTTcpSelectOneEx(RTSOCKET hSocket, uint32_t fEvents, uint32_t *pfEvents, RTMSINTERVAL cMillies);

#if 0 /* skipping these for now - RTTcpServer* handles this. */
/**
 * Listen for connection on a socket.
 *
 * @returns iprt status code.
 * @param   hSocket     Socket descriptor.
 * @param   cBackLog    The maximum length the queue of pending connections
 *                      may grow to.
 */
RTR3DECL(int)  RTTcpListen(RTSOCKET hSocket, int cBackLog);

/**
 * Accept a connection on a socket.
 *
 * @returns iprt status code.
 * @param   hSocket         Socket descriptor.
 * @param   uPort           The port for accepting connection.
 * @param   pSockAccepted   Where to store the handle to the accepted connection.
 */
RTR3DECL(int)  RTTcpAccept(RTSOCKET hSocket, unsigned uPort, PRTSOCKET pSockAccepted);

#endif

/**
 * Gets the address of the local side.
 *
 * @returns IPRT status code.
 * @param   hSocket         Socket descriptor.
 * @param   pAddr           Where to store the local address on success.
 */
RTR3DECL(int) RTTcpGetLocalAddress(RTSOCKET hSocket, PRTNETADDR pAddr);

/**
 * Gets the address of the other party.
 *
 * @returns IPRT status code.
 * @param   hSocket         Socket descriptor.
 * @param   pAddr           Where to store the peer address on success.
 */
RTR3DECL(int) RTTcpGetPeerAddress(RTSOCKET hSocket, PRTNETADDR pAddr);

/**
 * Send data from a scatter/gather buffer to a socket.
 *
 * @returns iprt status code.
 * @retval  VERR_INTERRUPTED if interrupted before anything was written.
 *
 * @param   hSocket     Socket descriptor.
 * @param   pSgBuf      Scatter/gather buffer to write data to socket.
 */
RTR3DECL(int)  RTTcpSgWrite(RTSOCKET hSocket, PCRTSGBUF pSgBuf);


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
RTR3DECL(int) RTTcpSgWriteL(RTSOCKET hSocket, size_t cSegs, ...);

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
 *                          sizes (size_t). Make 101% sure the pointer is
 *                          really size_t.
 */
RTR3DECL(int) RTTcpSgWriteLV(RTSOCKET hSocket, size_t cSegs, va_list va);

/**
 * Receive data from a socket.
 *
 * This version doesn't block if there is no data on the socket.
 *
 * @returns IPRT status code.
 *
 * @param   hSocket     Socket descriptor.
 * @param   pvBuffer    Where to put the data we read.
 * @param   cbBuffer    Read buffer size.
 * @param   pcbRead     Number of bytes read.
 */
RTR3DECL(int)  RTTcpReadNB(RTSOCKET hSocket, void *pvBuffer, size_t cbBuffer, size_t *pcbRead);

/**
 * Send data to a socket.
 *
 * This version doesn't block if there is not enough room for the message.
 *
 * @returns IPRT status code.
 *
 * @param   hSocket     Socket descriptor.
 * @param   pvBuffer    Buffer to write data to socket.
 * @param   cbBuffer    How much to write.
 * @param   pcbWritten  Number of bytes written.
 */
RTR3DECL(int)  RTTcpWriteNB(RTSOCKET hSocket, const void *pvBuffer, size_t cbBuffer, size_t *pcbWritten);

/**
 * Send data from a scatter/gather buffer to a socket.
 *
 * This version doesn't block if there is not enough room for the message.
 *
 * @returns iprt status code.
 * @retval  VERR_INTERRUPTED if interrupted before anything was written.
 *
 * @param   hSocket     Socket descriptor.
 * @param   pSgBuf      Scatter/gather buffer to write data to socket.
 * @param   pcbWritten  Number of bytes written.
 */
RTR3DECL(int)  RTTcpSgWriteNB(RTSOCKET hSocket, PCRTSGBUF pSgBuf, size_t *pcbWritten);


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
RTR3DECL(int) RTTcpSgWriteLNB(RTSOCKET hSocket, size_t cSegs, size_t *pcbWritten, ...);

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
RTR3DECL(int) RTTcpSgWriteLVNB(RTSOCKET hSocket, size_t cSegs, size_t *pcbWritten, va_list va);

/** @} */
RT_C_DECLS_END

#endif /* !IPRT_INCLUDED_tcp_h */


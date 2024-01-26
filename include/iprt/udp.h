/** @file
 * IPRT - UDP/IP.
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

#ifndef IPRT_INCLUDED_udp_h
#define IPRT_INCLUDED_udp_h
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

/** @defgroup grp_rt_udp    RTUdp - UDP/IP
 * @ingroup grp_rt
 * @{
 */


/**
 * Handle incoming UDP datagrams.
 *
 * @returns iprt status code.
 * @returns VERR_UDP_SERVER_STOP to terminate the server loop forcing
 *          the RTUdpCreateServer() call to return.
 * @param   Sock        The socket on which the datagram needs to be received.
 * @param   pvUser      User argument.
 */
typedef DECLCALLBACKTYPE(int, FNRTUDPSERVE,(RTSOCKET Sock, void *pvUser));
/** Pointer to a RTUDPSERVE(). */
typedef FNRTUDPSERVE *PFNRTUDPSERVE;

/**
 * Create single datagram at a time UDP Server in a separate thread.
 *
 * The thread will loop accepting datagrams and call pfnServe for
 * each of the incoming datagrams in turn. The pfnServe function can
 * return VERR_UDP_SERVER_STOP too terminate this loop. RTUdpServerDestroy()
 * should be used to terminate the server.
 *
 * @returns iprt status code.
 * @param   pszAddress      The address for creating a datagram socket.
 *                          If NULL or empty string the server is bound to all interfaces.
 * @param   uPort           The port for creating a datagram socket.
 * @param   enmType         The thread type.
 * @param   pszThrdName     The name of the worker thread.
 * @param   pfnServe        The function which will handle incoming datagrams.
 * @param   pvUser          User argument passed to pfnServe.
 * @param   ppServer        Where to store the serverhandle.
 */
RTR3DECL(int)  RTUdpServerCreate(const char *pszAddress, unsigned uPort, RTTHREADTYPE enmType, const char *pszThrdName,
                                 PFNRTUDPSERVE pfnServe, void *pvUser, PPRTUDPSERVER ppServer);

/**
 * Create single datagram at a time UDP Server.
 * The caller must call RTUdpServerReceive() to actually start the server.
 *
 * @returns iprt status code.
 * @param   pszAddress      The address for creating a datagram socket.
 *                          If NULL the server is bound to all interfaces.
 * @param   uPort           The port for creating a datagram socket.
 * @param   ppServer        Where to store the serverhandle.
 */
RTR3DECL(int) RTUdpServerCreateEx(const char *pszAddress, uint32_t uPort, PPRTUDPSERVER ppServer);

/**
 * Shuts down the server.
 *
 * @returns IPRT status code.
 * @param   pServer         Handle to the server.
 */
RTR3DECL(int) RTUdpServerShutdown(PRTUDPSERVER pServer);

/**
 * Closes down and frees a UDP Server.
 *
 * @returns iprt status code.
 * @param   pServer         Handle to the server.
 */
RTR3DECL(int) RTUdpServerDestroy(PRTUDPSERVER pServer);

/**
 * Listen for incoming datagrams.
 *
 * The function will loop waiting for datagrams and call pfnServe for
 * each of the incoming datagrams in turn. The pfnServe function can
 * return VERR_UDP_SERVER_STOP too terminate this loop. A stopped server
 * can only be destroyed.
 *
 * @returns iprt status code.
 * @param   pServer         The server handle as returned from RTUdpServerCreateEx().
 * @param   pfnServe        The function which will handle incoming datagrams.
 * @param   pvUser          User argument passed to pfnServe.
 */
RTR3DECL(int) RTUdpServerListen(PRTUDPSERVER pServer, PFNRTUDPSERVE pfnServe, void *pvUser);

/**
 * Receive data from a socket.
 *
 * @returns iprt status code.
 * @param   Sock        Socket descriptor.
 * @param   pvBuffer    Where to put the data we read.
 * @param   cbBuffer    Read buffer size.
 * @param   pcbRead     Number of bytes read. Must be non-NULL.
 * @param   pSrcAddr    The network address to read from.
 */
RTR3DECL(int)  RTUdpRead(RTSOCKET Sock, void *pvBuffer, size_t cbBuffer, size_t *pcbRead, PRTNETADDR pSrcAddr);

/**
 * Send data to a socket.
 *
 * @returns iprt status code.
 * @retval  VERR_INTERRUPTED if interrupted before anything was written.
 *
 * @param   pServer     Handle to the server.
 * @param   pvBuffer    Buffer to write data to socket.
 * @param   cbBuffer    How much to write.
 * @param   pDstAddr    Destination address.
 */
RTR3DECL(int)  RTUdpWrite(PRTUDPSERVER pServer, const void *pvBuffer,
                          size_t cbBuffer, PCRTNETADDR pDstAddr);

/**
 * Create and connect a data socket.
 *
 * @returns iprt status code.
 * @param   pszAddress          The address to connect to.
 * @param   uPort               The port to connect to.
 * @param   pLocalAddr          The local address to bind this socket to, can be
 *                              NULL.
 * @param   pSock               Where to store the handle to the established connection.
 */
RTR3DECL(int) RTUdpCreateClientSocket(const char *pszAddress, uint32_t uPort, PRTNETADDR pLocalAddr, PRTSOCKET pSock);

/**
 * Create a data socket acting as a server.
 *
 * @returns iprt status code.
 * @param   pszAddress          The address to connect to.
 * @param   uPort               The port to connect to.
 * @param   pSock               Where to store the handle to the established connection.
 */
RTR3DECL(int) RTUdpCreateServerSocket(const char *pszAddress, uint32_t uPort, PRTSOCKET pSock);

/** @} */
RT_C_DECLS_END

#endif /* !IPRT_INCLUDED_udp_h */


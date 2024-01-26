/** @file
 * IPRT Serial Port API.
 */

/*
 * Copyright (C) 2017-2023 Oracle and/or its affiliates.
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

#ifndef IPRT_INCLUDED_serialport_h
#define IPRT_INCLUDED_serialport_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <iprt/types.h>

RT_C_DECLS_BEGIN

/** @defgroup grp_rt_serial  IPRT Serial Port API
 * @ingroup grp_rt
 *
 * The IPRT serial port API provides a platform independent API to control a
 * serial port of the host. It supports receiving/transmitting data as well as
 * controlling and monitoring the status lines of a standard serial port.
 *
 * The user of the API is currently resposible for serializing calls to it.
 * The only exception is RTSerialPortEvtPollInterrupt() which can be called on
 * any thread to interrupt another thread waiting in RTSerialPortEvtPoll().
 *
 * @{
 */

/** Serial Port handle. */
typedef struct RTSERIALPORTINTERNAL  *RTSERIALPORT;
/** Pointer to a Serial Port handle. */
typedef RTSERIALPORT                 *PRTSERIALPORT;
/** NIL Serial Port handle value. */
#define NIL_RTSERIALPORT             ((RTSERIALPORT)0)


/**
 * Supported parity settings.
 */
typedef enum RTSERIALPORTPARITY
{
    /** Invalid parity setting. */
    RTSERIALPORTPARITY_INVALID = 0,
    /** No parity used. */
    RTSERIALPORTPARITY_NONE,
    /** Even parity used. */
    RTSERIALPORTPARITY_EVEN,
    /** Odd parity used. */
    RTSERIALPORTPARITY_ODD,
    /** Mark parity (parity bit always 1) used. */
    RTSERIALPORTPARITY_MARK,
    /** Space parity (parity bit always 0) used. */
    RTSERIALPORTPARITY_SPACE,
    /** 32bit hack. */
    RTSERIALPORTPARITY_32BIT_HACK = 0x7fffffff
} RTSERIALPORTPARITY;


/**
 * Supported data bit count setting.
 */
typedef enum RTSERIALPORTDATABITS
{
    /** Invalid bitcount setting. */
    RTSERIALPORTDATABITS_INVALID = 0,
    /** 5 data bits. */
    RTSERIALPORTDATABITS_5BITS,
    /** 6 data bits. */
    RTSERIALPORTDATABITS_6BITS,
    /** 7 data bits. */
    RTSERIALPORTDATABITS_7BITS,
    /** 8 data bits. */
    RTSERIALPORTDATABITS_8BITS,
    /** 32bit hack. */
    RTSERIALPORTDATABITS_32BIT_HACK = 0x7fffffff
} RTSERIALPORTDATABITS;


/**
 * Supported stop bit setting.
 */
typedef enum RTSERIALPORTSTOPBITS
{
    /** Invalid stop bit setting. */
    RTSERIALPORTSTOPBITS_INVALID = 0,
    /** One stop bit is used. */
    RTSERIALPORTSTOPBITS_ONE,
    /** 1.5 stop bits are used. */
    RTSERIALPORTSTOPBITS_ONEPOINTFIVE,
    /** 2 stop bits are used. */
    RTSERIALPORTSTOPBITS_TWO,
    /** 32bit hack. */
    RTSERIALPORTSTOPBITS_32BIT_HACK = 0x7fffffff
} RTSERIALPORTSTOPBITS;


/**
 * Serial port config structure.
 */
typedef struct RTSERIALPORTCFG
{
    /** Baud rate. */
    uint32_t                    uBaudRate;
    /** Used parity. */
    RTSERIALPORTPARITY          enmParity;
    /** Number of data bits. */
    RTSERIALPORTDATABITS        enmDataBitCount;
    /** Number of stop bits. */
    RTSERIALPORTSTOPBITS        enmStopBitCount;
} RTSERIALPORTCFG;
/** Pointer to a serial port config. */
typedef RTSERIALPORTCFG *PRTSERIALPORTCFG;
/** Pointer to a const serial port config. */
typedef const RTSERIALPORTCFG *PCRTSERIALPORTCFG;


/** @name RTSerialPortOpen flags
 * @{ */
/** Open the serial port with the receiver enabled to receive data. */
#define RTSERIALPORT_OPEN_F_READ                           RT_BIT(0)
/** Open the serial port with the transmitter enabled to transmit data. */
#define RTSERIALPORT_OPEN_F_WRITE                          RT_BIT(1)
/** Open the serial port with status line monitoring enabled to get notified about status line changes. */
#define RTSERIALPORT_OPEN_F_SUPPORT_STATUS_LINE_MONITORING RT_BIT(2)
/** Open the serial port with BREAK condition detection enabled (Requires extra work on some hosts). */
#define RTSERIALPORT_OPEN_F_DETECT_BREAK_CONDITION         RT_BIT(3)
/** Open the serial port with loopback mode enabled. */
#define RTSERIALPORT_OPEN_F_ENABLE_LOOPBACK                RT_BIT(4)
/** Bitmask of valid flags. */
#define RTSERIALPORT_OPEN_F_VALID_MASK                     UINT32_C(0x0000001f)
/** @} */


/** @name RTSerialPortChgModemLines flags
 * @{ */
/** Change the RTS (Ready To Send) line signal. */
#define RTSERIALPORT_CHG_STS_LINES_F_RTS                   RT_BIT(0)
/** Change the DTR (Data Terminal Ready) line signal. */
#define RTSERIALPORT_CHG_STS_LINES_F_DTR                   RT_BIT(1)
/** Bitmask of valid flags. */
#define RTSERIALPORT_CHG_STS_LINES_F_VALID_MASK            UINT32_C(0x00000003)
/** @} */


/** @name RTSerialPortQueryStatusLines flags
 * @{ */
/** The DCD (Data Carrier Detect) signal is active. */
#define RTSERIALPORT_STS_LINE_DCD                          RT_BIT(0)
/** The RI (Ring Indicator) signal is active. */
#define RTSERIALPORT_STS_LINE_RI                           RT_BIT(1)
/** The DSR (Data Set Ready) signal is active. */
#define RTSERIALPORT_STS_LINE_DSR                          RT_BIT(2)
/** The CTS (Clear To Send) signal is active. */
#define RTSERIALPORT_STS_LINE_CTS                          RT_BIT(3)
/** @} */


/** @name RTSerialPortEvtPoll flags
 * @{ */
/** Data was received and can be read. */
#define RTSERIALPORT_EVT_F_DATA_RX                         RT_BIT(0)
/** All data was transmitted and there is room again in the transmit buffer. */
#define RTSERIALPORT_EVT_F_DATA_TX                         RT_BIT(1)
/** A BREAK condition was detected on the communication channel.
 * Only available when BREAK condition detection was enabled when opening the serial port .*/
#define RTSERIALPORT_EVT_F_BREAK_DETECTED                  RT_BIT(2)
/** One of the monitored status lines changed, check with RTSerialPortQueryStatusLines().
 * Only available if status line monitoring was enabled when opening the serial port. */
#define RTSERIALPORT_EVT_F_STATUS_LINE_CHANGED             RT_BIT(3)
/** Status line monitor failed with an error and status line monitoring is disabled,
 * this cannot be given in the event mask but will be set if status line
 * monitoring is enabled and the monitor failed. */
#define RTSERIALPORT_EVT_F_STATUS_LINE_MONITOR_FAILED      RT_BIT(4)
/** Bitmask of valid flags. */
#define RTSERIALPORT_EVT_F_VALID_MASK                      UINT32_C(0x0000001f)
/** @} */


/**
 * Opens a serial port with the specified flags.
 *
 * @returns IPRT status code.
 * @param   phSerialPort            Where to store the IPRT serial port handle on success.
 * @param   pszPortAddress          The address of the serial port (host dependent).
 * @param   fFlags                  Flags to open the serial port with, see RTSERIALPORT_OPEN_F_*.
 */
RTDECL(int) RTSerialPortOpen(PRTSERIALPORT phSerialPort, const char *pszPortAddress, uint32_t fFlags);


/**
 * Closes the given serial port handle.
 *
 * @returns IPRT status code.
 * @param   hSerialPort             The IPRT serial port handle.
 */
RTDECL(int) RTSerialPortClose(RTSERIALPORT hSerialPort);


/**
 * Gets the native handle for an IPRT serial port handle.
 *
 * @returns The native handle. -1 on failure.
 * @param   hSerialPort             The IPRT serial port handle.
 */
RTDECL(RTHCINTPTR) RTSerialPortToNative(RTSERIALPORT hSerialPort);


/**
 * Tries to read the given number of bytes from the serial port, blocking version.
 *
 * @returns IPRT status code.
 * @retval  VERR_SERIALPORT_BREAK_DETECTED if a break was detected before the requested number of bytes was received.
 * @param   hSerialPort             The IPRT serial port handle.
 * @param   pvBuf                   Where to store the read data.
 * @param   cbToRead                How much to read from the serial port.
 * @param   pcbRead                 Where to store the number of bytes received until an error condition occurred, optional.
 */
RTDECL(int) RTSerialPortRead(RTSERIALPORT hSerialPort, void *pvBuf, size_t cbToRead, size_t *pcbRead);


/**
 * Tries to read the given number of bytes from the serial port, non-blocking version.
 *
 * @returns IPRT status code.
 * @retval  VERR_SERIALPORT_BREAK_DETECTED if a break was detected before anything could be received.
 * @retval  VINF_TRY_AGAIN if nothing could be read.
 * @param   hSerialPort             The IPRT serial port handle.
 * @param   pvBuf                   Where to store the read data.
 * @param   cbToRead                How much to read from the serial port.
 * @param   pcbRead                 Where to store the number of bytes received.
 */
RTDECL(int) RTSerialPortReadNB(RTSERIALPORT hSerialPort, void *pvBuf, size_t cbToRead, size_t *pcbRead);


/**
 * Writes the given data to the serial port, blocking version.
 *
 * @returns IPRT status code.
 * @param   hSerialPort             The IPRT serial port handle.
 * @param   pvBuf                   The data to write.
 * @param   cbToWrite               How much to write.
 * @param   pcbWritten              Where to store the number of bytes written until an error condition occurred, optional.
 */
RTDECL(int) RTSerialPortWrite(RTSERIALPORT hSerialPort, const void *pvBuf, size_t cbToWrite, size_t *pcbWritten);


/**
 * Writes the given data to the serial port, non-blocking version.
 *
 * @returns IPRT status code.
 * @retval  VINF_TRY_AGAIN if nothing could be written.
 * @param   hSerialPort             The IPRT serial port handle.
 * @param   pvBuf                   The data to write.
 * @param   cbToWrite               How much to write.
 * @param   pcbWritten              Where to store the number of bytes written.
 */
RTDECL(int) RTSerialPortWriteNB(RTSERIALPORT hSerialPort, const void *pvBuf, size_t cbToWrite, size_t *pcbWritten);


/**
 * Queries the current active serial port config.
 *
 * @returns IPRT status code.
 * @param   hSerialPort             The IPRT serial port handle.
 * @param   pCfg                    Where to store the current active config.
 */
RTDECL(int) RTSerialPortCfgQueryCurrent(RTSERIALPORT hSerialPort, PRTSERIALPORTCFG pCfg);


/**
 * Change the serial port to the given config.
 *
 * @returns IPRT status code.
 * @retval  VERR_SERIALPORT_INVALID_BAUDRATE if the baud rate is not supported on the serial port.
 * @param   hSerialPort             The IPRT serial port handle.
 * @param   pCfg                    The config to write.
 * @param   pErrInfo                Where to store additional information on error, optional.
 */
RTDECL(int) RTSerialPortCfgSet(RTSERIALPORT hSerialPort, PCRTSERIALPORTCFG pCfg, PRTERRINFO pErrInfo);


/**
 * Poll for an event on the given serial port.
 *
 * @returns IPRT status code.
 * @retval VERR_TIMEOUT if the timeout was reached before an event happened.
 * @retval VERR_INTERRUPTED if another thread interrupted the polling through RTSerialPortEvtPollInterrupt().
 * @param   hSerialPort             The IPRT serial port handle.
 * @param   fEvtMask                The mask of events to receive, see RTSERIALPORT_EVT_F_*
 * @param   pfEvtsRecv              Where to store the bitmask of events received.
 * @param   msTimeout               Number of milliseconds to wait for an event.
 */
RTDECL(int) RTSerialPortEvtPoll(RTSERIALPORT hSerialPort, uint32_t fEvtMask, uint32_t *pfEvtsRecv,
                                RTMSINTERVAL msTimeout);


/**
 * Interrupt another thread currently polling for an event.
 *
 * @returns IPRT status code.
 * @param   hSerialPort             The IPRT serial port handle.
 *
 * @note Any thread.
 */
RTDECL(int) RTSerialPortEvtPollInterrupt(RTSERIALPORT hSerialPort);


/**
 * Sets or clears a BREAK condition on the given serial port.
 *
 * @returns IPRT status code.
 * @param   hSerialPort             The IPRT serial port handle.
 * @param   fSet                    Flag whether to set the BREAK condition or clear it.
 */
RTDECL(int) RTSerialPortChgBreakCondition(RTSERIALPORT hSerialPort, bool fSet);


/**
 * Modify the status lines of the given serial port.
 *
 * @returns IPRT status code.
 * @param   hSerialPort             The IPRT serial port handle.
 * @param   fClear                  Combination of status lines to clear, see RTSERIALPORT_CHG_STS_LINES_F_*.
 * @param   fSet                    Combination of status lines to set, see RTSERIALPORT_CHG_STS_LINES_F_*.
 *
 * @note fClear takes precedence over fSet in case the same status line bit is set in both arguments.
 */
RTDECL(int) RTSerialPortChgStatusLines(RTSERIALPORT hSerialPort, uint32_t fClear, uint32_t fSet);


/**
 * Query the status of the status lines on the given serial port.
 *
 * @returns IPRT status code.
 * @param   hSerialPort             The IPRT serial port handle.
 * @param   pfStsLines              Where to store the bitmask of active status lines on success,
 *                                  see RTSERIALPORT_STS_LINE_*.
 */
RTDECL(int) RTSerialPortQueryStatusLines(RTSERIALPORT hSerialPort, uint32_t *pfStsLines);

/** @} */

RT_C_DECLS_END

#endif /* !IPRT_INCLUDED_serialport_h */


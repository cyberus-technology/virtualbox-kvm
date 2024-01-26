/** @file
 * IPRT - CRCs and Checksums.
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

#ifndef IPRT_INCLUDED_crc_h
#define IPRT_INCLUDED_crc_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <iprt/cdefs.h>
#include <iprt/types.h>


RT_C_DECLS_BEGIN

/** @defgroup grp_rt_crc        RTCrc - Checksums and CRCs.
 * @ingroup grp_rt
 * @{
 */


/** @defgroup grp_rt_crc32  CRC-32
 * @{ */
/**
 * Calculate CRC-32 for a memory block.
 *
 * @returns CRC-32 for the memory block.
 * @param   pv      Pointer to the memory block.
 * @param   cb      Size of the memory block in bytes.
 */
RTDECL(uint32_t)    RTCrc32(const void *pv, size_t cb);

/**
 * Start a multiblock CRC-32 calculation.
 *
 * @returns Start CRC-32.
 */
RTDECL(uint32_t)    RTCrc32Start(void);

/**
 * Processes a multiblock of a CRC-32 calculation.
 *
 * @returns Intermediate CRC-32 value.
 * @param   uCRC32  Current CRC-32 intermediate value.
 * @param   pv      The data block to process.
 * @param   cb      The size of the data block in bytes.
 */
RTDECL(uint32_t)    RTCrc32Process(uint32_t uCRC32, const void *pv, size_t cb);

/**
 * Complete a multiblock CRC-32 calculation.
 *
 * @returns CRC-32 value.
 * @param   uCRC32  Current CRC-32 intermediate value.
 */
RTDECL(uint32_t)    RTCrc32Finish(uint32_t uCRC32);
/** @} */


/** @defgroup grp_rt_crc64      CRC-64 Calculation
 * @{  */
/**
 * Calculate CRC-64 for a memory block.
 *
 * @returns CRC-64 for the memory block.
 * @param   pv      Pointer to the memory block.
 * @param   cb      Size of the memory block in bytes.
 */
RTDECL(uint64_t)    RTCrc64(const void *pv, size_t cb);

/**
 * Start a multiblock CRC-64 calculation.
 *
 * @returns Start CRC-64.
 */
RTDECL(uint64_t)    RTCrc64Start(void);

/**
 * Processes a multiblock of a CRC-64 calculation.
 *
 * @returns Intermediate CRC-64 value.
 * @param   uCRC64  Current CRC-64 intermediate value.
 * @param   pv      The data block to process.
 * @param   cb      The size of the data block in bytes.
 */
RTDECL(uint64_t)    RTCrc64Process(uint64_t uCRC64, const void *pv, size_t cb);

/**
 * Complete a multiblock CRC-64 calculation.
 *
 * @returns CRC-64 value.
 * @param   uCRC64  Current CRC-64 intermediate value.
 */
RTDECL(uint64_t)    RTCrc64Finish(uint64_t uCRC64);
/** @} */


/** @defgroup grp_rt_crc_adler32    Adler-32
 * @{ */
/**
 * Calculate Adler-32 for a memory block.
 *
 * @returns Adler-32 for the memory block.
 * @param   pv      Pointer to the memory block.
 * @param   cb      Size of the memory block in bytes.
 */
RTDECL(uint32_t)    RTCrcAdler32(void const *pv, size_t cb);

/**
 * Start a multiblock Adler-32 calculation.
 *
 * @returns Start Adler-32.
 */
RTDECL(uint32_t)    RTCrcAdler32Start(void);

/**
 * Processes a multiblock of a Adler-32 calculation.
 *
 * @returns Intermediate Adler-32 value.
 * @param   uCrc    Current Adler-32 intermediate value.
 * @param   pv      The data block to process.
 * @param   cb      The size of the data block in bytes.
 */
RTDECL(uint32_t)    RTCrcAdler32Process(uint32_t uCrc, void const *pv, size_t cb);

/**
 * Complete a multiblock Adler-32 calculation.
 *
 * @returns Adler-32 value.
 * @param   uCrc    Current Adler-32 intermediate value.
 */
RTDECL(uint32_t)    RTCrcAdler32Finish(uint32_t uCrc);

/** @} */


/** @defgroup grp_rt_crc32c  CRC-32C
 * @{ */
/**
 * Calculate CRC-32C for a memory block.
 *
 * @returns CRC-32C for the memory block.
 * @param   pv      Pointer to the memory block.
 * @param   cb      Size of the memory block in bytes.
 */
RTDECL(uint32_t)    RTCrc32C(const void *pv, size_t cb);

/**
 * Start a multiblock CRC-32 calculation.
 *
 * @returns Start CRC-32.
 */
RTDECL(uint32_t)    RTCrc32CStart(void);

/**
 * Processes a multiblock of a CRC-32C calculation.
 *
 * @returns Intermediate CRC-32C value.
 * @param   uCRC32C Current CRC-32C intermediate value.
 * @param   pv      The data block to process.
 * @param   cb      The size of the data block in bytes.
 */
RTDECL(uint32_t)    RTCrc32CProcess(uint32_t uCRC32C, const void *pv, size_t cb);

/**
 * Complete a multiblock CRC-32 calculation.
 *
 * @returns CRC-32 value.
 * @param   uCRC32  Current CRC-32 intermediate value.
 */
RTDECL(uint32_t)    RTCrc32CFinish(uint32_t uCRC32);

/** @} */


/** @defgroup grp_rt_crc16ccitt  CRC-16-CCITT
 * @{ */
/**
 * Calculate CRC-16-CCITT for a memory block.
 *
 * @returns CRC-16-CCITT for the memory block.
 * @param   pv      Pointer to the memory block.
 * @param   cb      Size of the memory block in bytes.
 */
RTDECL(uint16_t)    RTCrc16Ccitt(const void *pv, size_t cb);

/**
 * Start a multiblock CRC-16-CCITT calculation.
 *
 * @returns Start CRC-16-CCITT.
 */
RTDECL(uint16_t)    RTCrc16CcittStart(void);

/**
 * Processes a multiblock of a CRC-16-CCITT calculation.
 *
 * @returns Intermediate CRC-16-CCITT value.
 * @param   uCrc    Current CRC-16-CCITT intermediate value.
 * @param   pv      The data block to process.
 * @param   cb      The size of the data block in bytes.
 */
RTDECL(uint16_t)    RTCrc16CcittProcess(uint16_t uCrc, const void *pv, size_t cb);

/**
 * Complete a multiblock CRC-16-CCITT calculation.
 *
 * @returns CRC-16-CCITT value.
 * @param   uCrc    Current CRC-16-CCITT intermediate value.
 */
RTDECL(uint16_t)    RTCrc16CcittFinish(uint16_t uCrc);
/** @} */

/** @} */

RT_C_DECLS_END

#endif /* !IPRT_INCLUDED_crc_h */


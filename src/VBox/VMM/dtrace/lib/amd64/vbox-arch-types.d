/** @file
 * VBox & DTrace - Types and Constants for AMD64.
 */

/*
 * Copyright (C) 2012-2023 Oracle and/or its affiliates.
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
 * SPDX-License-Identifier: GPL-3.0-only
 */


inline unsigned HC_ARCH_BITS = 64;
typedef uint64_t    RTR3PTR;
typedef void       *RTR0PTR;
typedef uint64_t    RTHCPTR;



typedef union RTFLOAT80U
{
    uint16_t    au16[5];
} RTFLOAT80U;

typedef union RTFLOAT80U2
{
    uint16_t    au16[5];
} RTFLOAT80U2;

typedef struct uint128_t
{
    uint64_t    au64[2];
} uint128_t;


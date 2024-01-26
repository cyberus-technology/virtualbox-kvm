/** @file
 *
 * VBox HDD container test utility - I/O data generator.
 */

/*
 * Copyright (C) 2011-2023 Oracle and/or its affiliates.
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

#ifndef VBOX_INCLUDED_SRC_testcase_VDIoRnd_h
#define VBOX_INCLUDED_SRC_testcase_VDIoRnd_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/** Pointer to the I/O random number generator. */
typedef struct VDIORND *PVDIORND;
/** Pointer to a I/O random number generator pointer. */
typedef PVDIORND *PPVDIORND;

/**
 * Creates a I/O RNG.
 *
 * @returns VBox status code.
 *
 * @param ppIoRnd    Where to store the handle on success.
 * @param cbPattern  Size of the test pattern to create.
 * @param uSeed      Seed for the RNG.
 */
int VDIoRndCreate(PPVDIORND ppIoRnd, size_t cbPattern, uint64_t uSeed);

/**
 * Destroys the I/O RNG.
 *
 * @param pIoRnd     I/O RNG handle.
 */
void VDIoRndDestroy(PVDIORND pIoRnd);

/**
 * Returns a pointer filled with random data of the given size.
 *
 * @returns VBox status code.
 *
 * @param pIoRnd    I/O RNG handle.
 * @param ppv       Where to store the pointer on success.
 * @param cb        Size of the buffer.
 */
int VDIoRndGetBuffer(PVDIORND pIoRnd, void **ppv, size_t cb);

uint32_t VDIoRndGetU32Ex(PVDIORND pIoRnd, uint32_t uMin, uint32_t uMax);
#endif /* !VBOX_INCLUDED_SRC_testcase_VDIoRnd_h */

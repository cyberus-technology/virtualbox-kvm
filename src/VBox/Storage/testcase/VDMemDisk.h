/** @file
 *
 * VBox HDD container test utility, memory disk/file.
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

#ifndef VBOX_INCLUDED_SRC_testcase_VDMemDisk_h
#define VBOX_INCLUDED_SRC_testcase_VDMemDisk_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <iprt/sg.h>

/** Handle to the a memory disk. */
typedef struct VDMEMDISK *PVDMEMDISK;
/** Pointer to a memory disk handle. */
typedef PVDMEMDISK *PPVDMEMDISK;

/**
 * Creates a new memory disk with the given size.
 *
 * @returns VBOX status code.
 *
 * @param ppMemDisk    Where to store the memory disk handle.
 * @param cbSize       Size of the disk if it is fixed.
 *                     If 0 the disk grows when it is written to
 *                     and the size can be changed with
 *                     VDMemDiskSetSize().
 */
int VDMemDiskCreate(PPVDMEMDISK ppMemDisk, uint64_t cbSize);

/**
 * Destroys a memory disk.
 *
 * @param pMemDisk The memory disk to destroy.
 */
void VDMemDiskDestroy(PVDMEMDISK pMemDisk);

/**
 * Writes the specified amount of data from the S/G buffer at
 * the given offset.
 *
 * @returns VBox status code.
 *
 * @param pMemDisk    The memory disk handle.
 * @param off         Where to start writing to.
 * @param cbWrite     How many bytes to write.
 * @param pSgBuf      The S/G buffer to write from.
 */
int VDMemDiskWrite(PVDMEMDISK pMemDisk, uint64_t off, size_t cbWrite, PRTSGBUF pSgBuf);

/**
 * Reads the specified amount of data into the S/G buffer
 * starting from the given offset.
 *
 * @returns VBox status code.
 *
 * @param pMemDisk    The memory disk handle.
 * @param off         Where to start reading from.
 * @param cbRead      The amount of bytes to read.
 * @param pSgBuf      The S/G buffer to read into.
 */
int VDMemDiskRead(PVDMEMDISK pMemDisk, uint64_t off, size_t cbRead, PRTSGBUF pSgBuf);

/**
 * Sets the size of the memory disk.
 *
 * @returns VBox status code.
 *
 * @param pMemDisk    The memory disk handle.
 * @param cbSize      The new size to set.
 */
int VDMemDiskSetSize(PVDMEMDISK pMemDisk, uint64_t cbSize);

/**
 * Gets the current size of the memory disk.
 *
 * @returns VBox status code.
 *
 * @param pMemDisk    The memory disk handle.
 * @param pcbSize     Where to store the size of the memory
 *                    disk.
 */
int VDMemDiskGetSize(PVDMEMDISK pMemDisk, uint64_t *pcbSize);

/**
 * Dumps the memory disk to a file.
 *
 * @returns VBox status code.
 *
 * @param pMemDisk     The memory disk handle.
 * @param pcszFilename Where to dump the content.
 */
int VDMemDiskWriteToFile(PVDMEMDISK pMemDisk, const char *pcszFilename);

/**
 * Reads the content of a file into the given memory disk.
 * All data stored in the memory disk will be overwritten.
 *
 * @returns VBox status code.
 *
 * @param pMemDisk     The memory disk handle.
 * @param pcszFilename The file to load from.
 */
int VDMemDiskReadFromFile(PVDMEMDISK pMemDisk, const char *pcszFilename);

/**
 * Compares the given range of the memory disk with a provided S/G buffer.
 *
 * @returns whatever memcmp returns.
 *
 * @param   pMemDisk   The memory disk handle.
 * @param   off        Where to start comparing.
 * @param   cbCmp      How many bytes to compare.
 * @param   pSgBuf     The S/G buffer to compare with.
 */
int VDMemDiskCmp(PVDMEMDISK pMemDisk, uint64_t off, size_t cbCmp, PRTSGBUF pSgBuf);

#endif /* !VBOX_INCLUDED_SRC_testcase_VDMemDisk_h */

/* $Id */
/** @file
 * Provides a GUID and a data structure that can be used with EFI_FILE_PROTOCOL.GetInfo()
 * or EFI_FILE_PROTOCOL.SetInfo() to get or set the system's volume blessed file (for booting).
 */

/*
 * Copyright (C) 2019-2023 Oracle and/or its affiliates.
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

#ifndef __VBOX_FS_BLESSED_FILE_INFO_H__
#define __VBOX_FS_BLESSED_FILE_INFO_H__

#define VBOX_FS_BLESSED_FILE_ID \
  { \
    0xCC49FEFD, 0x41B7, 0x9823, { 0x98, 0x23, 0x0E, 0x8E, 0xBF, 0x35, 0x67, 0x7D } \
  }

typedef struct {
  ///
  /// The Null-terminated string that is the volume's label.
  ///
  CHAR16  BlessedFile[1];
} VBOX_FS_BLESSED_FILE;

#define SIZE_OF_VBOX_FS_BLESSED_FILE \
        OFFSET_OF (VBOX_FS_BLESSED_FILE, BlessedFile)

extern EFI_GUID gVBoxFsBlessedFileInfoGuid;

#endif

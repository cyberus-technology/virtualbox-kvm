/** @file
 * FAT binary definitions.
 */

/*
 * Copyright (C) 2009-2023 Oracle and/or its affiliates.
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

/** @todo Rename this file and/or reintegrate the FAT changes into MdePkg. */
#ifndef ___VBoxFatImage_h
#define ___VBoxFatImage_h

#define EFI_TE_IMAGE_HEADER_SIGNATURE  SIGNATURE_16('V', 'Z')
typedef struct _EFI_FAT_IMAGE_HEADER
{
  UINT32                    Signature;
  UINT32                    NFatArch;
} EFI_FAT_IMAGE_HEADER;

#define EFI_FAT_IMAGE_HEADER_SIGNATURE 0xef1fab9 /* Note: it's deiffer from 0xcafebabe */
typedef struct _EFI_FAT_IMAGE_HEADER_NLIST
{
    UINT32          CpuType;
    UINT32          CpuSubType;
    UINT32          Offset;
    UINT32          Size;
    UINT32          Align;
} EFI_FAT_IMAGE_HEADER_NLIST;

#define EFI_FAT_CPU_TYPE_I386   0x7
#define EFI_FAT_CPU_TYPE_X64    0x1000007

#define EFI_FAT_CPU_SUB_TYPE_PC   0x3


#endif /* !___VBoxFatImage_h */


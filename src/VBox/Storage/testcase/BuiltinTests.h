/** @file
 *
 * tstVDIo testing utility - builtin tests.
 */

/*
 * Copyright (C) 2014-2023 Oracle and/or its affiliates.
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

#ifndef VBOX_INCLUDED_SRC_testcase_BuiltinTests_h
#define VBOX_INCLUDED_SRC_testcase_BuiltinTests_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/**
 * Builtin Tests (in generated BuiltinTests.cpp)
 */
typedef struct TSTVDIOTESTENTRY
{
    /** Test name. */
    const char             *pszName;
    /** Pointer to the raw bytes. */
    const unsigned char    *pch;
    /** Number of bytes. */
    unsigned                cb;
} TSTVDIOTESTENTRY;
/** Pointer to a trust anchor table entry. */
typedef TSTVDIOTESTENTRY const *PCTSTVDIOTESTENTRY;

/** Macro for simplifying generating the trust anchor tables. */
#define TSTVDIOTESTENTRY_GEN(a_szName, a_abTest)      { #a_szName, &a_abTest[0], sizeof(a_abTest) }

/** All tests we know. */
extern TSTVDIOTESTENTRY const       g_aVDIoTests[];
/** Number of entries in g_aVDIoTests. */
extern unsigned const               g_cVDIoTests;

#endif /* !VBOX_INCLUDED_SRC_testcase_BuiltinTests_h */

/** @file
 * USBFilter - USB Filter constructs shared by kernel and user mode.
 * (DEV,HDrv,Main)
 */

/*
 * Copyright (C) 2007-2023 Oracle and/or its affiliates.
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

#ifndef VBOX_INCLUDED_usbfilter_h
#define VBOX_INCLUDED_usbfilter_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <iprt/types.h>
#include <iprt/assert.h>
#include <VBox/cdefs.h>
#include <VBox/usb.h>


/** @defgroup grp_usbfilter  USBFilter - USB Filter constructs shared by kernel and user mode
 * @ingroup grp_usblib
 * @{
 */

/**
 * How to match a field.
 *
 * @remarks     This is a binary interface (drivers).
 */
typedef enum USBFILTERMATCH
{
    /** The usual invalid first zero value. */
    USBFILTERMATCH_INVALID = 0,
    /** Ignore this field (always matching).
     * Device Data: No value present. */
    USBFILTERMATCH_IGNORE,
    /** Only require this field to be present on the device. */
    USBFILTERMATCH_PRESENT,

    /** Numeric Field: The first numeric field matching method. */
    USBFILTERMATCH_NUM_FIRST,
    /** Numeric Field: Exact match, required to be present. */
    USBFILTERMATCH_NUM_EXACT = USBFILTERMATCH_NUM_FIRST,
    /** Numeric Field: Exact match or not present. */
    USBFILTERMATCH_NUM_EXACT_NP,
    /** Numeric Field: The last numeric field matching method (inclusive). */
    USBFILTERMATCH_NUM_LAST = USBFILTERMATCH_NUM_EXACT_NP,

    /** String Field: The first string field matching method. */
    USBFILTERMATCH_STR_FIRST,
    /** String Field: Exact match, required to be present. */
    USBFILTERMATCH_STR_EXACT = USBFILTERMATCH_STR_FIRST,
    /** String Field: Exact match or not present. */
    USBFILTERMATCH_STR_EXACT_NP,
    /** String Field: Pattern match, required to be present.*/
    USBFILTERMATCH_STR_PATTERN,
    /** String Field: Pattern match or not present.*/
    USBFILTERMATCH_STR_PATTERN_NP,
    /** String Field: Numerical expression match, required to be present. */
    USBFILTERMATCH_NUM_EXPRESSION,
    /** String Field: Numerical expression match or not present. */
    USBFILTERMATCH_NUM_EXPRESSION_NP,
    /** String Field: The last string field matching method (inclusive). */
    USBFILTERMATCH_STR_LAST = USBFILTERMATCH_NUM_EXPRESSION_NP,

    /** The end of valid matching methods (exclusive). */
    USBFILTERMATCH_END
} USBFILTERMATCH;
AssertCompile(USBFILTERMATCH_END == 11);


/**
 * A USB filter field.
 *
 * @remarks     This is a binary interface (drivers).
 */
typedef struct USBFILTERFIELD
{
    /** The matching method. (USBFILTERMATCH) */
    uint16_t    enmMatch;
    /** The field value or offset into the string table.
     * The enmMatch field decides which it is. */
    uint16_t    u16Value;
} USBFILTERFIELD;
AssertCompileSize(USBFILTERFIELD, 4);
/** Pointer to a USB filter field. */
typedef USBFILTERFIELD *PUSBFILTERFIELD;
/** Pointer to a const USBFILTERFIELD. */
typedef const USBFILTERFIELD *PCUSBFILTERFIELD;


/**
 * USB filter field index.
 *
 * This is used as an index into the USBFILTER::aFields array.
 *
 * @remarks     This is a binary interface (drivers).
 */
typedef enum USBFILTERIDX
{
    /** idVendor (= 0) */
    USBFILTERIDX_VENDOR_ID = 0,
    /** idProduct (= 1) */
    USBFILTERIDX_PRODUCT_ID,
    /** bcdDevice (= 2)*/
    USBFILTERIDX_DEVICE_REV,
    USBFILTERIDX_DEVICE = USBFILTERIDX_DEVICE_REV,
    /** bDeviceClass (= 3) */
    USBFILTERIDX_DEVICE_CLASS,
    /** bDeviceSubClass (= 4) */
    USBFILTERIDX_DEVICE_SUB_CLASS,
    /** bDeviceProtocol (= 5) */
    USBFILTERIDX_DEVICE_PROTOCOL,
    /** bBus (= 6 )*/
    USBFILTERIDX_BUS,
    /** bPort (=7) */
    USBFILTERIDX_PORT,
    /** Manufacturer string. (=8) */
    USBFILTERIDX_MANUFACTURER_STR,
    /** Product string. (=9) */
    USBFILTERIDX_PRODUCT_STR,
    /** SerialNumber string. (=10) */
    USBFILTERIDX_SERIAL_NUMBER_STR,
    /** The end of the USB filter fields (exclusive). */
    USBFILTERIDX_END
} USBFILTERIDX;
AssertCompile(USBFILTERIDX_END == 11);


/**
 * USB Filter types.
 *
 * The filters types are list in priority order, i.e. highest priority first.
 *
 * @remarks     This is a binary interface (drivers).
 */
typedef enum USBFILTERTYPE
{
    /** The usual invalid first zero value. */
    USBFILTERTYPE_INVALID = 0,
    /** The first valid entry. */
    USBFILTERTYPE_FIRST,
    /** A one-shot ignore filter that's installed when releasing a device.
     * This filter will be automatically removedwhen the device re-appears,
     * or when ring-3 decides that time is up, or if ring-3 dies upon us. */
    USBFILTERTYPE_ONESHOT_IGNORE = USBFILTERTYPE_FIRST,
    /** A one-shot capture filter that's installed when hijacking a device that's already plugged.
     * This filter will be automatically removed when the device re-appears,
     * or when ring-3 decides that time is up, or if ring-3 dies upon us. */
    USBFILTERTYPE_ONESHOT_CAPTURE,
    /** Ignore filter.
     * This picks out devices that shouldn't be captured. */
    USBFILTERTYPE_IGNORE,
    /** A normal capture filter.
     * When a device matching the filter is attach, we'll take it. */
    USBFILTERTYPE_CAPTURE,
    /** The end of the valid filter types (exclusive). */
    USBFILTERTYPE_END,
    /** The usual 32-bit hack. */
    USBFILTERTYPE_32BIT_HACK = 0x7fffffff
} USBFILTERTYPE;
AssertCompileSize(USBFILTERTYPE, 4);
AssertCompile(USBFILTERTYPE_END == 5);


/**
 * USB Filter.
 *
 * Consider the an abstract data type, use the methods below to access it.
 *
 * @remarks     This is a binary interface (drivers).
 */
typedef struct USBFILTER
{
    /** Magic number (USBFILTER_MAGIC). */
    uint32_t            u32Magic;
    /** The filter type. */
    USBFILTERTYPE       enmType;
    /** The filter fields.
     * This array is indexed by USBFILTERIDX */
    USBFILTERFIELD      aFields[USBFILTERIDX_END];
    /** Offset to the end of the string table (last terminator). (used to speed up things) */
    uint32_t            offCurEnd;
    /** String table.
     * This is used for string and numeric patterns. */
    char                achStrTab[256];
} USBFILTER;
AssertCompileSize(USBFILTER, 312);

/** Pointer to a USBLib filter. */
typedef USBFILTER *PUSBFILTER;
/** Pointer to a const USBLib filter. */
typedef const USBFILTER *PCUSBFILTER;

/** USBFILTER::u32Magic (Yasuhiro Nightow). */
#define USBFILTER_MAGIC      UINT32_C(0x19670408)


RT_C_DECLS_BEGIN

USBLIB_DECL(void) USBFilterInit(PUSBFILTER pFilter, USBFILTERTYPE enmType);
USBLIB_DECL(void) USBFilterClone(PUSBFILTER pFilter, PCUSBFILTER pToClone);
USBLIB_DECL(void) USBFilterDelete(PUSBFILTER pFilter);
USBLIB_DECL(int)  USBFilterValidate(PCUSBFILTER pFilter);
USBLIB_DECL(bool) USBFilterMatch(PCUSBFILTER pFilter, PCUSBFILTER pDevice);
USBLIB_DECL(int)  USBFilterMatchRated(PCUSBFILTER pFilter, PCUSBFILTER pDevice);
USBLIB_DECL(bool) USBFilterMatchDevice(PCUSBFILTER pFilter, PCUSBDEVICE pDevice);
USBLIB_DECL(bool) USBFilterIsIdentical(PCUSBFILTER pFilter, PCUSBFILTER pFilter2);

USBLIB_DECL(int)  USBFilterSetFilterType(PUSBFILTER pFilter, USBFILTERTYPE enmType);
USBLIB_DECL(int)  USBFilterSetIgnore(PUSBFILTER pFilter, USBFILTERIDX enmFieldIdx);
USBLIB_DECL(int)  USBFilterSetPresentOnly(PUSBFILTER pFilter, USBFILTERIDX enmFieldIdx);
USBLIB_DECL(int)  USBFilterSetNumExact(PUSBFILTER pFilter, USBFILTERIDX enmFieldIdx, uint16_t u16Value, bool fMustBePresent);
USBLIB_DECL(int)  USBFilterSetNumExpression(PUSBFILTER pFilter, USBFILTERIDX enmFieldIdx, const char *pszExpression, bool fMustBePresent);
USBLIB_DECL(int)  USBFilterSetStringExact(PUSBFILTER pFilter, USBFILTERIDX enmFieldIdx, const char *pszValue,
                                          bool fMustBePresent, bool fPurge);
USBLIB_DECL(int)  USBFilterSetStringPattern(PUSBFILTER pFilter, USBFILTERIDX enmFieldIdx, const char *pszPattern, bool fMustBePresent);
USBLIB_DECL(int)  USBFilterSetMustBePresent(PUSBFILTER pFilter, USBFILTERIDX enmFieldIdx, bool fMustBePresent);

USBLIB_DECL(USBFILTERTYPE)   USBFilterGetFilterType(PCUSBFILTER pFilter);
USBLIB_DECL(USBFILTERMATCH)  USBFilterGetMatchingMethod(PCUSBFILTER pFilter, USBFILTERIDX enmFieldIdx);
USBLIB_DECL(int)             USBFilterQueryNum(PCUSBFILTER pFilter, USBFILTERIDX enmFieldIdx, uint16_t *pu16Value);
USBLIB_DECL(int)             USBFilterGetNum(PCUSBFILTER pFilter, USBFILTERIDX enmFieldIdx);
USBLIB_DECL(int)             USBFilterQueryString(PUSBFILTER pFilter, USBFILTERIDX enmFieldIdx, char *pszBuf, size_t cchBuf);
USBLIB_DECL(const char *)    USBFilterGetString(PCUSBFILTER pFilter, USBFILTERIDX enmFieldIdx);
USBLIB_DECL(ssize_t)         USBFilterGetStringLen(PCUSBFILTER pFilter, USBFILTERIDX enmFieldIdx);

USBLIB_DECL(bool) USBFilterHasAnySubstatialCriteria(PCUSBFILTER pFilter);
USBLIB_DECL(bool) USBFilterIsNumericField(USBFILTERIDX enmFieldIdx);
USBLIB_DECL(bool) USBFilterIsStringField(USBFILTERIDX enmFieldIdx);
USBLIB_DECL(bool) USBFilterIsMethodUsingNumericValue(USBFILTERMATCH enmMatchingMethod);
USBLIB_DECL(bool) USBFilterIsMethodUsingStringValue(USBFILTERMATCH enmMatchingMethod);
USBLIB_DECL(bool) USBFilterIsMethodNumeric(USBFILTERMATCH enmMatchingMethod);
USBLIB_DECL(bool) USBFilterIsMethodString(USBFILTERMATCH enmMatchingMethod);

RT_C_DECLS_END

/** @} */

#endif /* !VBOX_INCLUDED_usbfilter_h */

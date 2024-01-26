/** @file
 * IPRT - JavaScript Object Notation (JSON) Parser.
 */

/*
 * Copyright (C) 2016-2023 Oracle and/or its affiliates.
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

#ifndef IPRT_INCLUDED_json_h
#define IPRT_INCLUDED_json_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <iprt/types.h>

RT_C_DECLS_BEGIN


/** @defgroup grp_json      RTJson - JavaScript Object Notation (JSON) Parser
 * @ingroup grp_rt
 * @{
 */

/**
 * JSON value types.
 */
typedef enum RTJSONVALTYPE
{
    /** Invalid first value. */
    RTJSONVALTYPE_INVALID = 0,
    /** Value containing an object. */
    RTJSONVALTYPE_OBJECT,
    /** Value containing an array. */
    RTJSONVALTYPE_ARRAY,
    /** Value containing a string. */
    RTJSONVALTYPE_STRING,
    /** Value containg an integer number. */
    RTJSONVALTYPE_INTEGER,
    /** Value containg an floating point number. */
    RTJSONVALTYPE_NUMBER,
    /** Value containg the special null value. */
    RTJSONVALTYPE_NULL,
    /** Value containing true. */
    RTJSONVALTYPE_TRUE,
    /** Value containing false. */
    RTJSONVALTYPE_FALSE,
    /** 32-bit hack. */
    RTJSONVALTYPE_32BIT_HACK = 0x7fffffff
} RTJSONVALTYPE;
/** Pointer to a JSON value type. */
typedef RTJSONVALTYPE *PRTJSONVALTYPE;

/** JSON value handle. */
typedef struct RTJSONVALINT *RTJSONVAL;
/** Pointer to a JSON value handle. */
typedef RTJSONVAL *PRTJSONVAL;
/** NIL JSON value handle. */
#define NIL_RTJSONVAL ((RTJSONVAL)~(uintptr_t)0)

/** JSON iterator handle. */
typedef struct RTJSONITINT  *RTJSONIT;
/** Pointer to a JSON iterator handle. */
typedef RTJSONIT *PRTJSONIT;
/** NIL JSON iterator handle. */
#define NIL_RTJSONIT ((RTJSONIT)~(uintptr_t)0)

/**
 * Parses a JSON document in the provided buffer returning the root JSON value.
 *
 * @returns IPRT status code.
 * @retval  VERR_JSON_MALFORMED if the document does not conform to the spec.
 * @param   phJsonVal       Where to store the handle to the JSON value on success.
 * @param   pbBuf           The byte buffer containing the JSON document.
 * @param   cbBuf           Size of the buffer.
 * @param   pErrInfo        Where to store extended error info. Optional.
 *
 * @todo    r=bird: The use of uint8_t makes no sense here since the parser
 *          expects ASCII / UTF-8.  What's more, if this is a real buffer the
 *          type should be 'const void *' rather than 'const uint8_t *'.
 *          This function should be modified to reflect that it's really for
 *          handling unterminated strings.
 */
RTDECL(int) RTJsonParseFromBuf(PRTJSONVAL phJsonVal, const uint8_t *pbBuf, size_t cbBuf, PRTERRINFO pErrInfo);

/**
 * Parses a JSON document from the provided string returning the root JSON value.
 *
 * @returns IPRT status code.
 * @retval  VERR_JSON_MALFORMED if the document does not conform to the spec.
 * @param   phJsonVal       Where to store the handle to the JSON value on success.
 * @param   pszStr          The string containing the JSON document.
 * @param   pErrInfo        Where to store extended error info. Optional.
 */
RTDECL(int) RTJsonParseFromString(PRTJSONVAL phJsonVal, const char *pszStr, PRTERRINFO pErrInfo);

/**
 * Parses a JSON document from the file pointed to by the given filename
 * returning the root JSON value.
 *
 * @returns IPRT status code.
 * @retval  VERR_JSON_MALFORMED if the document does not conform to the spec.
 * @param   phJsonVal       Where to store the handle to the JSON value on success.
 * @param   pszFilename     The name of the file containing the JSON document.
 * @param   pErrInfo        Where to store extended error info. Optional.
 */
RTDECL(int) RTJsonParseFromFile(PRTJSONVAL phJsonVal, const char *pszFilename, PRTERRINFO pErrInfo);

/**
 * Parses a JSON document from the given VFS file
 * returning the root JSON value.
 *
 * @returns IPRT status code.
 * @retval  VERR_JSON_MALFORMED if the document does not conform to the spec.
 * @param   phJsonVal       Where to store the handle to the JSON value on success.
 * @param   hVfsFile        The VFS file to parse.
 * @param   pErrInfo        Where to store extended error info. Optional.
 */
RTDECL(int) RTJsonParseFromVfsFile(PRTJSONVAL phJsonVal, RTVFSFILE hVfsFile, PRTERRINFO pErrInfo);

/**
 * Retain a given JSON value.
 *
 * @returns New reference count.
 * @param   hJsonVal        The JSON value handle.
 */
RTDECL(uint32_t) RTJsonValueRetain(RTJSONVAL hJsonVal);

/**
 * Release a given JSON value.
 *
 * @returns New reference count, if this drops to 0 the value is freed.
 * @param   hJsonVal        The JSON value handle.
 */
RTDECL(uint32_t) RTJsonValueRelease(RTJSONVAL hJsonVal);

/**
 * Return the type of a given JSON value.
 *
 * @returns Type of the given JSON value.
 * @param   hJsonVal        The JSON value handle.
 */
RTDECL(RTJSONVALTYPE) RTJsonValueGetType(RTJSONVAL hJsonVal);

/**
 * Translates value type to a name.
 *
 * @returns Readonly name string
 * @param   enmType             The JSON value type to name.
 */
RTDECL(const char *) RTJsonValueTypeName(RTJSONVALTYPE enmType);

/**
 * Returns the string from a given JSON string value.
 *
 * @returns Pointer to the string of the JSON value, NULL if the value type
 *          doesn't indicate a string.
 * @param   hJsonVal        The JSON value handle.
 */
RTDECL(const char *) RTJsonValueGetString(RTJSONVAL hJsonVal);

/**
 * Returns the string from a given JSON string value, extended.
 *
 * @returns IPRT status code.
 * @retval  VERR_JSON_VALUE_INVALID_TYPE if the JSON value is not a string.
 * @param   hJsonVal        The JSON value handle.
 * @param   ppszStr         Where to store the pointer to the string on success.
 */
RTDECL(int) RTJsonValueQueryString(RTJSONVAL hJsonVal, const char **ppszStr);

/**
 * Returns the integer from a given JSON integer value.
 *
 * @returns IPRT status code.
 * @retval  VERR_JSON_VALUE_INVALID_TYPE if the JSON value is not a number.
 * @param   hJsonVal        The JSON value handle.
 * @param   pi64Num         WHere to store the number on success.
 * @sa      RTJsonValueQueryNumber
 */
RTDECL(int) RTJsonValueQueryInteger(RTJSONVAL hJsonVal, int64_t *pi64Num);

/**
 * Returns the floating point value from a given JSON number value.
 *
 * @returns IPRT status code.
 * @retval  VERR_JSON_VALUE_INVALID_TYPE if the JSON value is not a number.
 * @param   hJsonVal        The JSON value handle.
 * @param   prdNum          WHere to store the floating point number on success.
 * @sa      RTJsonValueQueryInteger
 */
RTDECL(int) RTJsonValueQueryNumber(RTJSONVAL hJsonVal, double *prdNum);

/**
 * Returns the value associated with a given name for the given JSON object value.
 *
 * @returns IPRT status code.
 * @retval  VERR_JSON_VALUE_INVALID_TYPE if the JSON value is not an object.
 * @retval  VERR_NOT_FOUND if the name is not known for this JSON object.
 * @param   hJsonVal        The JSON value handle.
 * @param   pszName         The member name of the object.
 * @param   phJsonVal       Where to store the handle to the JSON value on success.
 */
RTDECL(int) RTJsonValueQueryByName(RTJSONVAL hJsonVal, const char *pszName, PRTJSONVAL phJsonVal);

/**
 * Returns the number of a number value associated with a given name for the given JSON object value.
 *
 * @returns IPRT status code.
 * @retval  VERR_JSON_VALUE_INVALID_TYPE if the JSON value is not an object or
 *          the name does not point to an integer value.
 * @retval  VERR_NOT_FOUND if the name is not known for this JSON object.
 * @param   hJsonVal        The JSON value handle.
 * @param   pszName         The member name of the object.
 * @param   pi64Num         Where to store the number on success.
 */
RTDECL(int) RTJsonValueQueryIntegerByName(RTJSONVAL hJsonVal, const char *pszName, int64_t *pi64Num);

/**
 * Returns the number of a number value associated with a given name for the given JSON object value.
 *
 * @returns IPRT status code.
 * @retval  VERR_JSON_VALUE_INVALID_TYPE if the JSON value is not an object or
 *          the name does not point to a number value.
 * @retval  VERR_NOT_FOUND if the name is not known for this JSON object.
 * @param   hJsonVal        The JSON value handle.
 * @param   pszName         The member name of the object.
 * @param   prdNum          WHere to store the floating point number on success.
 */
RTDECL(int) RTJsonValueQueryNumberByName(RTJSONVAL hJsonVal, const char *pszName, double *prdNum);

/**
 * Returns the string of a string value associated with a given name for the given JSON object value.
 *
 * @returns IPRT status code.
 * @retval  VERR_JSON_VALUE_INVALID_TYPE if the JSON value is not an object or
 *          the name does not point to a string value.
 * @retval  VERR_NOT_FOUND if the name is not known for this JSON object.
 * @param   hJsonVal        The JSON value handle.
 * @param   pszName         The member name of the object.
 * @param   ppszStr         Where to store the pointer to the string on success.
 *                          Must be freed with RTStrFree().
 */
RTDECL(int) RTJsonValueQueryStringByName(RTJSONVAL hJsonVal, const char *pszName, char **ppszStr);

/**
 * Returns the boolean of a true/false value associated with a given name for the given JSON object value.
 *
 * @returns IPRT status code.
 * @retval  VERR_JSON_VALUE_INVALID_TYPE if the JSON value is not an object or
 *          the name does not point to a true/false value.
 * @retval  VERR_NOT_FOUND if the name is not known for this JSON object.
 * @param   hJsonVal        The JSON value handle.
 * @param   pszName         The member name of the object.
 * @param   pfBoolean       Where to store the boolean value on success.
 */
RTDECL(int) RTJsonValueQueryBooleanByName(RTJSONVAL hJsonVal, const char *pszName, bool *pfBoolean);

/**
 * Returns the size of a given JSON array value.
 *
 * @returns Size of the JSON array value.
 * @retval  0 if the array is empty or the JSON value is not an array.
 * @param   hJsonVal        The JSON value handle.
 */
RTDECL(unsigned) RTJsonValueGetArraySize(RTJSONVAL hJsonVal);

/**
 * Returns the size of a given JSON array value - extended version.
 *
 * @returns IPRT status code.
 * @retval  VERR_JSON_VALUE_INVALID_TYPE if the JSON value is not an array.
 * @param   hJsonVal        The JSON value handle.
 * @param   pcItems         Where to store the size of the JSON array value on success.
 */
RTDECL(int) RTJsonValueQueryArraySize(RTJSONVAL hJsonVal, unsigned *pcItems);

/**
 * Returns the value for the given index of a given JSON array value.
 *
 * @returns IPRT status code.
 * @retval  VERR_JSON_VALUE_INVALID_TYPE if the JSON value is not an array.
 * @retval  VERR_OUT_OF_RANGE if @a idx is out of bounds.
 *
 * @param   hJsonVal        The JSON value handle.
 * @param   idx             The index to get the value from.
 * @param   phJsonVal       Where to store the handle to the JSON value on success.
 */
RTDECL(int) RTJsonValueQueryByIndex(RTJSONVAL hJsonVal, unsigned idx, PRTJSONVAL phJsonVal);

/**
 * Creates an iterator for a given JSON array or object value.
 *
 * @returns IPRT status code.
 * @retval  VERR_JSON_VALUE_INVALID_TYPE if the JSON value is not an array or
 *          object.
 * @param   hJsonVal        The JSON value handle.
 * @param   phJsonIt        Where to store the JSON iterator handle on success.
 * @todo    Make return VERR_JSON_IS_EMPTY (or remove it).
 */
RTDECL(int) RTJsonIteratorBegin(RTJSONVAL hJsonVal, PRTJSONIT phJsonIt);

/**
 * Creates an iterator for a given JSON array value.
 *
 * @returns IPRT status code.
 * @retval  VERR_JSON_VALUE_INVALID_TYPE if the JSON value is not an array.
 * @retval  VERR_JSON_IS_EMPTY if no members.
 * @param   hJsonVal        The JSON value handle.
 * @param   phJsonIt        Where to store the JSON iterator handle on success.
 */
RTDECL(int) RTJsonIteratorBeginArray(RTJSONVAL hJsonVal, PRTJSONIT phJsonIt);

/**
 * Creates an iterator for a given JSON object value.
 *
 * @returns IPRT status code.
 * @retval  VERR_JSON_VALUE_INVALID_TYPE if the JSON value is not an object.
 * @retval  VERR_JSON_IS_EMPTY if no members.
 * @param   hJsonVal        The JSON value handle.
 * @param   phJsonIt        Where to store the JSON iterator handle on success.
 */
RTDECL(int) RTJsonIteratorBeginObject(RTJSONVAL hJsonVal, PRTJSONIT phJsonIt);

/**
 * Gets the value and optional name for the current iterator position.
 *
 * @returns IPRT status code.
 * @param   hJsonIt         The JSON iterator handle.
 * @param   phJsonVal       Where to store the handle to the JSON value on success.
 * @param   ppszName        Where to store the object member name for an object.
 *                          NULL is returned for arrays.
 */
RTDECL(int) RTJsonIteratorQueryValue(RTJSONIT hJsonIt, PRTJSONVAL phJsonVal, const char **ppszName);

/**
 * Advances to the next element in the referenced JSON value.
 *
 * @returns IPRT status code.
 * @retval  VERR_JSON_ITERATOR_END if the end for this iterator was reached.
 * @param   hJsonIt         The JSON iterator handle.
 */
RTDECL(int) RTJsonIteratorNext(RTJSONIT hJsonIt);

/**
 * Frees a given JSON iterator.
 *
 * @param   hJsonIt         The JSON iterator to free.
 */
RTDECL(void) RTJsonIteratorFree(RTJSONIT hJsonIt);

RT_C_DECLS_END

/** @} */

#endif /* !IPRT_INCLUDED_json_h */


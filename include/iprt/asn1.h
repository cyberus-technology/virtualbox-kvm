/** @file
 * IPRT - Abstract Syntax Notation One (ASN.1).
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

#ifndef IPRT_INCLUDED_asn1_h
#define IPRT_INCLUDED_asn1_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <iprt/time.h>
#include <iprt/stdarg.h>
#include <iprt/errcore.h>
#include <iprt/formats/asn1.h>


RT_C_DECLS_BEGIN

/** @defgroup grp_rt_asn1   RTAsn1 - Abstract Syntax Notation One
 * @ingroup grp_rt
 * @{
 */


/** Pointer to ASN.1 allocation information. */
typedef struct RTASN1ALLOCATION *PRTASN1ALLOCATION;
/** Pointer to ASN.1 array allocation information. */
typedef struct RTASN1ARRAYALLOCATION *PRTASN1ARRAYALLOCATION;
/** Pointer to a ASN.1 byte decoder cursor. */
typedef struct RTASN1CURSOR *PRTASN1CURSOR;


/**
 * Sketch of a custom ASN.1 allocator virtual method table.
 *
 * Any information required by the allocator should be associated with this
 * structure, i.e. use this as a kind of parent class.  This saves storage in
 * RTASN1ALLOCATORINFO and possibly reduces the number of parameters by one.
 */
typedef struct RTASN1ALLOCATORVTABLE
{
    /**
     * Free a chunk of memory allocated by this allocator.
     *
     * @returns IPRT status code.
     * @param   pThis           Pointer to the vtable structure.
     * @param   pAllocation     Pointer to the allocation info structure.
     * @param   pv              Pointer to the memory that shall be freed. Not NULL.
     */
    DECLCALLBACKMEMBER(void, pfnFree,(struct RTASN1ALLOCATORVTABLE const *pThis, PRTASN1ALLOCATION pAllocation,
                                      void *pv));
    /**
     * Allocates a chunk of memory, all initialized to zero.
     *
     * @returns IPRT status code.
     * @param   pThis           Pointer to the vtable structure.
     * @param   pAllocation     Pointer to the allocation info structure.
     * @param   ppv             Where to store the pointer on success.
     * @param   cb              The minimum number of bytes to allocate.  The actual
     *                          number of bytes allocated shall be stored in
     *                          pInfo->cbAllocated on success.
     */
    DECLCALLBACKMEMBER(int, pfnAlloc,(struct RTASN1ALLOCATORVTABLE const *pThis, PRTASN1ALLOCATION pAllocation,
                                      void **ppv, size_t cb));
    /**
     * Reallocates a memory allocation.
     *
     * New memory does not need to be initialized, the caller takes care of that.
     *
     * This will not need to deal with free (@a cbNew == 0) or the initial
     * allocation (@a pvOld == NULL), those calls will be directed to pfnFree and
     * pfnAlloc respectively.
     *
     * @returns IPRT status code.
     * @param   pThis           Pointer to the vtable structure.
     * @param   pAllocation     Pointer to the allocation info structure.
     * @param   pvOld           Pointer to the current allocation.  Shall remain
     *                          valid on failure, but may be invalid on success.
     * @param   ppvNew          Where to store the pointer on success.  Shall not be
     *                          touched, except on successful returns.
     * @param   cbNew           The new minimum allocation size.  The actual number
     *                          of bytes allocated shall be stored in
     *                          pInfo->cbAllocated on success.
     */
    DECLCALLBACKMEMBER(int, pfnRealloc,(struct RTASN1ALLOCATORVTABLE const *pThis, PRTASN1ALLOCATION pAllocation,
                                        void *pvOld, void **ppvNew, size_t cbNew));

    /**
     * Frees an array allocation (the array an all instances in it).
     *
     * @returns IPRT status code.
     * @param   pThis           Pointer to the vtable structure.
     * @param   pAllocation     Pointer to the allocation info structure.
     * @param   papvArray       Pointer to the pointer array to be freed.  Not NULL.
     */
    DECLCALLBACKMEMBER(void, pfnFreeArray,(struct RTASN1ALLOCATORVTABLE const *pThis, PRTASN1ARRAYALLOCATION pAllocation,
                                           void **papvArray));
    /**
     * Grows the array to at least @a cMinEntries.
     *
     * The entries are initalized with ZEROs.
     *
     * @returns IPRT status code.
     * @param   pThis           Pointer to the vtable structure.
     * @param   pAllocation     Pointer to the allocation info structure.
     * @param   ppapvArray      Pointer to the pointer to the array to be grown (or
     *                          allocated).
     * @param   cMinEntries     The minimum number of entries (array size and
     *                          instantiated entries) that must be available
     *                          on successful return.
     */
    DECLCALLBACKMEMBER(int, pfnGrowArray,(struct RTASN1ALLOCATORVTABLE const *pThis, PRTASN1ARRAYALLOCATION pAllocation,
                                          void ***ppapvArray, uint32_t cMinEntries));
    /**
     * Shrinks the array (depends on allocator policy).
     *
     * If memory isn't freed, the implementation must fill the entries being
     * shredded with ZEROs so the growth optimizations in RTAsn1MemResizeArray
     * returns ZEROed entries.
     *
     * @returns IPRT status code.
     * @param   pThis           Pointer to the vtable structure.
     * @param   pAllocation     Pointer to the allocation info structure.
     * @param   ppapvArray      Pointer to the pointer to the array to shrunk.
     * @param   cNew            The new entry count.
     * @param   cCurrent        The new entry count.
     */
    DECLCALLBACKMEMBER(void, pfnShrinkArray,(struct RTASN1ALLOCATORVTABLE const *pThis, PRTASN1ARRAYALLOCATION pAllocation,
                                             void ***ppapvArray, uint32_t cNew, uint32_t cCurrent));
} RTASN1ALLOCATORVTABLE;
/** Pointer to an ASN.1 allocator vtable. */
typedef RTASN1ALLOCATORVTABLE *PRTASN1ALLOCATORVTABLE;
/** Pointer to a const ASN.1 allocator vtable. */
typedef RTASN1ALLOCATORVTABLE const *PCRTASN1ALLOCATORVTABLE;

/** The default ASN.1 allocator. */
extern RTDATADECL(RTASN1ALLOCATORVTABLE const) g_RTAsn1DefaultAllocator;

/** The Electric Fence ASN.1 allocator. */
extern RTDATADECL(RTASN1ALLOCATORVTABLE const) g_RTAsn1EFenceAllocator;

/** The safer ASN.1 allocator for sensitive data. */
extern RTDATADECL(RTASN1ALLOCATORVTABLE const) g_RTAsn1SaferAllocator;


/**
 * Allocation information.
 */
typedef struct RTASN1ALLOCATION
{
    /** The number of bytes currently allocated. */
    uint32_t                    cbAllocated;
    /** Number of realloc calls. */
    uint16_t                    cReallocs;
    /** Reserved / padding. */
    uint16_t                    uReserved0;
    /** Allocator vtable, NULL for the default allocator. */
    PCRTASN1ALLOCATORVTABLE     pAllocator;
} RTASN1ALLOCATION;


/**
 * Pointer array allocation information.
 *
 * Used by SET OF and SEQUENCE OF structures (typically automatically
 * generated).
 */
typedef struct RTASN1ARRAYALLOCATION
{
    /** The size of the array entry. */
    uint32_t                    cbEntry;
    /** The size of the pointer array allocation. */
    uint32_t                    cPointersAllocated;
    /** Number of entry instances allocated.  This can be greater than the
     * official array size. */
    uint32_t                    cEntriesAllocated;
    /** Number of array resizing calls (for increasing growth rate).
     * Maintained by RTAsn1MemResizeArray().  */
    uint16_t                    cResizeCalls;
    /** Reserved / padding. */
    uint16_t                    uReserved0;
    /** Allocator vtable, NULL for the default allocator. */
    PCRTASN1ALLOCATORVTABLE     pAllocator;
} RTASN1ARRAYALLOCATION;


/**
 * Allocate a block of zero initialized memory.
 *
 * @returns IPRT status code.
 * @param   pAllocation The allocation record (initialized by
 *                      RTAsn1CursorInitAllocation or similar).
 * @param   ppvMem      Where to return the pointer to the block.
 * @param   cbMem       The minimum number of bytes to allocate.
 */
RTDECL(int) RTAsn1MemAllocZ(PRTASN1ALLOCATION pAllocation, void **ppvMem, size_t cbMem);

/**
 * Allocates a block of memory initialized to the content of @a pvSrc.
 *
 * @returns IPRT status code.
 * @param   pAllocation The allocation record (initialized by
 *                      RTAsn1CursorInitAllocation or similar).
 * @param   ppvMem      Where to return the pointer to the block.
 * @param   pvSrc       The source memory.
 * @param   cbMem       The minimum number of bytes to allocate.
 */
RTDECL(int) RTAsn1MemDup(PRTASN1ALLOCATION pAllocation, void **ppvMem, void const *pvSrc, size_t cbMem);

/**
 * Free a memory block.
 *
 * @param   pAllocation The allocation record (initialized by
 *                      RTAsn1CursorInitAllocation or similar).
 * @param   pv          The memory block to free.  NULL will be ignored.
 */
RTDECL(void) RTAsn1MemFree(PRTASN1ALLOCATION pAllocation, void *pv);

/**
 * Initalize an allocation.
 *
 * @returns pAllocation
 * @param   pAllocation The allocation record (initialized by
 *                      RTAsn1CursorInitAllocation or similar).
 * @param   pAllocator  The allocator
 */
RTDECL(PRTASN1ALLOCATION) RTAsn1MemInitAllocation(PRTASN1ALLOCATION pAllocation, PCRTASN1ALLOCATORVTABLE pAllocator);

/**
 * Initalize an array allocation.
 *
 * @returns pAllocation
 * @param   pAllocation The allocation record (initialized by
 *                      RTAsn1CursorInitAllocation or similar).
 * @param   pAllocator  The allocator
 * @param   cbEntry     The entry size.
 */
RTDECL(PRTASN1ARRAYALLOCATION) RTAsn1MemInitArrayAllocation(PRTASN1ARRAYALLOCATION pAllocation,
                                                            PCRTASN1ALLOCATORVTABLE pAllocator, size_t cbEntry);

/**
 * Resize an array with zero initialized memory.
 *
 * @returns IPRT status code.
 * @param   pAllocation The allocation record (initialized by
 *                      RTAsn1CursorInitAllocation or similar).
 * @param   ppapvArray  Pointer to the variable pointing to the array.  This is
 *                      both input and output.  Remains valid on failure.
 * @param   cCurrent    The current entry count.  (Relevant for zero
 *                      initialization of the new entries.)
 * @param   cNew        The new entry count.
 */
RTDECL(int) RTAsn1MemResizeArray(PRTASN1ARRAYALLOCATION pAllocation, void ***ppapvArray, uint32_t cCurrent, uint32_t cNew);

/**
 * Frees an array and all its entries.
 *
 * @param   pAllocation The array allocation record (initialized by
 *                      RTAsn1CursorInitArrayAllocation or similar).
 * @param   papvArray   The array to free.  NULL is ignored.
 */
RTDECL(void) RTAsn1MemFreeArray(PRTASN1ARRAYALLOCATION pAllocation, void **papvArray);


/** Pointer to a core ASN.1 encoding info structure. */
typedef struct RTASN1CORE *PRTASN1CORE;
/** Pointer to a const core ASN.1 encoding info structure. */
typedef struct RTASN1CORE const *PCRTASN1CORE;

RTDECL(int)  RTAsn1ContentAllocZ(struct RTASN1CORE *pAsn1Core, size_t cb, PCRTASN1ALLOCATORVTABLE pAllocator);
RTDECL(int)  RTAsn1ContentDup(struct RTASN1CORE *pAsn1Core, void const *pvSrc, size_t cbSrc, PCRTASN1ALLOCATORVTABLE pAllocator);
RTDECL(int)  RTAsn1ContentReallocZ(struct RTASN1CORE *pAsn1Core, size_t cb, PCRTASN1ALLOCATORVTABLE pAllocator);
RTDECL(void) RTAsn1ContentFree(struct RTASN1CORE *pAsn1Core);



/**
 * ASN.1 object enumeration callback.
 *
 * @returns IPRT status code. VINF_SUCCESS continues the enumberation, all
 *          others quit it and is returned to the caller's caller.
 * @param   pAsn1Core           The ASN.1 object we're called back about.
 * @param   pszName             The member name. Array member names ends with
 *                              '[#]'.
 * @param   uDepth              The current depth.
 * @param   pvUser              Callback user parameter.
 */
typedef DECLCALLBACKTYPE(int, FNRTASN1ENUMCALLBACK,(struct RTASN1CORE *pAsn1Core, const char *pszName, uint32_t uDepth,
                                                    void *pvUser));
/** Pointer to an ASN.1 object enumeration callback. */
typedef FNRTASN1ENUMCALLBACK *PFNRTASN1ENUMCALLBACK;

/**
 * ASN.1 object encoding writer callback.
 *
 * @returns IPRT status code.
 * @param   pvBuf               Pointer to the bytes to output.
 * @param   cbToWrite           The number of bytes to write.
 * @param   pvUser              Callback user parameter.
 * @param   pErrInfo            Where to store extended error info. Optional.
 */
typedef DECLCALLBACKTYPE(int, FNRTASN1ENCODEWRITER,(const void *pvBuf, size_t cbToWrite, void *pvUser, PRTERRINFO pErrInfo));
/** Pointer to an ASN.1 encoding writer callback. */
typedef FNRTASN1ENCODEWRITER *PFNRTASN1ENCODEWRITER;

/** @name ASN.1 Vtable Method Types
 * @{ */

/**
 * Destructor.
 *
 * RTAsn1Destroy will first destroy all children by recursive calls to pfnEnum,
 * afterwards it will call this method to release any memory or other resources
 * associated with this object.  The memory backing the object structure shall
 * not be freed by this method.
 *
 * @param   pThisCore       Pointer to the ASN.1 core to destroy.
 */
typedef DECLCALLBACKTYPE(void, FNRTASN1COREVTDTOR,(PRTASN1CORE pThisCore));
/** Pointer to a FNRTASN1COREVTDTOR method. */
typedef FNRTASN1COREVTDTOR *PFNRTASN1COREVTDTOR;

/**
 * Enumerate members (not necessary for primitive objects).
 *
 * @returns IPRT status code, any non VINF_SUCCESS value stems from pfnCallback.
 * @param   pThisCore       Pointer to the ASN.1 core to enumerate members of.
 * @param   pfnCallback     The callback.
 * @param   uDepth          The depth of this object. Children are at +1.
 * @param   pvUser          Callback user argument.
 */
typedef DECLCALLBACKTYPE(int, FNRTASN1COREVTENUM,(PRTASN1CORE pThisCore, PFNRTASN1ENUMCALLBACK pfnCallback,
                                                  uint32_t uDepth, void *pvUser));
/** Pointer to a FNRTASN1COREVTENUM method. */
typedef FNRTASN1COREVTENUM *PFNRTASN1COREVTENUM;

/**
 * Clone method.
 *
 * @param   pThisCore       Pointer to the ASN.1 core to initialize as a clone
 *                          of pSrcClone.  (The caller is responsible for making
 *                          sure there is sufficent space and such.)
 * @param   pSrcCore        The object to clone.
 * @param   pAllocator      The allocator to use.
 */
typedef DECLCALLBACKTYPE(int, FNRTASN1COREVTCLONE,(PRTASN1CORE pThisCore, PCRTASN1CORE pSrcCore,
                                                   PCRTASN1ALLOCATORVTABLE pAllocator));
/** Pointer to a FNRTASN1COREVTCLONE method. */
typedef FNRTASN1COREVTCLONE *PFNRTASN1COREVTCLONE;

/**
 * Compare method.
 *
 * The caller makes sure both cores are present and have the same Vtable.
 *
 * @returns 0 if equal, -1 if @a pLeft is smaller, 1 if @a pLeft is larger.
 * @param   pLeftCore       Pointer to the ASN.1 core of the left side object.
 * @param   pRightCore      Pointer to the ASN.1 core of the right side object.
 */
typedef DECLCALLBACKTYPE(int, FNRTASN1COREVTCOMPARE,(PCRTASN1CORE pLeftCore, PCRTASN1CORE pRightCore));
/** Pointer to a FNRTASN1COREVTCOMPARE method. */
typedef FNRTASN1COREVTCOMPARE *PFNRTASN1COREVTCOMPARE;

/**
 * Check sanity method.
 *
 * @returns IPRT status code.
 * @param   pThisCore       Pointer to the ASN.1 core of the object to check out.
 * @param   fFlags          See RTASN1_CHECK_SANITY_F_XXX.
 * @param   pErrInfo        Where to return additional error details. Optional.
 * @param   pszErrorTag     Tag for the additional error details.
 */
typedef DECLCALLBACKTYPE(int, FNRTASN1COREVTCHECKSANITY,(PCRTASN1CORE pThisCore, uint32_t fFlags,
                                                         PRTERRINFO pErrInfo, const char *pszErrorTag));
/** Pointer to a FNRTASN1COREVTCHECKSANITY method. */
typedef FNRTASN1COREVTCHECKSANITY *PFNRTASN1COREVTCHECKSANITY;

/**
 * Optional encoding preparations.
 *
 * On successful return, the pThisCore->cb value shall be valid and up to date.
 * Will be called for any present object, including ones with default values and
 * similar.
 *
 * @returns IPRT status code
 * @param   pThisCore       Pointer to the ASN.1 core to enumerate members of.
 * @param   fFlags          Encoding flags, RTASN1ENCODE_F_XXX.
 * @param   pErrInfo        Where to return extra error information. Optional.
 */
typedef DECLCALLBACKTYPE(int, FNRTASN1COREVTENCODEPREP,(PRTASN1CORE pThisCore, uint32_t fFlags, PRTERRINFO pErrInfo));
/** Pointer to a FNRTASN1COREVTENCODEWRITE method. */
typedef FNRTASN1COREVTENCODEPREP *PFNRTASN1COREVTENCODEPREP;

/**
 * Optional encoder writer.
 *
 * This writes the header as well as all the content.  Will be called for any
 * present object, including ones with default values and similar.
 *
 * @returns IPRT status code.
 * @param   pThisCore       Pointer to the ASN.1 core to enumerate members of.
 * @param   fFlags          Encoding flags, RTASN1ENCODE_F_XXX.
 * @param   pfnWriter       The output writer function.
 * @param   pvUser          The user context for the writer function.
 * @param   pErrInfo        Where to return extra error information. Optional.
 */
typedef DECLCALLBACKTYPE(int, FNRTASN1COREVTENCODEWRITE,(PRTASN1CORE pThisCore, uint32_t fFlags, PFNRTASN1ENCODEWRITER pfnWriter,
                                                         void *pvUser, PRTERRINFO pErrInfo));
/** Pointer to a FNRTASN1COREVTENCODEWRITE method. */
typedef FNRTASN1COREVTENCODEWRITE *PFNRTASN1COREVTENCODEWRITE;
/** @} */

/** Mask of common flags. These will be propagated during sanity checking.
 * Bits not in this mask are type specfic. */
#define RTASN1_CHECK_SANITY_F_COMMON_MASK       UINT32_C(0xffff0000)

/**
 * ASN.1 core vtable.
 */
typedef struct RTASN1COREVTABLE
{
    /** The name. */
    const char                 *pszName;
    /** Size of the structure. */
    uint32_t                    cbStruct;
    /** The default tag, UINT8_MAX if not applicable. */
    uint8_t                     uDefaultTag;
    /** The default class and flags.  */
    uint8_t                     fDefaultClass;
    /** Reserved for later / alignment. */
    uint16_t                    uReserved;
    /** @copydoc FNRTASN1COREVTDTOR */
    PFNRTASN1COREVTDTOR         pfnDtor;
    /** @copydoc FNRTASN1COREVTENUM */
    PFNRTASN1COREVTENUM         pfnEnum;
    /** @copydoc FNRTASN1COREVTCLONE */
    PFNRTASN1COREVTCLONE        pfnClone;
    /** @copydoc FNRTASN1COREVTCOMPARE */
    PFNRTASN1COREVTCOMPARE      pfnCompare;
    /** @copydoc FNRTASN1COREVTCHECKSANITY */
    PFNRTASN1COREVTCHECKSANITY  pfnCheckSanity;
    /** @copydoc FNRTASN1COREVTENCODEPREP */
    PFNRTASN1COREVTENCODEPREP   pfnEncodePrep;
    /** @copydoc FNRTASN1COREVTENUM */
    PFNRTASN1COREVTENCODEWRITE  pfnEncodeWrite;
} RTASN1COREVTABLE;
/** Pointer to an ASN.1 allocator vtable. */
typedef struct RTASN1COREVTABLE *PRTASN1COREVTABLE;
/** Pointer to a const ASN.1 allocator vtable. */
typedef RTASN1COREVTABLE const *PCRTASN1COREVTABLE;


/** @name Helper macros for prototyping standard functions for an ASN.1 type.
 * @{ */

#define RTASN1TYPE_STANDARD_PROTOTYPES_NO_GET_CORE(a_TypeNm, a_DeclMacro, a_ImplExtNm) \
    a_DeclMacro(int)  RT_CONCAT(a_ImplExtNm,_Init)(RT_CONCAT(P,a_TypeNm) pThis, PCRTASN1ALLOCATORVTABLE pAllocator); \
    a_DeclMacro(int)  RT_CONCAT(a_ImplExtNm,_Clone)(RT_CONCAT(P,a_TypeNm) pThis, RT_CONCAT(PC,a_TypeNm) pSrc, \
                                                    PCRTASN1ALLOCATORVTABLE pAllocator); \
    a_DeclMacro(void) RT_CONCAT(a_ImplExtNm,_Delete)(RT_CONCAT(P,a_TypeNm) pThis); \
    a_DeclMacro(int)  RT_CONCAT(a_ImplExtNm,_Enum)(RT_CONCAT(P,a_TypeNm) pThis, PFNRTASN1ENUMCALLBACK pfnCallback, \
                                                   uint32_t uDepth, void *pvUser); \
    a_DeclMacro(int)  RT_CONCAT(a_ImplExtNm,_Compare)(RT_CONCAT(PC,a_TypeNm) pLeft, RT_CONCAT(PC,a_TypeNm) pRight); \
    a_DeclMacro(int)  RT_CONCAT(a_ImplExtNm,_DecodeAsn1)(PRTASN1CURSOR pCursor, uint32_t fFlags, RT_CONCAT(P,a_TypeNm) pThis,\
                                                         const char *pszErrorTag); \
    a_DeclMacro(int)  RT_CONCAT(a_ImplExtNm,_CheckSanity)(RT_CONCAT(PC,a_TypeNm) pThis, uint32_t fFlags, \
                                                         PRTERRINFO pErrInfo, const char *pszErrorTag)


#define RTASN1TYPE_STANDARD_PROTOTYPES(a_TypeNm, a_DeclMacro, a_ImplExtNm, a_Asn1CoreNm) \
    DECL_FORCE_INLINE(PRTASN1CORE) RT_CONCAT(a_ImplExtNm,_GetAsn1Core)(RT_CONCAT(PC,a_TypeNm) pThis) \
    { return (PRTASN1CORE)&pThis->a_Asn1CoreNm; } \
    DECLINLINE(bool) RT_CONCAT(a_ImplExtNm,_IsPresent)(RT_CONCAT(PC,a_TypeNm) pThis) \
    { return pThis && RTASN1CORE_IS_PRESENT(&pThis->a_Asn1CoreNm); } \
    RTASN1TYPE_STANDARD_PROTOTYPES_NO_GET_CORE(a_TypeNm, a_DeclMacro, a_ImplExtNm)


/** Aliases two ASN.1 types, no method aliases. */
#define RTASN1TYPE_ALIAS_TYPE_ONLY(a_TypeNm, a_AliasType) \
    typedef a_AliasType a_TypeNm; \
    typedef a_TypeNm *RT_CONCAT(P,a_TypeNm); \
    typedef a_TypeNm const *RT_CONCAT(PC,a_TypeNm)

/** Aliases two ASN.1 types and methods. */
#define RTASN1TYPE_ALIAS(a_TypeNm, a_AliasType, a_ImplExtNm, a_AliasExtNm) \
    typedef a_AliasType a_TypeNm; \
    typedef a_TypeNm *RT_CONCAT(P,a_TypeNm); \
    \
    DECLINLINE(PRTASN1CORE) RT_CONCAT(a_ImplExtNm,_GetAsn1Core)(a_TypeNm const *pThis) \
    { return RT_CONCAT(a_AliasExtNm,_GetAsn1Core)(pThis); } \
    DECLINLINE(bool) RT_CONCAT(a_ImplExtNm,_IsPresent)(a_TypeNm const *pThis) \
    { return RT_CONCAT(a_AliasExtNm,_IsPresent)(pThis); } \
    \
    DECLINLINE(int)  RT_CONCAT(a_ImplExtNm,_Init)(RT_CONCAT(P,a_TypeNm) pThis, PCRTASN1ALLOCATORVTABLE pAllocator) \
    { return RT_CONCAT(a_AliasExtNm,_Init)(pThis, pAllocator); } \
    DECLINLINE(int)  RT_CONCAT(a_ImplExtNm,_Clone)(RT_CONCAT(P,a_TypeNm) pThis, a_TypeNm const *pSrc, \
                                                   PCRTASN1ALLOCATORVTABLE pAllocator) \
    { return RT_CONCAT(a_AliasExtNm,_Clone)(pThis, pSrc, pAllocator); } \
    DECLINLINE(void) RT_CONCAT(a_ImplExtNm,_Delete)(RT_CONCAT(P,a_TypeNm) pThis) \
    { RT_CONCAT(a_AliasExtNm,_Delete)(pThis); } \
    DECLINLINE(int)  RT_CONCAT(a_ImplExtNm,_Enum)(a_TypeNm *pThis, PFNRTASN1ENUMCALLBACK pfnCallback, \
                                                  uint32_t uDepth, void *pvUser) \
    { return RT_CONCAT(a_AliasExtNm,_Enum)(pThis, pfnCallback, uDepth, pvUser); } \
    DECLINLINE(int)  RT_CONCAT(a_ImplExtNm,_Compare)(a_TypeNm const *pLeft, a_TypeNm const *pRight) \
    { return RT_CONCAT(a_AliasExtNm,_Compare)(pLeft, pRight); } \
    DECLINLINE(int)  RT_CONCAT(a_ImplExtNm,_DecodeAsn1)(PRTASN1CURSOR pCursor, uint32_t fFlags, RT_CONCAT(P,a_TypeNm) pThis,\
                                                        const char *pszErrorTag) \
    { return RT_CONCAT(a_AliasExtNm,_DecodeAsn1)(pCursor, fFlags, pThis, pszErrorTag); } \
    DECLINLINE(int)  RT_CONCAT(a_ImplExtNm,_CheckSanity)(a_TypeNm const *pThis, uint32_t fFlags, \
                                                         PRTERRINFO pErrInfo, const char *pszErrorTag) \
    { return RT_CONCAT(a_AliasExtNm,_CheckSanity)(pThis, fFlags, pErrInfo, pszErrorTag); } \
    \
    typedef a_TypeNm const *RT_CONCAT(PC,a_TypeNm)

/** @} */


/**
 * Core ASN.1 structure for storing encoding details and data location.
 *
 * This is used as a 'parent' for all other decoded ASN.1 based structures.
 */
typedef struct RTASN1CORE
{
    /** The tag.
     * @remarks 32-bit should be enough for everyone... We don't currently
     *          implement decoding tags larger than 30 anyway. :-) */
    uint32_t                uTag;
    /** Tag class and flags (ASN1_TAGCLASS_XXX and ASN1_TAGFLAG_XXX). */
    uint8_t                 fClass;
    /** The real tag value for IMPLICT tag overrides. */
    uint8_t                 uRealTag;
    /** The real class value for IMPLICT tag overrides. */
    uint8_t                 fRealClass;
    /** The size of the tag and length ASN.1 header. */
    uint8_t                 cbHdr;
    /** Length.  */
    uint32_t                cb;
    /** IPRT flags (RTASN1CORE_F_XXX). */
    uint32_t                fFlags;
    /** Pointer to the data.
     * After decoding this generally points to the encoded data content.  When
     * preparting something for encoding or otherwise constructing things in memory,
     * this generally points heap memory or read-only constants.
     * @sa RTAsn1ContentAllocZ,  RTAsn1ContentReallocZ, RTAsn1ContentDup,
     *     RTAsn1ContentFree. */
    RTCPTRUNION             uData;
    /** Pointer to the virtual method table for this object. Optional. */
    PCRTASN1COREVTABLE      pOps;
} RTASN1CORE;
/** The Vtable for a RTASN1CORE structure when not in some way use used as a
 *  parent type/class. */
extern RTDATADECL(RTASN1COREVTABLE const) g_RTAsn1Core_Vtable;

RTASN1TYPE_STANDARD_PROTOTYPES_NO_GET_CORE(RTASN1CORE, RTDECL, RTAsn1Core);

/** @name RTASN1CORE_F_XXX - Flags for RTASN1CORE::fFlags
 * @{ */
/** Present/valid. */
#define RTASN1CORE_F_PRESENT            RT_BIT_32(0)
/** Not present in stream, using default value. */
#define RTASN1CORE_F_DEFAULT            RT_BIT_32(1)
/** The tag was overriden by an implict context tag or some such thing,
 * RTASN1CORE::uImplicitTag hold the universal tag value if one exists. */
#define RTASN1CORE_F_TAG_IMPLICIT       RT_BIT_32(2)
/** Primitive tag with the corresponding RTASN1XXX struct. */
#define RTASN1CORE_F_PRIMITE_TAG_STRUCT RT_BIT_32(3)
/** Dummy node typically used with choices, has children, not encoded, must be
 * ignored. */
#define RTASN1CORE_F_DUMMY              RT_BIT_32(4)
/** Allocated content (pointed to by uData).
 * The content should is still be considered 104% read-only by anyone other
 * than then type methods (pOps and associates). */
#define RTASN1CORE_F_ALLOCATED_CONTENT  RT_BIT_32(5)
/** Decoded content (pointed to by uData).
 * Mutually exclusive with RTASN1CORE_F_ALLOCATED_CONTENT.  If neither is
 * set, uData might be NULL or point to some shared static memory for
 * frequently used values. */
#define RTASN1CORE_F_DECODED_CONTENT    RT_BIT_32(6)
/** Indefinite length, still pending. */
#define RTASN1CORE_F_INDEFINITE_LENGTH  RT_BIT_32(7)
/** @} */


/** Checks whether an ASN.1 core object present in some way (default data,
 *  decoded data, ...). */
#define RTASN1CORE_IS_PRESENT(a_pAsn1Core)          ( RT_BOOL((a_pAsn1Core)->fFlags) )

/** Checks whether an ASN.1 core object is a dummy object (and is present). */
#define RTASN1CORE_IS_DUMMY(a_pAsn1Core)            ( RT_BOOL((a_pAsn1Core)->fFlags & RTASN1CORE_F_DUMMY) )

/**
 * Calculates pointer to the raw ASN.1 record.
 *
 * ASSUMES that it's decoded content and that cbHdr and uData are both valid.
 *
 * @returns Byte pointer to the first tag byte.
 * @param   a_pAsn1Core     The ASN.1 core.
 */
#define RTASN1CORE_GET_RAW_ASN1_PTR(a_pAsn1Core)    ( (a_pAsn1Core)->uData.pu8 - (a_pAsn1Core)->cbHdr )

/**
 * Calculates the length of the raw ASN.1 record to go with the
 * RTASN1CORE_GET_RAW_ASN1_PTR() result.
 *
 * ASSUMES that it's decoded content and that cbHdr and uData are both valid.
 *
 * @returns Size in bytes (uint32_t).
 * @param   a_pAsn1Core     The ASN.1 core.
 */
#define RTASN1CORE_GET_RAW_ASN1_SIZE(a_pAsn1Core)    ( (a_pAsn1Core)->cbHdr + (a_pAsn1Core)->cb )

/**
 * Retrievs the tag or implicit tag depending on the RTASN1CORE_F_TAG_IMPLICIT
 * flag.
 *
 * @returns The ASN.1 tag of the object.
 * @param   a_pAsn1Core     The ASN.1 core.
 */
#define RTASN1CORE_GET_TAG(a_pAsn1Core)     ( !((a_pAsn1Core)->fFlags & RTASN1CORE_F_TAG_IMPLICIT) ? (a_pAsn1Core)->uTag : (a_pAsn1Core)->uRealTag )


DECL_FORCE_INLINE(PRTASN1CORE) RTAsn1Core_GetAsn1Core(PCRTASN1CORE pThis)
{
    return (PRTASN1CORE)pThis;
}


DECL_FORCE_INLINE(bool) RTAsn1Core_IsPresent(PCRTASN1CORE pThis)
{
    return pThis && RTASN1CORE_IS_PRESENT(pThis);
}


RTDECL(int) RTAsn1Core_InitEx(PRTASN1CORE pAsn1Core, uint32_t uTag, uint8_t fClass, PCRTASN1COREVTABLE pOps, uint32_t fFlags);
/**
 * Initialize the ASN.1 core object representation to a default value.
 *
 * @returns VINF_SUCCESS
 * @param   pAsn1Core       The ASN.1 core.
 * @param   uTag            The tag number.
 * @param   fClass          The tag class and flags.
 */
RTDECL(int) RTAsn1Core_InitDefault(PRTASN1CORE pAsn1Core, uint32_t uTag, uint8_t fClass);
RTDECL(int) RTAsn1Core_CloneContent(PRTASN1CORE pThis, PCRTASN1CORE pSrc, PCRTASN1ALLOCATORVTABLE pAllocator);
RTDECL(int) RTAsn1Core_CloneNoContent(PRTASN1CORE pThis, PCRTASN1CORE pSrc);
RTDECL(int) RTAsn1Core_SetTagAndFlags(PRTASN1CORE pAsn1Core, uint32_t uTag, uint8_t fClass);
RTDECL(int) RTAsn1Core_ChangeTag(PRTASN1CORE pAsn1Core, uint32_t uTag);
RTDECL(void) RTAsn1Core_ResetImplict(PRTASN1CORE pThis);
RTDECL(int) RTAsn1Core_CompareEx(PCRTASN1CORE pLeft, PCRTASN1CORE pRight, bool fIgnoreTagAndClass);


/**
 * Dummy ASN.1 object for use in choices and similar non-sequence structures.
 *
 * This allows hooking up destructors, enumerators and such, as well as not
 * needing custom code for sequence-of / set-of collections.
 */
typedef struct RTASN1DUMMY
{
    /** Core ASN.1. */
    RTASN1CORE      Asn1Core;
} RTASN1DUMMY;
/** Pointer to a dummy record. */
typedef RTASN1DUMMY *PRTASN1DUMMY;


/**
 * Initalizes a dummy ASN.1 object.
 *
 * @returns VINF_SUCCESS.
 * @param   pThis               The dummy object.
 */
RTDECL(int) RTAsn1Dummy_InitEx(PRTASN1DUMMY pThis);

/**
 * Standard compliant initalizer.
 *
 * @returns VINF_SUCCESS.
 * @param   pThis               The dummy object.
 * @param   pAllocator          Ignored.
 */
DECLINLINE(int) RTAsn1Dummy_Init(PRTASN1DUMMY pThis, PCRTASN1ALLOCATORVTABLE pAllocator)
{
    NOREF(pAllocator);
    return RTAsn1Dummy_InitEx(pThis);
}


/**
 * ASN.1 sequence core (IPRT representation).
 */
typedef struct RTASN1SEQUENCECORE
{
    /** Core ASN.1 encoding details. */
    RTASN1CORE      Asn1Core;
} RTASN1SEQUENCECORE;
/** Pointer to an ASN.1 sequence core (IPRT representation). */
typedef RTASN1SEQUENCECORE *PRTASN1SEQUENCECORE;
/** Pointer to a const ASN.1 sequence core (IPRT representation). */
typedef RTASN1SEQUENCECORE const *PCRTASN1SEQUENCECORE;

RTDECL(int) RTAsn1SequenceCore_Init(PRTASN1SEQUENCECORE pSeqCore, PCRTASN1COREVTABLE pVtable);
RTDECL(int) RTAsn1SequenceCore_Clone(PRTASN1SEQUENCECORE pSeqCore, PCRTASN1COREVTABLE pVtable, PCRTASN1SEQUENCECORE pSrc);

/**
 * ASN.1 sequence-of core (IPRT representation).
 */
#if 0
typedef struct RTASN1SEQOFCORE
{
    /** Core ASN.1 encoding details. */
    RTASN1CORE      Asn1Core;
} RTASN1SEQUENCECORE;
/** Pointer to an ASN.1 sequence-of core (IPRT representation). */
typedef RTASN1SEQUENCECORE *PRTASN1SEQUENCECORE;
/** Pointer to a const ASN.1 sequence-of core (IPRT representation). */
typedef RTASN1SEQUENCECORE const *PCRTASN1SEQUENCECORE;
#else
# define RTASN1SEQOFCORE        RTASN1SEQUENCECORE
# define PRTASN1SEQOFCORE       PRTASN1SEQUENCECORE
# define PCRTASN1SEQOFCORE      PCRTASN1SEQUENCECORE
#endif
RTDECL(int) RTAsn1SeqOfCore_Init(PRTASN1SEQOFCORE pThis, PCRTASN1COREVTABLE pVtable);
RTDECL(int) RTAsn1SeqOfCore_Clone(PRTASN1SEQOFCORE pThis, PCRTASN1COREVTABLE pVtable, PCRTASN1SEQOFCORE pSrc);


/** Defines the typedefs and prototypes for a generic sequence-of/set-of type. */
#define RTASN1_IMPL_GEN_SEQ_OR_SET_OF_TYPEDEFS_AND_PROTOS(a_CoreType, a_CoreMember, \
                                                          a_ThisType, a_ItemType, a_DeclMacro, a_ImplExtNm) \
    typedef struct a_ThisType \
    { \
        /** Sequence/set core. */ \
        a_CoreType                  a_CoreMember; \
        /** The array allocation tracker. */ \
        RTASN1ARRAYALLOCATION       Allocation; \
        /** Items in the array. */ \
        uint32_t                    cItems; \
        /** Array. */ \
        RT_CONCAT(P,a_ItemType)    *papItems; \
    } a_ThisType; \
    typedef a_ThisType *RT_CONCAT(P,a_ThisType); \
    typedef a_ThisType const *RT_CONCAT(PC,a_ThisType); \
    a_DeclMacro(int)  RT_CONCAT(a_ImplExtNm,_Erase)(RT_CONCAT(P,a_ThisType) pThis, uint32_t iPosition); \
    a_DeclMacro(int)  RT_CONCAT(a_ImplExtNm,_InsertEx)(RT_CONCAT(P,a_ThisType) pThis, uint32_t iPosition, \
                                                       RT_CONCAT(PC,a_ItemType) pToClone, \
                                                       PCRTASN1ALLOCATORVTABLE pAllocator, uint32_t *piActualPos); \
    /** Appends entry with default content, returns index or negative error code. */ \
    DECLINLINE(int32_t) RT_CONCAT(a_ImplExtNm,_Append)(RT_CONCAT(P,a_ThisType) pThis) \
    { \
        uint32_t uPos = pThis->cItems; \
        int rc = RT_CONCAT(a_ImplExtNm,_InsertEx)(pThis, uPos, NULL /*pToClone*/, pThis->Allocation.pAllocator, &uPos); \
        if (RT_SUCCESS(rc)) \
            return (int32_t)uPos; \
        return rc; \
    } \
    RTASN1TYPE_STANDARD_PROTOTYPES(a_ThisType, a_DeclMacro, a_ImplExtNm, a_CoreMember.Asn1Core)

/** Defines the typedefs and prototypes for a generic sequence-of type. */
#define RTASN1_IMPL_GEN_SEQ_OF_TYPEDEFS_AND_PROTOS(a_SeqOfType, a_ItemType, a_DeclMacro, a_ImplExtNm) \
    RTASN1_IMPL_GEN_SEQ_OR_SET_OF_TYPEDEFS_AND_PROTOS(RTASN1SEQUENCECORE, SeqCore, a_SeqOfType, a_ItemType, a_DeclMacro, a_ImplExtNm)


/**
 * ASN.1 set core (IPRT representation).
 */
typedef struct RTASN1SETCORE
{
    /** Core ASN.1 encoding details. */
    RTASN1CORE      Asn1Core;
} RTASN1SETCORE;
/** Pointer to an ASN.1 set core (IPRT representation). */
typedef RTASN1SETCORE *PRTASN1SETCORE;
/** Pointer to a const ASN.1 set core (IPRT representation). */
typedef RTASN1SETCORE const *PCRTASN1SETCORE;

RTDECL(int) RTAsn1SetCore_Init(PRTASN1SETCORE pThis, PCRTASN1COREVTABLE pVtable);
RTDECL(int) RTAsn1SetCore_Clone(PRTASN1SETCORE pThis, PCRTASN1COREVTABLE pVtable, PCRTASN1SETCORE pSrc);

/**
 * ASN.1 set-of core (IPRT representation).
 */
#if 0
typedef struct RTASN1SETOFCORE
{
    /** Core ASN.1 encoding details. */
    RTASN1CORE      Asn1Core;
} RTASN1SETUENCECORE;
/** Pointer to an ASN.1 set-of core (IPRT representation). */
typedef RTASN1SETUENCECORE *PRTASN1SETUENCECORE;
/** Pointer to a const ASN.1 set-of core (IPRT representation). */
typedef RTASN1SETUENCECORE const *PCRTASN1SETUENCECORE;
#else
# define RTASN1SETOFCORE        RTASN1SETCORE
# define PRTASN1SETOFCORE       PRTASN1SETCORE
# define PCRTASN1SETOFCORE      PCRTASN1SETCORE
#endif
RTDECL(int) RTAsn1SetOfCore_Init(PRTASN1SETOFCORE pThis, PCRTASN1COREVTABLE pVtable);
RTDECL(int) RTAsn1SetOfCore_Clone(PRTASN1SETOFCORE pThis, PCRTASN1COREVTABLE pVtable, PCRTASN1SETOFCORE pSrc);


/** Defines the typedefs and prototypes for a generic set-of type. */
#define RTASN1_IMPL_GEN_SET_OF_TYPEDEFS_AND_PROTOS(a_SetOfType, a_ItemType, a_DeclMacro, a_ImplExtNm) \
    RTASN1_IMPL_GEN_SEQ_OR_SET_OF_TYPEDEFS_AND_PROTOS(RTASN1SETCORE, SetCore, a_SetOfType, a_ItemType, a_DeclMacro, a_ImplExtNm)


/*
 * Declare sets and sequences of the core structure.
 */
RTASN1_IMPL_GEN_SEQ_OF_TYPEDEFS_AND_PROTOS(RTASN1SEQOFCORES, RTASN1CORE, RTDECL, RTAsn1SeqOfCores);
RTASN1_IMPL_GEN_SET_OF_TYPEDEFS_AND_PROTOS(RTASN1SETOFCORES, RTASN1CORE, RTDECL, RTAsn1SetOfCores);


/**
 * ASN.1 null (IPRT representation).
 */
typedef struct RTASN1NULL
{
    /** Core ASN.1 encoding details. */
    RTASN1CORE      Asn1Core;
} RTASN1NULL;
/** Pointer to an ASN.1 null (IPRT representation). */
typedef RTASN1NULL *PRTASN1NULL;
/** Pointer to a const ASN.1 null (IPRT representation). */
typedef RTASN1NULL const *PCRTASN1NULL;
/** The Vtable for a RTASN1NULL structure. */
extern RTDATADECL(RTASN1COREVTABLE const) g_RTAsn1Null_Vtable;

RTASN1TYPE_STANDARD_PROTOTYPES(RTASN1NULL, RTDECL, RTAsn1Null, Asn1Core);


/**
 * ASN.1 integer (IPRT representation).
 */
typedef struct RTASN1INTEGER
{
    /** Core ASN.1 encoding details. */
    RTASN1CORE      Asn1Core;
    /** The unsigned C representation of the 64 least significant bits.
     * @note A ASN.1 integer doesn't define signed/unsigned and can have any
     *       length you like.  Thus, the user needs to check the size and
     *       preferably use the access APIs for signed numbers. */
    RTUINT64U       uValue;
} RTASN1INTEGER;
/** Pointer to an ASN.1 integer (IPRT representation). */
typedef RTASN1INTEGER *PRTASN1INTEGER;
/** Pointer to a const ASN.1 integer (IPRT representation). */
typedef RTASN1INTEGER const *PCRTASN1INTEGER;
/** The Vtable for a RTASN1INTEGER structure. */
extern RTDATADECL(RTASN1COREVTABLE const) g_RTAsn1Integer_Vtable;

RTASN1TYPE_STANDARD_PROTOTYPES(RTASN1INTEGER, RTDECL, RTAsn1Integer, Asn1Core);

/**
 * Initializes an interger object to a default value.
 * @returns VINF_SUCCESS.
 * @param   pInteger            The integer object representation.
 * @param   uValue              The default value (unsigned 64-bit).
 * @param   pAllocator          The allocator (pro forma).
 */
RTDECL(int) RTAsn1Integer_InitDefault(PRTASN1INTEGER pInteger, uint64_t uValue, PCRTASN1ALLOCATORVTABLE pAllocator);

RTDECL(int) RTAsn1Integer_InitU64(PRTASN1INTEGER pThis, uint64_t uValue, PCRTASN1ALLOCATORVTABLE pAllocator);

/**
 * Get the most significat bit that's set (1).
 *
 * @returns 0-base bit number, -1 if all clear.
 * @param   pInteger            The integer to check.
 */
RTDECL(int32_t) RTAsn1Integer_UnsignedLastBit(PCRTASN1INTEGER pInteger);

/**
 * Compares two ASN.1 unsigned integers.
 *
 * @returns 0 if equal, -1 if @a pLeft is smaller, 1 if @a pLeft is larger.
 * @param   pLeft               The first ASN.1 integer.
 * @param   pRight              The second ASN.1 integer.
 */
RTDECL(int) RTAsn1Integer_UnsignedCompare(PCRTASN1INTEGER pLeft, PCRTASN1INTEGER pRight);

/**
 * Compares an ASN.1 unsigned integer with a uint64_t.
 *
 * @returns 0 if equal, -1 if @a pInteger is smaller, 1 if @a pInteger is
 *          larger.
 * @param   pInteger            The ASN.1 integer to treat as unsigned.
 * @param   u64Const            The uint64_t constant to compare with.
 */
RTDECL(int) RTAsn1Integer_UnsignedCompareWithU64(PCRTASN1INTEGER pInteger, uint64_t u64Const);

/**
 * Compares an ASN.1 unsigned integer with a uint32_t.
 *
 * @returns 0 if equal, -1 if @a pInteger is smaller, 1 if @a pInteger is
 *          larger.
 * @param   pInteger            The ASN.1 integer to treat as unsigned.
 * @param   u32Const            The uint32_t constant to compare with.
 * @remarks We don't bother with U16 and U8 variants, just use this instead.
 */
RTDECL(int) RTAsn1Integer_UnsignedCompareWithU32(PCRTASN1INTEGER pInteger, uint32_t u32Const);


/**
 * Initializes a big integer number from an ASN.1 integer.
 *
 * @returns IPRT status code.
 * @param   pInteger            The ASN.1 integer.
 * @param   pBigNum             The big integer number structure to initialize.
 * @param   fBigNumInit         Subset of RTBIGNUMINIT_F_XXX that concerns
 *                              senitivity, signedness and endianness.
 */
RTDECL(int) RTAsn1Integer_ToBigNum(PCRTASN1INTEGER pInteger, PRTBIGNUM pBigNum, uint32_t fBigNumInit);
RTDECL(int) RTAsn1Integer_FromBigNum(PRTASN1INTEGER pThis, PCRTBIGNUM pBigNum, PCRTASN1ALLOCATORVTABLE pAllocator);

/**
 * Converts the integer to a string.
 *
 * This will produce a hex represenation of the number.  If it fits in 64-bit, a
 * C style hex number will be produced.  If larger than 64-bit, it will be
 * printed as a space separated string of hex bytes.
 *
 * @returns IPRT status code.
 * @param   pThis               The ASN.1 integer.
 * @param   pszBuf              The output buffer.
 * @param   cbBuf               The buffer size.
 * @param   fFlags              Flags reserved for future exploits. MBZ.
 * @param   pcbActual           Where to return the amount of buffer space used
 *                              (i.e. including terminator). Optional.
 *
 * @remarks Currently assume unsigned number.
 */
RTDECL(int) RTAsn1Integer_ToString(PCRTASN1INTEGER pThis, char *pszBuf, size_t cbBuf, uint32_t fFlags, size_t *pcbActual);

RTASN1_IMPL_GEN_SEQ_OF_TYPEDEFS_AND_PROTOS(RTASN1SEQOFINTEGERS, RTASN1INTEGER, RTDECL, RTAsn1SeqOfIntegers);
RTASN1_IMPL_GEN_SET_OF_TYPEDEFS_AND_PROTOS(RTASN1SETOFINTEGERS, RTASN1INTEGER, RTDECL, RTAsn1SetOfIntegers);



/**
 * ASN.1 boolean (IPRT representation).
 */
typedef struct RTASN1BOOLEAN
{
    /** Core ASN.1 encoding details. */
    RTASN1CORE                          Asn1Core;
    /** The boolean value. */
    bool                                fValue;
} RTASN1BOOLEAN;
/** Pointer to the IPRT representation of an ASN.1 boolean. */
typedef RTASN1BOOLEAN *PRTASN1BOOLEAN;
/** Pointer to the const IPRT representation of an ASN.1 boolean. */
typedef RTASN1BOOLEAN const *PCRTASN1BOOLEAN;
/** The Vtable for a RTASN1BOOLEAN structure. */
extern RTDATADECL(RTASN1COREVTABLE const) g_RTAsn1Boolean_Vtable;

RTASN1TYPE_STANDARD_PROTOTYPES(RTASN1BOOLEAN, RTDECL, RTAsn1Boolean, Asn1Core);

/**
 * Initializes a boolean object to a default value.
 * @returns VINF_SUCCESS
 * @param   pBoolean            The boolean object representation.
 * @param   fValue              The default value.
 * @param   pAllocator          The allocator (pro forma).
 */
RTDECL(int) RTAsn1Boolean_InitDefault(PRTASN1BOOLEAN pBoolean, bool fValue, PCRTASN1ALLOCATORVTABLE pAllocator);
RTDECL(int) RTAsn1Boolean_Set(PRTASN1BOOLEAN pThis, bool fValue);

RTASN1_IMPL_GEN_SEQ_OF_TYPEDEFS_AND_PROTOS(RTASN1SEQOFBOOLEANS, RTASN1BOOLEAN, RTDECL, RTAsn1SeqOfBooleans);
RTASN1_IMPL_GEN_SET_OF_TYPEDEFS_AND_PROTOS(RTASN1SETOFBOOLEANS, RTASN1BOOLEAN, RTDECL, RTAsn1SetOfBooleans);



/**
 * ASN.1 UTC and Generalized Time (IPRT representation).
 *
 * The two time types only differs in the precision the render (UTC time being
 * the one for which you go "WTF were they thinking?!!" for in 2014).
 */
typedef struct RTASN1TIME
{
    /** The core structure, either ASN1_TAG_UTC_TIME or
     *  ASN1_TAG_GENERALIZED_TIME. */
    RTASN1CORE                          Asn1Core;
    /** The exploded time. */
    RTTIME                              Time;
} RTASN1TIME;
/** Pointer to an IPRT representation of ASN.1 UTC/Generalized time. */
typedef RTASN1TIME *PRTASN1TIME;
/** Pointer to a const IPRT representation of ASN.1 UTC/Generalized time. */
typedef RTASN1TIME const *PCRTASN1TIME;
/** The Vtable for a RTASN1TIME structure. */
extern RTDATADECL(RTASN1COREVTABLE const) g_RTAsn1Time_Vtable;

RTASN1TYPE_STANDARD_PROTOTYPES(RTASN1TIME, RTDECL, RTAsn1Time, Asn1Core);

RTASN1TYPE_STANDARD_PROTOTYPES(RTASN1TIME, RTDECL, RTAsn1UtcTime, Asn1Core);
RTASN1TYPE_STANDARD_PROTOTYPES(RTASN1TIME, RTDECL, RTAsn1GeneralizedTime, Asn1Core);

/**
 * Compares two ASN.1 time values.
 *
 * @returns 0 if equal, -1 if @a pLeft is smaller, 1 if @a pLeft is larger.
 * @param   pLeft               The first ASN.1 time object.
 * @param   pTsRight            The second time to compare.
 */
RTDECL(int) RTAsn1Time_CompareWithTimeSpec(PCRTASN1TIME pLeft, PCRTTIMESPEC pTsRight);

/**
 * Extended init function that lets you select the kind of time object (UTC or
 * generalized).
 */
RTDECL(int) RTAsn1Time_InitEx(PRTASN1TIME pThis, uint32_t uTag, PCRTASN1ALLOCATORVTABLE pAllocator);

/**
 * Combines RTAsn1Time_InitEx() and RTAsn1Time_SetTime().
 */
RTDECL(int) RTAsn1Time_InitWithTime(PRTASN1TIME pThis, uint32_t uTag, PCRTASN1ALLOCATORVTABLE pAllocator, PCRTTIME pTime);

/**
 * Sets the ASN.1 time value to @a pTime.
 *
 * @returns IPRT status code.
 * @param   pThis               The ASN.1 time object to modify.
 * @param   pAllocator          The allocator to use.
 * @param   pTime               The time to set.
 */
RTDECL(int) RTAsn1Time_SetTime(PRTASN1TIME pThis, PCRTASN1ALLOCATORVTABLE pAllocator, PCRTTIME pTime);

/**
 * Sets the ASN.1 time value to @a pTimeSpec.
 *
 * @returns IPRT status code.
 * @param   pThis               The ASN.1 time object to modify.
 * @param   pAllocator          The allocator to use.
 * @param   pTimeSpec           The time to set.
 */
RTDECL(int) RTAsn1Time_SetTimeSpec(PRTASN1TIME pThis, PCRTASN1ALLOCATORVTABLE pAllocator, PCRTTIMESPEC pTimeSpec);

/** @name Predicate macros for determing the exact type of RTASN1TIME.
 * @{ */
/** True if UTC time. */
#define RTASN1TIME_IS_UTC_TIME(a_pAsn1Time)         ((a_pAsn1Time)->Asn1Core.uTag == ASN1_TAG_UTC_TIME)
/** True if generalized time. */
#define RTASN1TIME_IS_GENERALIZED_TIME(a_pAsn1Time) ((a_pAsn1Time)->Asn1Core.uTag == ASN1_TAG_GENERALIZED_TIME)
/** @}  */

RTASN1_IMPL_GEN_SEQ_OF_TYPEDEFS_AND_PROTOS(RTASN1SEQOFTIMES, RTASN1TIME, RTDECL, RTAsn1SeqOfTimes);
RTASN1_IMPL_GEN_SET_OF_TYPEDEFS_AND_PROTOS(RTASN1SETOFTIMES, RTASN1TIME, RTDECL, RTAsn1SetOfTimes);



/**
 * ASN.1 object identifier (IPRT representation).
 */
typedef struct RTASN1OBJID
{
    /** Core ASN.1 encoding details. */
    RTASN1CORE          Asn1Core;
    /** Coverning the paComponents memory allocation if there isn't enough room in
     * szObjId for both the dottet string and the component values. */
    RTASN1ALLOCATION    Allocation;
    /** Pointer to an array with the component values.
     * This may point within szObjId if there is enough space for both there. */
    uint32_t const     *pauComponents;
    /** The number of components in the object identifier.
     * This ASSUMES that nobody will be ever needing more than 255 components.  */
    uint8_t             cComponents;
    /** The dotted string representation of the object identifier.
     * If there is sufficient space after the string, we will place the array that
     * paComponents points to here and/or the raw content bytes (Asn1Core.uData).
     *
     * An analysis of dumpasn1.cfg, hl7.org and our own _OID defines indicates
     * that we need space for at least 10 components and 30-something chars.  We've
     * allocated 87 bytes, which we ASSUME should be enough for everyone. */
    char                szObjId[87];
} RTASN1OBJID;
/** Pointer to an ASN.1 object identifier representation. */
typedef RTASN1OBJID *PRTASN1OBJID;
/** Pointer to a const ASN.1 object identifier representation. */
typedef RTASN1OBJID const *PCRTASN1OBJID;
/** The Vtable for a RTASN1OBJID structure. */
extern RTDATADECL(RTASN1COREVTABLE const) g_RTAsn1ObjId_Vtable;

RTASN1TYPE_STANDARD_PROTOTYPES(RTASN1OBJID, RTDECL, RTAsn1ObjId, Asn1Core);

RTDECL(int) RTAsn1ObjId_InitFromString(PRTASN1OBJID pThis, const char *pszObjId, PCRTASN1ALLOCATORVTABLE pAllocator);
RTDECL(int) RTAsn1ObjId_SetFromString(PRTASN1OBJID pThis, const char *pszObjId, PCRTASN1ALLOCATORVTABLE pAllocator);

/**
 * Compares an ASN.1 object identifier with a dotted object identifier string.
 *
 * @returns 0 if equal, -1 if @a pLeft is smaller, 1 if @a pLeft is larger.
 * @param   pThis               The ASN.1 object identifier.
 * @param   pszRight            The dotted object identifier string.
 */
RTDECL(int) RTAsn1ObjId_CompareWithString(PCRTASN1OBJID pThis, const char *pszRight);

/**
 * Checks if an ASN.1 object identifier starts with the given dotted object
 * identifier string.
 *
 * The matching is only successful if the given string matches matches the last
 * component completely.
 *
 * @returns true / false.
 * @param   pThis               The ASN.1 object identifier.
 * @param   pszStartsWith       The dotted object identifier string.
 */
RTDECL(bool) RTAsn1ObjId_StartsWith(PCRTASN1OBJID pThis, const char *pszStartsWith);

RTDECL(uint8_t) RTAsn1ObjIdCountComponents(PCRTASN1OBJID pThis);
RTDECL(uint32_t) RTAsn1ObjIdGetComponentsAsUInt32(PCRTASN1OBJID pThis, uint8_t iComponent);
RTDECL(uint32_t) RTAsn1ObjIdGetLastComponentsAsUInt32(PCRTASN1OBJID pThis);

RTASN1_IMPL_GEN_SEQ_OF_TYPEDEFS_AND_PROTOS(RTASN1SEQOFOBJIDS, RTASN1OBJID, RTDECL, RTAsn1SeqOfObjIds);
RTASN1_IMPL_GEN_SET_OF_TYPEDEFS_AND_PROTOS(RTASN1SETOFOBJIDS, RTASN1OBJID, RTDECL, RTAsn1SetOfObjIds);
RTASN1_IMPL_GEN_SET_OF_TYPEDEFS_AND_PROTOS(RTASN1SETOFOBJIDSEQS, RTASN1SEQOFOBJIDS, RTDECL, RTAsn1SetOfObjIdSeqs);


/**
 * ASN.1 bit string (IPRT representation).
 */
typedef struct RTASN1BITSTRING
{
    /** Core ASN.1 encoding details. */
    RTASN1CORE          Asn1Core;
    /** The number of bits. */
    uint32_t            cBits;
    /** The max number of bits (given at decoding / construction). */
    uint32_t            cMaxBits;
    /** Pointer to the bits. */
    RTCPTRUNION         uBits;
    /** Pointer to user structure encapsulated in this string, if dynamically
     * allocated the EncapsulatedAllocation member can be used to track it and
     * trigger automatic cleanup on object destruction.  If EncapsulatedAllocation
     * is zero, any object pointed to will only be deleted. */
    PRTASN1CORE         pEncapsulated;
    /** Allocation tracking structure for pEncapsulated. */
    RTASN1ALLOCATION    EncapsulatedAllocation;
} RTASN1BITSTRING;
/** Pointer to the IPRT representation of an ASN.1 bit string. */
typedef RTASN1BITSTRING *PRTASN1BITSTRING;
/** Pointer to the const IPRT representation of an ASN.1 bit string. */
typedef RTASN1BITSTRING const *PCRTASN1BITSTRING;
/** The Vtable for a RTASN1BITSTRING structure. */
extern RTDATADECL(RTASN1COREVTABLE const) g_RTAsn1BitString_Vtable;

RTASN1TYPE_STANDARD_PROTOTYPES(RTASN1BITSTRING, RTDECL, RTAsn1BitString, Asn1Core);

/**
 * Calculates pointer to the first bit.
 *
 * @returns Byte pointer to the first bit.
 * @param   a_pBitString    The ASN.1 bit string.
 */
#define RTASN1BITSTRING_GET_BIT0_PTR(a_pBitString)  ( &(a_pBitString)->Asn1Core.uData.pu8[1] )

/**
 * Calculates the size in bytes.
 *
 * @returns Rounded up size in bytes.
 * @param   a_pBitString    The ASN.1 bit string.
 */
#define RTASN1BITSTRING_GET_BYTE_SIZE(a_pBitString)  ( ((a_pBitString)->cBits + 7U) >> 3 )

RTDECL(int) RTAsn1BitString_InitWithData(PRTASN1BITSTRING pThis, void const *pvSrc, uint32_t cSrcBits,
                                         PCRTASN1ALLOCATORVTABLE pAllocator);
RTDECL(int) RTAsn1BitString_DecodeAsn1Ex(PRTASN1CURSOR pCursor, uint32_t fFlags, uint32_t cMaxBits, PRTASN1BITSTRING pThis,
                                         const char *pszErrorTag);
RTDECL(uint64_t) RTAsn1BitString_GetAsUInt64(PCRTASN1BITSTRING pThis);
RTDECL(int) RTAsn1BitString_RefreshContent(PRTASN1BITSTRING pThis, uint32_t fFlags,
                                           PCRTASN1ALLOCATORVTABLE pAllocator, PRTERRINFO pErrInfo);
RTDECL(bool) RTAsn1BitString_AreContentBitsValid(PCRTASN1BITSTRING pThis, uint32_t fFlags);

RTASN1_IMPL_GEN_SEQ_OF_TYPEDEFS_AND_PROTOS(RTASN1SEQOFBITSTRINGS, RTASN1BITSTRING, RTDECL, RTAsn1SeqOfBitStrings);
RTASN1_IMPL_GEN_SET_OF_TYPEDEFS_AND_PROTOS(RTASN1SETOFBITSTRINGS, RTASN1BITSTRING, RTDECL, RTAsn1SetOfBitStrings);


/**
 * ASN.1 octet string (IPRT representation).
 */
typedef struct RTASN1OCTETSTRING
{
    /** Core ASN.1 encoding details. */
    RTASN1CORE          Asn1Core;
    /** Pointer to user structure encapsulated in this string.
     *
     * If dynamically allocated the EncapsulatedAllocation member can be used to
     * track it and trigger automatic cleanup on object destruction.  If
     * EncapsulatedAllocation is zero, any object pointed to will only be
     * deleted. */
    PRTASN1CORE         pEncapsulated;
    /** Allocation tracking structure for pEncapsulated. */
    RTASN1ALLOCATION    EncapsulatedAllocation;
} RTASN1OCTETSTRING;
/** Pointer to the IPRT representation of an ASN.1 octet string. */
typedef RTASN1OCTETSTRING *PRTASN1OCTETSTRING;
/** Pointer to the const IPRT representation of an ASN.1 octet string. */
typedef RTASN1OCTETSTRING const *PCRTASN1OCTETSTRING;
/** The Vtable for a RTASN1OCTETSTRING structure. */
extern RTDATADECL(RTASN1COREVTABLE const) g_RTAsn1OctetString_Vtable;

RTASN1TYPE_STANDARD_PROTOTYPES(RTASN1OCTETSTRING, RTDECL, RTAsn1OctetString, Asn1Core);

RTDECL(int) RTAsn1OctetString_AllocContent(PRTASN1OCTETSTRING pThis, void const *pvSrc, size_t cb,
                                           PCRTASN1ALLOCATORVTABLE pAllocator);
RTDECL(int) RTAsn1OctetString_SetContent(PRTASN1OCTETSTRING pThis, void const *pvSrc, size_t cbSrc,
                                         PCRTASN1ALLOCATORVTABLE pAllocator);
RTDECL(bool) RTAsn1OctetString_AreContentBytesValid(PCRTASN1OCTETSTRING pThis, uint32_t fFlags);
RTDECL(int) RTAsn1OctetString_RefreshContent(PRTASN1OCTETSTRING pThis, uint32_t fFlags,
                                             PCRTASN1ALLOCATORVTABLE pAllocator, PRTERRINFO pErrInfo);

RTASN1_IMPL_GEN_SEQ_OF_TYPEDEFS_AND_PROTOS(RTASN1SEQOFOCTETSTRINGS, RTASN1OCTETSTRING, RTDECL, RTAsn1SeqOfOctetStrings);
RTASN1_IMPL_GEN_SET_OF_TYPEDEFS_AND_PROTOS(RTASN1SETOFOCTETSTRINGS, RTASN1OCTETSTRING, RTDECL, RTAsn1SetOfOctetStrings);


/**
 * ASN.1 string (IPRT representation).
 * All char string types except 'character string (29)'.
 */
typedef struct RTASN1STRING
{
    /** Core ASN.1 encoding details. */
    RTASN1CORE          Asn1Core;
    /** Allocation tracking for pszUtf8. */
    RTASN1ALLOCATION    Allocation;
    /** If conversion to UTF-8 was requested, we cache that here.  */
    char const         *pszUtf8;
    /** The length (chars, not code points) of the above UTF-8 string if
     * present. */
    uint32_t            cchUtf8;
} RTASN1STRING;
/** Pointer to the IPRT representation of an ASN.1 string. */
typedef RTASN1STRING *PRTASN1STRING;
/** Pointer to the const IPRT representation of an ASN.1 string. */
typedef RTASN1STRING const *PCRTASN1STRING;
/** The Vtable for a RTASN1STRING structure. */
extern RTDATADECL(RTASN1COREVTABLE const) g_RTAsn1String_Vtable;

RTASN1TYPE_STANDARD_PROTOTYPES(RTASN1STRING, RTDECL, RTAsn1String, Asn1Core);

/** @name String type predicate macros.
 * @{ */
#define RTASN1STRING_IS_NUMERIC(a_pAsn1String)    ( RTASN1CORE_GET_TAG(&(a_pAsn1String)->Asn1Core) == ASN1_TAG_NUMERIC_STRING )
#define RTASN1STRING_IS_PRINTABLE(a_pAsn1String)  ( RTASN1CORE_GET_TAG(&(a_pAsn1String)->Asn1Core) == ASN1_TAG_PRINTABLE_STRING )
#define RTASN1STRING_IS_T61(a_pAsn1String)        ( RTASN1CORE_GET_TAG(&(a_pAsn1String)->Asn1Core) == ASN1_TAG_T61_STRING )
#define RTASN1STRING_IS_VIDEOTEX(a_pAsn1String)   ( RTASN1CORE_GET_TAG(&(a_pAsn1String)->Asn1Core) == ASN1_TAG_VIDEOTEX_STRING )
#define RTASN1STRING_IS_VISIBLE(a_pAsn1String)    ( RTASN1CORE_GET_TAG(&(a_pAsn1String)->Asn1Core) == ASN1_TAG_VISIBLE_STRING )
#define RTASN1STRING_IS_IA5(a_pAsn1String)        ( RTASN1CORE_GET_TAG(&(a_pAsn1String)->Asn1Core) == ASN1_TAG_IA5_STRING )
#define RTASN1STRING_IS_GRAPHIC(a_pAsn1String)    ( RTASN1CORE_GET_TAG(&(a_pAsn1String)->Asn1Core) == ASN1_TAG_GRAPHIC_STRING )
#define RTASN1STRING_IS_GENERAL(a_pAsn1String)    ( RTASN1CORE_GET_TAG(&(a_pAsn1String)->Asn1Core) == ASN1_TAG_GENERAL_STRING )
/** UTF-8. */
#define RTASN1STRING_IS_UTF8(a_pAsn1String)       ( RTASN1CORE_GET_TAG(&(a_pAsn1String)->Asn1Core) == ASN1_TAG_UTF8_STRING )
/** UCS-2. */
#define RTASN1STRING_IS_BMP(a_pAsn1String)        ( RTASN1CORE_GET_TAG(&(a_pAsn1String)->Asn1Core) == ASN1_TAG_BMP_STRING )
/** UCS-4. */
#define RTASN1STRING_IS_UNIVERSAL(a_pAsn1String)  ( RTASN1CORE_GET_TAG(&(a_pAsn1String)->Asn1Core) == ASN1_TAG_UNIVERSAL_STRING )
/** @}  */

RTASN1TYPE_STANDARD_PROTOTYPES(RTASN1STRING, RTDECL, RTAsn1NumericString, Asn1Core);
RTASN1TYPE_STANDARD_PROTOTYPES(RTASN1STRING, RTDECL, RTAsn1PrintableString, Asn1Core);
RTASN1TYPE_STANDARD_PROTOTYPES(RTASN1STRING, RTDECL, RTAsn1T61String, Asn1Core);
RTASN1TYPE_STANDARD_PROTOTYPES(RTASN1STRING, RTDECL, RTAsn1VideoTexString, Asn1Core);
RTASN1TYPE_STANDARD_PROTOTYPES(RTASN1STRING, RTDECL, RTAsn1VisibleString, Asn1Core);
RTASN1TYPE_STANDARD_PROTOTYPES(RTASN1STRING, RTDECL, RTAsn1Ia5String, Asn1Core);
RTASN1TYPE_STANDARD_PROTOTYPES(RTASN1STRING, RTDECL, RTAsn1GraphicString, Asn1Core);
RTASN1TYPE_STANDARD_PROTOTYPES(RTASN1STRING, RTDECL, RTAsn1GeneralString, Asn1Core);
RTASN1TYPE_STANDARD_PROTOTYPES(RTASN1STRING, RTDECL, RTAsn1Utf8String, Asn1Core);
RTASN1TYPE_STANDARD_PROTOTYPES(RTASN1STRING, RTDECL, RTAsn1BmpString, Asn1Core);
RTASN1TYPE_STANDARD_PROTOTYPES(RTASN1STRING, RTDECL, RTAsn1UniversalString, Asn1Core);

RTDECL(int) RTAsn1String_InitWithValue(PRTASN1STRING pThis, const char *pszUtf8Value, PCRTASN1ALLOCATORVTABLE pAllocator);
RTDECL(int) RTAsn1String_InitEx(PRTASN1STRING pThis, uint32_t uTag, void const *pvValue, size_t cbValue,
                                PCRTASN1ALLOCATORVTABLE pAllocator);

/**
 * Compares two strings values, extended version.
 *
 * @returns 0 if equal, -1 if @a pLeft is smaller, 1 if @a pLeft is larger.
 * @param   pLeft               The first string.
 * @param   pRight              The second string.
 * @param   fTypeToo            Set if the string types must match, false if
 *                              not.
 */
RTDECL(int) RTAsn1String_CompareEx(PCRTASN1STRING pLeft, PCRTASN1STRING pRight, bool fTypeToo);
RTDECL(int) RTAsn1String_CompareValues(PCRTASN1STRING pLeft, PCRTASN1STRING pRight);

/**
 * Compares a ASN.1 string object with an UTF-8 string.
 *
 * @returns 0 if equal, -1 if @a pThis is smaller, 1 if @a pThis is larger.
 * @param   pThis               The ASN.1 string object.
 * @param   pszString           The UTF-8 string.
 * @param   cchString           The length of @a pszString, or RTSTR_MAX.
 */
RTDECL(int) RTAsn1String_CompareWithString(PCRTASN1STRING pThis, const char *pszString, size_t cchString);

/**
 * Queries the UTF-8 length of an ASN.1 string object.
 *
 * This differs from RTAsn1String_QueryUtf8 in that it won't need to allocate
 * memory for the converted string, but just calculates the length.
 *
 * @returns IPRT status code.
 * @param   pThis               The ASN.1 string object.
 * @param   pcch                Where to return the string length.
 */
RTDECL(int) RTAsn1String_QueryUtf8Len(PCRTASN1STRING pThis, size_t *pcch);

/**
 * Queries the UTF-8 string for an ASN.1 string object.
 *
 * This may fail as it may require memory to be allocated for storing the
 * string.
 *
 * @returns IPRT status code.
 * @param   pString             The ASN.1 string object.  This is a const
 *                              parameter for making life easier on the caller,
 *                              however be aware that the object may be modified
 *                              by this call!
 * @param   ppsz                Where to return the pointer to the UTF-8 string.
 *                              Optional.
 * @param   pcch                Where to return the length (in 8-bit chars) to
 *                              of the UTF-8 string. Optional.
 */
RTDECL(int) RTAsn1String_QueryUtf8(PCRTASN1STRING pString, const char **ppsz, size_t *pcch);
RTDECL(int) RTAsn1String_RecodeAsUtf8(PRTASN1STRING pThis, PCRTASN1ALLOCATORVTABLE pAllocator);

RTASN1_IMPL_GEN_SEQ_OF_TYPEDEFS_AND_PROTOS(RTASN1SEQOFSTRINGS, RTASN1STRING, RTDECL, RTAsn1SeqOfStrings);
RTASN1_IMPL_GEN_SET_OF_TYPEDEFS_AND_PROTOS(RTASN1SETOFSTRINGS, RTASN1STRING, RTDECL, RTAsn1SetOfStrings);



/**
 * ASN.1 generic context specific tag (IPRT representation).
 *
 * Normally used to tag something that's optional, version specific or such.
 *
 * For the purpose of documenting the format with typedefs as well as possibly
 * making it a little more type safe, there's a set of typedefs for the most
 * commonly used tag values defined.  These typedefs have are identical to
 * RTASN1CONTEXTTAG, except from the C++ type system point of view.
 */
typedef struct RTASN1CONTEXTTAG
{
    /** Core ASN.1 encoding details. */
    RTASN1CORE                          Asn1Core;
} RTASN1CONTEXTTAG;
/** Pointer to an ASN.1 context tag (IPRT thing). */
typedef RTASN1CONTEXTTAG *PRTASN1CONTEXTTAG;
/** Pointer to a const ASN.1 context tag (IPRT thing). */
typedef RTASN1CONTEXTTAG const *PCRTASN1CONTEXTTAG;

RTDECL(int) RTAsn1ContextTagN_Init(PRTASN1CONTEXTTAG pThis, uint32_t uTag, PCRTASN1COREVTABLE pVtable);
RTDECL(int) RTAsn1ContextTagN_Clone(PRTASN1CONTEXTTAG pThis, PCRTASN1CONTEXTTAG pSrc, uint32_t uTag);


/** @internal  */
#define RTASN1CONTEXTTAG_DO_TYPEDEF_AND_INLINE(a_uTag) \
    typedef struct RT_CONCAT(RTASN1CONTEXTTAG,a_uTag) { RTASN1CORE Asn1Core; } RT_CONCAT(RTASN1CONTEXTTAG,a_uTag); \
    typedef RT_CONCAT(RTASN1CONTEXTTAG,a_uTag) *RT_CONCAT(PRTASN1CONTEXTTAG,a_uTag); \
    DECLINLINE(int) RT_CONCAT3(RTAsn1ContextTag,a_uTag,_Init)(RT_CONCAT(PRTASN1CONTEXTTAG,a_uTag) pThis, \
                                                              PCRTASN1COREVTABLE pVtable, PCRTASN1ALLOCATORVTABLE pAllocator) \
    { \
        NOREF(pAllocator); \
        return RTAsn1ContextTagN_Init((PRTASN1CONTEXTTAG)pThis, a_uTag, pVtable); \
    } \
    DECLINLINE(int) RT_CONCAT3(RTAsn1ContextTag,a_uTag,_Clone)(RT_CONCAT(PRTASN1CONTEXTTAG,a_uTag) pThis, \
                                                               RT_CONCAT(RTASN1CONTEXTTAG,a_uTag) const *pSrc) \
    {   return RTAsn1ContextTagN_Clone((PRTASN1CONTEXTTAG)pThis, (PCRTASN1CONTEXTTAG)pSrc, a_uTag); } \
    typedef RT_CONCAT(RTASN1CONTEXTTAG,a_uTag) const *RT_CONCAT(PCRTASN1CONTEXTTAG,a_uTag)
RTASN1CONTEXTTAG_DO_TYPEDEF_AND_INLINE(0);
RTASN1CONTEXTTAG_DO_TYPEDEF_AND_INLINE(1);
RTASN1CONTEXTTAG_DO_TYPEDEF_AND_INLINE(2);
RTASN1CONTEXTTAG_DO_TYPEDEF_AND_INLINE(3);
RTASN1CONTEXTTAG_DO_TYPEDEF_AND_INLINE(4);
RTASN1CONTEXTTAG_DO_TYPEDEF_AND_INLINE(5);
RTASN1CONTEXTTAG_DO_TYPEDEF_AND_INLINE(6);
RTASN1CONTEXTTAG_DO_TYPEDEF_AND_INLINE(7);
#undef RTASN1CONTEXTTAG_DO_TYPEDEF_AND_INLINE

/** Helper for comparing optional context tags.
 * This will return if both are not present or if their precense differs.
 * @internal */
#define RTASN1CONTEXTTAG_COMPARE_PRESENT_RETURN_INTERNAL(a_iDiff, a_pLeft, a_pRight, a_uTag) \
    do { \
        /* type checks */ \
        RT_CONCAT(PCRTASN1CONTEXTTAG,a_uTag) const pMyLeftInternal  = (a_pLeft); \
        RT_CONCAT(PCRTASN1CONTEXTTAG,a_uTag) const pMyRightInternal = (a_pRight); \
        (a_iDiff) = (int)RTASN1CORE_IS_PRESENT(&pMyLeftInternal->Asn1Core) \
                  - (int)RTASN1CORE_IS_PRESENT(&pMyRightInternal->Asn1Core); \
        if ((a_iDiff) || !RTASN1CORE_IS_PRESENT(&pMyLeftInternal->Asn1Core)) return iDiff; \
    } while (0)

/** @name Helpers for comparing optional context tags.
 * This will return if both are not present or if their precense differs.
 * @{ */
#define RTASN1CONTEXTTAG0_COMPARE_PRESENT_RETURN(a_iDiff, a_pLeft, a_pRight) RTASN1CONTEXTTAG_COMPARE_PRESENT_RETURN_INTERNAL(a_iDiff, a_pLeft, a_pRight, 0)
#define RTASN1CONTEXTTAG1_COMPARE_PRESENT_RETURN(a_iDiff, a_pLeft, a_pRight) RTASN1CONTEXTTAG_COMPARE_PRESENT_RETURN_INTERNAL(a_iDiff, a_pLeft, a_pRight, 1)
#define RTASN1CONTEXTTAG2_COMPARE_PRESENT_RETURN(a_iDiff, a_pLeft, a_pRight) RTASN1CONTEXTTAG_COMPARE_PRESENT_RETURN_INTERNAL(a_iDiff, a_pLeft, a_pRight, 2)
#define RTASN1CONTEXTTAG3_COMPARE_PRESENT_RETURN(a_iDiff, a_pLeft, a_pRight) RTASN1CONTEXTTAG_COMPARE_PRESENT_RETURN_INTERNAL(a_iDiff, a_pLeft, a_pRight, 3)
#define RTASN1CONTEXTTAG4_COMPARE_PRESENT_RETURN(a_iDiff, a_pLeft, a_pRight) RTASN1CONTEXTTAG_COMPARE_PRESENT_RETURN_INTERNAL(a_iDiff, a_pLeft, a_pRight, 4)
#define RTASN1CONTEXTTAG5_COMPARE_PRESENT_RETURN(a_iDiff, a_pLeft, a_pRight) RTASN1CONTEXTTAG_COMPARE_PRESENT_RETURN_INTERNAL(a_iDiff, a_pLeft, a_pRight, 5)
#define RTASN1CONTEXTTAG6_COMPARE_PRESENT_RETURN(a_iDiff, a_pLeft, a_pRight) RTASN1CONTEXTTAG_COMPARE_PRESENT_RETURN_INTERNAL(a_iDiff, a_pLeft, a_pRight, 6)
#define RTASN1CONTEXTTAG7_COMPARE_PRESENT_RETURN(a_iDiff, a_pLeft, a_pRight) RTASN1CONTEXTTAG_COMPARE_PRESENT_RETURN_INTERNAL(a_iDiff, a_pLeft, a_pRight, 7)
/** @} */


/**
 * Type information for dynamically bits (see RTASN1DYNTYPE).
 */
typedef enum RTASN1TYPE
{
    /** Not present. */
    RTASN1TYPE_NOT_PRESENT = 0,
    /** Generic ASN.1 for unknown tag/class. */
    RTASN1TYPE_CORE,
    /** ASN.1 NULL. */
    RTASN1TYPE_NULL,
    /** ASN.1 integer. */
    RTASN1TYPE_INTEGER,
    /** ASN.1 boolean. */
    RTASN1TYPE_BOOLEAN,
    /** ASN.1 character string. */
    RTASN1TYPE_STRING,
    /** ASN.1 octet string. */
    RTASN1TYPE_OCTET_STRING,
    /** ASN.1 bite string. */
    RTASN1TYPE_BIT_STRING,
    /** ASN.1 UTC or Generalize time. */
    RTASN1TYPE_TIME,
#if 0
    /** ASN.1 sequence core. */
    RTASN1TYPE_SEQUENCE_CORE,
    /** ASN.1 set core. */
    RTASN1TYPE_SET_CORE,
#endif
    /** ASN.1 object identifier. */
    RTASN1TYPE_OBJID,
    /** End of valid types. */
    RTASN1TYPE_END,
    /** Type size hack. */
    RTASN1TYPE_32BIT_HACK = 0x7fffffff
} RTASN1TYPE;


/**
 * ASN.1 dynamic type record.
 */
typedef struct RTASN1DYNTYPE
{
    /** Alternative interpretation provided by a user.
     * Before destroying this object, the user must explicitly free this and set
     * it to NULL, otherwise there will be memory leaks. */
    PRTASN1CORE                         pUser;
    /** The type of data we've got here. */
    RTASN1TYPE                          enmType;
    /** Union with data of the type dictated by enmType. */
    union
    {
        /** RTASN1TYPE_CORE. */
        RTASN1CORE                      Core;
        /** RTASN1TYPE_NULL. */
        RTASN1NULL                      Asn1Null;
        /** RTASN1TYPE_INTEGER. */
        RTASN1INTEGER                   Integer;
        /** RTASN1TYPE_BOOLEAN. */
        RTASN1BOOLEAN                   Boolean;
        /** RTASN1TYPE_STRING. */
        RTASN1STRING                    String;
        /** RTASN1TYPE_OCTET_STRING. */
        RTASN1OCTETSTRING               OctetString;
        /** RTASN1TYPE_BIT_STRING. */
        RTASN1BITSTRING                 BitString;
        /** RTASN1TYPE_TIME. */
        RTASN1TIME                      Time;
#if 0
        /** RTASN1TYPE_SEQUENCE_CORE. */
        RTASN1SEQUENCECORE              SeqCore;
        /** RTASN1TYPE_SET_CORE. */
        RTASN1SETCORE                   SetCore;
#endif
        /** RTASN1TYPE_OBJID. */
        RTASN1OBJID                     ObjId;
    } u;
} RTASN1DYNTYPE;
/** Pointer to an ASN.1 dynamic type record. */
typedef RTASN1DYNTYPE *PRTASN1DYNTYPE;
/** Pointer to a const ASN.1 dynamic type record. */
typedef RTASN1DYNTYPE const *PCRTASN1DYNTYPE;
RTASN1TYPE_STANDARD_PROTOTYPES(RTASN1DYNTYPE, RTDECL, RTAsn1DynType, u.Core);
RTDECL(int) RTAsn1DynType_SetToNull(PRTASN1DYNTYPE pThis);
RTDECL(int) RTAsn1DynType_SetToObjId(PRTASN1DYNTYPE pThis, PCRTASN1OBJID pSrc, PCRTASN1ALLOCATORVTABLE pAllocator);


/** @name Virtual Method Table Based API
 * @{ */
/**
 * Calls the destructor of the ASN.1 object.
 *
 * @param   pThisCore           The IPRT representation of an ASN.1 object.
 */
RTDECL(void) RTAsn1VtDelete(PRTASN1CORE pThisCore);

/**
 * Deep enumeration of all descendants.
 *
 * @returns IPRT status code, any non VINF_SUCCESS value stems from pfnCallback.
 * @param   pThisCore       Pointer to the ASN.1 core to enumerate members of.
 * @param   pfnCallback     The callback.
 * @param   uDepth          The depth of this object. Children are at +1.
 * @param   pvUser          Callback user argument.
 * @param   fDepthFirst     When set, recurse into child objects before calling
 *                          pfnCallback on then.  When clear, the child object
 *                          is first
 */
RTDECL(int) RTAsn1VtDeepEnum(PRTASN1CORE pThisCore, bool fDepthFirst, uint32_t uDepth,
                             PFNRTASN1ENUMCALLBACK pfnCallback, void *pvUser);

/**
 * Clones @a pSrcCore onto @a pThisCore.
 *
 * The caller must be sure that @a pSrcCore and @a pThisCore are of the same
 * types.
 *
 * @returns IPRT status code.
 * @param   pThisCore       Pointer to the ASN.1 core to clone onto.  This shall
 *                          be uninitialized.
 * @param   pSrcCore        Pointer to the ASN.1 core to clone.
 * @param   pAllocator      The allocator to use.
 */
RTDECL(int) RTAsn1VtClone(PRTASN1CORE pThisCore, PRTASN1CORE pSrcCore, PCRTASN1ALLOCATORVTABLE pAllocator);

/**
 * Compares two objects.
 *
 * @returns 0 if equal, -1 if @a pLeft is smaller, 1 if @a pLeft is larger.
 * @param   pLeftCore       Pointer to the ASN.1 core of the left side object.
 * @param   pRightCore      Pointer to the ASN.1 core of the right side object.
 */
RTDECL(int) RTAsn1VtCompare(PCRTASN1CORE pLeftCore, PCRTASN1CORE pRightCore);

/**
 * Check sanity.
 *
 * A primary criteria is that the object is present and initialized.
 *
 * @returns IPRT status code.
 * @param   pThisCore       Pointer to the ASN.1 core of the object to check out.
 * @param   fFlags          See RTASN1_CHECK_SANITY_F_XXX.
 * @param   pErrInfo        Where to return additional error details. Optional.
 * @param   pszErrorTag     Tag for the additional error details.
 */
RTDECL(int) RTAsn1VtCheckSanity(PCRTASN1CORE pThisCore, uint32_t fFlags,
                                PRTERRINFO pErrInfo, const char *pszErrorTag);
/** @}  */


/** @defgroup rp_asn1_encode RTAsn1Encode - ASN.1 Encoding
 * @{ */

/** @name RTASN1ENCODE_F_XXX
 *  @{ */
/** Use distinguished encoding rules (DER) to encode the object. */
#define RTASN1ENCODE_F_DER          UINT32_C(0x00000001)
/** Use base encoding rules (BER) to encode the object.
 * This is currently the same as DER for practical reasons. */
#define RTASN1ENCODE_F_BER          RTASN1ENCODE_F_DER
/** Mask of valid encoding rules.  */
#define RTASN1ENCODE_F_RULE_MASK    UINT32_C(0x00000007)
/** @} */


/**
 * Recalculates cbHdr of and ASN.1 object.
 *
 * @returns IPRT status code.
 * @retval  VINF_ASN1_NOT_ENCODED if the header size is zero (default value,
 *          whatever).
 * @param   pAsn1Core           The object in question.
 * @param   fFlags              Valid combination of the RTASN1ENCODE_F_XXX
 *                              flags.  Must include the encoding type.
 * @param   pErrInfo            Extended error info. Optional.
 */
RTDECL(int) RTAsn1EncodeRecalcHdrSize(PRTASN1CORE pAsn1Core, uint32_t fFlags, PRTERRINFO pErrInfo);

/**
 * Prepares the ASN.1 structure for encoding.
 *
 * The preparations is mainly calculating accurate object size, but may also
 * involve operations like recoding internal UTF-8 strings to the actual ASN.1
 * format and other things that may require memory to allocated/reallocated.
 *
 * @returns IPRT status code
 * @param   pRoot               The root of the ASN.1 object tree to encode.
 * @param   fFlags              Valid combination of the RTASN1ENCODE_F_XXX
 *                              flags.  Must include the encoding type.
 * @param   pcbEncoded          Where to return the encoded size. Optional.
 * @param   pErrInfo            Where to store extended error information.
 *                              Optional.
 */
RTDECL(int) RTAsn1EncodePrepare(PRTASN1CORE pRoot, uint32_t fFlags, uint32_t *pcbEncoded, PRTERRINFO pErrInfo);

/**
 * Encodes and writes the header of an ASN.1 object.
 *
 * @returns IPRT status code.
 * @retval  VINF_ASN1_NOT_ENCODED if nothing was written (default value,
 *          whatever).
 * @param   pAsn1Core           The object in question.
 * @param   fFlags              Valid combination of the RTASN1ENCODE_F_XXX
 *                              flags.  Must include the encoding type.
 * @param   pfnWriter           The output writer callback.
 * @param   pvUser              The user argument to pass to @a pfnWriter.
 * @param   pErrInfo            Where to store extended error information.
 *                              Optional.
 */
RTDECL(int) RTAsn1EncodeWriteHeader(PCRTASN1CORE pAsn1Core, uint32_t fFlags, FNRTASN1ENCODEWRITER pfnWriter, void *pvUser,
                                    PRTERRINFO pErrInfo);

/**
 * Encodes and writes an ASN.1 object.
 *
 * @returns IPRT status code
 * @param   pRoot               The root of the ASN.1 object tree to encode.
 * @param   fFlags              Valid combination of the RTASN1ENCODE_F_XXX
 *                              flags.  Must include the encoding type.
 * @param   pfnWriter           The output writer callback.
 * @param   pvUser              The user argument to pass to @a pfnWriter.
 * @param   pErrInfo            Where to store extended error information.
 *                              Optional.
 */
RTDECL(int) RTAsn1EncodeWrite(PCRTASN1CORE pRoot, uint32_t fFlags, FNRTASN1ENCODEWRITER pfnWriter, void *pvUser,
                              PRTERRINFO pErrInfo);

/**
 * Encodes and writes an ASN.1 object into a caller allocated memory buffer.
 *
 * @returns IPRT status code
 * @param   pRoot               The root of the ASN.1 object tree to encode.
 * @param   fFlags              Valid combination of the RTASN1ENCODE_F_XXX
 *                              flags.  Must include the encoding type.
 * @param   pvBuf               The output buffer.
 * @param   cbBuf               The buffer size.  This should have the size
 *                              returned by RTAsn1EncodePrepare().
 * @param   pErrInfo            Where to store extended error information.
 *                              Optional.
 */
RTDECL(int) RTAsn1EncodeToBuffer(PCRTASN1CORE pRoot, uint32_t fFlags, void *pvBuf, size_t cbBuf, PRTERRINFO pErrInfo);

/**
 * Helper for when DER encoded ASN.1 is needed for something.
 *
 * Handy when interfacing with OpenSSL and the many d2i_Xxxxx OpenSSL functions,
 * but also handy when structures needs to be digested or similar during signing
 * or verification.
 *
 * We sometimes can use the data we've decoded directly, but often we have to
 * encode it into a temporary heap buffer.
 *
 * @returns IPRT status code, details in @a pErrInfo if present.
 * @param   pRoot       The ASN.1 root of the structure to be passed to OpenSSL.
 * @param   ppbRaw      Where to return the pointer to raw encoded data.
 * @param   pcbRaw      Where to return the size of the raw encoded data.
 * @param   ppvFree     Where to return what to pass to RTMemTmpFree, i.e. NULL
 *                      if we use the previously decoded data directly and
 *                      non-NULL if we had to allocate heap and encode it.
 * @param   pErrInfo    Where to return details about encoding issues. Optional.
 */
RTDECL(int) RTAsn1EncodeQueryRawBits(PRTASN1CORE pRoot, const uint8_t **ppbRaw, uint32_t *pcbRaw,
                                     void **ppvFree, PRTERRINFO pErrInfo);

/** @} */



/** @defgroup rp_asn1_cursor RTAsn1Cursor - BER, DER, and CER cursor
 * @{ */

/**
 * ASN.1 decoder byte cursor.
 */
typedef struct RTASN1CURSOR
{
    /** Pointer to the current (next) byte.  */
    uint8_t const              *pbCur;
    /** Number of bytes left to decode. */
    uint32_t                    cbLeft;
    /** RTASN1CURSOR_FLAGS_XXX.  */
    uint8_t                     fFlags;
    /** The cursor depth. */
    uint8_t                     cDepth;
    /** Two bytes reserved for future tricks. */
    uint8_t                     abReserved[2];
    /** Pointer to the primary cursor. */
    struct RTASN1CURSORPRIMARY *pPrimary;
    /** Pointer to the parent cursor. */
    struct RTASN1CURSOR        *pUp;
    /** The error tag for this cursor level. */
    const char                 *pszErrorTag;
} RTASN1CURSOR;

/** @name RTASN1CURSOR_FLAGS_XXX - Cursor flags.
 * @{ */
/** Enforce DER rules. */
#define RTASN1CURSOR_FLAGS_DER                  RT_BIT(1)
/** Enforce CER rules. */
#define RTASN1CURSOR_FLAGS_CER                  RT_BIT(2)
/** Pending indefinite length encoding. */
#define RTASN1CURSOR_FLAGS_INDEFINITE_LENGTH    RT_BIT(3)
/** @} */


typedef struct RTASN1CURSORPRIMARY
{
    /** The normal cursor bits. */
    RTASN1CURSOR                Cursor;
    /** For error reporting. */
    PRTERRINFO                  pErrInfo;
    /** The allocator virtual method table. */
    PCRTASN1ALLOCATORVTABLE     pAllocator;
    /** Pointer to the first byte.  Useful for calculating offsets. */
    uint8_t const              *pbFirst;
} RTASN1CURSORPRIMARY;
typedef RTASN1CURSORPRIMARY *PRTASN1CURSORPRIMARY;


/**
 * Initializes a primary cursor.
 *
 * The primary cursor is special in that it stores information shared with the
 * sub-cursors created by methods like RTAsn1CursorGetContextTagNCursor and
 * RTAsn1CursorGetSequenceCursor.  Even if just sharing a few items at present,
 * it still important to save every possible byte since stack space is scarce in
 * some of the execution environments.
 *
 * @returns Pointer to pCursor->Cursor.
 * @param   pPrimaryCursor      The primary cursor structure to initialize.
 * @param   pvFirst             The first byte to decode.
 * @param   cb                  The number of bytes to decode.
 * @param   pErrInfo            Where to store error information.
 * @param   pAllocator          The allocator to use.
 * @param   fFlags              RTASN1CURSOR_FLAGS_XXX.
 * @param   pszErrorTag         The primary error tag.
 */
RTDECL(PRTASN1CURSOR) RTAsn1CursorInitPrimary(PRTASN1CURSORPRIMARY pPrimaryCursor, void const *pvFirst, uint32_t cb,
                                              PRTERRINFO pErrInfo, PCRTASN1ALLOCATORVTABLE pAllocator, uint32_t fFlags,
                                              const char *pszErrorTag);

RTDECL(int) RTAsn1CursorInitSub(PRTASN1CURSOR pParent, uint32_t cb, PRTASN1CURSOR pChild, const char *pszErrorTag);

/**
 * Initialize a sub-cursor for traversing the content of an ASN.1 object.
 *
 * @returns IPRT status code.
 * @param   pParent             The parent cursor.
 * @param   pAsn1Core           The ASN.1 object which content we should
 *                              traverse with the sub-cursor.
 * @param   pChild              The sub-cursor to initialize.
 * @param   pszErrorTag         The error tag of the sub-cursor.
 */
RTDECL(int) RTAsn1CursorInitSubFromCore(PRTASN1CURSOR pParent, PRTASN1CORE pAsn1Core,
                                        PRTASN1CURSOR pChild, const char *pszErrorTag);

/**
 * Initalizes the an allocation structure prior to making an allocation.
 *
 * To try unify and optimize memory managment for decoding and in-memory
 * construction of ASN.1 objects, each allocation has an allocation structure
 * associated with it.  This stores the allocator and keep statistics for
 * optimizing resizable allocations.
 *
 * @returns Pointer to the allocator info (for call in alloc parameter).
 * @param   pCursor             The cursor.
 * @param   pAllocation         The allocation structure to initialize.
 */
RTDECL(PRTASN1ALLOCATION) RTAsn1CursorInitAllocation(PRTASN1CURSOR pCursor, PRTASN1ALLOCATION pAllocation);

/**
 * Initalizes the an array allocation structure prior to making an allocation.
 *
 * This is a special case of RTAsn1CursorInitAllocation.  We store a little bit
 * more detail here in order to optimize growing and shrinking of arrays.
 *
 * @returns Pointer to the allocator info (for call in alloc parameter).
 * @param   pCursor             The cursor.
 * @param   pAllocation         The allocation structure to initialize.
 * @param   cbEntry             The array entry size.
 */
RTDECL(PRTASN1ARRAYALLOCATION) RTAsn1CursorInitArrayAllocation(PRTASN1CURSOR pCursor, PRTASN1ARRAYALLOCATION pAllocation,
                                                               size_t cbEntry);

/**
 * Wrapper around RTErrInfoSetV.
 *
 * @returns @a rc
 * @param   pCursor             The cursor.
 * @param   rc                  The return code to return.
 * @param   pszMsg              Message format string.
 * @param   ...                 Format arguments.
 */
RTDECL(int) RTAsn1CursorSetInfo(PRTASN1CURSOR pCursor, int rc, const char *pszMsg, ...) RT_IPRT_FORMAT_ATTR(3, 4);

/**
 * Wrapper around RTErrInfoSetV.
 *
 * @returns @a rc
 * @param   pCursor             The cursor.
 * @param   rc                  The return code to return.
 * @param   pszMsg              Message format string.
 * @param   va                  Format arguments.
 */
RTDECL(int) RTAsn1CursorSetInfoV(PRTASN1CURSOR pCursor, int rc, const char *pszMsg, va_list va) RT_IPRT_FORMAT_ATTR(3, 0);

/**
 * Checks that we've reached the end of the data for the cursor.
 *
 * This differs from RTAsn1CursorCheckEnd in that it does not consider the end
 * an error and therefore leaves the error buffer alone.
 *
 * @returns True if end, otherwise false.
 * @param   pCursor             The cursor we're decoding from.
 */
RTDECL(bool) RTAsn1CursorIsEnd(PRTASN1CURSOR pCursor);

/**
 * Checks that we've reached the end of the data for the cursor.
 *
 * @returns IPRT status code.
 * @param   pCursor             The cursor we're decoding from.
 */
RTDECL(int) RTAsn1CursorCheckEnd(PRTASN1CURSOR pCursor);

/**
 * Specialization of RTAsn1CursorCheckEnd for handling indefinite length sequences.
 *
 * Makes sure we've reached the end of the data for the cursor, and in case of a
 * an indefinite length sequence it may adjust sequence length and the parent
 * cursor.
 *
 * @returns IPRT status code.
 * @param   pCursor             The cursor we're decoding from.
 * @param   pSeqCore            The sequence core record.
 * @sa      RTAsn1CursorCheckSetEnd, RTAsn1CursorCheckOctStrEnd,
 *          RTAsn1CursorCheckEnd
 */
RTDECL(int) RTAsn1CursorCheckSeqEnd(PRTASN1CURSOR pCursor, PRTASN1SEQUENCECORE pSeqCore);

/**
 * Specialization of RTAsn1CursorCheckEnd for handling indefinite length sets.
 *
 * Makes sure we've reached the end of the data for the cursor, and in case of a
 * an indefinite length sets it may adjust set length and the parent cursor.
 *
 * @returns IPRT status code.
 * @param   pCursor             The cursor we're decoding from.
 * @param   pSetCore            The set core record.
 * @sa      RTAsn1CursorCheckSeqEnd, RTAsn1CursorCheckOctStrEnd,
 *          RTAsn1CursorCheckEnd
 */
RTDECL(int) RTAsn1CursorCheckSetEnd(PRTASN1CURSOR pCursor, PRTASN1SETCORE pSetCore);

/**
 * Specialization of RTAsn1CursorCheckEnd for handling indefinite length
 * constructed octet strings.
 *
 * This function must used when parsing the content of an octet string, like
 * for example the Content of a PKCS\#7 ContentInfo structure.  It makes sure
 * we've reached the end of the data for the cursor, and in case of a an
 * indefinite length sets it may adjust set length and the parent cursor.
 *
 * @returns IPRT status code.
 * @param   pCursor             The cursor we're decoding from.
 * @param   pOctetString        The octet string.
 * @sa      RTAsn1CursorCheckSeqEnd, RTAsn1CursorCheckSetEnd,
 *          RTAsn1CursorCheckEnd
 */
RTDECL(int) RTAsn1CursorCheckOctStrEnd(PRTASN1CURSOR pCursor, PRTASN1OCTETSTRING pOctetString);


/**
 * Skips a given number of bytes.
 *
 * @returns @a pCursor
 * @param   pCursor             The cursor.
 * @param   cb                  The number of bytes to skip.
 * @internal
 */
DECLINLINE(PRTASN1CURSOR) RTAsn1CursorSkip(PRTASN1CURSOR pCursor, uint32_t cb)
{
    if (cb <= pCursor->cbLeft)
    {
        pCursor->cbLeft -= cb;
        pCursor->pbCur  += cb;
    }
    else
    {
        pCursor->pbCur  += pCursor->cbLeft;
        pCursor->cbLeft  = 0;
    }

    return pCursor;
}

/**
 * Low-level function for reading an ASN.1 header.
 *
 * @returns IPRT status code.
 * @param   pCursor             The cursor we're decoding from.
 * @param   pAsn1Core           The output object core.
 * @param   pszErrorTag         Error tag.
 * @internal
 */
RTDECL(int) RTAsn1CursorReadHdr(PRTASN1CURSOR pCursor, PRTASN1CORE pAsn1Core, const char *pszErrorTag);

/**
 * Common helper for simple tag matching.
 *
 * @returns IPRT status code.
 * @param   pCursor             The cursor (for error reporting).
 * @param   pAsn1Core           The ASN.1 core structure.
 * @param   uTag                The expected tag.
 * @param   fClass              The expected class.
 * @param   fString             Set if it's a string type that shall follow
 *                              special CER and DER rules wrt to constructed and
 *                              primitive encoding.
 * @param   fFlags              The RTASN1CURSOR_GET_F_XXX flags.
 * @param   pszErrorTag         The error tag.
 * @param   pszWhat             The type/whatever name.
 */
RTDECL(int) RTAsn1CursorMatchTagClassFlagsEx(PRTASN1CURSOR pCursor, PRTASN1CORE pAsn1Core, uint32_t uTag, uint32_t fClass,
                                             bool fString, uint32_t fFlags, const char *pszErrorTag, const char *pszWhat);

/**
 * Common helper for simple tag matching.
 *
 * @returns IPRT status code.
 * @param   pCursor             The cursor (for error reporting).
 * @param   pAsn1Core           The ASN.1 core structure.
 * @param   uTag                The expected tag.
 * @param   fClass              The expected class.
 * @param   fFlags              The RTASN1CURSOR_GET_F_XXX flags.
 * @param   pszErrorTag         The error tag.
 * @param   pszWhat             The type/whatever name.
 * @internal
 */
DECLINLINE(int) RTAsn1CursorMatchTagClassFlags(PRTASN1CURSOR pCursor, PRTASN1CORE pAsn1Core, uint32_t uTag, uint32_t fClass,
                                               uint32_t fFlags, const char *pszErrorTag, const char *pszWhat)
{
    if (pAsn1Core->uTag == uTag && pAsn1Core->fClass == fClass)
        return VINF_SUCCESS;
    return RTAsn1CursorMatchTagClassFlagsEx(pCursor, pAsn1Core, uTag, fClass, false /*fString*/, fFlags, pszErrorTag, pszWhat);
}


/**
 * Common helper for simple tag matching for strings.
 *
 * Check string encoding considerations.
 *
 * @returns IPRT status code.
 * @param   pCursor             The cursor (for error reporting).
 * @param   pAsn1Core           The ASN.1 core structure.
 * @param   uTag                The expected tag.
 * @param   fClass              The expected class.
 * @param   fFlags              The RTASN1CURSOR_GET_F_XXX flags.
 * @param   pszErrorTag         The error tag.
 * @param   pszWhat             The type/whatever name.
 * @internal
 */
DECLINLINE(int) RTAsn1CursorMatchTagClassFlagsString(PRTASN1CURSOR pCursor, PRTASN1CORE pAsn1Core, uint32_t uTag, uint32_t fClass,
                                                     uint32_t fFlags, const char *pszErrorTag, const char *pszWhat)
{
    if (pAsn1Core->uTag == uTag && pAsn1Core->fClass == fClass)
        return VINF_SUCCESS;
    return RTAsn1CursorMatchTagClassFlagsEx(pCursor, pAsn1Core, uTag, fClass, true /*fString*/, fFlags, pszErrorTag, pszWhat);
}



/** @name RTASN1CURSOR_GET_F_XXX - Common flags for all the getters.
 * @{ */
/** Used for decoding objects with implicit tags assigned to them.  This only
 * works when calling getters with a unambigious types. */
#define RTASN1CURSOR_GET_F_IMPLICIT         RT_BIT_32(0)
/** @} */

/**
 * Read ANY object.
 *
 * @returns IPRT status code.
 * @param   pCursor             The cursor we're decoding from.
 * @param   fFlags              RTASN1CURSOR_GET_F_XXX.
 * @param   pAsn1Core           The output object core.
 * @param   pszErrorTag         Error tag.
 */
RTDECL(int) RTAsn1CursorGetCore(PRTASN1CURSOR pCursor, uint32_t fFlags, PRTASN1CORE pAsn1Core, const char *pszErrorTag);

/**
 * Read a NULL object.
 *
 * @returns IPRT status code.
 * @param   pCursor             The cursor we're decoding from.
 * @param   fFlags              RTASN1CURSOR_GET_F_XXX.
 * @param   pNull               The output NULL object.
 * @param   pszErrorTag         Error tag.
 */
RTDECL(int) RTAsn1CursorGetNull(PRTASN1CURSOR pCursor, uint32_t fFlags, PRTASN1NULL pNull, const char *pszErrorTag);

/**
 * Read an INTEGER object.
 *
 * @returns IPRT status code.
 * @param   pCursor             The cursor we're decoding from.
 * @param   fFlags              RTASN1CURSOR_GET_F_XXX.
 * @param   pInteger            The output integer object.
 * @param   pszErrorTag         Error tag.
 */
RTDECL(int) RTAsn1CursorGetInteger(PRTASN1CURSOR pCursor, uint32_t fFlags, PRTASN1INTEGER pInteger, const char *pszErrorTag);

/**
 * Read an BOOLEAN object.
 *
 * @returns IPRT status code.
 * @param   pCursor             The cursor we're decoding from.
 * @param   fFlags              RTASN1CURSOR_GET_F_XXX.
 * @param   pBoolean            The output boolean object.
 * @param   pszErrorTag         Error tag.
 */
RTDECL(int) RTAsn1CursorGetBoolean(PRTASN1CURSOR pCursor, uint32_t fFlags, PRTASN1BOOLEAN pBoolean, const char *pszErrorTag);

/**
 * Retrives an object identifier (aka ObjId or OID) item from the ASN.1 stream.
 *
 * @returns IPRT status code.
 * @param   pCursor             The cursor.
 * @param   fFlags              RTASN1CURSOR_GET_F_XXX.
 * @param   pObjId              The output ODI object.
 * @param   pszErrorTag         Error tag.
 */
RTDECL(int) RTAsn1CursorGetObjId(PRTASN1CURSOR pCursor, uint32_t fFlags, PRTASN1OBJID pObjId, const char *pszErrorTag);

/**
 * Retrives and verifies an object identifier.
 *
 * @returns IPRT status code.
 * @param   pCursor             The cursor.
 * @param   fFlags              RTASN1CURSOR_GET_F_XXX.
 * @param   pObjId              Where to return the parsed object ID, optional.
 * @param   pszExpectedObjId    The expected object identifier (dotted).
 * @param   pszErrorTag         Error tag.
 */
RTDECL(int) RTAsn1CursorGetAndCheckObjId(PRTASN1CURSOR pCursor, uint32_t fFlags, PRTASN1OBJID pObjId,
                                         const char *pszExpectedObjId, const char *pszErrorTag);

/**
 * Read an UTC TIME or GENERALIZED TIME object.
 *
 * @returns IPRT status code.
 * @param   pCursor             The cursor we're decoding from.
 * @param   fFlags              RTASN1CURSOR_GET_F_XXX.
 * @param   pTime               The output time object.
 * @param   pszErrorTag         Error tag.
 */
RTDECL(int) RTAsn1CursorGetTime(PRTASN1CURSOR pCursor, uint32_t fFlags, PRTASN1TIME pTime, const char *pszErrorTag);

/**
 * Read an BIT STRING object (skips past the content).
 *
 * @returns IPRT status ocde.
 * @param   pCursor             The cursor.
 * @param   fFlags              RTASN1CURSOR_GET_F_XXX.
 * @param   pBitString          The output bit string object.
 * @param   pszErrorTag         Error tag.
 */
RTDECL(int) RTAsn1CursorGetBitString(PRTASN1CURSOR pCursor, uint32_t fFlags, PRTASN1BITSTRING pBitString,
                                     const char *pszErrorTag);

/**
 * Read an BIT STRING object (skips past the content), extended version with
 * cMaxBits.
 *
 * @returns IPRT status ocde.
 * @param   pCursor             The cursor.
 * @param   fFlags              RTASN1CURSOR_GET_F_XXX.
 * @param   cMaxBits            The max length of the bit string in bits.  Pass
 *                              UINT32_MAX if variable size.
 * @param   pBitString          The output bit string object.
 * @param   pszErrorTag         Error tag.
 */
RTDECL(int) RTAsn1CursorGetBitStringEx(PRTASN1CURSOR pCursor, uint32_t fFlags, uint32_t cMaxBits, PRTASN1BITSTRING pBitString,
                                       const char *pszErrorTag);

/**
 * Read an OCTET STRING object (skips past the content).
 *
 * @returns IPRT status ocde.
 * @param   pCursor             The cursor.
 * @param   fFlags              RTASN1CURSOR_GET_F_XXX.
 * @param   pOctetString        The output octet string object.
 * @param   pszErrorTag         Error tag.
 */
RTDECL(int) RTAsn1CursorGetOctetString(PRTASN1CURSOR pCursor, uint32_t fFlags, PRTASN1OCTETSTRING pOctetString,
                                       const char *pszErrorTag);

/**
 * Read any kind of string object, except 'character string (29)'.
 *
 * @returns IPRT status code.
 * @param   pCursor             The cursor we're decoding from.
 * @param   fFlags              RTASN1CURSOR_GET_F_XXX.
 * @param   pString             The output boolean object.
 * @param   pszErrorTag         Error tag.
 */
RTDECL(int) RTAsn1CursorGetString(PRTASN1CURSOR pCursor, uint32_t fFlags, PRTASN1STRING pString, const char *pszErrorTag);

/**
 * Read a IA5 STRING object.
 *
 * @returns IPRT status code.
 * @param   pCursor             The cursor we're decoding from.
 * @param   fFlags              RTASN1CURSOR_GET_F_XXX.
 * @param   pString             The output boolean object.
 * @param   pszErrorTag         Error tag.
 */
RTDECL(int) RTAsn1CursorGetIa5String(PRTASN1CURSOR pCursor, uint32_t fFlags, PRTASN1STRING pString, const char *pszErrorTag);

/**
 * Read a UTF8 STRING object.
 *
 * @returns IPRT status code.
 * @param   pCursor             The cursor we're decoding from.
 * @param   fFlags              RTASN1CURSOR_GET_F_XXX.
 * @param   pString             The output boolean object.
 * @param   pszErrorTag         Error tag.
 */
RTDECL(int) RTAsn1CursorGetUtf8String(PRTASN1CURSOR pCursor, uint32_t fFlags, PRTASN1STRING pString, const char *pszErrorTag);

/**
 * Read a BMP STRING (UCS-2) object.
 *
 * @returns IPRT status code.
 * @param   pCursor             The cursor we're decoding from.
 * @param   fFlags              RTASN1CURSOR_GET_F_XXX.
 * @param   pString             The output boolean object.
 * @param   pszErrorTag         Error tag.
 */
RTDECL(int) RTAsn1CursorGetBmpString(PRTASN1CURSOR pCursor, uint32_t fFlags, PRTASN1STRING pString, const char *pszErrorTag);

/**
 * Read a SEQUENCE object and create a cursor for its content.
 *
 * @returns IPRT status code.
 * @param   pCursor             The cursor we're decoding from.
 * @param   fFlags              RTASN1CURSOR_GET_F_XXX.
 * @param   pSeqCore            The output sequence core object.
 * @param   pSeqCursor          The output cursor for the sequence content.
 * @param   pszErrorTag         Error tag, this will be associated with the
 *                              returned cursor.
 */
RTDECL(int) RTAsn1CursorGetSequenceCursor(PRTASN1CURSOR pCursor, uint32_t fFlags,
                                          PRTASN1SEQUENCECORE pSeqCore, PRTASN1CURSOR pSeqCursor, const char *pszErrorTag);

/**
 * Read a SET object and create a cursor for its content.
 *
 * @returns IPRT status code.
 * @param   pCursor             The cursor we're decoding from.
 * @param   fFlags              RTASN1CURSOR_GET_F_XXX.
 * @param   pSetCore            The output set core object.
 * @param   pSetCursor          The output cursor for the set content.
 * @param   pszErrorTag         Error tag, this will be associated with the
 *                              returned cursor.
 */
RTDECL(int) RTAsn1CursorGetSetCursor(PRTASN1CURSOR pCursor, uint32_t fFlags,
                                     PRTASN1SETCORE pSetCore, PRTASN1CURSOR pSetCursor, const char *pszErrorTag);

/**
 * Read a given constructed context tag and create a cursor for its content.
 *
 * @returns IPRT status code.
 * @param   pCursor             The cursor we're decoding from.
 * @param   fFlags              RTASN1CURSOR_GET_F_XXX.
 * @param   uExpectedTag        The expected tag.
 * @param   pVtable             The vtable for the context tag node (see
 *                              RTASN1TMPL_PASS_XTAG).
 * @param   pCtxTag             The output context tag object.
 * @param   pCtxTagCursor       The output cursor for the context tag content.
 * @param   pszErrorTag         Error tag, this will be associated with the
 *                              returned cursor.
 *
 * @remarks There are specialized version of this function for each of the
 *          numbered context tag structures, like for RTASN1CONTEXTTAG0 there is
 *          RTAsn1CursorGetContextTag0Cursor.
 */
RTDECL(int) RTAsn1CursorGetContextTagNCursor(PRTASN1CURSOR pCursor, uint32_t fFlags, uint32_t uExpectedTag,
                                             PCRTASN1COREVTABLE pVtable, PRTASN1CONTEXTTAG pCtxTag, PRTASN1CURSOR pCtxTagCursor,
                                             const char *pszErrorTag);

/**
 * Read a dynamic ASN.1 type.
 *
 * @returns IPRT status code.
 * @param   pCursor             The cursor we're decoding from.
 * @param   fFlags              RTASN1CURSOR_GET_F_XXX.
 * @param   pDynType            The output context tag object.
 * @param   pszErrorTag         Error tag.
 */
RTDECL(int) RTAsn1CursorGetDynType(PRTASN1CURSOR pCursor, uint32_t fFlags, PRTASN1DYNTYPE pDynType, const char *pszErrorTag);

/**
 * Peeks at the next ASN.1 object.
 *
 * @returns IPRT status code.
 * @param   pCursor         The cursore we're decoding from.
 * @param   pAsn1Core       Where to store the output of the peek.
 */
RTDECL(int) RTAsn1CursorPeek(PRTASN1CURSOR pCursor, PRTASN1CORE pAsn1Core);

/**
 * Checks if the next ASN.1 object matches the given tag and class/flags.
 *
 * @returns @c true on match, @c false on mismatch.
 * @param   pCursor         The cursore we're decoding from.
 * @param   uTag            The tag number to match against.
 * @param   fClass          The tag class and flags to match against.
 */
RTDECL(bool) RTAsn1CursorIsNextEx(PRTASN1CURSOR pCursor, uint32_t uTag, uint8_t fClass);



/** @internal  */
#define RTASN1CONTEXTTAG_IMPL_CURSOR_INLINES(a_uTag) \
    DECLINLINE(int) RT_CONCAT3(RTAsn1CursorGetContextTag,a_uTag,Cursor)(PRTASN1CURSOR pCursor, uint32_t fFlags, \
                                                                        PCRTASN1COREVTABLE pVtable, \
                                                                        RT_CONCAT(PRTASN1CONTEXTTAG,a_uTag) pCtxTag, \
                                                                        PRTASN1CURSOR pCtxTagCursor, const char *pszErrorTag) \
    { /* Constructed is automatically implied if you need a cursor to it. */ \
        return RTAsn1CursorGetContextTagNCursor(pCursor, fFlags, a_uTag, pVtable, (PRTASN1CONTEXTTAG)pCtxTag, pCtxTagCursor, pszErrorTag); \
    } \
    DECLINLINE(int) RT_CONCAT3(RTAsn1ContextTag,a_uTag,InitDefault)(RT_CONCAT(PRTASN1CONTEXTTAG,a_uTag) pCtxTag) \
    { /* Constructed is automatically implied if you need to init it with a default value. */ \
        return RTAsn1Core_InitDefault(&pCtxTag->Asn1Core, a_uTag, ASN1_TAGCLASS_CONTEXT | ASN1_TAGFLAG_CONSTRUCTED); \
    } \
    DECLINLINE(int)  RT_CONCAT3(RTAsn1CursorIsConstructedContextTag,a_uTag,Next)(PRTASN1CURSOR pCursor) \
    { \
        return RTAsn1CursorIsNextEx(pCursor, a_uTag, ASN1_TAGCLASS_CONTEXT | ASN1_TAGFLAG_CONSTRUCTED); \
    } \
    DECLINLINE(int)  RT_CONCAT3(RTAsn1CursorIsPrimitiveContextTag,a_uTag,Next)(PRTASN1CURSOR pCursor) \
    { \
        return RTAsn1CursorIsNextEx(pCursor, a_uTag, ASN1_TAGCLASS_CONTEXT | ASN1_TAGFLAG_PRIMITIVE); \
    } \
    DECLINLINE(int)  RT_CONCAT3(RTAsn1CursorIsAnyContextTag,a_uTag,Next)(PRTASN1CURSOR pCursor) \
    { \
        return RTAsn1CursorIsNextEx(pCursor, a_uTag, ASN1_TAGCLASS_CONTEXT | ASN1_TAGFLAG_CONSTRUCTED) \
            || RTAsn1CursorIsNextEx(pCursor, a_uTag, ASN1_TAGCLASS_CONTEXT | ASN1_TAGFLAG_PRIMITIVE);\
    } \

RTASN1CONTEXTTAG_IMPL_CURSOR_INLINES(0)
RTASN1CONTEXTTAG_IMPL_CURSOR_INLINES(1)
RTASN1CONTEXTTAG_IMPL_CURSOR_INLINES(2)
RTASN1CONTEXTTAG_IMPL_CURSOR_INLINES(3)
RTASN1CONTEXTTAG_IMPL_CURSOR_INLINES(4)
RTASN1CONTEXTTAG_IMPL_CURSOR_INLINES(5)
RTASN1CONTEXTTAG_IMPL_CURSOR_INLINES(6)
RTASN1CONTEXTTAG_IMPL_CURSOR_INLINES(7)
#undef RTASN1CONTEXTTAG_IMPL_CURSOR_INLINES


/**
 * Checks if the next object is a boolean.
 *
 * @returns true / false
 * @param   pCursor         The cursor we're decoding from.
 * @remarks May produce error info output on mismatch.
 */
DECLINLINE(bool) RTAsn1CursorIsBooleanNext(PRTASN1CURSOR pCursor)
{
    return RTAsn1CursorIsNextEx(pCursor, ASN1_TAG_BOOLEAN, ASN1_TAGFLAG_PRIMITIVE | ASN1_TAGCLASS_UNIVERSAL);
}


/**
 * Checks if the next object is a set.
 *
 * @returns true / false
 * @param   pCursor         The cursor we're decoding from.
 * @remarks May produce error info output on mismatch.
 */
DECLINLINE(bool) RTAsn1CursorIsSetNext(PRTASN1CURSOR pCursor)
{
    return RTAsn1CursorIsNextEx(pCursor, ASN1_TAG_SET, ASN1_TAGFLAG_CONSTRUCTED | ASN1_TAGCLASS_UNIVERSAL);
}


/** @} */


/** @name ASN.1 Utility APIs
 * @{ */

/**
 * Dumps an IPRT representation of a ASN.1 object tree.
 *
 * @returns IPRT status code.
 * @param   pAsn1Core           The ASN.1 object which members should be dumped.
 * @param   fFlags              RTASN1DUMP_F_XXX.
 * @param   uLevel              The indentation level to start at.
 * @param   pfnPrintfV          The output function.
 * @param   pvUser              Argument to the output function.
 */
RTDECL(int) RTAsn1Dump(PCRTASN1CORE pAsn1Core, uint32_t fFlags, uint32_t uLevel, PFNRTDUMPPRINTFV pfnPrintfV, void *pvUser);

/**
 * Queries the name for an object identifier.
 *
 * This API is very simple due to how we store the data.
 *
 * @returns IPRT status code.
 * @retval  VINF_SUCCESS on success.
 * @retval  VERR_NOT_FOUND if not found.
 * @retval  VERR_BUFFER_OVERFLOW if more buffer space is required.
 *
 * @param   pObjId          The object ID to name.
 * @param   pszDst          Where to store the name if found.
 * @param   cbDst           The size of the destination buffer.
 */
RTDECL(int) RTAsn1QueryObjIdName(PCRTASN1OBJID pObjId, char *pszDst, size_t cbDst);

/** @} */

/** @} */

RT_C_DECLS_END

#endif /* !IPRT_INCLUDED_asn1_h */


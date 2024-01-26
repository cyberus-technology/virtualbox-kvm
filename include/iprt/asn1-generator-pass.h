/** @file
 * IPRT - ASN.1 Code Generator, One Pass.
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

#ifndef ___iprt_asn1_generator_pass_h /* (special, only part of the file) */
#define ___iprt_asn1_generator_pass_h

#include <iprt/formats/asn1.h>
#include <iprt/err.h>


/** @def RTASN1TMPL_MEMBER_OPT_ANY
 * Used for optional entries without any specific type at the end of a
 * structure.
 *
 * For example PolicyQualifierInfo's qualifier member which is defined as:
 *      ANY DEFINED BY policyQualifierId
 *
 * Defaults to RTASN1TMPL_MEMBER_EX.
 */

/** @def RTASN1TMPL_MEMBER_OPT_ITAG_EX
 * Optional member with implict tag, extended version.
 *
 * This is what all the other RTASN1TMPL_MEMBER_OPT_ITAG* macros defere to.
 */
/** @def RTASN1TMPL_MEMBER_OPT_ITAG_CP
 * Optional member of a typical primitive type with an implicit context tag.
 *
 * Examples of this can be found in AuthorityKeyIdentifier where the first and
 * last member are primitive types (normally anyways).:
 *      keyIdentifier [1] OCTET STRING OPTIONAL,
 *      authorityCertSerialNumber [3] INTEGER OPTIONAL
 */
/** @def RTASN1TMPL_MEMBER_OPT_ITAG_UC
 * Optional member of a constructed type from the universal tag class.
 */
/** @def RTASN1TMPL_MEMBER_OPT_ITAG_UP
 * Optional member of a primitive type from the universal tag class.
 */


/** @name Expansion Passes (RTASN1TMPL_PASS values)
 * @{  */
#define RTASN1TMPL_PASS_INTERNAL_HEADER 1

#define RTASN1TMPL_PASS_XTAG            2
#define RTASN1TMPL_PASS_VTABLE          3
#define RTASN1TMPL_PASS_ENUM            4
#define RTASN1TMPL_PASS_DELETE          5
#define RTASN1TMPL_PASS_COMPARE         6

#define RTASN1TMPL_PASS_CHECK_SANITY    8

#define RTASN1TMPL_PASS_INIT           16
#define RTASN1TMPL_PASS_CLONE          17
#define RTASN1TMPL_PASS_SETTERS_1      18
#define RTASN1TMPL_PASS_SETTERS_2      19
#define RTASN1TMPL_PASS_ARRAY          20

#define RTASN1TMPL_PASS_DECODE         24
/** @} */

/** @name ITAG clues
 * @{  */
#define RTASN1TMPL_ITAG_F_CC            1 /**< context, constructed. */
#define RTASN1TMPL_ITAG_F_CP            2 /**< context, probably primary. (w/ numeric value) */
#define RTASN1TMPL_ITAG_F_UP            3 /**< universal, probably primary. (w/ ASN1_TAG_XXX value) */
#define RTASN1TMPL_ITAG_F_UC            4 /**< universal, constructed. (w/ ASN1_TAG_XXX value) */
/** @} */
/** Expands the ITAG clues into tag flag and tag class. */
#define RTASN1TMPL_ITAG_F_EXPAND(a_fClue) \
        (  a_fClue == RTASN1TMPL_ITAG_F_CC ? (ASN1_TAGCLASS_CONTEXT   | ASN1_TAGFLAG_CONSTRUCTED ) \
         : a_fClue == RTASN1TMPL_ITAG_F_CP ? (ASN1_TAGCLASS_CONTEXT   | ASN1_TAGFLAG_PRIMITIVE) \
         : a_fClue == RTASN1TMPL_ITAG_F_UP ? (ASN1_TAGCLASS_UNIVERSAL | ASN1_TAGFLAG_PRIMITIVE) \
         : a_fClue == RTASN1TMPL_ITAG_F_UC ? (ASN1_TAGCLASS_UNIVERSAL | ASN1_TAGFLAG_CONSTRUCTED) \
         : 0 )

#define RTASN1TMPL_SEMICOLON_DUMMY() typedef unsigned RTASN1TMPLSEMICOLONDUMMY

#endif /* !___iprt_asn1_generator_pass_h */


#if RTASN1TMPL_PASS == RTASN1TMPL_PASS_INTERNAL_HEADER
/*
 *
 * Internal header file.
 *
 */
# define RTASN1TMPL_BEGIN_COMMON() extern DECL_HIDDEN_DATA(RTASN1COREVTABLE const) RT_CONCAT3(g_,RTASN1TMPL_INT_NAME,_Vtable)

# define RTASN1TMPL_BEGIN_SEQCORE()                 RTASN1TMPL_BEGIN_COMMON()
# define RTASN1TMPL_BEGIN_SETCORE()                 RTASN1TMPL_BEGIN_COMMON()
# define RTASN1TMPL_MEMBER_EX(a_Name, a_Type, a_Api, a_Constraints)                                 RTASN1TMPL_SEMICOLON_DUMMY()
# define RTASN1TMPL_MEMBER_OPT_XTAG_EX(a_TnNm, a_CtxTagN, a_Name, a_Type, a_Api, a_uTag, a_Constraints) \
    extern "C" DECL_HIDDEN_DATA(RTASN1COREVTABLE const) RT_CONCAT5(g_,RTASN1TMPL_INT_NAME,_XTAG_,a_Name,_Vtable)

# define RTASN1TMPL_MEMBER_DYN_BEGIN(a_ObjIdMembNm, a_enmType, a_enmMembNm, a_Allocation)           RTASN1TMPL_SEMICOLON_DUMMY()
# define RTASN1TMPL_MEMBER_DYN_END(a_ObjIdMembNm, a_enmType, a_enmMembNm, a_Allocation)             RTASN1TMPL_SEMICOLON_DUMMY()
# define RTASN1TMPL_END_SEQCORE()                   RTASN1TMPL_SEMICOLON_DUMMY()
# define RTASN1TMPL_END_SETCORE()                   RTASN1TMPL_SEMICOLON_DUMMY()


# define RTASN1TMPL_BEGIN_PCHOICE()                 RTASN1TMPL_BEGIN_COMMON()
# define RTASN1TMPL_PCHOICE_ITAG_EX(a_uTag, a_enmChoice, a_PtrName, a_Name, a_Type, a_Api, a_fClue, a_Constraints) \
                                                                                                    RTASN1TMPL_SEMICOLON_DUMMY()
# define RTASN1TMPL_PCHOICE_XTAG_EX(a_uTag, a_enmChoice, a_PtrTnNm, a_CtxTagN, a_Name, a_Type, a_Api, a_Constraints) \
    extern "C" DECL_HIDDEN_DATA(RTASN1COREVTABLE const) RT_CONCAT5(g_,RTASN1TMPL_INT_NAME,_PCHOICE_XTAG_,a_Name,_Vtable)

# define RTASN1TMPL_END_PCHOICE()                   RTASN1TMPL_SEMICOLON_DUMMY()


# define RTASN1TMPL_SEQ_OF(a_ItemType, a_ItemApi)   RTASN1TMPL_BEGIN_COMMON()
# define RTASN1TMPL_SET_OF(a_ItemType, a_ItemApi)   RTASN1TMPL_BEGIN_COMMON()



#elif RTASN1TMPL_PASS == RTASN1TMPL_PASS_XTAG
/*
 *
 * Generate a vtable and associated methods for explicitly tagged items (XTAG).
 *
 * These turned out to be a little problematic during encoding since there are
 * two tags, the first encapsulating the second, thus the enumeration has to be
 * nested or we cannot calculate the size of the first tag.
 *
 *
 */
# define RTASN1TMPL_BEGIN_COMMON()                                                                  RTASN1TMPL_SEMICOLON_DUMMY()
# define RTASN1TMPL_BEGIN_SEQCORE()                                                                 RTASN1TMPL_SEMICOLON_DUMMY()
# define RTASN1TMPL_BEGIN_SETCORE()                                                                 RTASN1TMPL_SEMICOLON_DUMMY()
# define RTASN1TMPL_MEMBER_EX(a_Name, a_Type, a_Api, a_Constraints)                                 RTASN1TMPL_SEMICOLON_DUMMY()
# define RTASN1TMPL_MEMBER_DYN_BEGIN(a_ObjIdMembNm, a_enmType, a_enmMembNm, a_Allocation)           RTASN1TMPL_SEMICOLON_DUMMY()
# define RTASN1TMPL_MEMBER_DYN_END(a_ObjIdMembNm, a_enmType, a_enmMembNm, a_Allocation)             RTASN1TMPL_SEMICOLON_DUMMY()
# define RTASN1TMPL_MEMBER_OPT_XTAG_EX(a_TnNm, a_CtxTagN, a_Name, a_Type, a_Api, a_uTag, a_Constraints) \
    /* This is the method we need to make it work. */ \
    static DECLCALLBACK(int) RT_CONCAT4(RTASN1TMPL_INT_NAME,_XTAG_,a_Name,_Enum)(PRTASN1CORE pThisCore, \
                                                                                 PFNRTASN1ENUMCALLBACK pfnCallback, \
                                                                                 uint32_t uDepth, void *pvUser) \
    { \
        RTASN1TMPL_TYPE *pThis = RT_FROM_MEMBER(pThisCore, RTASN1TMPL_TYPE, a_TnNm.a_CtxTagN); \
        if (RTASN1CORE_IS_PRESENT(&pThis->a_TnNm.a_CtxTagN.Asn1Core)) \
            return pfnCallback(RT_CONCAT(a_Api,_GetAsn1Core)(&pThis->a_TnNm.a_Name), #a_TnNm "." #a_Name, uDepth + 1, pvUser); \
        return VINF_SUCCESS; \
    } \
    /* The reminder of the methods shouldn't normally be needed, just stub them. */ \
    static DECLCALLBACK(void) RT_CONCAT4(RTASN1TMPL_INT_NAME,_XTAG_,a_Name,_Delete)(PRTASN1CORE pThisCore) \
    { AssertFailed(); RT_NOREF_PV(pThisCore); } \
    static DECLCALLBACK(int) RT_CONCAT4(RTASN1TMPL_INT_NAME,_XTAG_,a_Name,_Clone)(PRTASN1CORE pThisCore, PCRTASN1CORE pSrcCore, \
                                                                                  PCRTASN1ALLOCATORVTABLE pAllocator) \
    { AssertFailed(); RT_NOREF_PV(pThisCore); RT_NOREF_PV(pSrcCore); RT_NOREF_PV(pAllocator); return VERR_INTERNAL_ERROR_2; } \
    static DECLCALLBACK(int) RT_CONCAT4(RTASN1TMPL_INT_NAME,_XTAG_,a_Name,_Compare)(PCRTASN1CORE pLeftCore, \
                                                                                    PCRTASN1CORE pRightCore) \
    { AssertFailed(); RT_NOREF_PV(pLeftCore); RT_NOREF_PV(pRightCore); return VERR_INTERNAL_ERROR_2; } \
    static DECLCALLBACK(int) RT_CONCAT4(RTASN1TMPL_INT_NAME,_XTAG_,a_Name,_CheckSanity)(PCRTASN1CORE pThisCore, uint32_t fFlags, \
                                                                                        PRTERRINFO pErrInfo, const char *pszErrorTag) \
    { AssertFailed(); RT_NOREF_PV(pThisCore); RT_NOREF_PV(fFlags); RT_NOREF_PV(pErrInfo); RT_NOREF_PV(pszErrorTag); \
      return VERR_INTERNAL_ERROR_2; } \
    DECL_HIDDEN_CONST(RTASN1COREVTABLE const) RT_CONCAT5(g_,RTASN1TMPL_INT_NAME,_XTAG_,a_Name,_Vtable) = \
    { \
        /* When the Asn1Core is at the start of the structure, we can reuse the _Delete and _Enum APIs here. */ \
        /* .pszName = */        RT_XSTR(RTASN1TMPL_INT_NAME) "_XTAG_" RT_XSTR(a_Name), \
        /* .cb = */             RT_SIZEOFMEMB(RTASN1TMPL_TYPE, a_TnNm), \
        /* .uDefaultTag = */    a_uTag, \
        /* .fDefaultClass = */  ASN1_TAGCLASS_CONTEXT, \
        /* .uReserved = */      0, \
        RT_CONCAT4(RTASN1TMPL_INT_NAME,_XTAG_,a_Name,_Delete), \
        RT_CONCAT4(RTASN1TMPL_INT_NAME,_XTAG_,a_Name,_Enum), \
        RT_CONCAT4(RTASN1TMPL_INT_NAME,_XTAG_,a_Name,_Clone), \
        RT_CONCAT4(RTASN1TMPL_INT_NAME,_XTAG_,a_Name,_Compare), \
        RT_CONCAT4(RTASN1TMPL_INT_NAME,_XTAG_,a_Name,_CheckSanity), \
        /*.pfnEncodePrep */ NULL, \
        /*.pfnEncodeWrite */ NULL \
    }


# define RTASN1TMPL_END_SEQCORE()                                                                   RTASN1TMPL_SEMICOLON_DUMMY()
# define RTASN1TMPL_END_SETCORE()                                                                   RTASN1TMPL_SEMICOLON_DUMMY()
# define RTASN1TMPL_BEGIN_PCHOICE()                                                                 RTASN1TMPL_SEMICOLON_DUMMY()
# define RTASN1TMPL_PCHOICE_ITAG_EX(a_uTag, a_enmChoice, a_PtrName, a_Name, a_Type, a_Api, a_fClue, a_Constraints) \
                                                                                                    RTASN1TMPL_SEMICOLON_DUMMY()
# define RTASN1TMPL_PCHOICE_XTAG_EX(a_uTag, a_enmChoice, a_PtrTnNm, a_CtxTagN, a_Name, a_Type, a_Api, a_Constraints) \
    /* This is the method we need to make it work. */ \
    static DECLCALLBACK(int) RT_CONCAT4(RTASN1TMPL_INT_NAME,_PC_XTAG_,a_Name,_Enum)(PRTASN1CORE pThisCore, \
                                                                                    PFNRTASN1ENUMCALLBACK pfnCallback, \
                                                                                    uint32_t uDepth, void *pvUser) \
    { \
        if (RTASN1CORE_IS_PRESENT(pThisCore)) \
        { \
            /** @todo optimize this one day, possibly change the PCHOICE+XTAG representation. */ \
            RTASN1TMPL_TYPE Tmp; \
            *(PRTASN1CORE *)&Tmp.a_PtrTnNm = pThisCore; \
            Assert(&Tmp.a_PtrTnNm->a_CtxTagN.Asn1Core == pThisCore); \
            return pfnCallback(RT_CONCAT(a_Api,_GetAsn1Core)(&Tmp.a_PtrTnNm->a_Name), "T" #a_uTag "." #a_Name, uDepth + 1, pvUser); \
        } \
        return VINF_SUCCESS; \
    } \
    /* The reminder of the methods shouldn't normally be needed, just stub them. */ \
    static DECLCALLBACK(void) RT_CONCAT4(RTASN1TMPL_INT_NAME,_PC_XTAG_,a_Name,_Delete)(PRTASN1CORE pThisCore) \
    { AssertFailed(); RT_NOREF_PV(pThisCore); } \
    static DECLCALLBACK(int) RT_CONCAT4(RTASN1TMPL_INT_NAME,_PC_XTAG_,a_Name,_Clone)(PRTASN1CORE pThisCore, PCRTASN1CORE pSrcCore, \
                                                                                     PCRTASN1ALLOCATORVTABLE pAllocator) \
    { AssertFailed(); RT_NOREF_PV(pThisCore); RT_NOREF_PV(pSrcCore); RT_NOREF_PV(pAllocator);  return VERR_INTERNAL_ERROR_3; } \
    static DECLCALLBACK(int) RT_CONCAT4(RTASN1TMPL_INT_NAME,_PC_XTAG_,a_Name,_Compare)(PCRTASN1CORE pLeftCore, \
                                                                                       PCRTASN1CORE pRightCore) \
    { AssertFailed(); RT_NOREF_PV(pLeftCore); RT_NOREF_PV(pRightCore);  return VERR_INTERNAL_ERROR_3; } \
    static DECLCALLBACK(int) RT_CONCAT4(RTASN1TMPL_INT_NAME,_PC_XTAG_,a_Name,_CheckSanity)(PCRTASN1CORE pThisCore, uint32_t fFlags, \
                                                                                           PRTERRINFO pErrInfo, const char *pszErrorTag) \
    { AssertFailed(); RT_NOREF_PV(pThisCore); RT_NOREF_PV(fFlags); RT_NOREF_PV(pErrInfo); RT_NOREF_PV(pszErrorTag); \
      return VERR_INTERNAL_ERROR_3; } \
    DECL_HIDDEN_CONST(RTASN1COREVTABLE const) RT_CONCAT5(g_,RTASN1TMPL_INT_NAME,_PCHOICE_XTAG_,a_Name,_Vtable) = \
    { \
        /* When the Asn1Core is at the start of the structure, we can reuse the _Delete and _Enum APIs here. */ \
        /* .pszName = */        RT_XSTR(RTASN1TMPL_INT_NAME) "_PCHOICE_XTAG_" RT_XSTR(a_Name), \
        /* .cb = */             sizeof(*((RTASN1TMPL_TYPE *)(void *)0)->a_PtrTnNm), \
        /* .uDefaultTag = */    a_uTag, \
        /* .fDefaultClass = */  ASN1_TAGCLASS_CONTEXT, \
        /* .uReserved = */      0, \
        RT_CONCAT4(RTASN1TMPL_INT_NAME,_PC_XTAG_,a_Name,_Delete), \
        RT_CONCAT4(RTASN1TMPL_INT_NAME,_PC_XTAG_,a_Name,_Enum), \
        RT_CONCAT4(RTASN1TMPL_INT_NAME,_PC_XTAG_,a_Name,_Clone), \
        RT_CONCAT4(RTASN1TMPL_INT_NAME,_PC_XTAG_,a_Name,_Compare), \
        RT_CONCAT4(RTASN1TMPL_INT_NAME,_PC_XTAG_,a_Name,_CheckSanity), \
        /*.pfnEncodePrep */ NULL, \
        /*.pfnEncodeWrite */ NULL \
    }



# define RTASN1TMPL_END_PCHOICE()                                                                   RTASN1TMPL_SEMICOLON_DUMMY()
# define RTASN1TMPL_SEQ_OF(a_ItemType, a_ItemApi)                                                   RTASN1TMPL_SEMICOLON_DUMMY()
# define RTASN1TMPL_SET_OF(a_ItemType, a_ItemApi)                                                   RTASN1TMPL_SEMICOLON_DUMMY()



#elif RTASN1TMPL_PASS == RTASN1TMPL_PASS_VTABLE
/*
 *
 * Internal header file.
 *
 */
# ifndef RTASN1TMPL_VTABLE_FN_ENCODE_PREP
#  define RTASN1TMPL_VTABLE_FN_ENCODE_PREP  NULL
# endif
# ifndef RTASN1TMPL_VTABLE_FN_ENCODE_WRITE
#  define RTASN1TMPL_VTABLE_FN_ENCODE_WRITE NULL
# endif
# define RTASN1TMPL_BEGIN_COMMON(a_uDefaultTag, a_fDefaultClass) \
    DECL_HIDDEN_CONST(RTASN1COREVTABLE const) RT_CONCAT3(g_,RTASN1TMPL_INT_NAME,_Vtable) = \
    { \
        /* When the Asn1Core is at the start of the structure, we can reuse the _Delete and _Enum APIs here. */ \
        /* .pszName = */        RT_XSTR(RTASN1TMPL_EXT_NAME), \
        /* .cb = */             sizeof(RTASN1TMPL_TYPE), \
        /* .uDefaultTag = */    a_uDefaultTag, \
        /* .fDefaultClass = */  a_fDefaultClass, \
        /* .uReserved = */      0, \
        (PFNRTASN1COREVTDTOR)RT_CONCAT(RTASN1TMPL_EXT_NAME,_Delete), \
        (PFNRTASN1COREVTENUM)RT_CONCAT(RTASN1TMPL_EXT_NAME,_Enum), \
        (PFNRTASN1COREVTCLONE)RT_CONCAT(RTASN1TMPL_EXT_NAME,_Clone), \
        (PFNRTASN1COREVTCOMPARE)RT_CONCAT(RTASN1TMPL_EXT_NAME,_Compare), \
        (PFNRTASN1COREVTCHECKSANITY)RT_CONCAT(RTASN1TMPL_EXT_NAME,_CheckSanity), \
        RTASN1TMPL_VTABLE_FN_ENCODE_PREP, \
        RTASN1TMPL_VTABLE_FN_ENCODE_WRITE \
    }

# define RTASN1TMPL_BEGIN_SEQCORE() \
    AssertCompileMemberOffset(RTASN1TMPL_TYPE, SeqCore, 0); \
    RTASN1TMPL_BEGIN_COMMON(ASN1_TAG_SEQUENCE, ASN1_TAGCLASS_UNIVERSAL | ASN1_TAGFLAG_CONSTRUCTED)
# define RTASN1TMPL_BEGIN_SETCORE() \
    AssertCompileMemberOffset(RTASN1TMPL_TYPE, SetCore, 0); \
    RTASN1TMPL_BEGIN_COMMON(ASN1_TAG_SET, ASN1_TAGCLASS_UNIVERSAL | ASN1_TAGFLAG_CONSTRUCTED)
# define RTASN1TMPL_MEMBER_EX(a_Name, a_Type, a_Api, a_Constraints)                                 RTASN1TMPL_SEMICOLON_DUMMY()
# define RTASN1TMPL_MEMBER_DYN_BEGIN(a_ObjIdMembNm, a_enmType, a_enmMembNm, a_Allocation)           RTASN1TMPL_SEMICOLON_DUMMY()
# define RTASN1TMPL_MEMBER_DYN_END(a_ObjIdMembNm, a_enmType, a_enmMembNm, a_Allocation)             RTASN1TMPL_SEMICOLON_DUMMY()
# define RTASN1TMPL_END_SEQCORE()                                                                   RTASN1TMPL_SEMICOLON_DUMMY()
# define RTASN1TMPL_END_SETCORE()                                                                   RTASN1TMPL_SEMICOLON_DUMMY()

# define RTASN1TMPL_BEGIN_PCHOICE() \
    AssertCompileMemberOffset(RTASN1TMPL_TYPE, Dummy, 0); \
    RTASN1TMPL_BEGIN_COMMON(UINT8_MAX, UINT8_MAX)
# define RTASN1TMPL_PCHOICE_ITAG_EX(a_uTag, a_enmChoice, a_PtrName, a_Name, a_Type, a_Api, a_fClue, a_Constraints) \
                                                                                                    RTASN1TMPL_SEMICOLON_DUMMY()
# define RTASN1TMPL_PCHOICE_XTAG_EX(a_uTag, a_enmChoice, a_PtrTnNm, a_CtxTagN, a_Name, a_Type, a_Api, a_Constraints) \
                                                                                                    RTASN1TMPL_SEMICOLON_DUMMY()
# define RTASN1TMPL_END_PCHOICE()                                                                   RTASN1TMPL_SEMICOLON_DUMMY()

# define RTASN1TMPL_SEQ_OF(a_ItemType, a_ItemApi) \
    AssertCompileMemberOffset(RTASN1TMPL_TYPE, SeqCore, 0); \
    RTASN1TMPL_BEGIN_COMMON(ASN1_TAG_SEQUENCE, ASN1_TAGCLASS_UNIVERSAL | ASN1_TAGFLAG_CONSTRUCTED)
# define RTASN1TMPL_SET_OF(a_ItemType, a_ItemApi) \
    AssertCompileMemberOffset(RTASN1TMPL_TYPE, SetCore, 0); \
    RTASN1TMPL_BEGIN_COMMON(ASN1_TAG_SET, ASN1_TAGCLASS_UNIVERSAL | ASN1_TAGFLAG_CONSTRUCTED)



#elif RTASN1TMPL_PASS == RTASN1TMPL_PASS_INIT
/*
 *
 * Initialization to standard / default values.
 *
 */
# define RTASN1TMPL_BEGIN_COMMON() \
RTASN1TMPL_DECL(int) RT_CONCAT(RTASN1TMPL_EXT_NAME,_Init)(RT_CONCAT(P,RTASN1TMPL_TYPE) pThis, PCRTASN1ALLOCATORVTABLE pAllocator) \
{ \
    RT_NOREF_PV(pAllocator); \
    RT_ZERO(*pThis)
# define RTASN1TMPL_END_COMMON() \
    return rc; \
} RTASN1TMPL_SEMICOLON_DUMMY()

# define RTASN1TMPL_BEGIN_SEQCORE() \
    RTASN1TMPL_BEGIN_COMMON(); \
    int rc = RTAsn1SequenceCore_Init(&pThis->SeqCore, &RT_CONCAT3(g_,RTASN1TMPL_INT_NAME,_Vtable))
# define RTASN1TMPL_BEGIN_SETCORE() \
    RTASN1TMPL_BEGIN_COMMON(); \
    int rc = RTAsn1SetCore_Init(&pThis->SetCore, &RT_CONCAT3(g_,RTASN1TMPL_INT_NAME,_Vtable))
# define RTASN1TMPL_MEMBER_EX(a_Name, a_Type, a_Api, a_Constraints) \
    if (RT_SUCCESS(rc)) \
        rc = RT_CONCAT(a_Api,_Init)(&pThis->a_Name, pAllocator)

# define RTASN1TMPL_MEMBER_DYN_BEGIN(a_ObjIdMembNm, a_enmType, a_enmMembNm, a_Allocation) \
    RTAsn1MemInitAllocation(&pThis->Allocation, pAllocator); \
    pThis->a_enmMembNm = RT_CONCAT(a_enmType,_NOT_PRESENT)
# define RTASN1TMPL_MEMBER_DYN_COMMON(a_UnionNm, a_PtrName, a_Type, a_Api, a_Allocation, a_enmMembNm, a_enmValue, a_IfStmt) \
                                                                                                    do { } while (0)
# define RTASN1TMPL_MEMBER_DYN_END(a_ObjIdMembNm, a_enmType, a_enmMembNm, a_Allocation)             do { } while (0)

# define RTASN1TMPL_MEMBER_DEF_ITAG_EX(a_Name, a_Type, a_Api, a_uTag, a_fClue, a_DefVal, a_Constraints) \
    if (RT_SUCCESS(rc)) \
    { \
        rc = RT_CONCAT(a_Api,_InitDefault)(&pThis->a_Name, a_DefVal, pAllocator); \
        if (RT_SUCCESS(rc)) \
            rc = RTAsn1Core_SetTagAndFlags(RT_CONCAT(a_Api,_GetAsn1Core)(&pThis->a_Name), \
                                           a_uTag, RTASN1TMPL_ITAG_F_EXPAND(a_fClue)); \
    }
# define RTASN1TMPL_MEMBER_OPT_EX(a_Name, a_Type, a_Api, a_Constraints) do { } while (0) /* All optional members are left as not-present. */
# define RTASN1TMPL_END_SEQCORE() \
    if (RT_FAILURE(rc)) \
        RT_CONCAT(RTASN1TMPL_EXT_NAME,_Delete)(pThis); \
    RTASN1TMPL_END_COMMON()
# define RTASN1TMPL_END_SETCORE() RTASN1TMPL_END_SEQCORE()

/* No choice, just an empty, non-present structure. */
# define RTASN1TMPL_BEGIN_PCHOICE() \
    RTASN1TMPL_BEGIN_COMMON(); \
    RTAsn1MemInitAllocation(&pThis->Allocation, pAllocator); \
    int rc = VINF_SUCCESS
# define RTASN1TMPL_PCHOICE_ITAG_EX(a_uTag, a_enmChoice, a_PtrName, a_Name, a_Type, a_Api, a_fClue, a_Constraints) \
                                                                                                    do { } while (0)
# define RTASN1TMPL_PCHOICE_XTAG_EX(a_uTag, a_enmChoice, a_PtrTnNm, a_CtxTagN, a_Name, a_Type, a_Api, a_Constraints) \
                                                                                                    do { } while (0)
# define RTASN1TMPL_END_PCHOICE() RTASN1TMPL_END_COMMON()


# define RTASN1TMPL_SET_SEQ_OF_COMMON(a_ItemType, a_ItemApi, a_OfApi, a_OfMember) \
    RTASN1TMPL_BEGIN_COMMON(); \
    RTAsn1MemInitArrayAllocation(&pThis->Allocation, pAllocator, sizeof(a_ItemType)); \
    int rc = RT_CONCAT(a_OfApi,_Init)(&pThis->a_OfMember, &RT_CONCAT3(g_,RTASN1TMPL_INT_NAME,_Vtable)); \
    if (RT_FAILURE(rc)) \
        RT_ZERO(*pThis); \
    RTASN1TMPL_END_COMMON()
# define RTASN1TMPL_SEQ_OF(a_ItemType, a_ItemApi) RTASN1TMPL_SET_SEQ_OF_COMMON(a_ItemType, a_ItemApi, RTAsn1SeqOfCore, SeqCore)
# define RTASN1TMPL_SET_OF(a_ItemType, a_ItemApi) RTASN1TMPL_SET_SEQ_OF_COMMON(a_ItemType, a_ItemApi, RTAsn1SetOfCore, SetCore)



#elif RTASN1TMPL_PASS == RTASN1TMPL_PASS_DECODE
/*
 *
 * Decode ASN.1.
 *
 */
# define RTASN1TMPL_BEGIN_COMMON() \
RTASN1TMPL_DECL(int) RT_CONCAT(RTASN1TMPL_EXT_NAME,_DecodeAsn1)(PRTASN1CURSOR pCursor, uint32_t fFlags, \
                                                                RT_CONCAT(P,RTASN1TMPL_TYPE) pThis, const char *pszErrorTag) \
{ \
    RT_ZERO(*pThis);

# define RTASN1TMPL_END_COMMON() \
    return rc; \
} RTASN1TMPL_SEMICOLON_DUMMY()


# define RTASN1TMPL_BEGIN_SEQCORE() \
    RTASN1TMPL_BEGIN_COMMON(); \
    RTASN1CURSOR ThisCursor; \
    int rc = RTAsn1CursorGetSequenceCursor(pCursor, fFlags, &pThis->SeqCore, &ThisCursor, pszErrorTag); \
    if (RT_FAILURE(rc)) \
        return rc; \
    pCursor = &ThisCursor; \
    pThis->SeqCore.Asn1Core.pOps = &RT_CONCAT3(g_,RTASN1TMPL_INT_NAME,_Vtable)
# define RTASN1TMPL_BEGIN_SETCORE() \
    RTASN1TMPL_BEGIN_COMMON(); \
    RTASN1CURSOR ThisCursor; \
    int rc = RTAsn1CursorGetSetCursor(pCursor, fFlags, &pThis->SetCore, &ThisCursor, pszErrorTag); \
    if (RT_FAILURE(rc)) \
        return rc; \
    pCursor = &ThisCursor; \
    pThis->SetCore.Asn1Core.pOps = &RT_CONCAT3(g_,RTASN1TMPL_INT_NAME,_Vtable)

# define RTASN1TMPL_MEMBER_EX(a_Name, a_Type, a_Api, a_Constraints) \
    if (RT_SUCCESS(rc)) \
         rc = RT_CONCAT(a_Api,_DecodeAsn1)(pCursor, 0, &pThis->a_Name, #a_Name)

# define RTASN1TMPL_MEMBER_DYN_BEGIN(a_ObjIdMembNm, a_enmType, a_enmMembNm, a_Allocation) \
    if (RT_SUCCESS(rc)) \
    { \
        int rc2; /* not initialized! */ \
        RTAsn1CursorInitAllocation(pCursor, &pThis->a_Allocation); \
        pThis->a_enmMembNm = RT_CONCAT(a_enmType, _INVALID); \
        if (false) do { /*nothing*/ } while (0)
# define RTASN1TMPL_MEMBER_DYN_COMMON(a_UnionNm, a_PtrName, a_Type, a_Api, a_Allocation, a_enmMembNm, a_enmValue, a_IfStmt) \
        else a_IfStmt \
        do { \
            rc2 = RTAsn1MemAllocZ(&pThis->a_Allocation, (void **)&pThis->a_UnionNm.a_PtrName, \
                                  sizeof(*pThis->a_UnionNm.a_PtrName)); \
            if (RT_SUCCESS(rc2)) \
            { \
                pThis->a_enmMembNm = a_enmValue; \
                rc2 = RT_CONCAT(a_Api,_DecodeAsn1)(pCursor, 0, pThis->a_UnionNm.a_PtrName, #a_UnionNm "." #a_PtrName); \
            } \
        } while (0)
# define RTASN1TMPL_MEMBER_DYN_END(a_ObjIdMembNm, a_enmType, a_enmMembNm, a_Allocation) \
        rc = rc2; /* Should trigger warning if a _DEFAULT is missing. */  \
    }

# define RTASN1TMPL_MEMBER_OPT_EX(a_Name, a_Type, a_Api, a_Constraints) \
    Error_Missing_Specific_Macro_In_Decode_Pass()

# define RTASN1TMPL_MEMBER_DEF_ITAG_EX(a_Name, a_Type, a_Api, a_uTag, a_fClue, a_DefVal, a_Constraints) \
    if (RT_SUCCESS(rc)) \
    { \
        if (RTAsn1CursorIsNextEx(pCursor, a_uTag, RTASN1TMPL_ITAG_F_EXPAND(a_fClue))) \
            rc = RT_CONCAT(a_Api,_DecodeAsn1)(pCursor, 0, &pThis->a_Name, #a_Name); \
        else \
            rc = RT_CONCAT(a_Api,_InitDefault)(&pThis->a_Name, a_DefVal, pCursor->pPrimary->pAllocator); \
        if (RT_SUCCESS(rc)) \
            rc = RTAsn1Core_SetTagAndFlags(RT_CONCAT(a_Api,_GetAsn1Core)(&pThis->a_Name), \
                                           a_uTag, RTASN1TMPL_ITAG_F_EXPAND(a_fClue)); \
    } do {} while (0)

# define RTASN1TMPL_MEMBER_OPT_UTF8_STRING_EX(a_Name, a_Constraints) \
    if (RT_SUCCESS(rc) && RTAsn1CursorIsNextEx(pCursor, ASN1_TAG_UTF8_STRING, ASN1_TAGCLASS_CONTEXT | ASN1_TAGFLAG_PRIMITIVE)) \
        rc = RTAsn1CursorGetUtf8String(pCursor, 0, &pThis->a_Name, #a_Name)

# define RTASN1TMPL_MEMBER_OPT_ITAG_EX(a_Name, a_Type, a_Api, a_uTag, a_fClue, a_Constraints) \
    if (RT_SUCCESS(rc) && RTAsn1CursorIsNextEx(pCursor, a_uTag, RTASN1TMPL_ITAG_F_EXPAND(a_fClue)) /** @todo || CER */) \
        rc = RT_CONCAT(a_Api,_DecodeAsn1)(pCursor, RTASN1CURSOR_GET_F_IMPLICIT, &pThis->a_Name, #a_Name)

# define RTASN1TMPL_MEMBER_OPT_ITAG_BITSTRING(a_Name, a_cMaxBits, a_uTag) \
    if (RT_SUCCESS(rc) && RTAsn1CursorIsNextEx(pCursor, a_uTag, ASN1_TAGCLASS_CONTEXT | ASN1_TAGFLAG_CONSTRUCTED)) \
        rc = RTAsn1CursorGetBitStringEx(pCursor, RTASN1CURSOR_GET_F_IMPLICIT, a_cMaxBits, &pThis->a_Name, #a_Name)

# define RTASN1TMPL_MEMBER_OPT_XTAG_EX(a_TnNm, a_CtxTagN, a_Name, a_Type, a_Api, a_uTag, a_Constraints) \
    if (RT_SUCCESS(rc) && RTAsn1CursorIsNextEx(pCursor, a_uTag, ASN1_TAGCLASS_CONTEXT | ASN1_TAGFLAG_CONSTRUCTED)) \
    { \
        RTASN1CURSOR CtxCursor; \
        rc = RT_CONCAT3(RTAsn1CursorGetContextTag,a_uTag,Cursor)(pCursor, 0, \
                                                                 &RT_CONCAT5(g_,RTASN1TMPL_INT_NAME,_XTAG_,a_Name,_Vtable), \
                                                                 &pThis->a_TnNm.a_CtxTagN, &CtxCursor, #a_TnNm); \
        if (RT_SUCCESS(rc)) \
        { \
            rc = RT_CONCAT(a_Api,_DecodeAsn1)(&CtxCursor, 0, &pThis->a_TnNm.a_Name, #a_Name); \
            if (RT_SUCCESS(rc)) \
                rc = RTAsn1CursorCheckEnd(&CtxCursor); \
        } \
    } do { } while (0)

# define RTASN1TMPL_MEMBER_OPT_ANY(a_Name, a_Type, a_Api) \
    if (RT_SUCCESS(rc) && pCursor->cbLeft > 0) \
        RTASN1TMPL_MEMBER_EX(a_Name, a_Type, a_Api, RT_NOTHING)

# define RTASN1TMPL_END_SEQCORE() \
    if (RT_SUCCESS(rc)) \
        rc = RTAsn1CursorCheckSeqEnd(&ThisCursor, &pThis->SeqCore); \
    if (RT_SUCCESS(rc)) \
        return VINF_SUCCESS; \
    RT_CONCAT(RTASN1TMPL_EXT_NAME,_Delete)(pThis); \
    RTASN1TMPL_END_COMMON()
# define RTASN1TMPL_END_SETCORE() \
    if (RT_SUCCESS(rc)) \
        rc = RTAsn1CursorCheckSetEnd(&ThisCursor, &pThis->SetCore); \
    if (RT_SUCCESS(rc)) \
        return VINF_SUCCESS; \
    RT_CONCAT(RTASN1TMPL_EXT_NAME,_Delete)(pThis); \
    RTASN1TMPL_END_COMMON()

# define RTASN1TMPL_BEGIN_PCHOICE() \
    RTASN1TMPL_BEGIN_COMMON(); \
    RT_NOREF_PV(fFlags); \
    RTAsn1Dummy_InitEx(&pThis->Dummy); \
    pThis->Dummy.Asn1Core.pOps = &RT_CONCAT3(g_,RTASN1TMPL_INT_NAME,_Vtable); \
    RTAsn1CursorInitAllocation(pCursor, &pThis->Allocation); \
    RTASN1CORE Asn1Peek; \
    int rc = RTAsn1CursorPeek(pCursor, &Asn1Peek); \
    if (RT_SUCCESS(rc)) \
    { \
        if (false) do {} while (0)
# define RTASN1TMPL_PCHOICE_ITAG_EX(a_uTag, a_enmChoice, a_PtrName, a_Name, a_Type, a_Api, a_fClue, a_Constraints) \
        else if (   Asn1Peek.uTag == (a_uTag) \
                 && (Asn1Peek.fClass == RTASN1TMPL_ITAG_F_EXPAND(a_fClue) /** @todo || CER */ ) ) \
        do { \
            pThis->enmChoice = a_enmChoice; \
            rc = RTAsn1MemAllocZ(&pThis->Allocation, (void **)&pThis->a_PtrName, sizeof(*pThis->a_PtrName)); \
            if (RT_SUCCESS(rc)) \
                rc = RT_CONCAT(a_Api,_DecodeAsn1)(pCursor, RTASN1CURSOR_GET_F_IMPLICIT, pThis->a_PtrName, #a_PtrName); \
        } while (0)
# define RTASN1TMPL_PCHOICE_XTAG_EX(a_uTag, a_enmChoice, a_PtrTnNm, a_CtxTagN, a_Name, a_Type, a_Api, a_Constraints) \
        else if (Asn1Peek.uTag == (a_uTag) && Asn1Peek.fClass == (ASN1_TAGCLASS_CONTEXT | ASN1_TAGFLAG_CONSTRUCTED)) \
        do { \
            pThis->enmChoice = a_enmChoice; \
            rc = RTAsn1MemAllocZ(&pThis->Allocation, (void **)&pThis->a_PtrTnNm, sizeof(*pThis->a_PtrTnNm)); \
            if (RT_SUCCESS(rc)) \
            { \
                RTASN1CURSOR CtxCursor; \
                rc = RT_CONCAT3(RTAsn1CursorGetContextTag,a_uTag,Cursor)(pCursor, 0, \
                                                                         &RT_CONCAT5(g_,RTASN1TMPL_INT_NAME,_PCHOICE_XTAG_,a_Name,_Vtable), \
                                                                         &pThis->a_PtrTnNm->a_CtxTagN, &CtxCursor, "T" #a_uTag); \
                if (RT_SUCCESS(rc)) \
                    rc = RT_CONCAT(a_Api,_DecodeAsn1)(&CtxCursor, RTASN1CURSOR_GET_F_IMPLICIT, \
                                                      &pThis->a_PtrTnNm->a_Name, #a_Name); \
                if (RT_SUCCESS(rc)) \
                    rc = RTAsn1CursorCheckEnd(&CtxCursor); \
            } \
        } while (0)
#define RTASN1TMPL_END_PCHOICE() \
        else \
            rc = RTAsn1CursorSetInfo(pCursor, VERR_GENERAL_FAILURE, "%s: Unknown choice: tag=%#x fClass=%#x", \
                                     pszErrorTag, Asn1Peek.uTag, Asn1Peek.fClass); \
        if (RT_SUCCESS(rc)) \
            return VINF_SUCCESS; \
    } \
    RT_CONCAT(RTASN1TMPL_EXT_NAME,_Delete)(pThis); \
    RTASN1TMPL_END_COMMON()


# define RTASN1TMPL_SET_SEQ_OF_COMMON(a_ItemType, a_ItemApi, a_OfApi, a_OfMember, a_fnGetCursor) \
    RTASN1TMPL_BEGIN_COMMON(); \
    RTASN1CURSOR ThisCursor; \
    int rc = a_fnGetCursor(pCursor, fFlags, &pThis->a_OfMember, &ThisCursor, pszErrorTag); \
    if (RT_SUCCESS(rc)) \
    { \
        pCursor = &ThisCursor; \
        pThis->a_OfMember.Asn1Core.pOps = &RT_CONCAT3(g_,RTASN1TMPL_INT_NAME,_Vtable); \
        RTAsn1CursorInitArrayAllocation(pCursor, &pThis->Allocation, sizeof(a_ItemType)); \
        \
        uint32_t i = 0; \
        while (   pCursor->cbLeft > 0 \
               && RT_SUCCESS(rc)) \
        { \
            rc = RTAsn1MemResizeArray(&pThis->Allocation, (void ***)&pThis->papItems, i, i + 1); \
            if (RT_SUCCESS(rc)) \
            { \
                rc = RT_CONCAT(a_ItemApi,_DecodeAsn1)(pCursor, 0, pThis->papItems[i], "papItems[#]"); \
                if (RT_SUCCESS(rc)) \
                { \
                    i++; \
                    pThis->cItems = i; \
                    continue; \
                } \
            } \
            break; \
        } \
        if (RT_SUCCESS(rc)) \
        { \
            rc = RTAsn1CursorCheckEnd(pCursor); \
            if (RT_SUCCESS(rc)) \
                return VINF_SUCCESS; \
        } \
        RT_CONCAT(RTASN1TMPL_EXT_NAME,_Delete)(pThis); \
    } \
    RTASN1TMPL_END_COMMON()
# define RTASN1TMPL_SEQ_OF(a_ItemType, a_ItemApi) \
    RTASN1TMPL_SET_SEQ_OF_COMMON(a_ItemType, a_ItemApi, RTAsn1SeqOfCore, SeqCore, RTAsn1CursorGetSequenceCursor)
# define RTASN1TMPL_SET_OF(a_ItemType, a_ItemApi) \
    RTASN1TMPL_SET_SEQ_OF_COMMON(a_ItemType, a_ItemApi, RTAsn1SetOfCore, SetCore, RTAsn1CursorGetSetCursor)


# define RTASN1TMPL_EXEC_DECODE(a_Expr) if (RT_SUCCESS(rc)) { a_Expr; }



#elif RTASN1TMPL_PASS == RTASN1TMPL_PASS_ENUM
/*
 *
 * Enumeration.
 *
 */
# define RTASN1TMPL_BEGIN_COMMON() \
RTASN1TMPL_DECL(int) RT_CONCAT(RTASN1TMPL_EXT_NAME,_Enum)(RT_CONCAT(P,RTASN1TMPL_TYPE) pThis, \
                                                          PFNRTASN1ENUMCALLBACK pfnCallback, \
                                                          uint32_t uDepth, void *pvUser) \
{ \
    if (!RT_CONCAT(RTASN1TMPL_EXT_NAME,_IsPresent)(pThis)) \
        return VINF_SUCCESS; \
    uDepth++; \
    int rc = VINF_SUCCESS

# define RTASN1TMPL_END_COMMON() \
    return rc; \
} RTASN1TMPL_SEMICOLON_DUMMY()

# define RTASN1TMPL_BEGIN_SEQCORE() RTASN1TMPL_BEGIN_COMMON()
# define RTASN1TMPL_BEGIN_SETCORE() RTASN1TMPL_BEGIN_COMMON()
# define RTASN1TMPL_MEMBER_EX(a_Name, a_Type, a_Api, a_Constraints) \
        if (rc == VINF_SUCCESS) \
            rc = pfnCallback(RT_CONCAT(a_Api,_GetAsn1Core)(&pThis->a_Name), #a_Name, uDepth, pvUser)
# define RTASN1TMPL_MEMBER_DYN_BEGIN(a_ObjIdMembNm, a_enmType, a_enmMembNm, a_Allocation) \
        if (rc == VINF_SUCCESS) \
            switch (pThis->a_enmMembNm) \
            { \
                default: rc = VERR_INTERNAL_ERROR_3; break
# define RTASN1TMPL_MEMBER_DYN_COMMON(a_UnionNm, a_PtrName, a_Type, a_Api, a_Allocation, a_enmMembNm, a_enmValue, a_IfStmt) \
                case a_enmValue: \
                    rc = pfnCallback(RT_CONCAT(a_Api,_GetAsn1Core)(pThis->a_UnionNm.a_PtrName), #a_UnionNm "." #a_PtrName, \
                                     uDepth, pvUser); \
                    break
# define RTASN1TMPL_MEMBER_DYN_END(a_ObjIdMembNm, a_enmType, a_enmMembNm, a_Allocation) \
                case RT_CONCAT(a_enmType,_NOT_PRESENT): break; \
            }
# define RTASN1TMPL_MEMBER_OPT_EX(a_Name, a_Type, a_Api, a_Constraints) \
        if (rc == VINF_SUCCESS && RT_CONCAT(a_Api,_IsPresent)(&pThis->a_Name)) \
            rc = pfnCallback(RT_CONCAT(a_Api,_GetAsn1Core)(&pThis->a_Name), #a_Name, uDepth, pvUser)
# define RTASN1TMPL_MEMBER_OPT_XTAG_EX(a_TnNm, a_CtxTagN, a_Name, a_Type, a_Api, a_uTag, a_Constraints) \
        if (rc == VINF_SUCCESS && RTASN1CORE_IS_PRESENT(&pThis->a_TnNm.a_CtxTagN.Asn1Core)) \
        { \
            rc = pfnCallback(&pThis->a_TnNm.a_CtxTagN.Asn1Core, #a_Name, uDepth, pvUser); \
        } do {} while (0)
# define RTASN1TMPL_END_SEQCORE()   RTASN1TMPL_END_COMMON()
# define RTASN1TMPL_END_SETCORE()   RTASN1TMPL_END_COMMON()


# define RTASN1TMPL_BEGIN_PCHOICE() \
    RTASN1TMPL_BEGIN_COMMON(); \
    switch (pThis->enmChoice) \
    { \
        default: rc = VERR_INTERNAL_ERROR_3; break
# define RTASN1TMPL_PCHOICE_ITAG_EX(a_uTag, a_enmChoice, a_PtrName, a_Name, a_Type, a_Api, a_fClue, a_Constraints) \
        case a_enmChoice: rc = pfnCallback(RT_CONCAT(a_Api,_GetAsn1Core)(pThis->a_PtrName), #a_PtrName, uDepth, pvUser); break
# define RTASN1TMPL_PCHOICE_XTAG_EX(a_uTag, a_enmChoice, a_PtrTnNm, a_CtxTagN, a_Name, a_Type, a_Api, a_Constraints) \
        case a_enmChoice: rc = pfnCallback(&pThis->a_PtrTnNm->a_CtxTagN.Asn1Core, "T" #a_uTag "." #a_CtxTagN, uDepth, pvUser); break
#define RTASN1TMPL_END_PCHOICE() \
    } \
    RTASN1TMPL_END_COMMON()

# define RTASN1TMPL_SET_SEQ_OF_COMMON(a_ItemType, a_ItemApi) \
    RTASN1TMPL_BEGIN_COMMON(); \
    for (uint32_t i = 0; i < pThis->cItems && rc == VINF_SUCCESS; i++) \
        rc = pfnCallback(RT_CONCAT(a_ItemApi,_GetAsn1Core)(pThis->papItems[i]), "papItems[#]", uDepth, pvUser); \
    RTASN1TMPL_END_COMMON()
# define RTASN1TMPL_SEQ_OF(a_ItemType, a_ItemApi) RTASN1TMPL_SET_SEQ_OF_COMMON(a_ItemType, a_ItemApi)
# define RTASN1TMPL_SET_OF(a_ItemType, a_ItemApi) RTASN1TMPL_SET_SEQ_OF_COMMON(a_ItemType, a_ItemApi)



#elif RTASN1TMPL_PASS == RTASN1TMPL_PASS_CLONE
/*
 *
 * Clone another instance of the type.
 *
 */
# define RTASN1TMPL_BEGIN_COMMON() \
RTASN1TMPL_DECL(int) RT_CONCAT(RTASN1TMPL_EXT_NAME,_Clone)(RT_CONCAT(P,RTASN1TMPL_TYPE) pThis, \
                                                           RT_CONCAT(PC,RTASN1TMPL_TYPE) pSrc, \
                                                           PCRTASN1ALLOCATORVTABLE pAllocator) \
{ \
    RT_ZERO(*pThis); \
    if (!RT_CONCAT(RTASN1TMPL_EXT_NAME,_IsPresent)(pSrc)) \
        return VINF_SUCCESS; \

# define RTASN1TMPL_END_COMMON() \
    return rc; \
} RTASN1TMPL_SEMICOLON_DUMMY()

# define RTASN1TMPL_BEGIN_SEQCORE() \
    RTASN1TMPL_BEGIN_COMMON(); \
    int rc = RTAsn1SequenceCore_Clone(&pThis->SeqCore, &RT_CONCAT3(g_, RTASN1TMPL_INT_NAME, _Vtable), &pSrc->SeqCore)
# define RTASN1TMPL_BEGIN_SETCORE() \
    RTASN1TMPL_BEGIN_COMMON(); \
    int rc = RTAsn1SetCore_Clone(&pThis->SetCore, &RT_CONCAT3(g_, RTASN1TMPL_INT_NAME, _Vtable), &pSrc->SetCore)

# define RTASN1TMPL_MEMBER_EX(a_Name, a_Type, a_Api, a_Constraints) \
    if (RT_SUCCESS(rc)) \
        rc = RT_CONCAT(a_Api,_Clone)(&pThis->a_Name, &pSrc->a_Name, pAllocator); \

# define RTASN1TMPL_MEMBER_DYN_BEGIN(a_ObjIdMembNm, a_enmType, a_enmMembNm, a_Allocation) \
    if (RT_SUCCESS(rc)) \
    { \
        RTAsn1MemInitAllocation(&pThis->Allocation, pAllocator); \
        pThis->a_enmMembNm = pSrc->a_enmMembNm; \
        switch (pSrc->a_enmMembNm) \
        { \
            default: rc = VERR_INTERNAL_ERROR_3; break
# define RTASN1TMPL_MEMBER_DYN_COMMON(a_UnionNm, a_PtrName, a_Type, a_Api, a_Allocation, a_enmMembNm, a_enmValue, a_IfStmt) \
            case a_enmValue: \
                rc = RTAsn1MemAllocZ(&pThis->a_Allocation, (void **)&pThis->a_UnionNm.a_PtrName, \
                                      sizeof(*pThis->a_UnionNm.a_PtrName)); \
                if (RT_SUCCESS(rc)) \
                    rc = RT_CONCAT(a_Api,_Clone)(pThis->a_UnionNm.a_PtrName, pSrc->a_UnionNm.a_PtrName, pAllocator); \
                break
# define RTASN1TMPL_MEMBER_DYN_END(a_ObjIdMembNm, a_enmType, a_enmMembNm, a_Allocation) \
            case RT_CONCAT(a_enmType,_NOT_PRESENT): break; \
        } \
     }

/* Optional members and members with defaults are the same as a normal member when cloning. */
# define RTASN1TMPL_MEMBER_OPT_UTF8_STRING_EX(a_Name, a_Constraints) \
    RTASN1TMPL_MEMBER_OPT_EX(a_Name, RTASN1STRING, RTAsn1Utf8String, a_Constraints RT_NOTHING)
# define RTASN1TMPL_MEMBER_OPT_XTAG_EX(a_TnNm, a_CtxTagN, a_Name, a_Type, a_Api, a_uTag, a_Constraints) \
    if (RTASN1CORE_IS_PRESENT(&pSrc->a_TnNm.a_CtxTagN.Asn1Core) && RT_SUCCESS(rc)) \
    { \
        rc = RT_CONCAT3(RTAsn1ContextTag,a_uTag,_Clone)(&pThis->a_TnNm.a_CtxTagN, &pSrc->a_TnNm.a_CtxTagN); \
        if (RT_SUCCESS(rc)) \
            rc = RT_CONCAT(a_Api,_Clone)(&pThis->a_TnNm.a_Name, &pSrc->a_TnNm.a_Name, pAllocator); \
    } do { } while (0)

# define RTASN1TMPL_END_SEQCORE() \
    if (RT_FAILURE(rc)) \
        RT_CONCAT(RTASN1TMPL_EXT_NAME,_Delete)(pThis); \
    RTASN1TMPL_END_COMMON()
# define RTASN1TMPL_END_SETCORE() RTASN1TMPL_END_SEQCORE()


# define RTASN1TMPL_BEGIN_PCHOICE() \
    RTASN1TMPL_BEGIN_COMMON(); \
    RTAsn1Dummy_InitEx(&pThis->Dummy); \
    pThis->Dummy.Asn1Core.pOps = &RT_CONCAT3(g_,RTASN1TMPL_INT_NAME,_Vtable); \
    RTAsn1MemInitAllocation(&pThis->Allocation, pAllocator); \
    int rc; \
    pThis->enmChoice = pSrc->enmChoice; \
    switch (pSrc->enmChoice) \
    { \
        default: rc = VERR_INTERNAL_ERROR_3; break
# define RTASN1TMPL_PCHOICE_ITAG_EX(a_uTag, a_enmChoice, a_PtrName, a_Name, a_Type, a_Api, a_fClue, a_Constraints) \
        case a_enmChoice: \
            rc = RTAsn1MemAllocZ(&pThis->Allocation, (void **)&pThis->a_PtrName, sizeof(*pThis->a_PtrName)); \
            if (RT_SUCCESS(rc)) \
                rc = RT_CONCAT(a_Api,_Clone)(pThis->a_PtrName, pSrc->a_PtrName, pAllocator); \
            break
# define RTASN1TMPL_PCHOICE_XTAG_EX(a_uTag, a_enmChoice, a_PtrTnNm, a_CtxTagN, a_Name, a_Type, a_Api, a_Constraints) \
        case a_enmChoice: /* A bit of presence paranoia here, but better safe than sorry... */ \
            rc = RTAsn1MemAllocZ(&pThis->Allocation, (void **)&pThis->a_PtrTnNm, sizeof(*pThis->a_PtrTnNm)); \
            if (RT_SUCCESS(rc) && RTASN1CORE_IS_PRESENT(&pSrc->a_PtrTnNm->a_CtxTagN.Asn1Core)) \
            { \
                RT_CONCAT3(RTAsn1ContextTag,a_uTag,_Clone)(&pThis->a_PtrTnNm->a_CtxTagN, &pSrc->a_PtrTnNm->a_CtxTagN); \
                rc = RT_CONCAT(a_Api,_Clone)(&pThis->a_PtrTnNm->a_Name, &pSrc->a_PtrTnNm->a_Name, pAllocator); \
            } \
            break
#define RTASN1TMPL_END_PCHOICE() \
    } \
    if (RT_FAILURE(rc)) \
        RT_CONCAT(RTASN1TMPL_EXT_NAME,_Delete)(pThis); \
    RTASN1TMPL_END_COMMON()


# define RTASN1TMPL_SET_SEQ_OF_COMMON(a_ItemType, a_ItemApi, a_OfApi, a_OfMember) \
    RTASN1TMPL_BEGIN_COMMON(); \
    int rc = RT_CONCAT(a_OfApi,_Clone)(&pThis->a_OfMember, &RT_CONCAT3(g_,RTASN1TMPL_INT_NAME,_Vtable), &pSrc->a_OfMember); \
    if (RT_SUCCESS(rc)) \
    { \
        RTAsn1MemInitArrayAllocation(&pThis->Allocation, pAllocator, sizeof(a_ItemType)); \
        uint32_t const cItems = pSrc->cItems; \
        if (cItems > 0) \
        { \
            rc = RTAsn1MemResizeArray(&pThis->Allocation, (void ***)&pThis->papItems, 0, cItems); \
            if (RT_SUCCESS(rc)) \
            { \
                uint32_t i = 0; \
                while (i < cItems) \
                { \
                    rc = RT_CONCAT(a_ItemApi,_Clone)(pThis->papItems[i], pSrc->papItems[i], pAllocator); \
                    if (RT_SUCCESS(rc)) \
                        pThis->cItems = ++i; \
                    else \
                    { \
                        pThis->cItems = i; \
                        RT_CONCAT(RTASN1TMPL_EXT_NAME,_Delete)(pThis); \
                        return rc; \
                    } \
                } \
            } \
            else \
                RT_ZERO(*pThis); \
        } \
    } \
    RTASN1TMPL_END_COMMON()
# define RTASN1TMPL_SEQ_OF(a_ItemType, a_ItemApi) RTASN1TMPL_SET_SEQ_OF_COMMON(a_ItemType, a_ItemApi, RTAsn1SeqOfCore, SeqCore)
# define RTASN1TMPL_SET_OF(a_ItemType, a_ItemApi) RTASN1TMPL_SET_SEQ_OF_COMMON(a_ItemType, a_ItemApi, RTAsn1SetOfCore, SetCore)

# define RTASN1TMPL_EXEC_CLONE(a_Expr) if (RT_SUCCESS(rc)) { a_Expr; }



#elif RTASN1TMPL_PASS == RTASN1TMPL_PASS_SETTERS_1
/*
 *
 * Member setter helpers.
 *
 */
# define RTASN1TMPL_BEGIN_SEQCORE()                                                 RTASN1TMPL_SEMICOLON_DUMMY()
# define RTASN1TMPL_BEGIN_SETCORE()                                                 RTASN1TMPL_SEMICOLON_DUMMY()
#if 1 /** @todo later */
# define RTASN1TMPL_MEMBER_EX(a_Name, a_Type, a_Api, a_Constraints)                     RTASN1TMPL_SEMICOLON_DUMMY()
#else
# define RTASN1TMPL_MEMBER_EX(a_Name, a_Type, a_Api, a_Constraints) \
    RTDECL(int) RT_CONCAT3(RTASN1TMPL_EXT_NAME,_Set,a_Name)(RTASN1TMPL_TYPE *pThis, a_Type const *pValue, \
                                                            PCRTASN1ALLOCATORVTABLE pAllocator) \
    { \
        if (RT_CONCAT(a_Api,_IsPresent)(&pThis->a_Name)) \
            RT_CONCAT(a_Api,_Delete)(&pThis->a_Name); \
        return RT_CONCAT(a_Api,_Clone)(&pThis->a_Name, pValue, pAllocator, true /* fResetImplicit */); \
    } RTASN1TMPL_SEMICOLON_DUMMY()
#endif

# define RTASN1TMPL_MEMBER_DYN_BEGIN(a_ObjIdMembNm, a_enmType, a_enmMembNm, a_Allocation)           RTASN1TMPL_SEMICOLON_DUMMY()
# define RTASN1TMPL_MEMBER_DYN_END(a_ObjIdMembNm, a_enmType, a_enmMembNm, a_Allocation)             RTASN1TMPL_SEMICOLON_DUMMY()
# define RTASN1TMPL_END_SEQCORE()                                                                   RTASN1TMPL_SEMICOLON_DUMMY()
# define RTASN1TMPL_END_SETCORE()                                                                   RTASN1TMPL_SEMICOLON_DUMMY()


# define RTASN1TMPL_BEGIN_PCHOICE()                                                                 RTASN1TMPL_SEMICOLON_DUMMY()
# define RTASN1TMPL_PCHOICE_ITAG_EX(a_uTag, a_enmChoice, a_PtrName, a_Name, a_Type, a_Api, a_fClue, a_Constraints)   RTASN1TMPL_SEMICOLON_DUMMY()
# define RTASN1TMPL_PCHOICE_XTAG_EX(a_uTag, a_enmChoice, a_PtrTnNm, a_CtxTagN, a_Name, a_Type, a_Api, a_Constraints) RTASN1TMPL_SEMICOLON_DUMMY()
# define RTASN1TMPL_END_PCHOICE()                                                                   RTASN1TMPL_SEMICOLON_DUMMY()


# define RTASN1TMPL_SEQ_OF(a_ItemType, a_ItemApi)                                                   RTASN1TMPL_SEMICOLON_DUMMY()
# define RTASN1TMPL_SET_OF(a_ItemType, a_ItemApi)                                                   RTASN1TMPL_SEMICOLON_DUMMY()



#elif RTASN1TMPL_PASS == RTASN1TMPL_PASS_SETTERS_2
/*
 *
 * Member setters.
 *
 */
# define RTASN1TMPL_BEGIN_SEQCORE()                                                                 RTASN1TMPL_SEMICOLON_DUMMY()
# define RTASN1TMPL_BEGIN_SETCORE()                                                                 RTASN1TMPL_SEMICOLON_DUMMY()
# define RTASN1TMPL_MEMBER_EX(a_Name, a_Type, a_Api, a_Constraints)                                 RTASN1TMPL_SEMICOLON_DUMMY()
# define RTASN1TMPL_MEMBER_DYN_BEGIN(a_ObjIdMembNm, a_enmType, a_enmMembNm, a_Allocation)           RTASN1TMPL_SEMICOLON_DUMMY()

# define RTASN1TMPL_MEMBER_DYN(a_UnionNm, a_PtrName, a_Name, a_Type, a_Api, a_Allocation, a_ObjIdMembNm, a_enmMembNm, a_enmValue, a_szObjId) \
RTASN1TMPL_DECL(int) RT_CONCAT3(RTASN1TMPL_EXT_NAME,_Set,a_Name)(RT_CONCAT(P,RTASN1TMPL_TYPE) pThis, \
                                                                 RT_CONCAT(PC,a_Type) pToClone,\
                                                                 PCRTASN1ALLOCATORVTABLE pAllocator) \
{ \
    AssertPtr(pThis); AssertPtrNull(pToClone); Assert(!pToClone || RT_CONCAT(a_Api,_IsPresent)(pToClone)); \
    AssertReturn(pThis->a_UnionNm.a_PtrName == NULL, VERR_INVALID_STATE); /* for now */ \
    /* Set the type */ \
    if (RTAsn1ObjId_IsPresent(&pThis->a_ObjIdMembNm)) \
        RTAsn1ObjId_Delete(&pThis->a_ObjIdMembNm); \
    int rc = RTAsn1ObjId_InitFromString(&pThis->a_ObjIdMembNm, a_szObjId, pAllocator); \
    if (RT_SUCCESS(rc)) \
    { \
        pThis->a_enmMembNm = a_enmValue; \
        \
        /* Allocate memory for the structure we're targeting. */ \
        rc = RTAsn1MemAllocZ(&pThis->a_Allocation, (void **)&pThis->a_UnionNm.a_PtrName, sizeof(*pThis->a_UnionNm.a_PtrName)); \
        if (RT_SUCCESS(rc)) \
        { \
            if (pToClone) /* If nothing to clone, just initialize the structure. */ \
                rc = RT_CONCAT(a_Api,_Clone)(pThis->a_UnionNm.a_PtrName, pToClone, pAllocator); \
            else \
                rc = RT_CONCAT(a_Api,_Init)(pThis->a_UnionNm.a_PtrName, pAllocator); \
        } \
    } \
    return rc; \
} RTASN1TMPL_SEMICOLON_DUMMY()



# define RTASN1TMPL_MEMBER_DYN_END(a_ObjIdMembNm, a_enmType, a_enmMembNm, a_Allocation)             RTASN1TMPL_SEMICOLON_DUMMY()
# define RTASN1TMPL_END_SEQCORE()                                                                   RTASN1TMPL_SEMICOLON_DUMMY()
# define RTASN1TMPL_END_SETCORE()                                                                   RTASN1TMPL_SEMICOLON_DUMMY()

# define RTASN1TMPL_MEMBER_OPT_ITAG_EX(a_Name, a_Type, a_Api, a_uTag, a_fClue, a_Constraints) \
RTASN1TMPL_DECL(int) RT_CONCAT3(RTASN1TMPL_EXT_NAME,_Set,a_Name)(RT_CONCAT(P,RTASN1TMPL_TYPE) pThis, \
                                                                 RT_CONCAT(PC,a_Type) pToClone,\
                                                                 PCRTASN1ALLOCATORVTABLE pAllocator) \
{ \
    AssertPtr(pThis); AssertPtrNull(pToClone); Assert(!pToClone || RT_CONCAT(a_Api,_IsPresent)(pToClone)); \
    if (RT_CONCAT(a_Api,_IsPresent)(&pThis->a_Name)) \
        RT_CONCAT(a_Api,_Delete)(&pThis->a_Name); \
    \
    int rc; \
    if (pToClone) \
        rc = RT_CONCAT(a_Api,_Clone)(&pThis->a_Name, pToClone, pAllocator); \
    else \
        rc = RT_CONCAT(a_Api,_Init)(&pThis->a_Name, pAllocator); \
    if (RT_SUCCESS(rc)) \
    { \
        RTAsn1Core_ResetImplict(RT_CONCAT(a_Api,_GetAsn1Core)(&pThis->a_Name)); /* probably not needed */ \
        rc = RTAsn1Core_SetTagAndFlags(RT_CONCAT(a_Api,_GetAsn1Core)(&pThis->a_Name), \
                                       a_uTag, RTASN1TMPL_ITAG_F_EXPAND(a_fClue)); \
    } \
    return rc; \
} RTASN1TMPL_SEMICOLON_DUMMY()

# define RTASN1TMPL_MEMBER_OPT_XTAG_EX(a_TnNm, a_CtxTagN, a_Name, a_Type, a_Api, a_uTag, a_Constraints) \
RTASN1TMPL_DECL(int) RT_CONCAT3(RTASN1TMPL_EXT_NAME,_Set,a_Name)(RT_CONCAT(P,RTASN1TMPL_TYPE) pThis, \
                                                                 RT_CONCAT(PC,a_Type) pToClone,\
                                                                 PCRTASN1ALLOCATORVTABLE pAllocator) \
{ \
    AssertPtr(pThis); AssertPtrNull(pToClone); Assert(!pToClone || RT_CONCAT(a_Api,_IsPresent)(pToClone)); \
    if (RTASN1CORE_IS_PRESENT(&pThis->a_TnNm.a_CtxTagN.Asn1Core)) \
        RT_CONCAT(a_Api,_Delete)(&pThis->a_TnNm.a_Name); \
    \
    int rc = RT_CONCAT3(RTAsn1ContextTag,a_uTag,_Init)(&pThis->a_TnNm.a_CtxTagN, \
                                                       &RT_CONCAT5(g_,RTASN1TMPL_INT_NAME,_XTAG_,a_Name,_Vtable), \
                                                       pAllocator); \
    if (RT_SUCCESS(rc)) \
    { \
        if (pToClone) \
            rc = RT_CONCAT(a_Api,_Clone)(&pThis->a_TnNm.a_Name, pToClone, pAllocator); \
        else \
            rc = RT_CONCAT(a_Api,_Init)(&pThis->a_TnNm.a_Name, pAllocator); \
        if (RT_SUCCESS(rc) && pToClone) \
            RTAsn1Core_ResetImplict(RT_CONCAT(a_Api,_GetAsn1Core)(&pThis->a_TnNm.a_Name)); \
    } \
    return rc; \
} RTASN1TMPL_SEMICOLON_DUMMY()

# define RTASN1TMPL_BEGIN_PCHOICE() RTASN1TMPL_SEMICOLON_DUMMY()

# define RTASN1TMPL_PCHOICE_ITAG_EX(a_uTag, a_enmChoice, a_PtrName, a_Name, a_Type, a_Api, a_fClue, a_Constraints) \
RTASN1TMPL_DECL(int) RT_CONCAT3(RTASN1TMPL_EXT_NAME,_Set,a_Name)(RT_CONCAT(P,RTASN1TMPL_TYPE) pThis, \
                                                                 RT_CONCAT(PC,a_Type) pToClone,\
                                                                 PCRTASN1ALLOCATORVTABLE pAllocator) \
{ \
    AssertPtrNull(pToClone); AssertPtr(pThis); \
    RT_CONCAT(RTASN1TMPL_EXT_NAME,_Delete)(pThis); /* See _Init. */ \
    RTAsn1Dummy_InitEx(&pThis->Dummy); \
    pThis->Dummy.Asn1Core.pOps = &RT_CONCAT3(g_,RTASN1TMPL_INT_NAME,_Vtable); \
    RTAsn1MemInitAllocation(&pThis->Allocation, pAllocator); \
    pThis->enmChoice = a_enmChoice; \
    int rc = RTAsn1MemAllocZ(&pThis->Allocation, (void **)&pThis->a_PtrName, sizeof(*pThis->a_PtrName)); \
    if (RT_SUCCESS(rc)) \
    { \
        if (pToClone) \
            rc = RT_CONCAT(a_Api,_Clone)(pThis->a_PtrName, pToClone, pAllocator); \
        else \
            rc = RT_CONCAT(a_Api,_Init)(pThis->a_PtrName, pAllocator); \
        if (RT_SUCCESS(rc)) \
        { \
            if (pToClone) \
                RTAsn1Core_ResetImplict(RT_CONCAT(a_Api,_GetAsn1Core)(pThis->a_PtrName)); \
            rc = RTAsn1Core_SetTagAndFlags(RT_CONCAT(a_Api,_GetAsn1Core)(pThis->a_PtrName), \
                                           a_uTag, RTASN1TMPL_ITAG_F_EXPAND(a_fClue)); \
        } \
    } \
    return rc; \
} RTASN1TMPL_SEMICOLON_DUMMY()

# define RTASN1TMPL_PCHOICE_XTAG_EX(a_uTag, a_enmChoice, a_PtrTnNm, a_CtxTagN, a_Name, a_Type, a_Api, a_Constraints) \
RTASN1TMPL_DECL(int) RT_CONCAT3(RTASN1TMPL_EXT_NAME,_Set,a_Name)(RT_CONCAT(P,RTASN1TMPL_TYPE) pThis, \
                                                                 RT_CONCAT(PC,a_Type) pToClone,\
                                                                 PCRTASN1ALLOCATORVTABLE pAllocator) \
{ \
    AssertPtr(pThis); AssertPtrNull(pToClone); Assert(!pToClone || RT_CONCAT(a_Api,_IsPresent)(pToClone)); \
    RT_CONCAT(RTASN1TMPL_EXT_NAME,_Delete)(pThis); /* See _Init. */ \
    RTAsn1Dummy_InitEx(&pThis->Dummy); \
    pThis->Dummy.Asn1Core.pOps = &RT_CONCAT3(g_,RTASN1TMPL_INT_NAME,_Vtable); \
    RTAsn1MemInitAllocation(&pThis->Allocation, pAllocator); \
    pThis->enmChoice = a_enmChoice; \
    int rc = RTAsn1MemAllocZ(&pThis->Allocation, (void **)&pThis->a_PtrTnNm, sizeof(*pThis->a_PtrTnNm)); \
    if (RT_SUCCESS(rc)) \
    { \
        rc = RT_CONCAT3(RTAsn1ContextTag,a_uTag,_Init)(&pThis->a_PtrTnNm->a_CtxTagN, \
                                                       &RT_CONCAT5(g_,RTASN1TMPL_INT_NAME,_PCHOICE_XTAG_,a_Name,_Vtable), \
                                                       pAllocator); \
        if (RT_SUCCESS(rc)) \
        { \
            if (pToClone) \
                rc = RT_CONCAT(a_Api,_Clone)(&pThis->a_PtrTnNm->a_Name, pToClone, pAllocator); \
            else \
                rc = RT_CONCAT(a_Api,_Init)(&pThis->a_PtrTnNm->a_Name, pAllocator); \
            if (RT_SUCCESS(rc) && pToClone) \
                RTAsn1Core_ResetImplict(RT_CONCAT(a_Api,_GetAsn1Core)(&pThis->a_PtrTnNm->a_Name)); \
        } \
    } \
    return rc; \
} RTASN1TMPL_SEMICOLON_DUMMY()

# define RTASN1TMPL_END_PCHOICE() RTASN1TMPL_SEMICOLON_DUMMY()


# define RTASN1TMPL_SEQ_OF(a_ItemType, a_ItemApi)   RTASN1TMPL_SEMICOLON_DUMMY()
# define RTASN1TMPL_SET_OF(a_ItemType, a_ItemApi)   RTASN1TMPL_SEMICOLON_DUMMY()


#elif RTASN1TMPL_PASS == RTASN1TMPL_PASS_ARRAY
/*
 *
 * Array operations.
 *
 */
# define RTASN1TMPL_BEGIN_SEQCORE()                                                                 RTASN1TMPL_SEMICOLON_DUMMY()
# define RTASN1TMPL_BEGIN_SETCORE()                                                                 RTASN1TMPL_SEMICOLON_DUMMY()
# define RTASN1TMPL_MEMBER_EX(a_Name, a_Type, a_Api, a_Constraints)                                 RTASN1TMPL_SEMICOLON_DUMMY()
# define RTASN1TMPL_MEMBER_DYN_BEGIN(a_ObjIdMembNm, a_enmType, a_enmMembNm, a_Allocation)           RTASN1TMPL_SEMICOLON_DUMMY()
# define RTASN1TMPL_MEMBER_DYN_END(a_ObjIdMembNm, a_enmType, a_enmMembNm, a_Allocation)             RTASN1TMPL_SEMICOLON_DUMMY()
# define RTASN1TMPL_END_SEQCORE()                                                                   RTASN1TMPL_SEMICOLON_DUMMY()
# define RTASN1TMPL_END_SETCORE()                                                                   RTASN1TMPL_SEMICOLON_DUMMY()
# define RTASN1TMPL_BEGIN_PCHOICE()                                                                 RTASN1TMPL_SEMICOLON_DUMMY()
# define RTASN1TMPL_PCHOICE_ITAG_EX(a_uTag, a_enmChoice, a_PtrName, a_Name, a_Type, a_Api, a_fClue, a_Constraints) \
                                                                                                    RTASN1TMPL_SEMICOLON_DUMMY()
# define RTASN1TMPL_PCHOICE_XTAG_EX(a_uTag, a_enmChoice, a_PtrTnNm, a_CtxTagN, a_Name, a_Type, a_Api, a_Constraints) \
                                                                                                    RTASN1TMPL_SEMICOLON_DUMMY()
# define RTASN1TMPL_END_PCHOICE()                                                                   RTASN1TMPL_SEMICOLON_DUMMY()

# define RTASN1TMPL_SET_SEQ_OF_COMMON(a_ItemType, a_ItemApi) \
    RTASN1TMPL_DECL(int) RT_CONCAT(RTASN1TMPL_EXT_NAME,_Erase)(RT_CONCAT(P,RTASN1TMPL_TYPE) pThis, uint32_t iPosition) \
    { \
        /* Check and adjust iPosition. */ \
        uint32_t const cItems = pThis->cItems; \
        if (iPosition < cItems) \
        { /* likely */ } \
        else \
        { \
            AssertReturn(iPosition == UINT32_MAX, VERR_OUT_OF_RANGE); \
            AssertReturn(cItems > 0, VERR_OUT_OF_RANGE); \
            iPosition = cItems - 1; \
        } \
        \
        /* Delete the entry instance. */ \
        RT_CONCAT(P, a_ItemType) pErased = pThis->papItems[iPosition]; \
        if (RT_CONCAT(a_ItemApi,_IsPresent)(pErased)) \
            RT_CONCAT(a_ItemApi,_Delete)(pErased); \
        \
        /* If not the final entry, shift the other entries up and place the erased on at the end. */ \
        if (iPosition < cItems - 1) \
        { \
            memmove(&pThis->papItems[iPosition], &pThis->papItems[iPosition + 1], (cItems - iPosition - 1) * sizeof(void *)); \
            pThis->papItems[cItems - 1] = pErased; \
        } \
        /* Commit the new array size. */ \
        pThis->cItems = cItems - 1; \
        \
        /* Call the allocator to resize the array (ignore return). */ \
        RTAsn1MemResizeArray(&pThis->Allocation, (void ***)&pThis->papItems, cItems - 1, cItems); \
        return VINF_SUCCESS; \
    } \
    \
    RTASN1TMPL_DECL(int) RT_CONCAT(RTASN1TMPL_EXT_NAME,_InsertEx)(RT_CONCAT(P,RTASN1TMPL_TYPE) pThis, uint32_t iPosition, \
                                                                  RT_CONCAT(PC, a_ItemType) pToClone, \
                                                                  PCRTASN1ALLOCATORVTABLE pAllocator, uint32_t *piActualPos) \
    { \
        /* Check and adjust iPosition. */ \
        uint32_t const cItems = pThis->cItems; \
        if (iPosition <= cItems) \
        { /* likely */ } \
        else \
        { \
            AssertReturn(iPosition == UINT32_MAX, VERR_OUT_OF_RANGE); \
            iPosition = cItems; \
        } \
        \
        /* Ensure we've got space in the array. */ \
        int rc = RTAsn1MemResizeArray(&pThis->Allocation, (void ***)&pThis->papItems, cItems, cItems + 1); \
        if (RT_SUCCESS(rc)) \
        { \
            /* Initialize the new entry (which is currently at the end of the array) either with defaults or as a clone. */ \
            RT_CONCAT(P,a_ItemType) pInserted = pThis->papItems[cItems]; \
            if (RT_CONCAT(a_ItemApi,_IsPresent)(pToClone)) \
                rc = RT_CONCAT(a_ItemApi,_Clone)(pInserted, pToClone, pAllocator); \
            else \
                rc = RT_CONCAT(a_ItemApi,_Init)(pInserted, pAllocator); \
            if (RT_SUCCESS(rc)) \
            { \
                pThis->cItems = cItems + 1; \
                \
                /* If not inserting at the end of the array, shift existing items out of the way and insert the new as req. */ \
                if (iPosition != cItems) \
                { \
                    memmove(&pThis->papItems[iPosition + 1], &pThis->papItems[iPosition], (cItems - iPosition) * sizeof(void *)); \
                    pThis->papItems[iPosition] = pInserted; \
                } \
                \
                /* Done! */ \
                if (piActualPos) \
                    *piActualPos = iPosition; \
                return VINF_SUCCESS; \
            } \
            RTAsn1MemResizeArray(&pThis->Allocation, (void ***)&pThis->papItems, cItems + 1, cItems); \
        } \
        return rc; \
    } RTASN1TMPL_SEMICOLON_DUMMY()

# define RTASN1TMPL_SEQ_OF(a_ItemType, a_ItemApi)   RTASN1TMPL_SET_SEQ_OF_COMMON(a_ItemType, a_ItemApi)
# define RTASN1TMPL_SET_OF(a_ItemType, a_ItemApi)   RTASN1TMPL_SET_SEQ_OF_COMMON(a_ItemType, a_ItemApi)


#elif RTASN1TMPL_PASS == RTASN1TMPL_PASS_COMPARE
/*
 *
 * Compare two instances of the type.
 *
 */
# define RTASN1TMPL_BEGIN_COMMON() \
RTASN1TMPL_DECL(int) RT_CONCAT(RTASN1TMPL_EXT_NAME,_Compare)(RT_CONCAT(PC,RTASN1TMPL_TYPE) pLeft, \
                                                             RT_CONCAT(PC,RTASN1TMPL_TYPE) pRight) \
{ \
    if (!RT_CONCAT(RTASN1TMPL_EXT_NAME,_IsPresent)(pLeft)) \
        return 0 - (int)RT_CONCAT(RTASN1TMPL_EXT_NAME,_IsPresent)(pRight); \
    if (!RT_CONCAT(RTASN1TMPL_EXT_NAME,_IsPresent)(pRight)) \
        return -1; \
    int iDiff = 0

# define RTASN1TMPL_END_COMMON() \
    return iDiff; \
} RTASN1TMPL_SEMICOLON_DUMMY()

# define RTASN1TMPL_BEGIN_SEQCORE() RTASN1TMPL_BEGIN_COMMON()
# define RTASN1TMPL_BEGIN_SETCORE() RTASN1TMPL_BEGIN_COMMON()
# define RTASN1TMPL_MEMBER_EX(a_Name, a_Type, a_Api, a_Constraints) \
    if (!iDiff) \
        iDiff = RT_CONCAT(a_Api,_Compare)(&pLeft->a_Name, &pRight->a_Name)
# define RTASN1TMPL_MEMBER_DYN_BEGIN(a_ObjIdMembNm, a_enmType, a_enmMembNm, a_Allocation) \
    if (!iDiff && pLeft->a_enmMembNm != pRight->a_enmMembNm) \
        iDiff = pLeft->a_enmMembNm < pRight->a_enmMembNm ? -1 : 1; \
    else if (!iDiff) \
        switch (pLeft->a_enmMembNm) \
        { \
            default: break
# define RTASN1TMPL_MEMBER_DYN_COMMON(a_UnionNm, a_PtrName, a_Type, a_Api, a_Allocation, a_enmMembNm, a_enmValue, a_IfStmt) \
            case a_enmValue: iDiff = RT_CONCAT(a_Api,_Compare)(pLeft->a_UnionNm.a_PtrName, pRight->a_UnionNm.a_PtrName); break
# define RTASN1TMPL_MEMBER_DYN_END(a_ObjIdMembNm, a_enmType, a_enmMembNm, a_Allocation) \
            case RT_CONCAT(a_enmType,_NOT_PRESENT): break; \
        }
# define RTASN1TMPL_MEMBER_OPT_XTAG_EX(a_TnNm, a_CtxTagN, a_Name, a_Type, a_Api, a_uTag, a_Constraints) \
    if (!iDiff) \
    { \
        if (RTASN1CORE_IS_PRESENT(&pLeft->a_TnNm.a_CtxTagN.Asn1Core)) \
        { \
            if (RTASN1CORE_IS_PRESENT(&pRight->a_TnNm.a_CtxTagN.Asn1Core)) \
                iDiff = RT_CONCAT(a_Api,_Compare)(&pLeft->a_TnNm.a_Name, &pRight->a_TnNm.a_Name); \
            else \
                iDiff = -1; \
        } \
        else \
            iDiff = 0 - (int)RTASN1CORE_IS_PRESENT(&pRight->a_TnNm.a_CtxTagN.Asn1Core); \
    } do { } while (0)
# define RTASN1TMPL_END_SEQCORE()   RTASN1TMPL_END_COMMON()
# define RTASN1TMPL_END_SETCORE()   RTASN1TMPL_END_COMMON()

# define RTASN1TMPL_BEGIN_PCHOICE() \
    RTASN1TMPL_BEGIN_COMMON(); \
    if (pLeft->enmChoice != pRight->enmChoice) \
        return pLeft->enmChoice < pRight->enmChoice ? -1 : 1; \
    switch (pLeft->enmChoice) \
    { \
        default: break
# define RTASN1TMPL_PCHOICE_ITAG_EX(a_uTag, a_enmChoice, a_PtrName, a_Name, a_Type, a_Api, a_fClue, a_Constraints) \
        case a_enmChoice: iDiff = RT_CONCAT(a_Api,_Compare)(pLeft->a_PtrName, pRight->a_PtrName); break
# define RTASN1TMPL_PCHOICE_XTAG_EX(a_uTag, a_enmChoice, a_PtrTnNm, a_CtxTagN, a_Name, a_Type, a_Api, a_Constraints) \
        case a_enmChoice: iDiff = RT_CONCAT(a_Api,_Compare)(&pLeft->a_PtrTnNm->a_Name, &pRight->a_PtrTnNm->a_Name); break
#define RTASN1TMPL_END_PCHOICE() \
    } \
    RTASN1TMPL_END_COMMON()


# define RTASN1TMPL_SET_SEQ_OF_COMMON(a_ItemType, a_ItemApi) \
    RTASN1TMPL_BEGIN_COMMON(); \
    uint32_t cItems = pLeft->cItems; \
    if (cItems == pRight->cItems) \
        for (uint32_t i = 0; iDiff == 0 && i < cItems; i++) \
            iDiff = RT_CONCAT(a_ItemApi,_Compare)(pLeft->papItems[i], pRight->papItems[i]); \
    else \
        iDiff = cItems < pRight->cItems ? -1 : 1; \
    RTASN1TMPL_END_COMMON()
# define RTASN1TMPL_SEQ_OF(a_ItemType, a_ItemApi) RTASN1TMPL_SET_SEQ_OF_COMMON(a_ItemType, a_ItemApi)
# define RTASN1TMPL_SET_OF(a_ItemType, a_ItemApi) RTASN1TMPL_SET_SEQ_OF_COMMON(a_ItemType, a_ItemApi)



#elif RTASN1TMPL_PASS == RTASN1TMPL_PASS_CHECK_SANITY
/*
 *
 * Checks the sanity of the type.
 *
 */
# ifndef RTASN1TMPL_SANITY_CHECK_EXPR
#  define RTASN1TMPL_SANITY_CHECK_EXPR() VINF_SUCCESS
# endif
# define RTASN1TMPL_BEGIN_COMMON() \
RTASN1TMPL_DECL(int) RT_CONCAT(RTASN1TMPL_EXT_NAME,_CheckSanity)(RT_CONCAT(PC,RTASN1TMPL_TYPE) pThis, uint32_t fFlags,  \
                                                                 PRTERRINFO pErrInfo, const char *pszErrorTag) \
{ \
    if (RT_LIKELY(RT_CONCAT(RTASN1TMPL_EXT_NAME,_IsPresent)(pThis))) \
    { /* likely */ } \
    else \
        return RTErrInfoSetF(pErrInfo, VERR_GENERAL_FAILURE, "%s: Missing (%s).", pszErrorTag, RT_XSTR(RTASN1TMPL_TYPE)); \
    int rc = VINF_SUCCESS

# define RTASN1TMPL_END_COMMON() \
    if (RT_SUCCESS(rc)) \
        rc = (RTASN1TMPL_SANITY_CHECK_EXPR()); \
    return rc; \
} RTASN1TMPL_SEMICOLON_DUMMY()

# define RTASN1TMPL_BEGIN_SEQCORE() RTASN1TMPL_BEGIN_COMMON()
# define RTASN1TMPL_BEGIN_SETCORE() RTASN1TMPL_BEGIN_COMMON()
# define RTASN1TMPL_MEMBER_EX(a_Name, a_Type, a_Api, a_Constraints) \
    if (RT_SUCCESS(rc)) \
    { \
        if (RT_LIKELY(RT_CONCAT(a_Api,_IsPresent)(&pThis->a_Name))) \
        { \
            rc = RT_CONCAT(a_Api,_CheckSanity)(&pThis->a_Name, fFlags & RTASN1_CHECK_SANITY_F_COMMON_MASK, \
                                               pErrInfo, RT_XSTR(RTASN1TMPL_TYPE) "::" #a_Name); \
            { a_Constraints } \
        } \
        else \
            rc = RTErrInfoSetF(pErrInfo, VERR_GENERAL_FAILURE, "%s: Missing member %s (%s).", \
                               pszErrorTag, #a_Name, RT_XSTR(RTASN1TMPL_TYPE)); \
    } do {} while (0)
# define RTASN1TMPL_MEMBER_DYN_BEGIN(a_ObjIdMembNm, a_enmType, a_enmMembNm, a_Allocation) \
    if (RT_SUCCESS(rc)) \
        switch (pThis->a_enmMembNm) \
        { \
            default: \
                rc = RTErrInfoSetF(pErrInfo, VERR_GENERAL_FAILURE, \
                                   "%s: Invalid " #a_enmMembNm " value: %d", pszErrorTag, pThis->a_enmMembNm); \
                break
# define RTASN1TMPL_MEMBER_DYN_COMMON(a_UnionNm, a_PtrName, a_Type, a_Api, a_Allocation, a_enmMembNm, a_enmValue, a_IfStmt) \
            case a_enmValue: \
                rc = RT_CONCAT(a_Api,_CheckSanity)(pThis->a_UnionNm.a_PtrName, fFlags & RTASN1_CHECK_SANITY_F_COMMON_MASK, \
                                                   pErrInfo, RT_XSTR(RTASN1TMPL_TYPE) "::" #a_UnionNm "." #a_PtrName); \
                break
# define RTASN1TMPL_MEMBER_DYN_END(a_ObjIdMembNm, a_enmType, a_enmMembNm, a_Allocation) \
            case RT_CONCAT(a_enmType,_NOT_PRESENT): \
                rc = RTErrInfoSetF(pErrInfo, VERR_GENERAL_FAILURE, \
                                   "%s: Invalid " #a_enmMembNm " value: " #a_enmType "_NOT_PRESENT", pszErrorTag); \
                break; \
        }
# define RTASN1TMPL_MEMBER_OPT_EX(a_Name, a_Type, a_Api, a_Constraints) \
    if (RT_SUCCESS(rc) && RT_CONCAT(a_Api,_IsPresent)(&pThis->a_Name)) \
    { \
        rc = RT_CONCAT(a_Api,_CheckSanity)(&pThis->a_Name, fFlags & RTASN1_CHECK_SANITY_F_COMMON_MASK, \
                                           pErrInfo, RT_XSTR(RTASN1TMPL_TYPE) "::" #a_Name); \
        { a_Constraints } \
    }
# define RTASN1TMPL_MEMBER_OPT_XTAG_EX(a_TnNm, a_CtxTagN, a_Name, a_Type, a_Api, a_uTag, a_Constraints) \
    if (RT_SUCCESS(rc)) \
    { \
        bool const fOuterPresent = RTASN1CORE_IS_PRESENT(&pThis->a_TnNm.a_CtxTagN.Asn1Core); \
        bool const fInnerPresent = RT_CONCAT(a_Api,_IsPresent)(&pThis->a_TnNm.a_Name); \
        if (fOuterPresent && fInnerPresent) \
        { \
            rc = RT_CONCAT(a_Api,_CheckSanity)(&pThis->a_TnNm.a_Name, fFlags & RTASN1_CHECK_SANITY_F_COMMON_MASK, \
                                               pErrInfo, RT_XSTR(RTASN1TMPL_TYPE) "::" #a_Name); \
            { a_Constraints } \
        } \
        else if (RT_LIKELY(RTASN1CORE_IS_PRESENT(&pThis->a_TnNm.a_CtxTagN.Asn1Core) == fInnerPresent)) \
        { /* likely */ } \
        else \
            rc = RTErrInfoSetF(pErrInfo, VERR_GENERAL_FAILURE, \
                               "%s::" #a_TnNm "." #a_Name ": Explict tag precense mixup; " #a_CtxTagN "=%d " #a_Name "=%d.", \
                               pszErrorTag, fOuterPresent, fInnerPresent); \
    } do { } while (0)
# define RTASN1TMPL_END_SEQCORE()   RTASN1TMPL_END_COMMON()
# define RTASN1TMPL_END_SETCORE()   RTASN1TMPL_END_COMMON()


# define RTASN1TMPL_BEGIN_PCHOICE() \
    RTASN1TMPL_BEGIN_COMMON(); \
    switch (pThis->enmChoice) \
    { \
        default: \
            rc = RTErrInfoSetF(pErrInfo, VERR_GENERAL_FAILURE, \
                               "%s: Invalid enmChoice value: %d", pszErrorTag, pThis->enmChoice); \
            break
# define RTASN1TMPL_PCHOICE_ITAG_EX(a_uTag, a_enmChoice, a_PtrName, a_Name, a_Type, a_Api, a_fClue, a_Constraints) \
        case a_enmChoice: \
            if (pThis->a_PtrName && RT_CONCAT(a_Api,_IsPresent)(pThis->a_PtrName)) \
            { \
                PCRTASN1CORE pCore = RT_CONCAT(a_Api,_GetAsn1Core)(pThis->a_PtrName); \
                if (pCore->uTag == a_uTag && pCore->fClass == RTASN1TMPL_ITAG_F_EXPAND(a_fClue)) \
                { \
                    rc = RT_CONCAT(a_Api,_CheckSanity)(pThis->a_PtrName, fFlags & RTASN1_CHECK_SANITY_F_COMMON_MASK, \
                                                       pErrInfo, RT_XSTR(RTASN1TMPL_TYPE) "::" #a_Name); \
                    { a_Constraints } \
                } \
                else \
                    rc = RTErrInfoSetF(pErrInfo, VERR_GENERAL_FAILURE, \
                                       "%s::" #a_Name ": Tag/class mismatch: expected %#x/%#x, actual %#x/%x.", \
                                       pszErrorTag, a_uTag, RTASN1TMPL_ITAG_F_EXPAND(a_fClue), pCore->uTag, pCore->fClass); \
            } \
            else \
                rc = RTErrInfoSetF(pErrInfo, VERR_GENERAL_FAILURE, "%s::" #a_Name ": Not present.", pszErrorTag); \
            break
# define RTASN1TMPL_PCHOICE_XTAG_EX(a_uTag, a_enmChoice, a_PtrTnNm, a_CtxTagN, a_Name, a_Type, a_Api, a_Constraints) \
        case a_enmChoice: \
            if (   pThis->a_PtrTnNm \
                && RTASN1CORE_IS_PRESENT(&(pThis->a_PtrTnNm->a_CtxTagN.Asn1Core)) \
                && RT_CONCAT(a_Api,_IsPresent)(&pThis->a_PtrTnNm->a_Name) ) \
            { \
                rc = RT_CONCAT(a_Api,_CheckSanity)(&pThis->a_PtrTnNm->a_Name, fFlags & RTASN1_CHECK_SANITY_F_COMMON_MASK, \
                                                   pErrInfo, RT_XSTR(RTASN1TMPL_TYPE) "::" #a_Name); \
                { a_Constraints } \
            } \
            else \
                rc = RTErrInfoSetF(pErrInfo, VERR_GENERAL_FAILURE, "%s::" #a_Name ": Not present.", pszErrorTag); \
            break
#define RTASN1TMPL_END_PCHOICE() \
    } \
    RTASN1TMPL_END_COMMON()


# define RTASN1TMPL_SET_SEQ_OF_COMMON(a_ItemType, a_ItemApi) \
    RTASN1TMPL_BEGIN_COMMON(); \
    for (uint32_t i = 0; RT_SUCCESS(rc) && i < pThis->cItems; i++) \
        rc = RT_CONCAT(a_ItemApi,_CheckSanity)(pThis->papItems[i], fFlags & RTASN1_CHECK_SANITY_F_COMMON_MASK, \
                                               pErrInfo, RT_XSTR(RTASN1TMPL_TYPE) "::papItems[#]"); \
    if (RT_SUCCESS(rc)) { RTASN1TMPL_SET_SEQ_EXEC_CHECK_SANITY(); } \
    RTASN1TMPL_END_COMMON()
# define RTASN1TMPL_SEQ_OF(a_ItemType, a_ItemApi) RTASN1TMPL_SET_SEQ_OF_COMMON(a_ItemType, a_ItemApi)
# define RTASN1TMPL_SET_OF(a_ItemType, a_ItemApi) RTASN1TMPL_SET_SEQ_OF_COMMON(a_ItemType, a_ItemApi)

/* The constraints. */
# define RTASN1TMPL_MEMBER_CONSTR_MIN_MAX(a_Name, a_Type, a_Api, cbMin, cbMax, a_MoreConstraints) \
    if (RT_SUCCESS(rc) && ((cbMin) != 0 || (cbMax) != UINT32_MAX)) \
    { \
        PCRTASN1CORE pCore = RT_CONCAT(a_Api,_GetAsn1Core)(&pThis->a_Name); \
        if (RT_LIKELY(pCore->cb >= (cbMin) && pCore->cb <= (cbMax))) \
        { /* likely */ } \
        else \
            rc = RTErrInfoSetF(pErrInfo, VERR_GENERAL_FAILURE, \
                               "%s::" #a_Name ": Content size is out of range: %#x not in {%#x..%#x}", \
                               pszErrorTag, pCore->cb, cbMin, cbMax); \
    } \
    { a_MoreConstraints }

# define RTASN1TMPL_MEMBER_CONSTR_BITSTRING_MIN_MAX(a_Name, cMinBits, cMaxBits, a_MoreConstraints) \
    if (RT_SUCCESS(rc) && ((cMinBits) != 0 || (cMaxBits) != UINT32_MAX)) \
    { \
        if (RT_LIKELY(   ((cMinBits) == 0          ? true : pThis->a_Name.cBits + 1U >= (cMinBits) + 1U /* warning avoiding */) \
                      && ((cMaxBits) == UINT32_MAX ? true : pThis->a_Name.cBits + 1U <= (cMaxBits) + 1U /* ditto */) ) ) \
        { /* likely */ } \
        else \
            rc = RTErrInfoSetF(pErrInfo, VERR_GENERAL_FAILURE, \
                               "%s::" #a_Name ": Bit size is out of range: %#x not in {%#x..%#x}", \
                               pszErrorTag, pThis->a_Name.cBits, cMinBits, cMaxBits); \
    } \
    { a_MoreConstraints }

# define RTASN1TMPL_MEMBER_CONSTR_U64_MIN_MAX(a_Name, uMin, uMax, a_MoreConstraints) \
    if (RT_SUCCESS(rc)) \
    { \
        if (RT_LIKELY(   RTAsn1Integer_UnsignedCompareWithU64(&pThis->a_Name, uMin) >= 0 \
                      && RTAsn1Integer_UnsignedCompareWithU64(&pThis->a_Name, uMax) <= 0) ) \
        { /* likely */ } \
        else \
            rc = RTErrInfoSetF(pErrInfo, VERR_GENERAL_FAILURE, \
                               "%s::" #a_Name ": Out of range: %#x not in {%#llx..%#llx}", \
                               pszErrorTag, pThis->a_Name.Asn1Core.cb > 8 ? UINT64_MAX : pThis->a_Name.uValue.u, \
                               (uint64_t)(uMin), (uint64_t)(uMax)); \
    } \
    { a_MoreConstraints }

# define RTASN1TMPL_MEMBER_CONSTR_PRESENT(a_Name, a_Api, a_MoreConstraints) \
    if (RT_SUCCESS(rc)) \
    { \
        if (RT_LIKELY(RT_CONCAT(a_Api,_IsPresent)(&pThis->a_Name))) \
        { /* likely */ } \
        else \
            rc = RTErrInfoSetF(pErrInfo, VERR_GENERAL_FAILURE, "%s::" #a_Name ": Missing.", pszErrorTag); \
    } \
    { a_MoreConstraints }



# define RTASN1TMPL_EXEC_CHECK_SANITY(a_Expr) if (RT_SUCCESS(rc)) { a_Expr; }


#elif RTASN1TMPL_PASS == RTASN1TMPL_PASS_DELETE
/*
 *
 * Delete wrappers.
 *
 */
# define RTASN1TMPL_BEGIN_COMMON() \
RTASN1TMPL_DECL(void) RT_CONCAT(RTASN1TMPL_EXT_NAME,_Delete)(RT_CONCAT(P,RTASN1TMPL_TYPE) pThis) \
{ \
    if (RT_CONCAT(RTASN1TMPL_EXT_NAME,_IsPresent)(pThis)) \
    {   do { } while (0)

# define RTASN1TMPL_END_COMMON() \
    } \
    RT_ZERO(*pThis); \
} RTASN1TMPL_SEMICOLON_DUMMY()

# define RTASN1TMPL_BEGIN_SEQCORE()                                 RTASN1TMPL_BEGIN_COMMON()
# define RTASN1TMPL_BEGIN_SETCORE()                                 RTASN1TMPL_BEGIN_COMMON()
# define RTASN1TMPL_MEMBER_EX(a_Name, a_Type, a_Api, a_Constraints)     RT_CONCAT(a_Api,_Delete)(&pThis->a_Name)
# define RTASN1TMPL_MEMBER_DYN_BEGIN(a_ObjIdMembNm, a_enmType, a_enmMembNm, a_Allocation) \
    switch (pThis->a_enmMembNm) \
    { \
        default: break
# define RTASN1TMPL_MEMBER_DYN_COMMON(a_UnionNm, a_PtrName, a_Type, a_Api, a_Allocation, a_enmMembNm, a_enmValue, a_IfStmt) \
        case a_enmValue: \
            if (pThis->a_UnionNm.a_PtrName) \
            { \
                RT_CONCAT(a_Api,_Delete)(pThis->a_UnionNm.a_PtrName); \
                RTAsn1MemFree(&pThis->Allocation, pThis->a_UnionNm.a_PtrName); \
                pThis->a_UnionNm.a_PtrName = NULL; \
            } \
            break
# define RTASN1TMPL_MEMBER_DYN_END(a_ObjIdMembNm, a_enmType, a_enmMembNm, a_Allocation) \
    }
# define RTASN1TMPL_END_SEQCORE()                                   RTASN1TMPL_END_COMMON()
# define RTASN1TMPL_END_SETCORE()                                   RTASN1TMPL_END_COMMON()


# define RTASN1TMPL_BEGIN_PCHOICE() \
    RTASN1TMPL_BEGIN_COMMON(); \
    switch (pThis->enmChoice) \
    { \
        default: break
# define RTASN1TMPL_PCHOICE_ITAG_EX(a_uTag, a_enmChoice, a_PtrName, a_Name, a_Type, a_Api, a_fClue, a_Constraints) \
        case a_enmChoice: \
            if (pThis->a_PtrName) \
            { \
                RT_CONCAT(a_Api,_Delete)(pThis->a_PtrName); \
                RTAsn1MemFree(&pThis->Allocation, pThis->a_PtrName); \
                pThis->a_PtrName = NULL; \
            } \
            break
# define RTASN1TMPL_PCHOICE_XTAG_EX(a_uTag, a_enmChoice, a_PtrTnNm, a_CtxTagN, a_Name, a_Type, a_Api, a_Constraints) \
        case a_enmChoice: \
            if (pThis->a_PtrTnNm) \
            { \
                RT_CONCAT(a_Api,_Delete)(&pThis->a_PtrTnNm->a_Name); \
                RTAsn1MemFree(&pThis->Allocation, pThis->a_PtrTnNm); \
                pThis->a_PtrTnNm = NULL; \
            } \
            break
# define RTASN1TMPL_END_PCHOICE() \
    } \
    RTASN1TMPL_END_COMMON()


# define RTASN1TMPL_SET_SEQ_OF_COMMON(a_ItemType, a_ItemApi) \
    RTASN1TMPL_BEGIN_COMMON(); \
    uint32_t i = pThis->cItems; \
    while (i-- > 0) \
        RT_CONCAT(a_ItemApi,_Delete)(pThis->papItems[i]); \
    RTAsn1MemFreeArray(&pThis->Allocation, (void **)pThis->papItems); \
    pThis->papItems = NULL; \
    pThis->cItems   = 0; \
    RTASN1TMPL_END_COMMON()
# define RTASN1TMPL_SEQ_OF(a_ItemType, a_ItemApi) RTASN1TMPL_SET_SEQ_OF_COMMON(a_ItemType, a_ItemApi)
# define RTASN1TMPL_SET_OF(a_ItemType, a_ItemApi) RTASN1TMPL_SET_SEQ_OF_COMMON(a_ItemType, a_ItemApi)


#else
# error "Invalid/missing RTASN1TMPL_PASS value."
#endif



/*
 * Default aliases for simplified versions of macros if no specialization
 * was required above.
 */
/* Non-optional members. */
#ifndef RTASN1TMPL_MEMBER
# define RTASN1TMPL_MEMBER(a_Name, a_Type, a_Api) \
    RTASN1TMPL_MEMBER_EX(a_Name, a_Type, a_Api, RT_NOTHING)
#endif

#ifndef RTASN1TMPL_MEMBER_UTF8_STRING_MIN_MAX
# define RTASN1TMPL_MEMBER_UTF8_STRING_MIN_MAX(a_Name) \
    RTASN1TMPL_MEMBER(a_Name, RTASN1STRING, RTAsn1String)
#endif
#ifndef RTASN1TMPL_MEMBER_UTF8_STRING
# define RTASN1TMPL_MEMBER_UTF8_STRING(a_Name) \
    RTASN1TMPL_MEMBER_UTF8_STRING_MIN_MAX(a_Name, 0, UINT32_MAX)
#endif

#ifndef RTASN1TMPL_MEMBER_STRING_MIN_MAX
# define RTASN1TMPL_MEMBER_STRING_MIN_MAX(a_Name, a_cbMin, a_cbMax) \
    RTASN1TMPL_MEMBER(a_Name, RTASN1STRING, RTAsn1String)
#endif
#ifndef RTASN1TMPL_MEMBER_STRING
# define RTASN1TMPL_MEMBER_STRING(a_Name) \
    RTASN1TMPL_MEMBER_STRING_MIN_MAX(a_Name, 0, UINT32_MAX)
#endif
#ifndef RTASN1TMPL_MEMBER_XTAG_EX
# define RTASN1TMPL_MEMBER_XTAG_EX(a_TnNm, a_CtxTagN, a_Name, a_Type, a_Api, a_uTag, a_Constraints) \
    RTASN1TMPL_MEMBER_EX(a_TnNm.a_Name, a_Type, a_Api, a_Constraints RT_NOTHING)
#endif

/* Any/dynamic members.  */
#ifndef RTASN1TMPL_MEMBER_DYN_BEGIN
# define RTASN1TMPL_MEMBER_DYN_BEGIN(a_ObjIdMembNm, a_enmType, a_enmMembNm, a_Allocation)   do { } while (0)
#endif
#ifndef RTASN1TMPL_MEMBER_DYN_END
# define RTASN1TMPL_MEMBER_DYN_END(a_ObjIdMembNm, a_enmType, a_enmMembNm, a_Allocation)     do { } while (0)
#endif
#ifndef RTASN1TMPL_MEMBER_DYN_COMMON
# define RTASN1TMPL_MEMBER_DYN_COMMON(a_UnionNm, a_PtrName, a_Type, a_Api, a_Allocation, a_enmMembNm, a_enmValue, a_IfStmt) \
    RTASN1TMPL_MEMBER(a_UnionNm.a_PtrName, a_Type, a_Api)
#endif
#ifndef RTASN1TMPL_MEMBER_DYN
# define RTASN1TMPL_MEMBER_DYN(a_UnionNm, a_PtrName, a_Name, a_Type, a_Api, a_Allocation, a_ObjIdMembNm, a_enmMembNm, a_enmValue, a_szObjId) \
    RTASN1TMPL_MEMBER_DYN_COMMON(a_UnionNm, a_PtrName, a_Type, a_Api, a_Allocation, a_enmMembNm, a_enmValue, if (RTAsn1ObjId_CompareWithString(&pThis->a_ObjIdMembNm, a_szObjId) == 0))
#endif
#ifndef RTASN1TMPL_MEMBER_DYN_DEFAULT
# define RTASN1TMPL_MEMBER_DYN_DEFAULT(a_UnionNm, a_PtrName, a_Type, a_Api, a_Allocation, a_ObjIdMembNm, a_enmMembNm, a_enmValue) \
    RTASN1TMPL_MEMBER_DYN_COMMON(a_UnionNm, a_PtrName, a_Type, a_Api, a_Allocation, a_enmMembNm, a_enmValue, RT_NOTHING)
#endif

/* Optional members. */
#ifndef RTASN1TMPL_MEMBER_OPT_EX
# define RTASN1TMPL_MEMBER_OPT_EX(a_Name, a_Type, a_Api, a_Constraints) \
    RTASN1TMPL_MEMBER_EX(a_Name, a_Type, a_Api, a_Constraints RT_NOTHING)
#endif
#ifndef RTASN1TMPL_MEMBER_OPT
# define RTASN1TMPL_MEMBER_OPT(a_Name, a_Type, a_Api) \
    RTASN1TMPL_MEMBER_OPT_EX(a_Name, a_Type, a_Api, RT_NOTHING)
#endif

#ifndef RTASN1TMPL_MEMBER_OPT_XTAG_EX
# define RTASN1TMPL_MEMBER_OPT_XTAG_EX(a_TnNm, a_CtxTagN, a_Name, a_Type, a_Api, a_uTag, a_Constraints) \
    RTASN1TMPL_MEMBER_OPT_EX(a_TnNm.a_Name, a_Type, a_Api, a_Constraints RT_NOTHING)
#endif
#ifndef RTASN1TMPL_MEMBER_OPT_XTAG
# define RTASN1TMPL_MEMBER_OPT_XTAG(a_TnNm, a_CtxTagN, a_Name, a_Type, a_Api, a_uTag) \
    RTASN1TMPL_MEMBER_OPT_XTAG_EX(a_TnNm, a_CtxTagN, a_Name, a_Type, a_Api, a_uTag, RT_NOTHING)
#endif

#ifndef RTASN1TMPL_MEMBER_OPT_ITAG_EX
# define RTASN1TMPL_MEMBER_OPT_ITAG_EX(a_Name, a_Type, a_Api, a_uTag, a_fClue, a_Constraints) \
    RTASN1TMPL_MEMBER_OPT_EX(a_Name, a_Type, a_Api, a_Constraints RT_NOTHING)
#endif
#ifndef RTASN1TMPL_MEMBER_OPT_ITAG_UP
# define RTASN1TMPL_MEMBER_OPT_ITAG_UP(a_Name, a_Type, a_Api, a_uTag) \
    RTASN1TMPL_MEMBER_OPT_ITAG_EX(a_Name, a_Type, a_Api, a_uTag, RTASN1TMPL_ITAG_F_UP, RT_NOTHING)
#endif
#ifndef RTASN1TMPL_MEMBER_OPT_ITAG_UC
# define RTASN1TMPL_MEMBER_OPT_ITAG_UC(a_Name, a_Type, a_Api, a_uTag) \
    RTASN1TMPL_MEMBER_OPT_ITAG_EX(a_Name, a_Type, a_Api, a_uTag, RTASN1TMPL_ITAG_F_UC, RT_NOTHING)
#endif
#ifndef RTASN1TMPL_MEMBER_OPT_ITAG_CP
# define RTASN1TMPL_MEMBER_OPT_ITAG_CP(a_Name, a_Type, a_Api, a_uTag) \
    RTASN1TMPL_MEMBER_OPT_ITAG_EX(a_Name, a_Type, a_Api, a_uTag, RTASN1TMPL_ITAG_F_CP, RT_NOTHING)
#endif
#ifndef RTASN1TMPL_MEMBER_OPT_ITAG
# define RTASN1TMPL_MEMBER_OPT_ITAG(a_Name, a_Type, a_Api, a_uTag) \
    RTASN1TMPL_MEMBER_OPT_ITAG_EX(a_Name, a_Type, a_Api, a_uTag, RTASN1TMPL_ITAG_F_CC, RT_NOTHING)
#endif
#ifndef RTASN1TMPL_MEMBER_OPT_ANY
# define RTASN1TMPL_MEMBER_OPT_ANY(a_Name, a_Type, a_Api) \
    RTASN1TMPL_MEMBER_OPT_EX(a_Name, a_Type, a_Api, RT_NOTHING)
#endif

#ifndef RTASN1TMPL_MEMBER_DEF_ITAG_EX
# define RTASN1TMPL_MEMBER_DEF_ITAG_EX(a_Name, a_Type, a_Api, a_uTag, a_fClue, a_DefVal, a_Constraints) \
    RTASN1TMPL_MEMBER_OPT_ITAG_EX(a_Name, a_Type, a_Api, a_uTag, a_fClue, a_Constraints RT_NOTHING)
#endif
#ifndef RTASN1TMPL_MEMBER_DEF_ITAG_UP
# define RTASN1TMPL_MEMBER_DEF_ITAG_UP(a_Name, a_Type, a_Api, a_uTag, a_DefVal) \
    RTASN1TMPL_MEMBER_DEF_ITAG_EX(a_Name, a_Type, a_Api, a_uTag, RTASN1TMPL_ITAG_F_UP, a_DefVal, RT_NOTHING)
#endif

#ifndef RTASN1TMPL_MEMBER_OPT_ITAG_BITSTRING
# define RTASN1TMPL_MEMBER_OPT_ITAG_BITSTRING(a_Name, a_cMaxBits, a_uTag) \
    RTASN1TMPL_MEMBER_OPT_ITAG_EX(a_Name, RTASN1BITSTRING, RTAsn1BitString, a_uTag, RTASN1TMPL_ITAG_F_CP, \
                                  RTASN1TMPL_MEMBER_CONSTR_BITSTRING_MIN_MAX(a_Name, 0, a_cMaxBits, RT_NOTHING))
#endif

#ifndef RTASN1TMPL_MEMBER_OPT_UTF8_STRING_EX
# define RTASN1TMPL_MEMBER_OPT_UTF8_STRING_EX(a_Name, a_Constraints) \
    RTASN1TMPL_MEMBER_OPT_EX(a_Name, RTASN1STRING, RTAsn1String, a_Constraints RT_NOTHING)
#endif
#ifndef RTASN1TMPL_MEMBER_OPT_UTF8_STRING
# define RTASN1TMPL_MEMBER_OPT_UTF8_STRING(a_Name) \
    RTASN1TMPL_MEMBER_OPT_UTF8_STRING_EX(a_Name, RT_NOTHING)
#endif

#ifndef RTASN1TMPL_MEMBER_OPT_STRING_EX
# define RTASN1TMPL_MEMBER_OPT_STRING_EX(a_Name, a_Constraints) \
    RTASN1TMPL_MEMBER_OPT_EX(a_Name, RTASN1STRING, RTAsn1String, a_Constraints RT_NOTHING)
#endif
#ifndef RTASN1TMPL_MEMBER_OPT_STRING
# define RTASN1TMPL_MEMBER_OPT_STRING(a_Name) \
    RTASN1TMPL_MEMBER_OPT_STRING_EX(a_Name, RT_NOTHING)
#endif

/* Pointer choices. */
#ifndef RTASN1TMPL_PCHOICE_ITAG_UP
# define RTASN1TMPL_PCHOICE_ITAG_UP(a_uTag, a_enmChoice, a_PtrName, a_Name, a_Type, a_Api) \
    RTASN1TMPL_PCHOICE_ITAG_EX(a_uTag, a_enmChoice, a_PtrName, a_Name, a_Type, a_Api, RTASN1TMPL_ITAG_F_UP, RT_NOTHING)
#endif
#ifndef RTASN1TMPL_PCHOICE_ITAG_UC
# define RTASN1TMPL_PCHOICE_ITAG_UC(a_uTag, a_enmChoice, a_PtrName, a_Name, a_Type, a_Api) \
    RTASN1TMPL_PCHOICE_ITAG_EX(a_uTag, a_enmChoice, a_PtrName, a_Name, a_Type, a_Api, RTASN1TMPL_ITAG_F_UC, RT_NOTHING)
#endif
#ifndef RTASN1TMPL_PCHOICE_ITAG_CP
# define RTASN1TMPL_PCHOICE_ITAG_CP(a_uTag, a_enmChoice, a_PtrName, a_Name, a_Type, a_Api) \
    RTASN1TMPL_PCHOICE_ITAG_EX(a_uTag, a_enmChoice, a_PtrName, a_Name, a_Type, a_Api, RTASN1TMPL_ITAG_F_CP, RT_NOTHING)
#endif
#ifndef RTASN1TMPL_PCHOICE_ITAG
# define RTASN1TMPL_PCHOICE_ITAG(a_uTag, a_enmChoice, a_PtrName, a_Name, a_Type, a_Api) \
    RTASN1TMPL_PCHOICE_ITAG_EX(a_uTag, a_enmChoice, a_PtrName, a_Name, a_Type, a_Api, RTASN1TMPL_ITAG_F_CC, RT_NOTHING)
#endif

#ifndef RTASN1TMPL_PCHOICE_XTAG
# define RTASN1TMPL_PCHOICE_XTAG(a_uTag, a_enmChoice, a_PtrTnNm, a_CtxTagN, a_Name, a_Type, a_Api) \
    RTASN1TMPL_PCHOICE_XTAG_EX(a_uTag, a_enmChoice, a_PtrTnNm, a_CtxTagN, a_Name, a_Type, a_Api, RT_NOTHING)
#endif


/*
 * Constraints are only used in the sanity check pass, so provide subs for the
 * others passes.
 */
#ifndef RTASN1TMPL_MEMBER_CONSTR_MIN_MAX
# define RTASN1TMPL_MEMBER_CONSTR_MIN_MAX(a_Name, a_Type, a_Api, cbMin, cbMax, a_MoreConstraints)
#endif
#ifndef RTASN1TMPL_MEMBER_CONSTR_BITSTRING_MIN_MAX
# define RTASN1TMPL_MEMBER_CONSTR_BITSTRING_MIN_MAX(a_Name, cMinBits, cMaxBits, a_MoreConstraints)
#endif
#ifndef RTASN1TMPL_MEMBER_CONSTR_U64_MIN_MAX
# define RTASN1TMPL_MEMBER_CONSTR_U64_MIN_MAX(a_Name, uMin, uMax, a_MoreConstraints)
#endif
#ifndef RTASN1TMPL_MEMBER_CONSTR_PRESENT
# define RTASN1TMPL_MEMBER_CONSTR_PRESENT(a_Name, a_Api, a_MoreConstraints)
#endif


/*
 * Stub exec hacks.
 */
#ifndef RTASN1TMPL_EXEC_DECODE
# define RTASN1TMPL_EXEC_DECODE(a_Expr) /* no semi colon allowed after this */
#endif
#ifndef RTASN1TMPL_EXEC_CLONE
# define RTASN1TMPL_EXEC_CLONE(a_Expr)  /* no semi colon allowed after this */
#endif
#ifndef RTASN1TMPL_EXEC_CHECK_SANITY
# define RTASN1TMPL_EXEC_CHECK_SANITY(a_Expr)  /* no semi colon allowed after this */
#endif

#define RTASN1TMPL_SET_SEQ_EXEC_CHECK_SANITY() do { } while (0)


/*
 * Generate the requested code.
 */
#ifndef RTASN1TMPL_TEMPLATE_FILE
# error "No template file (RTASN1TMPL_TEMPLATE_FILE) is specified."
#endif
#include RTASN1TMPL_TEMPLATE_FILE



/*
 * Undo all the macros.
 */
#undef RTASN1TMPL_DECL
#undef RTASN1TMPL_TYPE
#undef RTASN1TMPL_EXT_NAME
#undef RTASN1TMPL_INT_NAME

#undef RTASN1TMPL_PASS

#undef RTASN1TMPL_BEGIN_COMMON
#undef RTASN1TMPL_END_COMMON
#undef RTASN1TMPL_BEGIN_SEQCORE
#undef RTASN1TMPL_BEGIN_SETCORE
#undef RTASN1TMPL_MEMBER
#undef RTASN1TMPL_MEMBER_EX
#undef RTASN1TMPL_MEMBER_DYN_BEGIN
#undef RTASN1TMPL_MEMBER_DYN
#undef RTASN1TMPL_MEMBER_DYN_DEFAULT
#undef RTASN1TMPL_MEMBER_DYN_COMMON
#undef RTASN1TMPL_MEMBER_DYN_END
#undef RTASN1TMPL_MEMBER_OPT
#undef RTASN1TMPL_MEMBER_OPT_EX
#undef RTASN1TMPL_MEMBER_OPT_ITAG
#undef RTASN1TMPL_MEMBER_OPT_ITAG_EX
#undef RTASN1TMPL_MEMBER_OPT_ITAG_CP
#undef RTASN1TMPL_MEMBER_OPT_ITAG_UC
#undef RTASN1TMPL_MEMBER_OPT_ITAG_UP
#undef RTASN1TMPL_MEMBER_OPT_ITAG_BITSTRING
#undef RTASN1TMPL_MEMBER_OPT_UTF8_STRING
#undef RTASN1TMPL_MEMBER_OPT_UTF8_STRING_EX
#undef RTASN1TMPL_MEMBER_OPT_XTAG
#undef RTASN1TMPL_MEMBER_OPT_XTAG_EX
#undef RTASN1TMPL_MEMBER_OPT_ANY
#undef RTASN1TMPL_MEMBER_DEF_ITAG_UP
#undef RTASN1TMPL_MEMBER_DEF_ITAG_EX
#undef RTASN1TMPL_END_SEQCORE
#undef RTASN1TMPL_END_SETCORE

#undef RTASN1TMPL_BEGIN_PCHOICE
#undef RTASN1TMPL_PCHOICE_ITAG
#undef RTASN1TMPL_PCHOICE_ITAG_UP
#undef RTASN1TMPL_PCHOICE_ITAG_CP
#undef RTASN1TMPL_PCHOICE_ITAG_EX
#undef RTASN1TMPL_PCHOICE_XTAG
#undef RTASN1TMPL_PCHOICE_XTAG_EX
#undef RTASN1TMPL_END_PCHOICE

#undef RTASN1TMPL_SET_SEQ_OF_COMMON
#undef RTASN1TMPL_SEQ_OF
#undef RTASN1TMPL_SET_OF

#undef RTASN1TMPL_VTABLE_FN_ENCODE_PREP
#undef RTASN1TMPL_VTABLE_FN_ENCODE_WRITE

#undef RTASN1TMPL_MEMBER_CONSTR_MIN_MAX
#undef RTASN1TMPL_MEMBER_CONSTR_BITSTRING_MIN_MAX
#undef RTASN1TMPL_MEMBER_CONSTR_U64_MIN_MAX
#undef RTASN1TMPL_MEMBER_CONSTR_PRESENT

#undef RTASN1TMPL_SANITY_CHECK_EXPR

#undef RTASN1TMPL_EXEC_DECODE
#undef RTASN1TMPL_EXEC_CLONE
#undef RTASN1TMPL_EXEC_CHECK_SANITY

#undef RTASN1TMPL_SET_SEQ_EXEC_CHECK_SANITY


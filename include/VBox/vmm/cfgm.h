/** @file
 * CFGM - Configuration Manager.
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

#ifndef VBOX_INCLUDED_vmm_cfgm_h
#define VBOX_INCLUDED_vmm_cfgm_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <VBox/types.h>
#include <iprt/stdarg.h>

/** @defgroup   grp_cfgm        The Configuration Manager API
 * @ingroup grp_vmm
 * @{
 */

/**
 * Configuration manager value type.
 */
typedef enum CFGMVALUETYPE
{
    /** Integer value. */
    CFGMVALUETYPE_INTEGER = 1,
    /** String value. */
    CFGMVALUETYPE_STRING,
    /** Bytestring value. */
    CFGMVALUETYPE_BYTES,
    /** Password value, same as String but hides the content in dump. */
    CFGMVALUETYPE_PASSWORD
} CFGMVALUETYPE;
/** Pointer to configuration manager property type. */
typedef CFGMVALUETYPE *PCFGMVALUETYPE;



RT_C_DECLS_BEGIN

#ifdef IN_RING3
/** @defgroup   grp_cfgm_r3     The CFGM Host Context Ring-3 API
 * @{
 */

typedef enum CFGMCONFIGTYPE
{
    /** pvConfig points to nothing, use defaults. */
    CFGMCONFIGTYPE_NONE = 0,
    /** pvConfig points to a IMachine interface. */
    CFGMCONFIGTYPE_IMACHINE
} CFGMCONFIGTYPE;


/**
 * CFGM init callback for constructing the configuration tree.
 *
 * This is called from the emulation thread, and the one interfacing the VM
 * can make any necessary per-thread initializations at this point.
 *
 * @returns VBox status code.
 * @param   pUVM        The user mode VM handle.
 * @param   pVM         The cross context VM structure.
 * @param   pVMM        The VMM R3 vtable.
 * @param   pvUser      The argument supplied to VMR3Create().
 */
typedef DECLCALLBACKTYPE(int, FNCFGMCONSTRUCTOR,(PUVM pUVM, PVM pVM, PCVMMR3VTABLE pVMM, void *pvUser));
/** Pointer to a FNCFGMCONSTRUCTOR(). */
typedef FNCFGMCONSTRUCTOR *PFNCFGMCONSTRUCTOR;

VMMR3DECL(int)          CFGMR3Init(PVM pVM, PFNCFGMCONSTRUCTOR pfnCFGMConstructor, void *pvUser);
VMMR3DECL(int)          CFGMR3Term(PVM pVM);
VMMR3DECL(int)          CFGMR3ConstructDefaultTree(PVM pVM);

VMMR3DECL(PCFGMNODE)    CFGMR3CreateTree(PUVM pUVM);
VMMR3DECL(int)          CFGMR3DestroyTree(PCFGMNODE pRoot);
VMMR3DECL(void)         CFGMR3Dump(PCFGMNODE pRoot);
VMMR3DECL(int)          CFGMR3DuplicateSubTree(PCFGMNODE pRoot, PCFGMNODE *ppCopy);
VMMR3DECL(int)          CFGMR3ReplaceSubTree(PCFGMNODE pRoot, PCFGMNODE pNewRoot);
VMMR3DECL(int)          CFGMR3InsertSubTree(PCFGMNODE pNode, const char *pszName, PCFGMNODE pSubTree, PCFGMNODE *ppChild);
VMMR3DECL(int)          CFGMR3InsertNode(PCFGMNODE pNode, const char *pszName, PCFGMNODE *ppChild);
VMMR3DECL(int)          CFGMR3InsertNodeF(PCFGMNODE pNode, PCFGMNODE *ppChild,
                                          const char *pszNameFormat, ...) RT_IPRT_FORMAT_ATTR(3, 4);
VMMR3DECL(int)          CFGMR3InsertNodeFV(PCFGMNODE pNode, PCFGMNODE *ppChild,
                                           const char *pszNameFormat, va_list Args) RT_IPRT_FORMAT_ATTR(3, 0);
VMMR3DECL(void)         CFGMR3SetRestrictedRoot(PCFGMNODE pNode);
VMMR3DECL(void)         CFGMR3RemoveNode(PCFGMNODE pNode);
VMMR3DECL(int)          CFGMR3InsertInteger(PCFGMNODE pNode, const char *pszName, uint64_t u64Integer);
VMMR3DECL(int)          CFGMR3InsertString(PCFGMNODE pNode, const char *pszName, const char *pszString);
VMMR3DECL(int)          CFGMR3InsertStringN(PCFGMNODE pNode, const char *pszName, const char *pszString, size_t cchString);
VMMR3DECL(int)          CFGMR3InsertStringF(PCFGMNODE pNode, const char *pszName,
                                            const char *pszFormat, ...) RT_IPRT_FORMAT_ATTR(3, 4);
VMMR3DECL(int)          CFGMR3InsertStringFV(PCFGMNODE pNode, const char *pszName,
                                             const char *pszFormat, va_list va) RT_IPRT_FORMAT_ATTR(3, 0);
VMMR3DECL(int)          CFGMR3InsertStringW(PCFGMNODE pNode, const char *pszName, PCRTUTF16 pwszValue);
VMMR3DECL(int)          CFGMR3InsertBytes(PCFGMNODE pNode, const char *pszName, const void *pvBytes, size_t cbBytes);
VMMR3DECL(int)          CFGMR3InsertPassword(PCFGMNODE pNode, const char *pszName, const char *pszString);
VMMR3DECL(int)          CFGMR3InsertPasswordN(PCFGMNODE pNode, const char *pszName, const char *pszString, size_t cchString);
VMMR3DECL(int)          CFGMR3InsertValue(PCFGMNODE pNode, PCFGMLEAF pValue);
VMMR3DECL(int)          CFGMR3RemoveValue(PCFGMNODE pNode, const char *pszName);

/** @name CFGMR3CopyTree flags.
 * @{ */
/** Reserved value disposition \#0.  */
#define CFGM_COPY_FLAGS_RESERVED_VALUE_DISP_0   UINT32_C(0x00000000)
/** Reserved value disposition \#1.  */
#define CFGM_COPY_FLAGS_RESERVED_VALUE_DISP_1   UINT32_C(0x00000001)
/** Replace exiting values.  */
#define CFGM_COPY_FLAGS_REPLACE_VALUES          UINT32_C(0x00000002)
/** Ignore exiting values.  */
#define CFGM_COPY_FLAGS_IGNORE_EXISTING_VALUES  UINT32_C(0x00000003)
/** Value disposition mask.  */
#define CFGM_COPY_FLAGS_VALUE_DISP_MASK         UINT32_C(0x00000003)

/** Replace exiting keys.  */
#define CFGM_COPY_FLAGS_RESERVED_KEY_DISP       UINT32_C(0x00000000)
/** Replace exiting keys.  */
#define CFGM_COPY_FLAGS_MERGE_KEYS              UINT32_C(0x00000010)
/** Replace exiting keys.  */
#define CFGM_COPY_FLAGS_REPLACE_KEYS            UINT32_C(0x00000020)
/** Ignore existing keys.  */
#define CFGM_COPY_FLAGS_IGNORE_EXISTING_KEYS    UINT32_C(0x00000030)
/** Key disposition.  */
#define CFGM_COPY_FLAGS_KEY_DISP_MASK           UINT32_C(0x00000030)
/** @} */
VMMR3DECL(int)          CFGMR3CopyTree(PCFGMNODE pDstTree, PCFGMNODE pSrcTree, uint32_t fFlags);

VMMR3DECL(bool)         CFGMR3Exists(           PCFGMNODE pNode, const char *pszName);
VMMR3DECL(int)          CFGMR3QueryType(        PCFGMNODE pNode, const char *pszName, PCFGMVALUETYPE penmType);
VMMR3DECL(int)          CFGMR3QuerySize(        PCFGMNODE pNode, const char *pszName, size_t *pcb);
VMMR3DECL(int)          CFGMR3QueryInteger(     PCFGMNODE pNode, const char *pszName, uint64_t *pu64);
VMMR3DECL(int)          CFGMR3QueryIntegerDef(  PCFGMNODE pNode, const char *pszName, uint64_t *pu64, uint64_t u64Def);
VMMR3DECL(int)          CFGMR3QueryString(      PCFGMNODE pNode, const char *pszName, char *pszString, size_t cchString);
VMMR3DECL(int)          CFGMR3QueryStringDef(   PCFGMNODE pNode, const char *pszName, char *pszString, size_t cchString, const char *pszDef);
VMMR3DECL(int)          CFGMR3QueryPassword(    PCFGMNODE pNode, const char *pszName, char *pszString, size_t cchString);
VMMR3DECL(int)          CFGMR3QueryPasswordDef( PCFGMNODE pNode, const char *pszName, char *pszString, size_t cchString, const char *pszDef);
VMMR3DECL(int)          CFGMR3QueryBytes(       PCFGMNODE pNode, const char *pszName, void *pvData, size_t cbData);


/** @name Helpers
 * @{
 */
VMMR3DECL(int)          CFGMR3QueryU64(         PCFGMNODE pNode, const char *pszName, uint64_t *pu64);
VMMR3DECL(int)          CFGMR3QueryU64Def(      PCFGMNODE pNode, const char *pszName, uint64_t *pu64, uint64_t u64Def);
VMMR3DECL(int)          CFGMR3QueryS64(         PCFGMNODE pNode, const char *pszName, int64_t *pi64);
VMMR3DECL(int)          CFGMR3QueryS64Def(      PCFGMNODE pNode, const char *pszName, int64_t *pi64, int64_t i64Def);
VMMR3DECL(int)          CFGMR3QueryU32(         PCFGMNODE pNode, const char *pszName, uint32_t *pu32);
VMMR3DECL(int)          CFGMR3QueryU32Def(      PCFGMNODE pNode, const char *pszName, uint32_t *pu32, uint32_t u32Def);
VMMR3DECL(int)          CFGMR3QueryS32(         PCFGMNODE pNode, const char *pszName, int32_t *pi32);
VMMR3DECL(int)          CFGMR3QueryS32Def(      PCFGMNODE pNode, const char *pszName, int32_t *pi32, int32_t i32Def);
VMMR3DECL(int)          CFGMR3QueryU16(         PCFGMNODE pNode, const char *pszName, uint16_t *pu16);
VMMR3DECL(int)          CFGMR3QueryU16Def(      PCFGMNODE pNode, const char *pszName, uint16_t *pu16, uint16_t u16Def);
VMMR3DECL(int)          CFGMR3QueryS16(         PCFGMNODE pNode, const char *pszName, int16_t *pi16);
VMMR3DECL(int)          CFGMR3QueryS16Def(      PCFGMNODE pNode, const char *pszName, int16_t *pi16, int16_t i16Def);
VMMR3DECL(int)          CFGMR3QueryU8(          PCFGMNODE pNode, const char *pszName, uint8_t *pu8);
VMMR3DECL(int)          CFGMR3QueryU8Def(       PCFGMNODE pNode, const char *pszName, uint8_t *pu8, uint8_t u8Def);
VMMR3DECL(int)          CFGMR3QueryS8(          PCFGMNODE pNode, const char *pszName, int8_t *pi8);
VMMR3DECL(int)          CFGMR3QueryS8Def(       PCFGMNODE pNode, const char *pszName, int8_t *pi8, int8_t i8Def);
VMMR3DECL(int)          CFGMR3QueryBool(        PCFGMNODE pNode, const char *pszName, bool *pf);
VMMR3DECL(int)          CFGMR3QueryBoolDef(     PCFGMNODE pNode, const char *pszName, bool *pf, bool fDef);
VMMR3DECL(int)          CFGMR3QueryPort(        PCFGMNODE pNode, const char *pszName, PRTIOPORT pPort);
VMMR3DECL(int)          CFGMR3QueryPortDef(     PCFGMNODE pNode, const char *pszName, PRTIOPORT pPort, RTIOPORT PortDef);
VMMR3DECL(int)          CFGMR3QueryUInt(        PCFGMNODE pNode, const char *pszName, unsigned int *pu);
VMMR3DECL(int)          CFGMR3QueryUIntDef(     PCFGMNODE pNode, const char *pszName, unsigned int *pu, unsigned int uDef);
VMMR3DECL(int)          CFGMR3QuerySInt(        PCFGMNODE pNode, const char *pszName, signed int *pi);
VMMR3DECL(int)          CFGMR3QuerySIntDef(     PCFGMNODE pNode, const char *pszName, signed int *pi, signed int iDef);
VMMR3DECL(int)          CFGMR3QueryGCPtr(       PCFGMNODE pNode, const char *pszName, PRTGCPTR pGCPtr);
VMMR3DECL(int)          CFGMR3QueryGCPtrDef(    PCFGMNODE pNode, const char *pszName, PRTGCPTR pGCPtr, RTGCPTR GCPtrDef);
VMMR3DECL(int)          CFGMR3QueryGCPtrU(      PCFGMNODE pNode, const char *pszName, PRTGCUINTPTR pGCPtr);
VMMR3DECL(int)          CFGMR3QueryGCPtrUDef(   PCFGMNODE pNode, const char *pszName, PRTGCUINTPTR pGCPtr, RTGCUINTPTR GCPtrDef);
VMMR3DECL(int)          CFGMR3QueryGCPtrS(      PCFGMNODE pNode, const char *pszName, PRTGCINTPTR pGCPtr);
VMMR3DECL(int)          CFGMR3QueryGCPtrSDef(   PCFGMNODE pNode, const char *pszName, PRTGCINTPTR pGCPtr, RTGCINTPTR GCPtrDef);
VMMR3DECL(int)          CFGMR3QueryStringAlloc( PCFGMNODE pNode, const char *pszName, char **ppszString);
VMMR3DECL(int)          CFGMR3QueryStringAllocDef(PCFGMNODE pNode, const char *pszName, char **ppszString, const char *pszDef);

/** @} */

/** @name Tree Navigation and Enumeration.
 * @{
 */
VMMR3DECL(PCFGMNODE)    CFGMR3GetRoot(PVM pVM);
VMMR3DECL(PCFGMNODE)    CFGMR3GetRootU(PUVM pUVM);
VMMR3DECL(PCFGMNODE)    CFGMR3GetParent(PCFGMNODE pNode);
VMMR3DECL(PCFGMNODE)    CFGMR3GetParentEx(PVM pVM, PCFGMNODE pNode);
VMMR3DECL(PCFGMNODE)    CFGMR3GetChild(PCFGMNODE pNode, const char *pszPath);
VMMR3DECL(PCFGMNODE)    CFGMR3GetChildF(PCFGMNODE pNode, const char *pszPathFormat, ...) RT_IPRT_FORMAT_ATTR(2, 3);
VMMR3DECL(PCFGMNODE)    CFGMR3GetChildFV(PCFGMNODE pNode, const char *pszPathFormat, va_list Args) RT_IPRT_FORMAT_ATTR(3, 0);
VMMR3DECL(PCFGMNODE)    CFGMR3GetFirstChild(PCFGMNODE pNode);
VMMR3DECL(PCFGMNODE)    CFGMR3GetNextChild(PCFGMNODE pCur);
VMMR3DECL(int)          CFGMR3GetName(PCFGMNODE pCur, char *pszName, size_t cchName);
VMMR3DECL(size_t)       CFGMR3GetNameLen(PCFGMNODE pCur);
VMMR3DECL(bool)         CFGMR3AreChildrenValid(PCFGMNODE pNode, const char *pszzValid);
VMMR3DECL(PCFGMLEAF)    CFGMR3GetFirstValue(PCFGMNODE pCur);
VMMR3DECL(PCFGMLEAF)    CFGMR3GetNextValue(PCFGMLEAF pCur);
VMMR3DECL(int)          CFGMR3GetValueName(PCFGMLEAF pCur, char *pszName, size_t cchName);
VMMR3DECL(size_t)       CFGMR3GetValueNameLen(PCFGMLEAF pCur);
VMMR3DECL(CFGMVALUETYPE) CFGMR3GetValueType(PCFGMLEAF pCur);
VMMR3DECL(bool)         CFGMR3AreValuesValid(PCFGMNODE pNode, const char *pszzValid);
VMMR3DECL(int)          CFGMR3ValidateConfig(PCFGMNODE pNode, const char *pszNode,
                                             const char *pszValidValues, const char *pszValidNodes,
                                             const char *pszWho, uint32_t uInstance);

/** @} */


/** @} */
#endif  /* IN_RING3 */


RT_C_DECLS_END

/** @} */

#endif /* !VBOX_INCLUDED_vmm_cfgm_h */


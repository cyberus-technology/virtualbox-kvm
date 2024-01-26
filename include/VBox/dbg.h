/** @file
 * Debugger Interfaces. (VBoxDbg)
 *
 * This header covers all external interfaces of the Debugger module.
 * However, it does not cover the DBGF interface since that part of the
 * VMM. Use dbgf.h for that.
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

#ifndef VBOX_INCLUDED_dbg_h
#define VBOX_INCLUDED_dbg_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <VBox/cdefs.h>
#include <VBox/types.h>
#include <VBox/vmm/dbgf.h>

#include <iprt/stdarg.h>
#ifdef IN_RING3
# include <iprt/errcore.h>
#endif

RT_C_DECLS_BEGIN



/** @defgroup grp_dbg       The VirtualBox Debugger
 * @{
 */

#ifdef IN_RING3 /* The debugger stuff is ring-3 only. */

/** @defgroup grp_dbgc     The Debugger Console API
 * @{
 */

/** @def VBOX_WITH_DEBUGGER
 * The build is with debugger module. Test if this is defined before registering
 * external debugger commands. This is normally defined in Config.kmk.
 */
#ifdef DOXYGEN_RUNNING
# define VBOX_WITH_DEBUGGER
#endif


/**
 * DBGC variable category.
 *
 * Used to describe an argument to a command or function and a functions
 * return value.
 */
typedef enum DBGCVARCAT
{
    /** Any type is fine. */
    DBGCVAR_CAT_ANY = 0,
    /** Any kind of pointer or number. */
    DBGCVAR_CAT_POINTER_NUMBER,
    /** Any kind of pointer or number, no range. */
    DBGCVAR_CAT_POINTER_NUMBER_NO_RANGE,
    /** Any kind of pointer. */
    DBGCVAR_CAT_POINTER,
    /** Any kind of pointer with no range option. */
    DBGCVAR_CAT_POINTER_NO_RANGE,
    /** GC pointer. */
    DBGCVAR_CAT_GC_POINTER,
    /** GC pointer with no range option. */
    DBGCVAR_CAT_GC_POINTER_NO_RANGE,
    /** Numeric argument. */
    DBGCVAR_CAT_NUMBER,
    /** Numeric argument with no range option. */
    DBGCVAR_CAT_NUMBER_NO_RANGE,
    /** String. */
    DBGCVAR_CAT_STRING,
    /** Symbol. */
    DBGCVAR_CAT_SYMBOL,
    /** Option. */
    DBGCVAR_CAT_OPTION,
    /** Option + string. */
    DBGCVAR_CAT_OPTION_STRING,
    /** Option + number. */
    DBGCVAR_CAT_OPTION_NUMBER
} DBGCVARCAT;


/**
 * DBGC variable type.
 */
typedef enum DBGCVARTYPE
{
    /** unknown... */
    DBGCVAR_TYPE_UNKNOWN = 0,
    /** Flat GC pointer. */
    DBGCVAR_TYPE_GC_FLAT,
    /** Segmented GC pointer. */
    DBGCVAR_TYPE_GC_FAR,
    /** Physical GC pointer. */
    DBGCVAR_TYPE_GC_PHYS,
    /** Flat HC pointer. */
    DBGCVAR_TYPE_HC_FLAT,
    /** Physical HC pointer. */
    DBGCVAR_TYPE_HC_PHYS,
    /** Number. */
    DBGCVAR_TYPE_NUMBER,
    /** String. */
    DBGCVAR_TYPE_STRING,
    /** Symbol. */
    DBGCVAR_TYPE_SYMBOL,
    /** Special type used when querying symbols. */
    DBGCVAR_TYPE_ANY
} DBGCVARTYPE;

/** @todo Rename to DBGCVAR_IS_xyz. */

/** Checks if the specified variable type is of a pointer persuasion. */
#define DBGCVAR_ISPOINTER(enmType)      ((enmType) >= DBGCVAR_TYPE_GC_FLAT && enmType <= DBGCVAR_TYPE_HC_PHYS)
/** Checks if the specified variable type is of a pointer persuasion. */
#define DBGCVAR_IS_FAR_PTR(enmType)     ((enmType) == DBGCVAR_TYPE_GC_FAR)
/** Checks if the specified variable type is of a pointer persuasion and of the guest context sort. */
#define DBGCVAR_ISGCPOINTER(enmType)    ((enmType) >= DBGCVAR_TYPE_GC_FLAT && (enmType) <= DBGCVAR_TYPE_GC_PHYS)
/** Checks if the specified variable type is of a pointer persuasion and of the host context sort. */
#define DBGCVAR_ISHCPOINTER(enmType)    ((enmType) >= DBGCVAR_TYPE_HC_FLAT && (enmType) <= DBGCVAR_TYPE_HC_PHYS)


/**
 * DBGC variable range type.
 */
typedef enum DBGCVARRANGETYPE
{
    /** No range appliable or no range specified. */
    DBGCVAR_RANGE_NONE = 0,
    /** Number of elements. */
    DBGCVAR_RANGE_ELEMENTS,
    /** Number of bytes. */
    DBGCVAR_RANGE_BYTES
} DBGCVARRANGETYPE;


/**
 * Variable descriptor.
 */
typedef struct DBGCVARDESC
{
    /** The minimal number of times this argument may occur.
     * Use 0 here to inidicate that the argument is optional. */
    unsigned    cTimesMin;
    /** Maximum number of occurrences.
     * Use ~0 here to indicate infinite. */
    unsigned    cTimesMax;
    /** Argument category. */
    DBGCVARCAT  enmCategory;
    /** Flags, DBGCVD_FLAGS_* */
    unsigned    fFlags;
    /** Argument name. */
    const char *pszName;
    /** Argument name. */
    const char *pszDescription;
} DBGCVARDESC;
/** Pointer to an argument descriptor. */
typedef DBGCVARDESC *PDBGCVARDESC;
/** Pointer to a const argument descriptor. */
typedef const DBGCVARDESC *PCDBGCVARDESC;

/** Variable descriptor flags.
 * @{ */
/** Indicates that the variable depends on the previous being present. */
#define DBGCVD_FLAGS_DEP_PREV       RT_BIT(1)
/** @} */


/**
 * DBGC variable.
 */
typedef struct DBGCVAR
{
    /** Pointer to the argument descriptor. */
    PCDBGCVARDESC   pDesc;
    /** Pointer to the next argument. */
    struct DBGCVAR *pNext;

    /** Argument type. */
    DBGCVARTYPE     enmType;
    /** Type specific. */
    union
    {
        /** Flat GC Address.        (DBGCVAR_TYPE_GC_FLAT) */
        RTGCPTR         GCFlat;
        /** Far (16:32) GC Address. (DBGCVAR_TYPE_GC_FAR) */
        RTFAR32         GCFar;
        /** Physical GC Address.    (DBGCVAR_TYPE_GC_PHYS) */
        RTGCPHYS        GCPhys;
        /** Flat HC Address.        (DBGCVAR_TYPE_HC_FLAT) */
        void           *pvHCFlat;
        /** Physical GC Address.    (DBGCVAR_TYPE_HC_PHYS) */
        RTHCPHYS        HCPhys;
        /** String.                 (DBGCVAR_TYPE_STRING)
         * The basic idea is the the this is a pointer to the expression we're
         * parsing, so no messing with freeing. */
        const char     *pszString;
        /** Number.                 (DBGCVAR_TYPE_NUMBER) */
        uint64_t        u64Number;
    } u;

    /** Range type. */
    DBGCVARRANGETYPE    enmRangeType;
    /** Range. The use of the content depends on the enmRangeType. */
    uint64_t            u64Range;
} DBGCVAR;
/** Pointer to a command argument. */
typedef DBGCVAR *PDBGCVAR;
/** Pointer to a const command argument. */
typedef const DBGCVAR *PCDBGCVAR;


/**
 * Macro for initializing a DBGC variable with defaults.
 * The result is an unknown variable type without any range.
 */
#define DBGCVAR_INIT(pVar) \
        do { \
            (pVar)->pDesc = NULL;\
            (pVar)->pNext = NULL; \
            (pVar)->enmType = DBGCVAR_TYPE_UNKNOWN; \
            (pVar)->u.u64Number = 0; \
            (pVar)->enmRangeType = DBGCVAR_RANGE_NONE; \
            (pVar)->u64Range = 0; \
        } while (0)

/**
 * Macro for initializing a DBGC variable with a HC physical address.
 */
#define DBGCVAR_INIT_HC_PHYS(pVar, Phys) \
        do { \
            DBGCVAR_INIT(pVar); \
            (pVar)->enmType = DBGCVAR_TYPE_HC_PHYS; \
            (pVar)->u.HCPhys = (Phys); \
        } while (0)

/**
 * Macro for initializing a DBGC variable with a HC flat address.
 */
#define DBGCVAR_INIT_HC_FLAT(pVar, Flat) \
        do { \
            DBGCVAR_INIT(pVar); \
            (pVar)->enmType = DBGCVAR_TYPE_HC_FLAT; \
            (pVar)->u.pvHCFlat = (Flat); \
        } while (0)

/**
 * Macro for initializing a DBGC variable with a GC physical address.
 */
#define DBGCVAR_INIT_GC_PHYS(pVar, Phys) \
        do { \
            DBGCVAR_INIT(pVar); \
            (pVar)->enmType = DBGCVAR_TYPE_GC_PHYS; \
            (pVar)->u.GCPhys = (Phys); \
        } while (0)

/**
 * Macro for initializing a DBGC variable with a GC flat address.
 */
#define DBGCVAR_INIT_GC_FLAT(pVar, Flat) \
        do { \
            DBGCVAR_INIT(pVar); \
            (pVar)->enmType = DBGCVAR_TYPE_GC_FLAT; \
            (pVar)->u.GCFlat = (Flat); \
        } while (0)

/**
 * Macro for initializing a DBGC variable with a GC flat address.
 */
#define DBGCVAR_INIT_GC_FLAT_BYTE_RANGE(pVar, Flat, cbRange) \
        do { \
            DBGCVAR_INIT(pVar); \
            (pVar)->enmType = DBGCVAR_TYPE_GC_FLAT; \
            (pVar)->u.GCFlat = (Flat); \
            DBGCVAR_SET_RANGE(pVar, DBGCVAR_RANGE_BYTES, cbRange); \
        } while (0)

/**
 * Macro for initializing a DBGC variable with a GC far address.
 */
#define DBGCVAR_INIT_GC_FAR(pVar, _sel, _off) \
        do { \
            DBGCVAR_INIT(pVar); \
            (pVar)->enmType = DBGCVAR_TYPE_GC_FAR; \
            (pVar)->u.GCFar.sel = (_sel); \
            (pVar)->u.GCFar.off = (_off); \
        } while (0)

/**
 * Macro for initializing a DBGC variable with a number.
 */
#define DBGCVAR_INIT_NUMBER(pVar, Value) \
        do { \
            DBGCVAR_INIT(pVar); \
            (pVar)->enmType = DBGCVAR_TYPE_NUMBER; \
            (pVar)->u.u64Number = (Value); \
        } while (0)

/**
 * Macro for initializing a DBGC variable with a string.
 */
#define DBGCVAR_INIT_STRING(pVar, a_pszString) \
        do { \
            DBGCVAR_INIT(pVar); \
            (pVar)->enmType      = DBGCVAR_TYPE_STRING; \
            (pVar)->enmRangeType = DBGCVAR_RANGE_BYTES; \
            (pVar)->u.pszString  = (a_pszString); \
            (pVar)->u64Range     = strlen(a_pszString); \
        } while (0)


/**
 * Macro for initializing a DBGC variable with a symbol.
 */
#define DBGCVAR_INIT_SYMBOL(pVar, a_pszSymbol) \
        do { \
            DBGCVAR_INIT(pVar); \
            (pVar)->enmType      = DBGCVAR_TYPE_SYMBOL; \
            (pVar)->enmRangeType = DBGCVAR_RANGE_BYTES; \
            (pVar)->u.pszString  = (a_pszSymbol); \
            (pVar)->u64Range     = strlen(a_pszSymbol); \
        } while (0)


/**
 * Macro for setting the range of a DBGC variable.
 * @param   pVar            The variable.
 * @param   _enmRangeType   The range type.
 * @param   Value           The range length value.
 */
#define DBGCVAR_SET_RANGE(pVar, _enmRangeType, Value) \
    do { \
        (pVar)->enmRangeType = (_enmRangeType); \
        (pVar)->u64Range = (Value); \
    } while (0)


/**
 * Macro for setting the range of a DBGC variable.
 * @param   a_pVar          The variable.
 * @param   a_cbRange       The range, in bytes.
 */
#define DBGCVAR_SET_BYTE_RANGE(a_pVar, a_cbRange) \
    DBGCVAR_SET_RANGE(a_pVar, DBGCVAR_RANGE_BYTES, a_cbRange)


/**
 * Macro for resetting the range a DBGC variable.
 * @param   a_pVar          The variable.
 */
#define DBGCVAR_ZAP_RANGE(a_pVar) \
    do { \
        (a_pVar)->enmRangeType  = DBGCVAR_RANGE_NONE; \
        (a_pVar)->u64Range      = 0; \
    } while (0)


/**
 * Macro for assigning one DBGC variable to another.
 * @param   a_pResult       The result (target) variable.
 * @param   a_pVar          The source variable.
 */
#define DBGCVAR_ASSIGN(a_pResult, a_pVar) \
    do { \
        *(a_pResult) = *(a_pVar); \
    } while (0)


/** Pointer to a command descriptor. */
typedef struct DBGCCMD *PDBGCCMD;
/** Pointer to a const command descriptor. */
typedef const struct DBGCCMD *PCDBGCCMD;

/** Pointer to a function descriptor. */
typedef struct DBGCFUNC *PDBGCFUNC;
/** Pointer to a const function descriptor. */
typedef const struct DBGCFUNC *PCDBGCFUNC;

/** Pointer to helper functions for commands. */
typedef struct DBGCCMDHLP *PDBGCCMDHLP;


/**
 * Helper functions for commands.
 */
typedef struct DBGCCMDHLP
{
    /** Magic value (DBGCCMDHLP_MAGIC). */
    uint32_t                u32Magic;

    /**
     * Command helper for writing formatted text to the debug console.
     *
     * @returns VBox status.
     * @param   pCmdHlp     Pointer to the command callback structure.
     * @param   pcbWritten  Where to store the number of bytes written.
     *                      This is optional.
     * @param   pszFormat   The format string.  This may use all IPRT extensions as
     *                      well as the debugger ones.
     * @param   ...         Arguments specified in the format string.
     */
    DECLCALLBACKMEMBER(int, pfnPrintf,(PDBGCCMDHLP pCmdHlp, size_t *pcbWritten,
                                       const char *pszFormat, ...)) RT_IPRT_FORMAT_ATTR(3, 4);

    /**
     * Command helper for writing formatted text to the debug console.
     *
     * @returns VBox status.
     * @param   pCmdHlp     Pointer to the command callback structure.
     * @param   pcbWritten  Where to store the number of bytes written.
     *                      This is optional.
     * @param   pszFormat   The format string.  This may use all IPRT extensions as
     *                      well as the debugger ones.
     * @param   args        Arguments specified in the format string.
     */
    DECLCALLBACKMEMBER(int, pfnPrintfV,(PDBGCCMDHLP pCmdHlp, size_t *pcbWritten,
                                        const char *pszFormat, va_list args)) RT_IPRT_FORMAT_ATTR(3, 0);

    /**
     * Command helper for formatting a string with debugger format specifiers.
     *
     * @returns The number of bytes written.
     * @param   pCmdHlp     Pointer to the command callback structure.
     * @param   pszBuf      The output buffer.
     * @param   cbBuf       The size of the output buffer.
     * @param   pszFormat   The format string.  This may use all IPRT extensions as
     *                      well as the debugger ones.
     * @param   ...         Arguments specified in the format string.
     */
    DECLCALLBACKMEMBER(size_t, pfnStrPrintf,(PDBGCCMDHLP pCmdHlp, char *pszBuf, size_t cbBuf,
                                             const char *pszFormat, ...)) RT_IPRT_FORMAT_ATTR(4, 5);

    /**
     * Command helper for formatting a string with debugger format specifiers.
     *
     * @returns The number of bytes written.
     * @param   pCmdHlp     Pointer to the command callback structure.
     * @param   pszBuf      The output buffer.
     * @param   cbBuf       The size of the output buffer.
     * @param   pszFormat   The format string.  This may use all IPRT extensions as
     *                      well as the debugger ones.
     * @param   va          Arguments specified in the format string.
     */
    DECLCALLBACKMEMBER(size_t, pfnStrPrintfV,(PDBGCCMDHLP pCmdHlp, char *pszBuf, size_t cbBuf,
                                              const char *pszFormat, va_list va)) RT_IPRT_FORMAT_ATTR(4, 0);

    /**
     * Command helper for formatting and error message for a VBox status code.
     *
     * @returns VBox status code appropriate to return from a command.
     * @param   pCmdHlp     Pointer to the command callback structure.
     * @param   rc          The VBox status code.
     * @param   pszFormat   Format string for additional messages. Can be NULL.
     * @param   ...         Format arguments, optional.
     */
    DECLCALLBACKMEMBER(int, pfnVBoxError,(PDBGCCMDHLP pCmdHlp, int rc, const char *pszFormat, ...)) RT_IPRT_FORMAT_ATTR(3, 4);

    /**
     * Command helper for formatting and error message for a VBox status code.
     *
     * @returns VBox status code appropriate to return from a command.
     * @param   pCmdHlp     Pointer to the command callback structure.
     * @param   rc          The VBox status code.
     * @param   pszFormat   Format string for additional messages. Can be NULL.
     * @param   args        Format arguments, optional.
     */
    DECLCALLBACKMEMBER(int, pfnVBoxErrorV,(PDBGCCMDHLP pCmdHlp, int rc,
                                           const char *pszFormat, va_list args)) RT_IPRT_FORMAT_ATTR(3, 0);

    /**
     * Command helper for reading memory specified by a DBGC variable.
     *
     * @returns VBox status code appropriate to return from a command.
     * @param   pCmdHlp     Pointer to the command callback structure.
     * @param   pvBuffer    Where to store the read data.
     * @param   cbRead      Number of bytes to read.
     * @param   pVarPointer DBGC variable specifying where to start reading.
     * @param   pcbRead     Where to store the number of bytes actually read.
     *                      This optional, but it's useful when read GC virtual memory where a
     *                      page in the requested range might not be present.
     *                      If not specified not-present failure or end of a HC physical page
     *                      will cause failure.
     */
    DECLCALLBACKMEMBER(int, pfnMemRead,(PDBGCCMDHLP pCmdHlp, void *pvBuffer, size_t cbRead, PCDBGCVAR pVarPointer, size_t *pcbRead));

    /**
     * Command helper for writing memory specified by a DBGC variable.
     *
     * @returns VBox status code appropriate to return from a command.
     * @param   pCmdHlp     Pointer to the command callback structure.
     * @param   pvBuffer    What to write.
     * @param   cbWrite     Number of bytes to write.
     * @param   pVarPointer DBGC variable specifying where to start reading.
     * @param   pcbWritten  Where to store the number of bytes written.
     *                      This is optional. If NULL be aware that some of the buffer
     *                      might have been written to the specified address.
     */
    DECLCALLBACKMEMBER(int, pfnMemWrite,(PDBGCCMDHLP pCmdHlp, const void *pvBuffer, size_t cbWrite, PCDBGCVAR pVarPointer, size_t *pcbWritten));

    /**
     * Executes command an expression.
     * (Hopefully the parser and functions are fully reentrant.)
     *
     * @returns VBox status code appropriate to return from a command.
     * @param   pCmdHlp     Pointer to the command callback structure.
     * @param   pszExpr     The expression. Format string with the format DBGC extensions.
     * @param   ...         Format arguments.
     */
    DECLCALLBACKMEMBER(int, pfnExec,(PDBGCCMDHLP pCmdHlp, const char *pszExpr, ...)) RT_IPRT_FORMAT_ATTR(2, 3);

    /**
     * Evaluates an expression.
     * (Hopefully the parser and functions are fully reentrant.)
     *
     * @returns VBox status code appropriate to return from a command.
     * @param   pCmdHlp     Pointer to the command callback structure.
     * @param   pResult     Where to store the result.
     * @param   pszExpr     The expression. Format string with the format DBGC extensions.
     * @param   va          Format arguments.
     */
    DECLCALLBACKMEMBER(int, pfnEvalV,(PDBGCCMDHLP pCmdHlp, PDBGCVAR pResult,
                                      const char *pszExpr, va_list va)) RT_IPRT_FORMAT_ATTR(3, 0);

    /**
     * Print an error and fail the current command.
     *
     * @returns VBox status code to pass upwards.
     *
     * @param   pCmdHlp     Pointer to the command callback structure.
     * @param   pCmd        The failing command.
     * @param   pszFormat   The error message format string.
     * @param   va          Format arguments.
     */
    DECLCALLBACKMEMBER(int, pfnFailV,(PDBGCCMDHLP pCmdHlp, PCDBGCCMD pCmd,
                                      const char *pszFormat, va_list va)) RT_IPRT_FORMAT_ATTR(3, 0);

    /**
     * Print an error and fail the current command.
     *
     * @returns VBox status code to pass upwards.
     *
     * @param   pCmdHlp     Pointer to the command callback structure.
     * @param   pCmd        The failing command.
     * @param   rc          The status code indicating the failure.  This will
     *                      be appended to the message after a colon (': ').
     * @param   pszFormat   The error message format string.
     * @param   va          Format arguments.
     *
     * @see     DBGCCmdHlpFailRc
     */
    DECLCALLBACKMEMBER(int, pfnFailRcV,(PDBGCCMDHLP pCmdHlp, PCDBGCCMD pCmd, int rc,
                                        const char *pszFormat, va_list va)) RT_IPRT_FORMAT_ATTR(4, 0);

    /**
     * Parser error.
     *
     * @returns VBox status code to pass upwards.
     *
     * @param   pCmdHlp     Pointer to the command callback structure.
     * @param   pCmd        The failing command, can be NULL but shouldn't.
     * @param   iArg        The offending argument, -1 when lazy.
     * @param   pszExpr     The expression.
     * @param   iLine       The line number.
     */
    DECLCALLBACKMEMBER(int, pfnParserError,(PDBGCCMDHLP pCmdHlp, PCDBGCCMD pCmd, int iArg, const char *pszExpr, unsigned iLine));

    /**
     * Converts a DBGC variable to a DBGF address structure.
     *
     * @returns VBox status code.
     * @param   pCmdHlp     Pointer to the command callback structure.
     * @param   pVar        The variable to convert.
     * @param   pAddress    The target address.
     */
    DECLCALLBACKMEMBER(int, pfnVarToDbgfAddr,(PDBGCCMDHLP pCmdHlp, PCDBGCVAR pVar, PDBGFADDRESS pAddress));

    /**
     * Converts a DBGF address structure to a DBGC variable.
     *
     * @returns VBox status code.
     * @param   pCmdHlp     Pointer to the command callback structure.
     * @param   pAddress    The source address.
     * @param   pResult     The result variable.
     */
    DECLCALLBACKMEMBER(int, pfnVarFromDbgfAddr,(PDBGCCMDHLP pCmdHlp, PCDBGFADDRESS pAddress, PDBGCVAR pResult));

    /**
     * Converts a DBGC variable to a 64-bit number.
     *
     * @returns VBox status code.
     * @param   pCmdHlp     Pointer to the command callback structure.
     * @param   pVar        The variable to convert.
     * @param   pu64Number  Where to store the number.
     */
    DECLCALLBACKMEMBER(int, pfnVarToNumber,(PDBGCCMDHLP pCmdHlp, PCDBGCVAR pVar, uint64_t *pu64Number));

    /**
     * Converts a DBGC variable to a boolean.
     *
     * @returns VBox status code.
     * @param   pCmdHlp     Pointer to the command callback structure.
     * @param   pVar        The variable to convert.
     * @param   pf          Where to store the boolean.
     */
    DECLCALLBACKMEMBER(int, pfnVarToBool,(PDBGCCMDHLP pCmdHlp, PCDBGCVAR pVar, bool *pf));

    /**
     * Get the range of a variable in bytes, resolving symbols if necessary.
     *
     * @returns VBox status code.
     * @param   pCmdHlp     Pointer to the command callback structure.
     * @param   pVar        The variable to convert.
     * @param   cbElement   Conversion factor for element ranges.
     * @param   cbDefault   The default range.
     * @param   pcbRange    The length of the range.
     */
    DECLCALLBACKMEMBER(int, pfnVarGetRange,(PDBGCCMDHLP pCmdHlp, PCDBGCVAR pVar, uint64_t cbElement, uint64_t cbDefault,
                                            uint64_t *pcbRange));

    /**
     * Converts a variable to one with the specified type.
     *
     * This preserves the range.
     *
     * @returns VBox status code.
     * @param   pCmdHlp     Pointer to the command callback structure.
     * @param   pVar        The variable to convert.
     * @param   enmToType   The target type.
     * @param   fConvSyms   If @c true, then attempt to resolve symbols.
     * @param   pResult     The output variable. Can be the same as @a pVar.
     */
    DECLCALLBACKMEMBER(int, pfnVarConvert,(PDBGCCMDHLP pCmdHlp, PCDBGCVAR pVar, DBGCVARTYPE enmToType, bool fConvSyms,
                                           PDBGCVAR pResult));

    /**
     * Gets a DBGF output helper that directs the output to the debugger
     * console.
     *
     * @returns Pointer to the helper structure.
     * @param   pCmdHlp     Pointer to the command callback structure.
     */
    DECLCALLBACKMEMBER(PCDBGFINFOHLP, pfnGetDbgfOutputHlp,(PDBGCCMDHLP pCmdHlp));

    /**
     * Gets the ID currently selected CPU.
     *
     * @returns Current CPU ID.
     * @param   pCmdHlp     Pointer to the command callback structure.
     */
    DECLCALLBACKMEMBER(VMCPUID, pfnGetCurrentCpu,(PDBGCCMDHLP pCmdHlp));

    /**
     * Gets the mode the currently selected CPU is running in, in the current
     * context.
     *
     * @returns Current CPU mode.
     * @param   pCmdHlp     Pointer to the command callback structure.
     */
    DECLCALLBACKMEMBER(CPUMMODE, pfnGetCpuMode,(PDBGCCMDHLP pCmdHlp));

    /**
     * Prints the register set of the given CPU.
     *
     * @returns VBox status code.
     * @param   pCmdHlp     Pointer to the command callback structure.
     * @param   idCpu       The CPU ID to print the register set of.
     * @param   f64BitMode  True to dump 64-bit state, false to dump 32-bit state,
     *                      -1 to use the current CPU mode.
     * @param   fTerse      Flag to indicate whether to dump the complete register set.
     */
    DECLCALLBACKMEMBER(int, pfnRegPrintf, (PDBGCCMDHLP pCmdHlp, VMCPUID idCpu, int f64BitMode, bool fTerse));

    /** End marker (DBGCCMDHLP_MAGIC). */
    uint32_t                u32EndMarker;
} DBGCCMDHLP;

/** Magic value for DBGCCMDHLP::u32Magic. (Fyodor Mikhaylovich Dostoyevsky) */
#define DBGCCMDHLP_MAGIC    UINT32_C(18211111)


#if defined(IN_RING3) || defined(IN_SLICKEDIT)

/**
 * Command helper for writing formatted text to the debug console.
 *
 * @returns VBox status.
 * @param   pCmdHlp     Pointer to the command callback structure.
 * @param   pszFormat   The format string.  This may use all IPRT extensions as
 *                      well as the debugger ones.
 * @param   ...         Arguments specified in the format string.
 */
DECLINLINE(int) RT_IPRT_FORMAT_ATTR(2, 3) DBGCCmdHlpPrintf(PDBGCCMDHLP pCmdHlp, const char *pszFormat, ...)
{
    va_list va;
    int     rc;

    va_start(va, pszFormat);
    rc = pCmdHlp->pfnPrintfV(pCmdHlp, NULL, pszFormat, va);
    va_end(va);

    return rc;
}

/**
 * Command helper for writing formatted text to the debug console.
 *
 * @returns VBox status.
 * @param   pCmdHlp     Pointer to the command callback structure.
 * @param   pcbWritten  Where to store the amount of written characters on success.
 * @param   pszFormat   The format string.  This may use all IPRT extensions as
 *                      well as the debugger ones.
 * @param   ...         Arguments specified in the format string.
 */
DECLINLINE(int) RT_IPRT_FORMAT_ATTR(3, 4) DBGCCmdHlpPrintfEx(PDBGCCMDHLP pCmdHlp, size_t *pcbWritten,
                                                             const char *pszFormat, ...)
{
    va_list va;
    int     rc;

    va_start(va, pszFormat);
    rc = pCmdHlp->pfnPrintfV(pCmdHlp, pcbWritten, pszFormat, va);
    va_end(va);

    return rc;
}

/**
 * Command helper for writing formatted text to the debug console.
 *
 * @returns Number of bytes written.
 * @param   pCmdHlp     Pointer to the command callback structure.
 * @param   pszFormat   The format string.  This may use all IPRT extensions as
 *                      well as the debugger ones.
 * @param   ...         Arguments specified in the format string.
 */
DECLINLINE(size_t) RT_IPRT_FORMAT_ATTR(2, 3) DBGCCmdHlpPrintfLen(PDBGCCMDHLP pCmdHlp, const char *pszFormat, ...)
{
    va_list va;
    int     rc;
    size_t  cbWritten = 0;

    va_start(va, pszFormat);
    rc = pCmdHlp->pfnPrintfV(pCmdHlp, &cbWritten, pszFormat, va);
    va_end(va);

    return RT_SUCCESS(rc) ? cbWritten : 0;
}

/**
 * @copydoc DBGCCMDHLP::pfnStrPrintf
 */
DECLINLINE(size_t) RT_IPRT_FORMAT_ATTR(4, 5) DBGCCmdHlpStrPrintf(PDBGCCMDHLP pCmdHlp, char *pszBuf, size_t cbBuf,
                                                                 const char *pszFormat, ...)
{
    va_list va;
    size_t  cch;

    va_start(va, pszFormat);
    cch = pCmdHlp->pfnStrPrintfV(pCmdHlp, pszBuf, cbBuf, pszFormat, va);
    va_end(va);

    return cch;
}

/**
 * @copydoc DBGCCMDHLP::pfnVBoxError
 */
DECLINLINE(int) RT_IPRT_FORMAT_ATTR(3, 4) DBGCCmdHlpVBoxError(PDBGCCMDHLP pCmdHlp, int rc, const char *pszFormat, ...)
{
    va_list va;

    va_start(va, pszFormat);
    rc = pCmdHlp->pfnVBoxErrorV(pCmdHlp, rc, pszFormat, va);
    va_end(va);

    return rc;
}

/**
 * @copydoc DBGCCMDHLP::pfnMemRead
 */
DECLINLINE(int) DBGCCmdHlpMemRead(PDBGCCMDHLP pCmdHlp, void *pvBuffer, size_t cbRead, PCDBGCVAR pVarPointer, size_t *pcbRead)
{
    return pCmdHlp->pfnMemRead(pCmdHlp, pvBuffer, cbRead, pVarPointer, pcbRead);
}

/**
 * Evaluates an expression.
 * (Hopefully the parser and functions are fully reentrant.)
 *
 * @returns VBox status code appropriate to return from a command.
 * @param   pCmdHlp     Pointer to the command callback structure.
 * @param   pResult     Where to store the result.
 * @param   pszExpr     The expression. Format string with the format DBGC extensions.
 * @param   ...         Format arguments.
 */
DECLINLINE(int) RT_IPRT_FORMAT_ATTR(3, 4) DBGCCmdHlpEval(PDBGCCMDHLP pCmdHlp, PDBGCVAR pResult, const char *pszExpr, ...)
{
    va_list va;
    int     rc;

    va_start(va, pszExpr);
    rc = pCmdHlp->pfnEvalV(pCmdHlp, pResult, pszExpr, va);
    va_end(va);

    return rc;
}

/**
 * Print an error and fail the current command.
 *
 * @returns VBox status code to pass upwards.
 *
 * @param   pCmdHlp     Pointer to the command callback structure.
 * @param   pCmd        The failing command.
 * @param   pszFormat   The error message format string.
 * @param   ...         Format arguments.
 */
DECLINLINE(int) RT_IPRT_FORMAT_ATTR(3, 4) DBGCCmdHlpFail(PDBGCCMDHLP pCmdHlp, PCDBGCCMD pCmd, const char *pszFormat, ...)
{
    va_list va;
    int     rc;

    va_start(va, pszFormat);
    rc = pCmdHlp->pfnFailV(pCmdHlp, pCmd, pszFormat, va);
    va_end(va);

    return rc;
}

/**
 * Print an error and fail the current command.
 *
 * Usage example:
 * @code
    int rc = VMMR3Something(pVM);
    if (RT_FAILURE(rc))
        return DBGCCmdHlpFailRc(pCmdHlp, pCmd, rc, "VMMR3Something");
    return VINF_SUCCESS;
 * @endcode
 *
 * @returns VBox status code to pass upwards.
 *
 * @param   pCmdHlp     Pointer to the command callback structure.
 * @param   pCmd        The failing command.
 * @param   rc          The status code indicating the failure.
 * @param   pszFormat   The error message format string.
 * @param   ...         Format arguments.
 */
DECLINLINE(int) RT_IPRT_FORMAT_ATTR(4, 5) DBGCCmdHlpFailRc(PDBGCCMDHLP pCmdHlp, PCDBGCCMD pCmd, int rc,
                                                           const char *pszFormat, ...)
{
    va_list va;

    va_start(va, pszFormat);
    rc = pCmdHlp->pfnFailRcV(pCmdHlp, pCmd, rc, pszFormat, va);
    va_end(va);

    return rc;
}

/**
 * @copydoc DBGCCMDHLP::pfnParserError
 */
DECLINLINE(int) DBGCCmdHlpParserError(PDBGCCMDHLP pCmdHlp, PCDBGCCMD pCmd, int iArg, const char *pszExpr, unsigned iLine)
{
    return pCmdHlp->pfnParserError(pCmdHlp, pCmd, iArg, pszExpr, iLine);
}

/** Assert+return like macro for checking parser sanity.
 * Returns with failure if the precodition is not met. */
#define DBGC_CMDHLP_ASSERT_PARSER_RET(pCmdHlp, pCmd, iArg, expr) \
    do { \
        if (!(expr)) \
            return DBGCCmdHlpParserError(pCmdHlp, pCmd, iArg, #expr, __LINE__); \
    } while (0)

/** Assert+return like macro that the VM handle is present.
 * Returns with failure if the VM handle is NIL.  */
#define DBGC_CMDHLP_REQ_UVM_RET(pCmdHlp, pCmd, pUVM) \
    do { \
        if (!(pUVM)) \
            return DBGCCmdHlpFail(pCmdHlp, pCmd, "No VM selected"); \
    } while (0)

/**
 * @copydoc DBGCCMDHLP::pfnVarToDbgfAddr
 */
DECLINLINE(int) DBGCCmdHlpVarToDbgfAddr(PDBGCCMDHLP pCmdHlp, PCDBGCVAR pVar, PDBGFADDRESS pAddress)
{
    return pCmdHlp->pfnVarToDbgfAddr(pCmdHlp, pVar, pAddress);
}

/**
 * @copydoc DBGCCMDHLP::pfnVarFromDbgfAddr
 */
DECLINLINE(int) DBGCCmdHlpVarFromDbgfAddr(PDBGCCMDHLP pCmdHlp, PCDBGFADDRESS pAddress, PDBGCVAR pResult)
{
    return pCmdHlp->pfnVarFromDbgfAddr(pCmdHlp, pAddress, pResult);
}

/**
 * Converts an variable to a flat address.
 *
 * @returns VBox status code.
 * @param   pCmdHlp     Pointer to the command callback structure.
 * @param   pVar        The variable to convert.
 * @param   pFlatPtr    Where to store the flat address.
 */
DECLINLINE(int) DBGCCmdHlpVarToFlatAddr(PDBGCCMDHLP pCmdHlp, PCDBGCVAR pVar, PRTGCPTR pFlatPtr)
{
    DBGFADDRESS Addr;
    int rc = pCmdHlp->pfnVarToDbgfAddr(pCmdHlp, pVar, &Addr);
    if (RT_SUCCESS(rc))
        *pFlatPtr = Addr.FlatPtr;
    return rc;
}

/**
 * @copydoc DBGCCMDHLP::pfnVarToNumber
 */
DECLINLINE(int) DBGCCmdHlpVarToNumber(PDBGCCMDHLP pCmdHlp, PCDBGCVAR pVar, uint64_t *pu64Number)
{
    return pCmdHlp->pfnVarToNumber(pCmdHlp, pVar, pu64Number);
}

/**
 * @copydoc DBGCCMDHLP::pfnVarToBool
 */
DECLINLINE(int) DBGCCmdHlpVarToBool(PDBGCCMDHLP pCmdHlp, PCDBGCVAR pVar, bool *pf)
{
    return pCmdHlp->pfnVarToBool(pCmdHlp, pVar, pf);
}

/**
 * @copydoc DBGCCMDHLP::pfnVarGetRange
 */
DECLINLINE(int) DBGCCmdHlpVarGetRange(PDBGCCMDHLP pCmdHlp, PCDBGCVAR pVar, uint64_t cbElement, uint64_t cbDefault, uint64_t *pcbRange)
{
    return pCmdHlp->pfnVarGetRange(pCmdHlp, pVar, cbElement, cbDefault, pcbRange);
}

/**
 * @copydoc DBGCCMDHLP::pfnVarConvert
 */
DECLINLINE(int) DBGCCmdHlpConvert(PDBGCCMDHLP pCmdHlp, PCDBGCVAR pVar, DBGCVARTYPE enmToType, bool fConvSyms, PDBGCVAR pResult)
{
    return pCmdHlp->pfnVarConvert(pCmdHlp, pVar, enmToType, fConvSyms, pResult);
}

/**
 * @copydoc DBGCCMDHLP::pfnGetDbgfOutputHlp
 */
DECLINLINE(PCDBGFINFOHLP) DBGCCmdHlpGetDbgfOutputHlp(PDBGCCMDHLP pCmdHlp)
{
    return pCmdHlp->pfnGetDbgfOutputHlp(pCmdHlp);
}

/**
 * @copydoc DBGCCMDHLP::pfnGetCurrentCpu
 */
DECLINLINE(VMCPUID) DBGCCmdHlpGetCurrentCpu(PDBGCCMDHLP pCmdHlp)
{
    return pCmdHlp->pfnGetCurrentCpu(pCmdHlp);
}

/**
 * @copydoc DBGCCMDHLP::pfnGetCpuMode
 */
DECLINLINE(CPUMMODE) DBGCCmdHlpGetCpuMode(PDBGCCMDHLP pCmdHlp)
{
    return pCmdHlp->pfnGetCpuMode(pCmdHlp);
}

/**
 * @copydoc DBGCCMDHLP::pfnRegPrintf
 */
DECLINLINE(int) DBGCCmdHlpRegPrintf(PDBGCCMDHLP pCmdHlp, VMCPUID idCpu, int f64BitMode, bool fTerse)
{
    return pCmdHlp->pfnRegPrintf(pCmdHlp, idCpu, f64BitMode, fTerse);
}

#endif /* IN_RING3 */



/**
 * Command handler.
 *
 * The console will call the handler for a command once it's finished
 * parsing the user input.  The command handler function is responsible
 * for executing the command itself.
 *
 * @returns VBox status.
 * @param   pCmd        Pointer to the command descriptor (as registered).
 * @param   pCmdHlp     Pointer to command helper functions.
 * @param   pUVM        The user mode VM handle, can in theory be NULL.
 * @param   paArgs      Pointer to (readonly) array of arguments.
 * @param   cArgs       Number of arguments in the array.
 */
typedef DECLCALLBACKTYPE(int, FNDBGCCMD,(PCDBGCCMD pCmd, PDBGCCMDHLP pCmdHlp, PUVM pUVM, PCDBGCVAR paArgs, unsigned cArgs));
/** Pointer to a FNDBGCCMD() function. */
typedef FNDBGCCMD *PFNDBGCCMD;

/**
 * DBGC command descriptor.
 */
typedef struct DBGCCMD
{
    /** Command string. */
    const char     *pszCmd;
    /** Minimum number of arguments. */
    unsigned        cArgsMin;
    /** Max number of arguments. */
    unsigned        cArgsMax;
    /** Argument descriptors (array). */
    PCDBGCVARDESC   paArgDescs;
    /** Number of argument descriptors. */
    unsigned        cArgDescs;
    /** flags. (reserved for now) */
    unsigned        fFlags;
    /** Handler function. */
    PFNDBGCCMD      pfnHandler;
    /** Command syntax. */
    const char     *pszSyntax;
    /** Command description. */
    const char     *pszDescription;
} DBGCCMD;

/** DBGCCMD Flags.
 * @{
 */
/** @} */


/**
 * Function handler.
 *
 * The console will call the handler for a command once it's finished
 * parsing the user input.  The command handler function is responsible
 * for executing the command itself.
 *
 * @returns VBox status.
 * @param   pFunc       Pointer to the function descriptor (as registered).
 * @param   pCmdHlp     Pointer to command helper functions.
 * @param   pUVM        The user mode VM handle, can in theory be NULL.
 * @param   paArgs      Pointer to (readonly) array of arguments.
 * @param   cArgs       Number of arguments in the array.
 * @param   pResult     Where to return the result.
 */
typedef DECLCALLBACKTYPE(int, FNDBGCFUNC,(PCDBGCFUNC pFunc, PDBGCCMDHLP pCmdHlp, PUVM pUVM, PCDBGCVAR paArgs, unsigned cArgs,
                                          PDBGCVAR pResult));
/** Pointer to a FNDBGCFUNC() function. */
typedef FNDBGCFUNC *PFNDBGCFUNC;

/**
 * DBGC function descriptor.
 */
typedef struct DBGCFUNC
{
    /** Command string. */
    const char     *pszFuncNm;
    /** Minimum number of arguments. */
    unsigned        cArgsMin;
    /** Max number of arguments. */
    unsigned        cArgsMax;
    /** Argument descriptors (array). */
    PCDBGCVARDESC   paArgDescs;
    /** Number of argument descriptors. */
    unsigned        cArgDescs;
    /** flags. (reserved for now) */
    unsigned        fFlags;
    /** Handler function. */
    PFNDBGCFUNC     pfnHandler;
    /** Function syntax. */
    const char     *pszSyntax;
    /** Function description. */
    const char     *pszDescription;
} DBGCFUNC;


/** Pointer to a const I/O callback table. */
typedef const struct DBGCIO *PCDBGCIO;

/**
 * I/O callback table.
 */
typedef struct DBGCIO
{
    /**
     * Destroys the given I/O instance.
     *
     * @param   pIo         Pointer to the I/O structure supplied by the I/O provider.
     */
    DECLCALLBACKMEMBER(void, pfnDestroy, (PCDBGCIO pIo));

    /**
     * Wait for input available for reading.
     *
     * @returns Flag whether there is input ready upon return.
     * @retval  true if there is input ready.
     * @retval  false if there not input ready.
     * @param   pIo         Pointer to the I/O structure supplied by
     *                      the I/O provider. The backend can use this to find it's instance data.
     * @param   cMillies    Number of milliseconds to wait on input data.
     */
    DECLCALLBACKMEMBER(bool, pfnInput, (PCDBGCIO pIo, uint32_t cMillies));

    /**
     * Read input.
     *
     * @returns VBox status code.
     * @param   pIo         Pointer to the I/O structure supplied by
     *                      the I/O provider. The backend can use this to find it's instance data.
     * @param   pvBuf       Where to put the bytes we read.
     * @param   cbBuf       Maximum nymber of bytes to read.
     * @param   pcbRead     Where to store the number of bytes actually read.
     *                      If NULL the entire buffer must be filled for a
     *                      successful return.
     */
    DECLCALLBACKMEMBER(int, pfnRead, (PCDBGCIO pIo, void *pvBuf, size_t cbBuf, size_t *pcbRead));

    /**
     * Write (output).
     *
     * @returns VBox status code.
     * @param   pIo         Pointer to the I/O structure supplied by
     *                      the I/O provider. The backend can use this to find it's instance data.
     * @param   pvBuf       What to write.
     * @param   cbBuf       Number of bytes to write.
     * @param   pcbWritten  Where to store the number of bytes actually written.
     *                      If NULL the entire buffer must be successfully written.
     */
    DECLCALLBACKMEMBER(int, pfnWrite, (PCDBGCIO pIo, const void *pvBuf, size_t cbBuf, size_t *pcbWritten));

    /**
     * Marks the beginning of a new packet being sent - optional.
     *
     * @returns VBox status code.
     * @param   pIo         Pointer to the I/O structure supplied by
     *                      the I/O provider. The backend can use this to find it's instance data.
     * @param   cbPktHint   Size of the packet in bytes, serves as a hint for the I/O provider to arrange buffers.
     *                      Give 0 if size is unknown upfront.
     */
    DECLCALLBACKMEMBER(int, pfnPktBegin, (PCDBGCIO pIo, size_t cbPktHint));

    /**
     * Marks the end of the packet - optional.
     *
     * @returns VBox status code.
     * @param   pIo         Pointer to the I/O structure supplied by
     *                      the I/O provider. The backend can use this to find it's instance data.
     *
     * @note Some I/O providers might decide to send data only when this is called not in the
     *       DBGCIO::pfnWrite callback.
     */
    DECLCALLBACKMEMBER(int, pfnPktEnd, (PCDBGCIO pIo));

    /**
     * Ready / busy notification.
     *
     * @param   pIo         Pointer to the I/O structure supplied by
     *                      the I/O provider. The backend can use this to find it's instance data.
     * @param   fReady      Whether it's ready (true) or busy (false).
     */
    DECLCALLBACKMEMBER(void, pfnSetReady, (PCDBGCIO pIo, bool fReady));

} DBGCIO;
/** Pointer to an I/O callback table. */
typedef DBGCIO *PDBGCIO;


DBGDECL(int)    DBGCCreate(PUVM pUVM, PCDBGCIO pIo, unsigned fFlags);
DBGDECL(int)    DBGCRegisterCommands(PCDBGCCMD paCommands, unsigned cCommands);
DBGDECL(int)    DBGCDeregisterCommands(PCDBGCCMD paCommands, unsigned cCommands);

DBGDECL(int)    DBGCIoCreate(PUVM pUVM, void **ppvData);
DBGDECL(int)    DBGCIoTerminate(PUVM pUVM, void *pvData);

/** @} */

#endif /* IN_RING3 */

/** @} */
RT_C_DECLS_END

#endif /* !VBOX_INCLUDED_dbg_h */

/** @file
 * DBGF - Debugger Facility, Guest execution flow tracing.
 */

/*
 * Copyright (C) 2020-2023 Oracle and/or its affiliates.
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

#ifndef VBOX_INCLUDED_vmm_dbgfflowtrace_h
#define VBOX_INCLUDED_vmm_dbgfflowtrace_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <VBox/types.h>
#include <VBox/vmm/dbgf.h>

RT_C_DECLS_BEGIN
/** @defgroup grp_dbgf_flowtrace  Guest Execution Flow Tracing
 * @ingroup grp_dbgf
 *
 * @{
 */

/** A DBGF flow trace module handle. */
typedef struct DBGFFLOWTRACEMODINT *DBGFFLOWTRACEMOD;
/** Pointer to a DBGF flow trace module handle. */
typedef DBGFFLOWTRACEMOD *PDBGFFLOWTRACEMOD;
/** A DBGF flow trace probe handle. */
typedef struct DBGFFLOWTRACEPROBEINT *DBGFFLOWTRACEPROBE;
/** Pointer to a DBGF flow trace state probe handle. */
typedef DBGFFLOWTRACEPROBE *PDBGFFLOWTRACEPROBE;
/** A DBGF flow trace report handle. */
typedef struct DBGFFLOWTRACEREPORTINT *DBGFFLOWTRACEREPORT;
/** Pointer to a DBGF flow trace report handle. */
typedef DBGFFLOWTRACEREPORT *PDBGFFLOWTRACEREPORT;
/** A DBGF flow trace record handle. */
typedef struct DBGFFLOWTRACERECORDINT *DBGFFLOWTRACERECORD;
/** Pointer to a DBGF flow trace record handle. */
typedef DBGFFLOWTRACERECORD *PDBGFFLOWTRACERECORD;


/** Pointer to a flow trace probe entry. */
typedef struct DBGFFLOWTRACEPROBEENTRY *PDBGFFLOWTRACEPROBEENTRY;
/** Pointer to a const flow trace probe entry. */
typedef const struct DBGFFLOWTRACEPROBEENTRY *PCDBGFFLOWTRACEPROBEENTRY;

/** @name Flags controlling the type of the addition of a single probe.
 * @{ */
/** Default options. */
#define DBGF_FLOW_TRACE_PROBE_ADD_F_DEFAULT                       DBGF_FLOW_TRACE_PROBE_ADD_F_BEFORE_EXEC
/** Collects the data specified by the data probe before the instruction is executed. */
#define DBGF_FLOW_TRACE_PROBE_ADD_F_BEFORE_EXEC                   RT_BIT_32(0)
/** Collects the data specified by the data probe after the instruction was executed. */
#define DBGF_FLOW_TRACE_PROBE_ADD_F_AFTER_EXEC                    RT_BIT_32(1)
/** Mask of all valid flags. */
#define DBGF_FLOW_TRACE_PROBE_ADD_F_VALID_MASK                    (  DBGF_FLOW_TRACE_PROBE_ADD_F_BEFORE_EXEC \
                                                                   | DBGF_FLOW_TRACE_PROBE_ADD_F_AFTER_EXEC)
/** @} */


/**
 * Probe entry type.
 */
typedef enum DBGFFLOWTRACEPROBEENTRYTYPE
{
    /** Invalid type. */
    DBGFFLOWTRACEPROBEENTRYTYPE_INVALID = 0,
    /** Register. */
    DBGFFLOWTRACEPROBEENTRYTYPE_REG,
    /** Constant memory buffer pointer. */
    DBGFFLOWTRACEPROBEENTRYTYPE_CONST_MEM,
    /** Indirect memory buffer pointer, obtained from the base and index register
     * and a constant scale. */
    DBGFFLOWTRACEPROBEENTRYTYPE_INDIRECT_MEM,
    /** Callback. */
    DBGFFLOWTRACEPROBEENTRYTYPE_CALLBACK,
    /** Halt in the debugger when the entry is collected. */
    DBGFFLOWTRACEPROBEENTRYTYPE_DEBUGGER,
    /** 32bit hack. */
    DBGFFLOWTRACEPROBEENTRYTYPE_32BIT_HACK = 0x7fffffff
} DBGFFLOWTRACEPROBEENTRYTYPE;


/**
 * Register descriptor for a probe entry.
 */
typedef struct DBGFFLOWTRACEPROBEENTRYREG
{
    /** The register name - see DBGFR3RegNm*. */
    const char     *pszName;
    /** The size of the value in bytes. */
    DBGFREGVALTYPE  enmType;
} DBGFFLOWTRACEPROBEENTRYREG;
/** Pointer to data probe register entry. */
typedef DBGFFLOWTRACEPROBEENTRYREG *PDBGFFLOWTRACEPROBEENTRYREG;
/** Pointer to a const probe register entry. */
typedef const DBGFFLOWTRACEPROBEENTRYREG *PCDBGFFLOWTRACEPROBEENTRYREG;


/**
 * Flow trace probe callback.
 *
 * @returns VBox status code, any error aborts continuing fetching the data for the
 *          probe containing this callback.
 * @param   pUVM             The usermode VM handle.
 * @param   idCpu            The ID of the vCPU the probe as fired.
 * @param   hFlowTraceMod    The handle to the flow trace module the probe was fired for.
 * @param   pAddrProbe       The guest address the probe was fired at.
 * @param   hFlowTraceProbe  The flow trace probe handle.this callback is in.
 * @param   pProbeEntry      The probe entry this callback is part of.
 * @param   pvUser           The opaque user data for the callback.
 */
typedef DECLCALLBACKTYPE(int, FNDBGFFLOWTRACEPROBECALLBACK, (PUVM pUVM, VMCPUID idCpu, DBGFFLOWTRACEMOD hFlowTraceMod,
                                                             PCDBGFADDRESS pAddrProbe, DBGFFLOWTRACEPROBE hFlowTraceProbe,
                                                             PCDBGFFLOWTRACEPROBEENTRY pProbeEntry,
                                                             void *pvUser));
/** Pointer to a flow trace probe callback. */
typedef FNDBGFFLOWTRACEPROBECALLBACK *PFNDBGFFLOWTRACEPROBECALLBACK;


/**
 * Trace flow probe entry.
 */
typedef struct DBGFFLOWTRACEPROBEENTRY
{
    /** Entry type. */
    DBGFFLOWTRACEPROBEENTRYTYPE           enmType;
    /** Description for this entry, optional. */
    const char                            *pszDesc;
    /** The data based on the entry type. */
    union
    {
        /** Register. */
        DBGFFLOWTRACEPROBEENTRYREG        Reg;
        /** Constant memory pointer. */
        struct
        {
            /** The address of the memory buffer. */
            DBGFADDRESS                   AddrMem;
            /** Number of bytes to log. */
            size_t                        cbMem;
        } ConstMem;
        /** Indirect memory */
        struct
        {
            /** The base register. */
            DBGFFLOWTRACEPROBEENTRYREG    RegBase;
            /** The index register. */
            DBGFFLOWTRACEPROBEENTRYREG    RegIndex;
            /** The scale to apply to the index. */
            uint8_t                       uScale;
            /** A constant offset which is applied at the end. */
            RTGCINTPTR                    iOffset;
            /** Number of bytes to log. */
            size_t                        cbMem;
        } IndirectMem;
        /** Callback. */
        struct
        {
            /** The callback to call. */
            PFNDBGFFLOWTRACEPROBECALLBACK pfnCallback;
            /** The opaque user data to provide. */
            void                          *pvUser;
        } Callback;
    } Type;
} DBGFFLOWTRACEPROBEENTRY;


/**
 * Flow trace probe value.
 */
typedef struct DBGFFLOWTRACEPROBEVAL
{
    /** Pointer to the flow trace probe entry this value is for. */
    PCDBGFFLOWTRACEPROBEENTRY             pProbeEntry;
    /** Data based on the type in the entry. */
    union
    {
        /** Register value. */
        DBGFREGENTRYNM                    Reg;
        /** Memory value (constant pointer or indirect). */
        struct
        {
            /** The guest address logged. */
            DBGFADDRESS                   Addr;
            /** Pointer to the data logged. */
            const void                   *pvBuf;
            /** Number of bytes logged. */
            size_t                        cbBuf;
        } Mem;
    } Type;
} DBGFFLOWTRACEPROBEVAL;
/** Pointer to a flow trace probe value. */
typedef DBGFFLOWTRACEPROBEVAL *PDBGFFLOWTRACEPROBEVAL;
/** Pointer to a const flow trace probe value. */
typedef const DBGFFLOWTRACEPROBEVAL *PCDBGFFLOWTRACEPROBEVAL;

/**
 * Flow trace report filter operation.
 */
typedef enum DBGFFLOWTRACEREPORTFILTEROP
{
    /** Invalid filter operation. */
    DBGFFLOWTRACEREPORTFILTEROP_INVALID = 0,
    /** All filters must match with the record. */
    DBGFFLOWTRACEREPORTFILTEROP_AND,
    /** Only one filter must match with the record. */
    DBGFFLOWTRACEREPORTFILTEROP_OR,
    /** 32bit hack. */
    DBGFFLOWTRACEREPORTFILTEROP_32BIT_HACK = 0x7fffffff
} DBGFFLOWTRACEREPORTFILTEROP;


/**
 * Flow trace report filter type.
 */
typedef enum DBGFFLOWTRACEREPORTFILTERTYPE
{
    /** Invalid filter type. */
    DBGFFLOWTRACEREPORTFILTERTYPE_INVALID = 0,
    /** Filter by sequence number. */
    DBGFFLOWTRACEREPORTFILTERTYPE_SEQ_NUM,
    /** Filter by timestamp. */
    DBGFFLOWTRACEREPORTFILTERTYPE_TIMESTAMP,
    /** Filter by probe address. */
    DBGFFLOWTRACEREPORTFILTERTYPE_ADDR,
    /** Filter by CPU ID. */
    DBGFFLOWTRACEREPORTFILTERTYPE_VMCPU_ID,
    /** Filter by specific probe data. */
    DBGFFLOWTRACEREPORTFILTERTYPE_PROBE_DATA,
    /** 32bit hack. */
    DBGFFLOWTRACEREPORTFILTERTYPE_32BIT_HACK = 0x7fffffff
} DBGFFLOWTRACEREPORTFILTERTYPE;


/**
 * Flow trace report filter.
 */
typedef struct DBGFFLOWTRACEREPORTFILTER
{
    /** Filter type. */
    DBGFFLOWTRACEREPORTFILTERTYPE         enmType;
    /** Filter data, type dependent. */
    struct
    {
        /** Sequence number filtering. */
        struct
        {
            /** Sequence number filtering, start value. */
            uint64_t                      u64SeqNoFirst;
            /** Sequence number filtering, last value. */
            uint64_t                      u64SeqNoLast;
        } SeqNo;
        /** Timestamp filtering. */
        struct
        {
            /** Start value. */
            uint64_t                      u64TsFirst;
            /** Last value. */
            uint64_t                      u64TsLast;
        } Timestamp;
        /** Probe address filtering. */
        struct
        {
            /** Start address. */
            DBGFADDRESS                   AddrStart;
            /** Last address. */
            DBGFADDRESS                   AddrLast;
        } Addr;
        /** vCPU id filtering. */
        struct
        {
            /** Start CPU id. */
            VMCPUID                       idCpuStart;
            /** Last CPU id. */
            VMCPUID                       idCpuLast;
        } VCpuId;
        /** Probe data filtering. */
        struct
        {
            /** Pointer to the probe value array. */
            PCDBGFFLOWTRACEPROBEVAL       paVal;
            /** Number of entries in the array for filtering. */
            uint32_t                      cVals;
            /** Flag whether to look into the common values or the probe specific ones. */
            bool                          fValCmn;
        } ProbeData;
    } Type;
} DBGFFLOWTRACEREPORTFILTER;
/** Pointer to a flow trace report filter. */
typedef DBGFFLOWTRACEREPORTFILTER *PDBGFFLOWTRACEREPORTFILTER;


/** @name Flags controlling filtering records.
 * @{ */
/** Add records which don't match the filter. */
#define DBGF_FLOW_TRACE_REPORT_FILTER_F_REVERSE                   RT_BIT_32(0)
/** Mask of all valid flags. */
#define DBGF_FLOW_TRACE_REPORT_FILTER_F_VALID                     (DBGF_FLOW_TRACE_REPORT_FILTER_F_REVERSE)
/** @} */


/**
 * Flow trace report enumeration callback.
 *
 * @returns VBox status code, any non VINF_SUCCESS code aborts the enumeration and is returned
 *          by DBGFR3FlowTraceReportEnumRecords().
 * @param   hFlowTraceReport The flow trace report handle being enumerated.
 * @param   hFlowTraceRecord The flow trace record handle.
 * @param   pvUser           Opaque user data given in DBGFR3FlowTraceReportEnumRecords().
 */
typedef DECLCALLBACKTYPE(int, FNDBGFFLOWTRACEREPORTENUMCLBK,(DBGFFLOWTRACEREPORT hFlowTraceReport,
                                                             DBGFFLOWTRACERECORD hFlowTraceRecord,
                                                             void *pvUser));
/** Pointer to a record enumeration callback. */
typedef FNDBGFFLOWTRACEREPORTENUMCLBK *PFNDBGFFLOWTRACEREPORTENUMCLBK;


VMMR3DECL(int)      DBGFR3FlowTraceModCreate(PUVM pUVM, VMCPUID idCpu,
                                             DBGFFLOWTRACEPROBE hFlowTraceProbeCommon,
                                             PDBGFFLOWTRACEMOD phFlowTraceMod);
VMMR3DECL(int)      DBGFR3FlowTraceModCreateFromFlowGraph(PUVM pUVM, VMCPUID idCpu, DBGFFLOW hFlow,
                                                          DBGFFLOWTRACEPROBE hFlowTraceProbeCommon,
                                                          DBGFFLOWTRACEPROBE hFlowTraceProbeEntry,
                                                          DBGFFLOWTRACEPROBE hFlowTraceProbeRegular,
                                                          DBGFFLOWTRACEPROBE hFlowTraceProbeExit,
                                                          PDBGFFLOWTRACEMOD phFlowTraceMod);
VMMR3DECL(uint32_t) DBGFR3FlowTraceModRetain(DBGFFLOWTRACEMOD hFlowTraceMod);
VMMR3DECL(uint32_t) DBGFR3FlowTraceModRelease(DBGFFLOWTRACEMOD hFlowTraceMod);
VMMR3DECL(int)      DBGFR3FlowTraceModEnable(DBGFFLOWTRACEMOD hFlowTraceMod, uint32_t cHits, uint32_t cRecordsMax);
VMMR3DECL(int)      DBGFR3FlowTraceModDisable(DBGFFLOWTRACEMOD hFlowTraceMod);
VMMR3DECL(int)      DBGFR3FlowTraceModQueryReport(DBGFFLOWTRACEMOD hFlowTraceMod,
                                                  PDBGFFLOWTRACEREPORT phFlowTraceReport);
VMMR3DECL(int)      DBGFR3FlowTraceModClear(DBGFFLOWTRACEMOD hFlowTraceMod);
VMMR3DECL(int)      DBGFR3FlowTraceModAddProbe(DBGFFLOWTRACEMOD hFlowTraceMod, PCDBGFADDRESS pAddrProbe,
                                               DBGFFLOWTRACEPROBE hFlowTraceProbe, uint32_t fFlags);

VMMR3DECL(int)      DBGFR3FlowTraceProbeCreate(PUVM pUVM, const char *pszDescr, PDBGFFLOWTRACEPROBE phFlowTraceProbe);
VMMR3DECL(uint32_t) DBGFR3FlowTraceProbeRetain(DBGFFLOWTRACEPROBE hFlowTraceProbe);
VMMR3DECL(uint32_t) DBGFR3FlowTraceProbeRelease(DBGFFLOWTRACEPROBE hFlowTraceProbe);
VMMR3DECL(int)      DBGFR3FlowTraceProbeEntriesAdd(DBGFFLOWTRACEPROBE hFlowTraceProbe,
                                                   PCDBGFFLOWTRACEPROBEENTRY paEntries, uint32_t cEntries);

VMMR3DECL(uint32_t) DBGFR3FlowTraceReportRetain(DBGFFLOWTRACEREPORT hFlowTraceReport);
VMMR3DECL(uint32_t) DBGFR3FlowTraceReportRelease(DBGFFLOWTRACEREPORT hFlowTraceReport);
VMMR3DECL(uint32_t) DBGFR3FlowTraceReportGetRecordCount(DBGFFLOWTRACEREPORT hFlowTraceReport);
VMMR3DECL(int)      DBGFR3FlowTraceReportQueryRecord(DBGFFLOWTRACEREPORT hFlowTraceReport, uint32_t idxRec, PDBGFFLOWTRACERECORD phFlowTraceRec);
VMMR3DECL(int)      DBGFR3FlowTraceReportQueryFiltered(DBGFFLOWTRACEREPORT hFlowTraceReport, uint32_t fFlags,
                                                       PDBGFFLOWTRACEREPORTFILTER paFilters, uint32_t cFilters,
                                                       DBGFFLOWTRACEREPORTFILTEROP enmOp,
                                                       PDBGFFLOWTRACEREPORT phFlowTraceReportFiltered);
VMMR3DECL(int)      DBGFR3FlowTraceReportEnumRecords(DBGFFLOWTRACEREPORT hFlowTraceReport,
                                                     PFNDBGFFLOWTRACEREPORTENUMCLBK pfnEnum,
                                                     void *pvUser);

VMMR3DECL(uint32_t)                DBGFR3FlowTraceRecordRetain(DBGFFLOWTRACERECORD hFlowTraceRecord);
VMMR3DECL(uint32_t)                DBGFR3FlowTraceRecordRelease(DBGFFLOWTRACERECORD hFlowTraceRecord);
VMMR3DECL(uint64_t)                DBGFR3FlowTraceRecordGetSeqNo(DBGFFLOWTRACERECORD hFlowTraceRecord);
VMMR3DECL(uint64_t)                DBGFR3FlowTraceRecordGetTimestamp(DBGFFLOWTRACERECORD hFlowTraceRecord);
VMMR3DECL(PDBGFADDRESS)            DBGFR3FlowTraceRecordGetAddr(DBGFFLOWTRACERECORD hFlowTraceRecord, PDBGFADDRESS pAddr);
VMMR3DECL(DBGFFLOWTRACEPROBE)      DBGFR3FlowTraceRecordGetProbe(DBGFFLOWTRACERECORD hFlowTraceRecord);
VMMR3DECL(uint32_t)                DBGFR3FlowTraceRecordGetValCount(DBGFFLOWTRACERECORD hFlowTraceRecord);
VMMR3DECL(uint32_t)                DBGFR3FlowTraceRecordGetValCommonCount(DBGFFLOWTRACERECORD hFlowTraceRecord);
VMMR3DECL(PCDBGFFLOWTRACEPROBEVAL) DBGFR3FlowTraceRecordGetVals(DBGFFLOWTRACERECORD hFlowTraceRecord);
VMMR3DECL(PCDBGFFLOWTRACEPROBEVAL) DBGFR3FlowTraceRecordGetValsCommon(DBGFFLOWTRACERECORD hFlowTraceRecord);
VMMR3DECL(VMCPUID)                 DBGFR3FlowTraceRecordGetCpuId(DBGFFLOWTRACERECORD hFlowTraceRecord);

/** @} */
RT_C_DECLS_END

#endif /* !VBOX_INCLUDED_vmm_dbgfflowtrace_h */


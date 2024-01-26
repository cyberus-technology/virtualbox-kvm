/** @file
 * X86 (and AMD64) Local APIC registers (VMM,++).
 *
 * apic.mac is generated from this file by running 'kmk incs' in the root.
 */

/*
 * Copyright (C) 2010-2023 Oracle and/or its affiliates.
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

#ifndef VBOX_INCLUDED_apic_h
#define VBOX_INCLUDED_apic_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <iprt/types.h>
#include <iprt/x86.h>

/** @todo These are defines used by CPUM and perhaps some assembly code. Remove
 *        these and use the XAPIC counterpart defines below later. */
#define APIC_REG_VERSION                        0x0030
#define APIC_REG_VERSION_GET_VER(u32)           (u32 & 0xff)
#define APIC_REG_VERSION_GET_MAX_LVT(u32)       ((u32 & 0xff0000) >> 16)

/* Defines according to Figure 10-8 of the Intel Software Developers Manual Vol 3A */
#define APIC_REG_LVT_LINT0                      0x0350
#define APIC_REG_LVT_LINT1                      0x0360
#define APIC_REG_LVT_ERR                        0x0370
#define APIC_REG_LVT_PC                         0x0340
#define APIC_REG_LVT_THMR                       0x0330
#define APIC_REG_LVT_CMCI                       0x02F0
#define APIC_REG_EILVT0                         0x0500
#define APIC_REG_EILVT1                         0x0510
#define APIC_REG_EILVT2                         0x0520
#define APIC_REG_EILVT3                         0x0530
#define APIC_REG_LVT_MODE_MASK                  (RT_BIT(8) | RT_BIT(9) | RT_BIT(10))
#define APIC_REG_LVT_MODE_FIXED                 0
#define APIC_REG_LVT_MODE_NMI                   RT_BIT(10)
#define APIC_REG_LVT_MODE_EXTINT                (RT_BIT(8) | RT_BIT(9) | RT_BIT(10))
#define APIC_REG_LVT_PIN_POLARIY                RT_BIT(13)
#define APIC_REG_LVT_REMOTE_IRR                 RT_BIT(14)
#define APIC_REG_LVT_LEVEL_TRIGGER              RT_BIT(15)
#define APIC_REG_LVT_MASKED                     RT_BIT(16)

/** The APIC hardware version number for Pentium 4. */
#define XAPIC_HARDWARE_VERSION_P4            UINT8_C(0x14)
/** Maximum number of LVT entries for Pentium 4. */
#define XAPIC_MAX_LVT_ENTRIES_P4             UINT8_C(6)
/** Size of the APIC ID bits for Pentium 4. */
#define XAPIC_APIC_ID_BIT_COUNT_P4           UINT8_C(8)

/** The APIC hardware version number for Pentium 6. */
#define XAPIC_HARDWARE_VERSION_P6            UINT8_C(0x10)
/** Maximum number of LVT entries for Pentium 6. */
#define XAPIC_MAX_LVT_ENTRIES_P6             UINT8_C(4)
/** Size of the APIC ID bits for Pentium 6. */
#define XAPIC_APIC_ID_BIT_COUNT_P6           UINT8_C(4)

/** Illegal APIC vector value start. */
#define XAPIC_ILLEGAL_VECTOR_START           UINT8_C(0)
/** Illegal APIC vector value end (inclusive). */
#define XAPIC_ILLEGAL_VECTOR_END             UINT8_C(15)
/** Reserved APIC vector value start. */
#define XAPIC_RSVD_VECTOR_START              UINT8_C(16)
/** Reserved APIC vector value end (inclusive). */
#define XAPIC_RSVD_VECTOR_END                UINT8_C(31)

/** ESR - Send checksum error for Pentium 6. */
# define XAPIC_ESR_SEND_CHKSUM_ERROR_P6      RT_BIT(0)
/** ESR - Send accept error for Pentium 6. */
# define XAPIC_ESR_RECV_CHKSUM_ERROR_P6      RT_BIT(1)
/** ESR - Send accept error for Pentium 6. */
# define XAPIC_ESR_SEND_ACCEPT_ERROR_P6      RT_BIT(2)
/** ESR - Receive accept error for Pentium 6. */
# define XAPIC_ESR_RECV_ACCEPT_ERROR_P6      RT_BIT(3)

/** ESR - Redirectable IPI. */
#define XAPIC_ESR_REDIRECTABLE_IPI           RT_BIT(4)
/** ESR - Send accept error. */
#define XAPIC_ESR_SEND_ILLEGAL_VECTOR        RT_BIT(5)
/** ESR - Send accept error. */
#define XAPIC_ESR_RECV_ILLEGAL_VECTOR        RT_BIT(6)
/** ESR - Send accept error. */
#define XAPIC_ESR_ILLEGAL_REG_ADDRESS        RT_BIT(7)
/** ESR - Valid write-only bits. */
#define XAPIC_ESR_WO_VALID                   UINT32_C(0x0)

/** TPR - Valid bits. */
#define XAPIC_TPR_VALID                      UINT32_C(0xff)
/** TPR - Task-priority class. */
#define XAPIC_TPR_TP                         UINT32_C(0xf0)
/** TPR - Task-priority subclass. */
#define XAPIC_TPR_TP_SUBCLASS                UINT32_C(0x0f)
/** TPR - Gets the task-priority class. */
#define XAPIC_TPR_GET_TP(a_Tpr)              ((a_Tpr) & XAPIC_TPR_TP)
/** TPR - Gets the task-priority subclass. */
#define XAPIC_TPR_GET_TP_SUBCLASS(a_Tpr)     ((a_Tpr) & XAPIC_TPR_TP_SUBCLASS)

/** PPR - Valid bits. */
#define XAPIC_PPR_VALID                      UINT32_C(0xff)
/** PPR - Processor-priority class. */
#define XAPIC_PPR_PP                         UINT32_C(0xf0)
/** PPR - Processor-priority subclass. */
#define XAPIC_PPR_PP_SUBCLASS                UINT32_C(0x0f)
/** PPR - Get the processor-priority class. */
#define XAPIC_PPR_GET_PP(a_Ppr)              ((a_Ppr) & XAPIC_PPR_PP)
/** PPR - Get the processor-priority subclass. */
#define XAPIC_PPR_GET_PP_SUBCLASS(a_Ppr)     ((a_Ppr) & XAPIC_PPR_PP_SUBCLASS)

/** Timer mode - One-shot. */
#define XAPIC_TIMER_MODE_ONESHOT             UINT32_C(0)
/** Timer mode - Periodic. */
#define XAPIC_TIMER_MODE_PERIODIC            UINT32_C(1)
/** Timer mode - TSC deadline. */
#define XAPIC_TIMER_MODE_TSC_DEADLINE        UINT32_C(2)

/** LVT - The vector. */
#define XAPIC_LVT_VECTOR                     UINT32_C(0xff)
/** LVT - Gets the vector from an LVT entry. */
#define XAPIC_LVT_GET_VECTOR(a_Lvt)          ((a_Lvt) & XAPIC_LVT_VECTOR)
/** LVT - The mask. */
#define XAPIC_LVT_MASK                       RT_BIT(16)
/** LVT - Is the LVT masked? */
#define XAPIC_LVT_IS_MASKED(a_Lvt)           RT_BOOL((a_Lvt) & XAPIC_LVT_MASK)
/** LVT - Timer mode. */
#define XAPIC_LVT_TIMER_MODE                 RT_BIT(17)
/** LVT - Timer TSC-deadline timer mode. */
#define XAPIC_LVT_TIMER_TSCDEADLINE          RT_BIT(18)
/** LVT - Gets the timer mode. */
#define XAPIC_LVT_GET_TIMER_MODE(a_Lvt)      (XAPICTIMERMODE)(((a_Lvt) >> 17) & UINT32_C(3))
/** LVT - Delivery mode. */
#define XAPIC_LVT_DELIVERY_MODE              (RT_BIT(8) | RT_BIT(9) | RT_BIT(10))
/** LVT - Gets the delivery mode. */
#define XAPIC_LVT_GET_DELIVERY_MODE(a_Lvt)   (XAPICDELIVERYMODE)(((a_Lvt) >> 8) & UINT32_C(7))
/** LVT - Delivery status. */
#define XAPIC_LVT_DELIVERY_STATUS            RT_BIT(12)
/** LVT - Trigger mode. */
#define XAPIC_LVT_TRIGGER_MODE               RT_BIT(15)
/** LVT - Gets the trigger mode. */
#define XAPIC_LVT_GET_TRIGGER_MODE(a_Lvt)    (XAPICTRIGGERMODE)(((a_Lvt) >> 15) & UINT32_C(1))
/** LVT - Remote IRR. */
#define XAPIC_LVT_REMOTE_IRR                 RT_BIT(14)
/** LVT - Gets the Remote IRR. */
#define XAPIC_LVT_GET_REMOTE_IRR(a_Lvt)      (((a_Lvt) >> 14) & 1)
/** LVT - Interrupt Input Pin Polarity. */
#define XAPIC_LVT_POLARITY                   RT_BIT(13)
/** LVT - Gets the Interrupt Input Pin Polarity. */
#define XAPIC_LVT_GET_POLARITY(a_Lvt)        (((a_Lvt) >> 13) & 1)
/** LVT - Valid bits common to all LVTs. */
#define XAPIC_LVT_COMMON_VALID               (XAPIC_LVT_VECTOR | XAPIC_LVT_DELIVERY_STATUS | XAPIC_LVT_MASK)
/** LVT CMCI - Valid bits. */
#define XAPIC_LVT_CMCI_VALID                 (XAPIC_LVT_COMMON_VALID | XAPIC_LVT_DELIVERY_MODE)
/** LVT Timer - Valid bits. */
#define XAPIC_LVT_TIMER_VALID                (XAPIC_LVT_COMMON_VALID | XAPIC_LVT_TIMER_MODE | XAPIC_LVT_TIMER_TSCDEADLINE)
/** LVT Thermal - Valid bits. */
#define XAPIC_LVT_THERMAL_VALID              (XAPIC_LVT_COMMON_VALID | XAPIC_LVT_DELIVERY_MODE)
/** LVT Perf - Valid bits. */
#define XAPIC_LVT_PERF_VALID                 (XAPIC_LVT_COMMON_VALID | XAPIC_LVT_DELIVERY_MODE)
/** LVT LINTx - Valid bits. */
#define XAPIC_LVT_LINT_VALID                 (  XAPIC_LVT_COMMON_VALID | XAPIC_LVT_DELIVERY_MODE | XAPIC_LVT_DELIVERY_STATUS \
                                              | XAPIC_LVT_POLARITY | XAPIC_LVT_REMOTE_IRR | XAPIC_LVT_TRIGGER_MODE)
/** LVT Error - Valid bits. */
#define XAPIC_LVT_ERROR_VALID                (XAPIC_LVT_COMMON_VALID)

/** SVR - The vector. */
#define XAPIC_SVR_VECTOR                     UINT32_C(0xff)
/** SVR - APIC Software enable. */
#define XAPIC_SVR_SOFTWARE_ENABLE            RT_BIT(8)
/** SVR - Supress EOI broadcast. */
#define XAPIC_SVR_SUPRESS_EOI_BROADCAST      RT_BIT(12)
/** SVR - Valid bits for Pentium 4. */
# define XAPIC_SVR_VALID_P4                  (XAPIC_SVR_VECTOR | XAPIC_SVR_SOFTWARE_ENABLE)
/** @todo SVR - Valid bits for Pentium 6. */

/** DFR - Valid bits. */
#define XAPIC_DFR_VALID                      UINT32_C(0xf0000000)
/** DFR - Reserved bits that must always remain set. */
#define XAPIC_DFR_RSVD_MB1                   UINT32_C(0x0fffffff)
/** DFR - The model. */
#define XAPIC_DFR_MODEL                      UINT32_C(0xf)
/** DFR - Gets the destination model. */
#define XAPIC_DFR_GET_MODEL(a_uReg)          (((a_uReg) >> 28) & XAPIC_DFR_MODEL)

/** LDR - Valid bits. */
#define XAPIC_LDR_VALID                      UINT32_C(0xff000000)
/** LDR - Cluster ID mask (x2APIC). */
#define X2APIC_LDR_CLUSTER_ID                UINT32_C(0xffff0000)
/** LDR - Mask of the LDR cluster ID (x2APIC). */
#define X2APIC_LDR_GET_CLUSTER_ID(a_uReg)    ((a_uReg) & X2APIC_LDR_CLUSTER_ID)
/** LDR - Mask of the LDR logical ID (x2APIC). */
#define X2APIC_LDR_LOGICAL_ID                UINT32_C(0x0000ffff)

/** LDR - Flat mode logical ID mask. */
#define XAPIC_LDR_FLAT_LOGICAL_ID            UINT32_C(0xff)
/** LDR - Clustered mode cluster ID mask. */
#define XAPIC_LDR_CLUSTERED_CLUSTER_ID       UINT32_C(0xf0)
/** LDR - Clustered mode logical ID mask. */
#define XAPIC_LDR_CLUSTERED_LOGICAL_ID       UINT32_C(0x0f)
/** LDR - Gets the clustered mode cluster ID. */
#define XAPIC_LDR_CLUSTERED_GET_CLUSTER_ID(a_uReg)   ((a_uReg) & XAPIC_LDR_CLUSTERED_CLUSTER_ID)


/** EOI - Valid write-only bits. */
#define XAPIC_EOI_WO_VALID                   UINT32_C(0x0)
/** Timer ICR - Valid bits. */
#define XAPIC_TIMER_ICR_VALID                UINT32_C(0xffffffff)
/** Timer DCR - Valid bits. */
#define XAPIC_TIMER_DCR_VALID                (RT_BIT(0) | RT_BIT(1) | RT_BIT(3))

/** Self IPI - Valid bits. */
#define XAPIC_SELF_IPI_VALID                 UINT32_C(0xff)
/** Self IPI - The vector. */
#define XAPIC_SELF_IPI_VECTOR                UINT32_C(0xff)
/** Self IPI - Gets the vector. */
#define XAPIC_SELF_IPI_GET_VECTOR(a_uReg)    ((a_uReg) & XAPIC_SELF_IPI_VECTOR)

/** ICR Low - The Vector. */
#define XAPIC_ICR_LO_VECTOR                  UINT32_C(0xff)
/** ICR Low - Gets the vector. */
#define XAPIC_ICR_LO_GET_VECTOR(a_uIcr)      ((a_uIcr) & XAPIC_ICR_LO_VECTOR)
/** ICR Low - The delivery mode. */
#define XAPIC_ICR_LO_DELIVERY_MODE           (RT_BIT(8) | RT_BIT(9) | RT_BIT(10))
/** ICR Low - The destination mode. */
#define XAPIC_ICR_LO_DEST_MODE               RT_BIT(11)
/** ICR Low - The delivery status. */
#define XAPIC_ICR_LO_DELIVERY_STATUS         RT_BIT(12)
/** ICR Low - The level. */
#define XAPIC_ICR_LO_LEVEL                   RT_BIT(14)
/** ICR Low - The trigger mode. */
#define XAPIC_ICR_TRIGGER_MODE               RT_BIT(15)
/** ICR Low - The destination shorthand. */
#define XAPIC_ICR_LO_DEST_SHORTHAND          (RT_BIT(18) | RT_BIT(19))
/** ICR Low - Valid write bits. */
#define XAPIC_ICR_LO_WR_VALID                (  XAPIC_ICR_LO_VECTOR | XAPIC_ICR_LO_DELIVERY_MODE | XAPIC_ICR_LO_DEST_MODE \
                                              | XAPIC_ICR_LO_LEVEL | XAPIC_ICR_TRIGGER_MODE | XAPIC_ICR_LO_DEST_SHORTHAND)

/** ICR High - The destination field. */
#define XAPIC_ICR_HI_DEST                    UINT32_C(0xff000000)
/** ICR High - Get the destination field. */
#define XAPIC_ICR_HI_GET_DEST(a_u32IcrHi)    (((a_u32IcrHi) >> 24) & XAPIC_ICR_HI_DEST)
/** ICR High - Valid write bits in xAPIC mode. */
#define XAPIC_ICR_HI_WR_VALID                XAPIC_ICR_HI_DEST

/** APIC ID broadcast mask - x2APIC mode. */
#define X2APIC_ID_BROADCAST_MASK             UINT32_C(0xffffffff)
/** APIC ID broadcast mask - xAPIC mode for Pentium 4. */
# define XAPIC_ID_BROADCAST_MASK_P4          UINT32_C(0xff)
/** @todo Broadcast mask for Pentium 6. */

/** Get an xAPIC page offset for an x2APIC MSR value. */
#define X2APIC_GET_XAPIC_OFF(a_uMsr)         ((((a_uMsr) - MSR_IA32_X2APIC_START) << 4) & UINT32_C(0xff0))
/** Get an x2APIC MSR for an xAPIC page offset. */
#define XAPIC_GET_X2APIC_MSR(a_offReg)       ((((a_offReg) & UINT32_C(0xff0)) >> 4) | MSR_IA32_X2APIC_START)

/** @name xAPIC and x2APIC register offsets.
 * See Intel spec. 10.4.1 "The Local APIC Block Diagram".
 * @{ */
/** Offset of APIC ID Register. */
#define XAPIC_OFF_ID                         0x020
/** Offset of APIC Version Register. */
#define XAPIC_OFF_VERSION                    0x030
/** Offset of Task Priority Register. */
#define XAPIC_OFF_TPR                        0x080
/** Offset of Arbitrartion Priority register. */
#define XAPIC_OFF_APR                        0x090
/** Offset of Processor Priority register. */
#define XAPIC_OFF_PPR                        0x0A0
/** Offset of End Of Interrupt register. */
#define XAPIC_OFF_EOI                        0x0B0
/** Offset of Remote Read Register. */
#define XAPIC_OFF_RRD                        0x0C0
/** Offset of Logical Destination Register. */
#define XAPIC_OFF_LDR                        0x0D0
/** Offset of Destination Format Register. */
#define XAPIC_OFF_DFR                        0x0E0
/** Offset of Spurious Interrupt Vector Register. */
#define XAPIC_OFF_SVR                        0x0F0
/** Offset of In-service Register (bits 31:0). */
#define XAPIC_OFF_ISR0                       0x100
/** Offset of In-service Register (bits 63:32). */
#define XAPIC_OFF_ISR1                       0x110
/** Offset of In-service Register (bits 95:64). */
#define XAPIC_OFF_ISR2                       0x120
/** Offset of In-service Register (bits 127:96). */
#define XAPIC_OFF_ISR3                       0x130
/** Offset of In-service Register (bits 159:128). */
#define XAPIC_OFF_ISR4                       0x140
/** Offset of In-service Register (bits 191:160). */
#define XAPIC_OFF_ISR5                       0x150
/** Offset of In-service Register (bits 223:192). */
#define XAPIC_OFF_ISR6                       0x160
/** Offset of In-service Register (bits 255:224). */
#define XAPIC_OFF_ISR7                       0x170
/** Offset of Trigger Mode Register (bits 31:0). */
#define XAPIC_OFF_TMR0                       0x180
/** Offset of Trigger Mode Register (bits 63:32). */
#define XAPIC_OFF_TMR1                       0x190
/** Offset of Trigger Mode Register (bits 95:64). */
#define XAPIC_OFF_TMR2                       0x1A0
/** Offset of Trigger Mode Register (bits 127:96). */
#define XAPIC_OFF_TMR3                       0x1B0
/** Offset of Trigger Mode Register (bits 159:128). */
#define XAPIC_OFF_TMR4                       0x1C0
/** Offset of Trigger Mode Register (bits 191:160). */
#define XAPIC_OFF_TMR5                       0x1D0
/** Offset of Trigger Mode Register (bits 223:192). */
#define XAPIC_OFF_TMR6                       0x1E0
/** Offset of Trigger Mode Register (bits 255:224). */
#define XAPIC_OFF_TMR7                       0x1F0
/** Offset of Interrupt Request Register (bits 31:0). */
#define XAPIC_OFF_IRR0                       0x200
/** Offset of Interrupt Request Register (bits 63:32). */
#define XAPIC_OFF_IRR1                       0x210
/** Offset of Interrupt Request Register (bits 95:64). */
#define XAPIC_OFF_IRR2                       0x220
/** Offset of Interrupt Request Register (bits 127:96). */
#define XAPIC_OFF_IRR3                       0x230
/** Offset of Interrupt Request Register (bits 159:128). */
#define XAPIC_OFF_IRR4                       0x240
/** Offset of Interrupt Request Register (bits 191:160). */
#define XAPIC_OFF_IRR5                       0x250
/** Offset of Interrupt Request Register (bits 223:192). */
#define XAPIC_OFF_IRR6                       0x260
/** Offset of Interrupt Request Register (bits 255:224). */
#define XAPIC_OFF_IRR7                       0x270
/** Offset of Error Status Register. */
#define XAPIC_OFF_ESR                        0x280
/** Offset of LVT CMCI Register. */
#define XAPIC_OFF_LVT_CMCI                   0x2F0
/** Offset of Interrupt Command Register - Lo. */
#define XAPIC_OFF_ICR_LO                     0x300
/** Offset of Interrupt Command Register - Hi. */
#define XAPIC_OFF_ICR_HI                     0x310
/** Offset of LVT Timer Register. */
#define XAPIC_OFF_LVT_TIMER                  0x320
/** Offset of LVT Thermal Sensor Register. */
#define XAPIC_OFF_LVT_THERMAL                0x330
/** Offset of LVT Performance Counter Register. */
#define XAPIC_OFF_LVT_PERF                   0x340
/** Offset of LVT LINT0 Register. */
#define XAPIC_OFF_LVT_LINT0                  0x350
/** Offset of LVT LINT1 Register. */
#define XAPIC_OFF_LVT_LINT1                  0x360
/** Offset of LVT Error Register . */
#define XAPIC_OFF_LVT_ERROR                  0x370
/** Offset of Timer Initial Count Register. */
#define XAPIC_OFF_TIMER_ICR                  0x380
/** Offset of Timer Current Count Register. */
#define XAPIC_OFF_TIMER_CCR                  0x390
/** Offset of Timer Divide Configuration Register. */
#define XAPIC_OFF_TIMER_DCR                  0x3E0
/** Offset of Self-IPI Register (x2APIC only). */
#define X2APIC_OFF_SELF_IPI                  0x3F0

/** Offset of LVT range start. */
#define XAPIC_OFF_LVT_START                  XAPIC_OFF_LVT_TIMER
/** Offset of LVT range end (inclusive).  */
#define XAPIC_OFF_LVT_END                    XAPIC_OFF_LVT_ERROR
/** Offset of LVT extended range start. */
#define XAPIC_OFF_LVT_EXT_START              XAPIC_OFF_LVT_CMCI
/** Offset of LVT extended range end (inclusive). */
#define XAPIC_OFF_LVT_EXT_END                XAPIC_OFF_LVT_CMCI
/** Offset of the last register (incl. reserved) in the xAPIC/x2APIC range. */
#define XAPIC_OFF_END                        0x3F0
/** @} */

/** @name xAPIC Destination Format Register bits.
 * See Intel spec. 10.6.2.2 "Logical Destination Mode".
 * @{ */
typedef enum XAPICDESTFORMAT
{
    XAPICDESTFORMAT_FLAT    = 0xf,
    XAPICDESTFORMAT_CLUSTER = 0
} XAPICDESTFORMAT;
/** @} */

/** @name xAPIC Timer Mode bits.
 * See Intel spec. 10.5.1 "Local Vector Table".
 * @{ */
typedef enum XAPICTIMERMODE
{
    XAPICTIMERMODE_ONESHOT      = XAPIC_TIMER_MODE_ONESHOT,
    XAPICTIMERMODE_PERIODIC     = XAPIC_TIMER_MODE_PERIODIC,
    XAPICTIMERMODE_TSC_DEADLINE = XAPIC_TIMER_MODE_TSC_DEADLINE
} XAPICTIMERMODE;
/** @} */

/** @name xAPIC Interrupt Command Register bits.
 * See Intel spec. 10.6.1 "Interrupt Command Register (ICR)".
 * See Intel spec. 10.5.1 "Local Vector Table".
 * @{ */
/**
 * xAPIC destination shorthand.
 */
typedef enum XAPICDESTSHORTHAND
{
    XAPICDESTSHORTHAND_NONE = 0,
    XAPICDESTSHORTHAND_SELF,
    XAPIDDESTSHORTHAND_ALL_INCL_SELF,
    XAPICDESTSHORTHAND_ALL_EXCL_SELF
} XAPICDESTSHORTHAND;

/**
 * xAPIC INIT level de-assert delivery mode.
 */
typedef enum XAPICINITLEVEL
{
    XAPICINITLEVEL_DEASSERT = 0,
    XAPICINITLEVEL_ASSERT
} XAPICLEVEL;

/**
 * xAPIC destination mode.
 */
typedef enum XAPICDESTMODE
{
    XAPICDESTMODE_PHYSICAL = 0,
    XAPICDESTMODE_LOGICAL
} XAPICDESTMODE;

/**
 * xAPIC delivery mode type.
 */
typedef enum XAPICDELIVERYMODE
{
    XAPICDELIVERYMODE_FIXED               = 0,
    XAPICDELIVERYMODE_LOWEST_PRIO         = 1,
    XAPICDELIVERYMODE_SMI                 = 2,
    XAPICDELIVERYMODE_NMI                 = 4,
    XAPICDELIVERYMODE_INIT                = 5,
    XAPICDELIVERYMODE_STARTUP             = 6,
    XAPICDELIVERYMODE_EXTINT              = 7
} XAPICDELIVERYMODE;

/**
 * xAPIC trigger mode.
 */
typedef enum XAPICTRIGGERMODE
{
    XAPICTRIGGERMODE_EDGE = 0,
    XAPICTRIGGERMODE_LEVEL
} XAPICTRIGGERMODE;
/** @} */


DECLINLINE(uint32_t) ApicRegRead(void *pvBase, uint32_t offReg)
{
    return *(const volatile uint32_t *)((uintptr_t)pvBase + offReg);
}


#ifdef IPRT_INCLUDED_asm_amd64_x86_h
/**
 * Reads an X2APIC register.
 *
 * @param   offReg      MMIO offset, APIC_REG_XXX.
 */
DECLINLINE(uint32_t) ApicX2RegRead32(uint32_t offReg)
{
    return ASMRdMsr((offReg >> 4) + MSR_IA32_X2APIC_START);
}
#endif

#endif /* !VBOX_INCLUDED_apic_h */


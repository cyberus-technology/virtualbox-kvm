#!/usr/bin/env python
# -*- coding: utf-8 -*-
# $Id: IEMAllInstructionsPython.py $

"""
IEM instruction extractor.

This script/module parses the IEMAllInstruction*.cpp.h files next to it and
collects information about the instructions.  It can then be used to generate
disassembler tables and tests.
"""

__copyright__ = \
"""
Copyright (C) 2017-2023 Oracle and/or its affiliates.

This file is part of VirtualBox base platform packages, as
available from https://www.virtualbox.org.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation, in version 3 of the
License.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, see <https://www.gnu.org/licenses>.

The contents of this file may alternatively be used under the terms
of the Common Development and Distribution License Version 1.0
(CDDL), a copy of it is provided in the "COPYING.CDDL" file included
in the VirtualBox distribution, in which case the provisions of the
CDDL are applicable instead of those of the GPL.

You may elect to license modified versions of this file under the
terms and conditions of either the GPL or the CDDL or both.

SPDX-License-Identifier: GPL-3.0-only OR CDDL-1.0
"""
__version__ = "$Revision: 155244 $"

# pylint: disable=anomalous-backslash-in-string

# Standard python imports.
import os
import re
import sys

## Only the main script needs to modify the path.
#g_ksValidationKitDir = os.path.join(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))),
#                                    'ValidationKit');
#sys.path.append(g_ksValidationKitDir);
#
#from common import utils; - Windows build boxes doesn't have pywin32.

# Python 3 hacks:
if sys.version_info[0] >= 3:
    long = int;     # pylint: disable=redefined-builtin,invalid-name


g_kdX86EFlagsConstants = {
    'X86_EFL_CF':          0x00000001, # RT_BIT_32(0)
    'X86_EFL_1':           0x00000002, # RT_BIT_32(1)
    'X86_EFL_PF':          0x00000004, # RT_BIT_32(2)
    'X86_EFL_AF':          0x00000010, # RT_BIT_32(4)
    'X86_EFL_ZF':          0x00000040, # RT_BIT_32(6)
    'X86_EFL_SF':          0x00000080, # RT_BIT_32(7)
    'X86_EFL_TF':          0x00000100, # RT_BIT_32(8)
    'X86_EFL_IF':          0x00000200, # RT_BIT_32(9)
    'X86_EFL_DF':          0x00000400, # RT_BIT_32(10)
    'X86_EFL_OF':          0x00000800, # RT_BIT_32(11)
    'X86_EFL_IOPL':        0x00003000, # (RT_BIT_32(12) | RT_BIT_32(13))
    'X86_EFL_NT':          0x00004000, # RT_BIT_32(14)
    'X86_EFL_RF':          0x00010000, # RT_BIT_32(16)
    'X86_EFL_VM':          0x00020000, # RT_BIT_32(17)
    'X86_EFL_AC':          0x00040000, # RT_BIT_32(18)
    'X86_EFL_VIF':         0x00080000, # RT_BIT_32(19)
    'X86_EFL_VIP':         0x00100000, # RT_BIT_32(20)
    'X86_EFL_ID':          0x00200000, # RT_BIT_32(21)
    'X86_EFL_LIVE_MASK':   0x003f7fd5, # UINT32_C(0x003f7fd5)
    'X86_EFL_RA1_MASK':    0x00000002, # RT_BIT_32(1)
};

## EFlags values allowed in \@opfltest, \@opflmodify, \@opflundef, \@opflset, and \@opflclear.
g_kdEFlagsMnemonics = {
    # Debugger flag notation (sorted by value):
    'cf':   'X86_EFL_CF',   ##< Carry Flag.
    'nc':  '!X86_EFL_CF',   ##< No Carry.

    'po':   'X86_EFL_PF',   ##< Parity Pdd.
    'pe':  '!X86_EFL_PF',   ##< Parity Even.

    'af':   'X86_EFL_AF',   ##< Aux Flag.
    'na':  '!X86_EFL_AF',   ##< No Aux.

    'zr':   'X86_EFL_ZF',   ##< ZeRo.
    'nz':  '!X86_EFL_ZF',   ##< No Zero.

    'ng':   'X86_EFL_SF',   ##< NeGative (sign).
    'pl':  '!X86_EFL_SF',   ##< PLuss (sign).

    'tf':   'X86_EFL_TF',   ##< Trap flag.

    'ei':   'X86_EFL_IF',   ##< Enabled Interrupts.
    'di':  '!X86_EFL_IF',   ##< Disabled Interrupts.

    'dn':   'X86_EFL_DF',   ##< DowN (string op direction).
    'up':  '!X86_EFL_DF',   ##< UP (string op direction).

    'ov':   'X86_EFL_OF',   ##< OVerflow.
    'nv':  '!X86_EFL_OF',   ##< No Overflow.

    'nt':   'X86_EFL_NT',   ##< Nested Task.
    'rf':   'X86_EFL_RF',   ##< Resume Flag.
    'vm':   'X86_EFL_VM',   ##< Virtual-8086 Mode.
    'ac':   'X86_EFL_AC',   ##< Alignment Check.
    'vif':  'X86_EFL_VIF',  ##< Virtual Interrupt Flag.
    'vip':  'X86_EFL_VIP',  ##< Virtual Interrupt Pending.

    # Reference manual notation not covered above (sorted by value):
    'pf':   'X86_EFL_PF',
    'zf':   'X86_EFL_ZF',
    'sf':   'X86_EFL_SF',
    'if':   'X86_EFL_IF',
    'df':   'X86_EFL_DF',
    'of':   'X86_EFL_OF',
    'iopl': 'X86_EFL_IOPL',
    'id':   'X86_EFL_ID',
};

## Constants and values for CR0.
g_kdX86Cr0Constants = {
    'X86_CR0_PE':           0x00000001, # RT_BIT_32(0)
    'X86_CR0_MP':           0x00000002, # RT_BIT_32(1)
    'X86_CR0_EM':           0x00000004, # RT_BIT_32(2)
    'X86_CR0_TS':           0x00000008, # RT_BIT_32(3)
    'X86_CR0_ET':           0x00000010, # RT_BIT_32(4)
    'X86_CR0_NE':           0x00000020, # RT_BIT_32(5)
    'X86_CR0_WP':           0x00010000, # RT_BIT_32(16)
    'X86_CR0_AM':           0x00040000, # RT_BIT_32(18)
    'X86_CR0_NW':           0x20000000, # RT_BIT_32(29)
    'X86_CR0_CD':           0x40000000, # RT_BIT_32(30)
    'X86_CR0_PG':           0x80000000, # RT_BIT_32(31)
};

## Constants and values for CR4.
g_kdX86Cr4Constants = {
    'X86_CR4_VME':          0x00000001, # RT_BIT_32(0)
    'X86_CR4_PVI':          0x00000002, # RT_BIT_32(1)
    'X86_CR4_TSD':          0x00000004, # RT_BIT_32(2)
    'X86_CR4_DE':           0x00000008, # RT_BIT_32(3)
    'X86_CR4_PSE':          0x00000010, # RT_BIT_32(4)
    'X86_CR4_PAE':          0x00000020, # RT_BIT_32(5)
    'X86_CR4_MCE':          0x00000040, # RT_BIT_32(6)
    'X86_CR4_PGE':          0x00000080, # RT_BIT_32(7)
    'X86_CR4_PCE':          0x00000100, # RT_BIT_32(8)
    'X86_CR4_OSFXSR':       0x00000200, # RT_BIT_32(9)
    'X86_CR4_OSXMMEEXCPT':  0x00000400, # RT_BIT_32(10)
    'X86_CR4_VMXE':         0x00002000, # RT_BIT_32(13)
    'X86_CR4_SMXE':         0x00004000, # RT_BIT_32(14)
    'X86_CR4_PCIDE':        0x00020000, # RT_BIT_32(17)
    'X86_CR4_OSXSAVE':      0x00040000, # RT_BIT_32(18)
    'X86_CR4_SMEP':         0x00100000, # RT_BIT_32(20)
    'X86_CR4_SMAP':         0x00200000, # RT_BIT_32(21)
    'X86_CR4_PKE':          0x00400000, # RT_BIT_32(22)
};

## XSAVE components (XCR0).
g_kdX86XSaveCConstants = {
    'XSAVE_C_X87':          0x00000001,
    'XSAVE_C_SSE':          0x00000002,
    'XSAVE_C_YMM':          0x00000004,
    'XSAVE_C_BNDREGS':      0x00000008,
    'XSAVE_C_BNDCSR':       0x00000010,
    'XSAVE_C_OPMASK':       0x00000020,
    'XSAVE_C_ZMM_HI256':    0x00000040,
    'XSAVE_C_ZMM_16HI':     0x00000080,
    'XSAVE_C_PKRU':         0x00000200,
    'XSAVE_C_LWP':          0x4000000000000000,
    'XSAVE_C_X':            0x8000000000000000,
    'XSAVE_C_ALL_AVX':      0x000000c4, # For clearing all AVX bits.
    'XSAVE_C_ALL_AVX_SSE':  0x000000c6, # For clearing all AVX and SSE bits.
};


## \@op[1-4] locations
g_kdOpLocations = {
    'reg':      [], ## modrm.reg
    'rm':       [], ## modrm.rm
    'imm':      [], ## immediate instruction data
    'vvvv':     [], ## VEX.vvvv

    # fixed registers.
    'AL':       [],
    'rAX':      [],
    'rDX':      [],
    'rSI':      [],
    'rDI':      [],
    'rFLAGS':   [],
    'CS':       [],
    'DS':       [],
    'ES':       [],
    'FS':       [],
    'GS':       [],
    'SS':       [],
};

## \@op[1-4] types
##
## Value fields:
##    - 0: the normal IDX_ParseXXX handler (IDX_UseModRM == IDX_ParseModRM).
##    - 1: the location (g_kdOpLocations).
##    - 2: disassembler format string version of the type.
##    - 3: disassembler OP_PARAM_XXX (XXX only).
##    - 4: IEM form matching instruction.
##
## Note! See the A.2.1 in SDM vol 2 for the type names.
g_kdOpTypes = {
    # Fixed addresses
    'Ap':           ( 'IDX_ParseImmAddrF',  'imm',    '%Ap',  'Ap',      'FIXED', ),

    # ModR/M.rm
    'Eb':           ( 'IDX_UseModRM',       'rm',     '%Eb',  'Eb',      'RM',    ),
    'Ed':           ( 'IDX_UseModRM',       'rm',     '%Ed',  'Ed',      'RM',    ),
    'Ed_WO':        ( 'IDX_UseModRM',       'rm',     '%Ed',  'Ed',      'RM',    ),
    'Eq':           ( 'IDX_UseModRM',       'rm',     '%Eq',  'Eq',      'RM',    ),
    'Eq_WO':        ( 'IDX_UseModRM',       'rm',     '%Eq',  'Eq',      'RM',    ),
    'Ew':           ( 'IDX_UseModRM',       'rm',     '%Ew',  'Ew',      'RM',    ),
    'Ev':           ( 'IDX_UseModRM',       'rm',     '%Ev',  'Ev',      'RM',    ),
    'Ey':           ( 'IDX_UseModRM',       'rm',     '%Ey',  'Ey',      'RM',    ),
    'Qd':           ( 'IDX_UseModRM',       'rm',     '%Qd',  'Qd',      'RM',    ),
    'Qq':           ( 'IDX_UseModRM',       'rm',     '%Qq',  'Qq',      'RM',    ),
    'Qq_WO':        ( 'IDX_UseModRM',       'rm',     '%Qq',  'Qq',      'RM',    ),
    'Wss':          ( 'IDX_UseModRM',       'rm',     '%Wss', 'Wss',     'RM',    ),
    'Wss_WO':       ( 'IDX_UseModRM',       'rm',     '%Wss', 'Wss',     'RM',    ),
    'Wsd':          ( 'IDX_UseModRM',       'rm',     '%Wsd', 'Wsd',     'RM',    ),
    'Wsd_WO':       ( 'IDX_UseModRM',       'rm',     '%Wsd', 'Wsd',     'RM',    ),
    'Wps':          ( 'IDX_UseModRM',       'rm',     '%Wps', 'Wps',     'RM',    ),
    'Wps_WO':       ( 'IDX_UseModRM',       'rm',     '%Wps', 'Wps',     'RM',    ),
    'Wpd':          ( 'IDX_UseModRM',       'rm',     '%Wpd', 'Wpd',     'RM',    ),
    'Wpd_WO':       ( 'IDX_UseModRM',       'rm',     '%Wpd', 'Wpd',     'RM',    ),
    'Wdq':          ( 'IDX_UseModRM',       'rm',     '%Wdq', 'Wdq',     'RM',    ),
    'Wdq_WO':       ( 'IDX_UseModRM',       'rm',     '%Wdq', 'Wdq',     'RM',    ),
    'Wq':           ( 'IDX_UseModRM',       'rm',     '%Wq',  'Wq',      'RM',    ),
    'Wq_WO':        ( 'IDX_UseModRM',       'rm',     '%Wq',  'Wq',      'RM',    ),
    'WqZxReg_WO':   ( 'IDX_UseModRM',       'rm',     '%Wq',  'Wq',      'RM',    ),
    'Wx':           ( 'IDX_UseModRM',       'rm',     '%Wx',  'Wx',      'RM',    ),
    'Wx_WO':        ( 'IDX_UseModRM',       'rm',     '%Wx',  'Wx',      'RM',    ),

    # ModR/M.rm - register only.
    'Uq':           ( 'IDX_UseModRM',       'rm',     '%Uq',  'Uq',      'REG'    ),
    'UqHi':         ( 'IDX_UseModRM',       'rm',     '%Uq',  'UqHi',    'REG'    ),
    'Uss':          ( 'IDX_UseModRM',       'rm',     '%Uss', 'Uss',     'REG'    ),
    'Uss_WO':       ( 'IDX_UseModRM',       'rm',     '%Uss', 'Uss',     'REG'    ),
    'Usd':          ( 'IDX_UseModRM',       'rm',     '%Usd', 'Usd',     'REG'    ),
    'Usd_WO':       ( 'IDX_UseModRM',       'rm',     '%Usd', 'Usd',     'REG'    ),
    'Ux':           ( 'IDX_UseModRM',       'rm',     '%Ux',  'Ux',      'REG'    ),
    'Nq':           ( 'IDX_UseModRM',       'rm',     '%Qq',  'Nq',      'REG'    ),

    # ModR/M.rm - memory only.
    'Ma':           ( 'IDX_UseModRM',       'rm',     '%Ma',  'Ma',      'MEM',   ), ##< Only used by BOUND.
    'Mb_RO':        ( 'IDX_UseModRM',       'rm',     '%Mb',  'Mb',      'MEM',   ),
    'Md':           ( 'IDX_UseModRM',       'rm',     '%Md',  'Md',      'MEM',   ),
    'Md_RO':        ( 'IDX_UseModRM',       'rm',     '%Md',  'Md',      'MEM',   ),
    'Md_WO':        ( 'IDX_UseModRM',       'rm',     '%Md',  'Md',      'MEM',   ),
    'Mdq':          ( 'IDX_UseModRM',       'rm',     '%Mdq', 'Mdq',     'MEM',   ),
    'Mdq_WO':       ( 'IDX_UseModRM',       'rm',     '%Mdq', 'Mdq',     'MEM',   ),
    'Mq':           ( 'IDX_UseModRM',       'rm',     '%Mq',  'Mq',      'MEM',   ),
    'Mq_WO':        ( 'IDX_UseModRM',       'rm',     '%Mq',  'Mq',      'MEM',   ),
    'Mps_WO':       ( 'IDX_UseModRM',       'rm',     '%Mps', 'Mps',     'MEM',   ),
    'Mpd_WO':       ( 'IDX_UseModRM',       'rm',     '%Mpd', 'Mpd',     'MEM',   ),
    'Mx':           ( 'IDX_UseModRM',       'rm',     '%Mx',  'Mx',      'MEM',   ),
    'Mx_WO':        ( 'IDX_UseModRM',       'rm',     '%Mx',  'Mx',      'MEM',   ),
    'M_RO':         ( 'IDX_UseModRM',       'rm',     '%M',   'M',       'MEM',   ),
    'M_RW':         ( 'IDX_UseModRM',       'rm',     '%M',   'M',       'MEM',   ),

    # ModR/M.reg
    'Gb':           ( 'IDX_UseModRM',       'reg',    '%Gb',  'Gb',      '',      ),
    'Gw':           ( 'IDX_UseModRM',       'reg',    '%Gw',  'Gw',      '',      ),
    'Gd':           ( 'IDX_UseModRM',       'reg',    '%Gd',  'Gd',      '',      ),
    'Gv':           ( 'IDX_UseModRM',       'reg',    '%Gv',  'Gv',      '',      ),
    'Gv_RO':        ( 'IDX_UseModRM',       'reg',    '%Gv',  'Gv',      '',      ),
    'Gy':           ( 'IDX_UseModRM',       'reg',    '%Gy',  'Gy',      '',      ),
    'Pd':           ( 'IDX_UseModRM',       'reg',    '%Pd',  'Pd',      '',      ),
    'PdZx_WO':      ( 'IDX_UseModRM',       'reg',    '%Pd',  'PdZx',    '',      ),
    'Pq':           ( 'IDX_UseModRM',       'reg',    '%Pq',  'Pq',      '',      ),
    'Pq_WO':        ( 'IDX_UseModRM',       'reg',    '%Pq',  'Pq',      '',      ),
    'Vd':           ( 'IDX_UseModRM',       'reg',    '%Vd',  'Vd',      '',      ),
    'Vd_WO':        ( 'IDX_UseModRM',       'reg',    '%Vd',  'Vd',      '',      ),
    'VdZx_WO':      ( 'IDX_UseModRM',       'reg',    '%Vd',  'Vd',      '',      ),
    'Vdq':          ( 'IDX_UseModRM',       'reg',    '%Vdq', 'Vdq',     '',      ),
    'Vss':          ( 'IDX_UseModRM',       'reg',    '%Vss', 'Vss',     '',      ),
    'Vss_WO':       ( 'IDX_UseModRM',       'reg',    '%Vss', 'Vss',     '',      ),
    'VssZx_WO':     ( 'IDX_UseModRM',       'reg',    '%Vss', 'Vss',     '',      ),
    'Vsd':          ( 'IDX_UseModRM',       'reg',    '%Vsd', 'Vsd',     '',      ),
    'Vsd_WO':       ( 'IDX_UseModRM',       'reg',    '%Vsd', 'Vsd',     '',      ),
    'VsdZx_WO':     ( 'IDX_UseModRM',       'reg',    '%Vsd', 'Vsd',     '',      ),
    'Vps':          ( 'IDX_UseModRM',       'reg',    '%Vps', 'Vps',     '',      ),
    'Vps_WO':       ( 'IDX_UseModRM',       'reg',    '%Vps', 'Vps',     '',      ),
    'Vpd':          ( 'IDX_UseModRM',       'reg',    '%Vpd', 'Vpd',     '',      ),
    'Vpd_WO':       ( 'IDX_UseModRM',       'reg',    '%Vpd', 'Vpd',     '',      ),
    'Vq':           ( 'IDX_UseModRM',       'reg',    '%Vq',  'Vq',      '',      ),
    'Vq_WO':        ( 'IDX_UseModRM',       'reg',    '%Vq',  'Vq',      '',      ),
    'Vdq_WO':       ( 'IDX_UseModRM',       'reg',    '%Vdq', 'Vdq',     '',      ),
    'VqHi':         ( 'IDX_UseModRM',       'reg',    '%Vdq', 'VdqHi',   '',      ),
    'VqHi_WO':      ( 'IDX_UseModRM',       'reg',    '%Vdq', 'VdqHi',   '',      ),
    'VqZx_WO':      ( 'IDX_UseModRM',       'reg',    '%Vq',  'VqZx',    '',      ),
    'Vx':           ( 'IDX_UseModRM',       'reg',    '%Vx',  'Vx',      '',      ),
    'Vx_WO':        ( 'IDX_UseModRM',       'reg',    '%Vx',  'Vx',      '',      ),

    # VEX.vvvv
    'By':           ( 'IDX_UseModRM',       'vvvv',   '%By',  'By',      'V',     ),
    'Hps':          ( 'IDX_UseModRM',       'vvvv',   '%Hps', 'Hps',     'V',     ),
    'Hpd':          ( 'IDX_UseModRM',       'vvvv',   '%Hpd', 'Hpd',     'V',     ),
    'HssHi':        ( 'IDX_UseModRM',       'vvvv',   '%Hx',  'HssHi',   'V',     ),
    'HsdHi':        ( 'IDX_UseModRM',       'vvvv',   '%Hx',  'HsdHi',   'V',     ),
    'Hq':           ( 'IDX_UseModRM',       'vvvv',   '%Hq',  'Hq',      'V',     ),
    'HqHi':         ( 'IDX_UseModRM',       'vvvv',   '%Hq',  'HqHi',    'V',     ),
    'Hx':           ( 'IDX_UseModRM',       'vvvv',   '%Hx',  'Hx',      'V',     ),

    # Immediate values.
    'Ib':           ( 'IDX_ParseImmByte',   'imm',    '%Ib',  'Ib',   '', ), ##< NB! Could be IDX_ParseImmByteSX for some instrs.
    'Iw':           ( 'IDX_ParseImmUshort', 'imm',    '%Iw',  'Iw',      '',      ),
    'Id':           ( 'IDX_ParseImmUlong',  'imm',    '%Id',  'Id',      '',      ),
    'Iq':           ( 'IDX_ParseImmQword',  'imm',    '%Iq',  'Iq',      '',      ),
    'Iv':           ( 'IDX_ParseImmV',      'imm',    '%Iv',  'Iv',      '',      ), ##< o16: word, o32: dword, o64: qword
    'Iz':           ( 'IDX_ParseImmZ',      'imm',    '%Iz',  'Iz',      '',      ), ##< o16: word, o32|o64:dword

    # Address operands (no ModR/M).
    'Ob':           ( 'IDX_ParseImmAddr',   'imm',    '%Ob',  'Ob',      '',      ),
    'Ov':           ( 'IDX_ParseImmAddr',   'imm',    '%Ov',  'Ov',      '',      ),

    # Relative jump targets
    'Jb':           ( 'IDX_ParseImmBRel',   'imm',    '%Jb',  'Jb',      '',      ),
    'Jv':           ( 'IDX_ParseImmVRel',   'imm',    '%Jv',  'Jv',      '',      ),

    # DS:rSI
    'Xb':           ( 'IDX_ParseXb',        'rSI',    '%eSI', 'Xb',      '',      ),
    'Xv':           ( 'IDX_ParseXv',        'rSI',    '%eSI', 'Xv',      '',      ),
    # ES:rDI
    'Yb':           ( 'IDX_ParseYb',        'rDI',    '%eDI', 'Yb',      '',      ),
    'Yv':           ( 'IDX_ParseYv',        'rDI',    '%eDI', 'Yv',      '',      ),

    'Fv':           ( 'IDX_ParseFixedReg',  'rFLAGS', '%Fv',  'Fv',      '',      ),

    # Fixed registers.
    'AL':           ( 'IDX_ParseFixedReg',  'AL',     'al',   'REG_AL',  '',      ),
    'rAX':          ( 'IDX_ParseFixedReg',  'rAX',    '%eAX', 'REG_EAX', '',      ),
    'rDX':          ( 'IDX_ParseFixedReg',  'rDX',    '%eDX', 'REG_EDX', '',      ),
    'CS':           ( 'IDX_ParseFixedReg',  'CS',     'cs',   'REG_CS',  '',      ), # 8086: push CS
    'DS':           ( 'IDX_ParseFixedReg',  'DS',     'ds',   'REG_DS',  '',      ),
    'ES':           ( 'IDX_ParseFixedReg',  'ES',     'es',   'REG_ES',  '',      ),
    'FS':           ( 'IDX_ParseFixedReg',  'FS',     'fs',   'REG_FS',  '',      ),
    'GS':           ( 'IDX_ParseFixedReg',  'GS',     'gs',   'REG_GS',  '',      ),
    'SS':           ( 'IDX_ParseFixedReg',  'SS',     'ss',   'REG_SS',  '',      ),
};

# IDX_ParseFixedReg
# IDX_ParseVexDest


## IEMFORM_XXX mappings.
g_kdIemForms = {     # sEncoding,   [ sWhere1, ... ]         opcodesub      ),
    'RM':           ( 'ModR/M',     [ 'reg', 'rm' ],         '',            ),
    'RM_REG':       ( 'ModR/M',     [ 'reg', 'rm' ],         '11 mr/reg',   ),
    'RM_MEM':       ( 'ModR/M',     [ 'reg', 'rm' ],         '!11 mr/reg',  ),
    'RMI':          ( 'ModR/M',     [ 'reg', 'rm', 'imm' ],  '',            ),
    'RMI_REG':      ( 'ModR/M',     [ 'reg', 'rm', 'imm' ],  '11 mr/reg',   ),
    'RMI_MEM':      ( 'ModR/M',     [ 'reg', 'rm', 'imm' ],  '!11 mr/reg',  ),
    'MR':           ( 'ModR/M',     [ 'rm', 'reg' ],         '',            ),
    'MR_REG':       ( 'ModR/M',     [ 'rm', 'reg' ],         '11 mr/reg',   ),
    'MR_MEM':       ( 'ModR/M',     [ 'rm', 'reg' ],         '!11 mr/reg',  ),
    'MRI':          ( 'ModR/M',     [ 'rm', 'reg', 'imm' ],  '',            ),
    'MRI_REG':      ( 'ModR/M',     [ 'rm', 'reg', 'imm' ],  '11 mr/reg',   ),
    'MRI_MEM':      ( 'ModR/M',     [ 'rm', 'reg', 'imm' ],  '!11 mr/reg',  ),
    'M':            ( 'ModR/M',     [ 'rm', ],               '',            ),
    'M_REG':        ( 'ModR/M',     [ 'rm', ],               '',            ),
    'M_MEM':        ( 'ModR/M',     [ 'rm', ],               '',            ),
    'R':            ( 'ModR/M',     [ 'reg', ],              '',            ),

    'VEX_RM':       ( 'VEX.ModR/M', [ 'reg', 'rm' ],         '',            ),
    'VEX_RM_REG':   ( 'VEX.ModR/M', [ 'reg', 'rm' ],         '11 mr/reg',   ),
    'VEX_RM_MEM':   ( 'VEX.ModR/M', [ 'reg', 'rm' ],         '!11 mr/reg',  ),
    'VEX_MR':       ( 'VEX.ModR/M', [ 'rm', 'reg' ],         '',            ),
    'VEX_MR_REG':   ( 'VEX.ModR/M', [ 'rm', 'reg' ],         '11 mr/reg',   ),
    'VEX_MR_MEM':   ( 'VEX.ModR/M', [ 'rm', 'reg' ],         '!11 mr/reg',  ),
    'VEX_M':        ( 'VEX.ModR/M', [ 'rm', ],               '' ),
    'VEX_M_REG':    ( 'VEX.ModR/M', [ 'rm', ],               '' ),
    'VEX_M_MEM':    ( 'VEX.ModR/M', [ 'rm', ],               '' ),
    'VEX_R':        ( 'VEX.ModR/M', [ 'reg', ],              '' ),
    'VEX_RVM':      ( 'VEX.ModR/M', [ 'reg', 'vvvv', 'rm' ], '',            ),
    'VEX_RVM_REG':  ( 'VEX.ModR/M', [ 'reg', 'vvvv', 'rm' ], '11 mr/reg',   ),
    'VEX_RVM_MEM':  ( 'VEX.ModR/M', [ 'reg', 'vvvv', 'rm' ], '!11 mr/reg',  ),
    'VEX_RMV':      ( 'VEX.ModR/M', [ 'reg', 'rm', 'vvvv' ], '',            ),
    'VEX_RMV_REG':  ( 'VEX.ModR/M', [ 'reg', 'rm', 'vvvv' ], '11 mr/reg',   ),
    'VEX_RMV_MEM':  ( 'VEX.ModR/M', [ 'reg', 'rm', 'vvvv' ], '!11 mr/reg',  ),
    'VEX_RMI':      ( 'VEX.ModR/M', [ 'reg', 'rm', 'imm' ],  '',            ),
    'VEX_RMI_REG':  ( 'VEX.ModR/M', [ 'reg', 'rm', 'imm' ],  '11 mr/reg',   ),
    'VEX_RMI_MEM':  ( 'VEX.ModR/M', [ 'reg', 'rm', 'imm' ],  '!11 mr/reg',  ),
    'VEX_MVR':      ( 'VEX.ModR/M', [ 'rm', 'vvvv', 'reg' ], '',            ),
    'VEX_MVR_REG':  ( 'VEX.ModR/M', [ 'rm', 'vvvv', 'reg' ], '11 mr/reg',   ),
    'VEX_MVR_MEM':  ( 'VEX.ModR/M', [ 'rm', 'vvvv', 'reg' ], '!11 mr/reg',  ),

    'VEX_VM':       ( 'VEX.ModR/M', [ 'vvvv', 'rm' ],        '',            ),
    'VEX_VM_REG':   ( 'VEX.ModR/M', [ 'vvvv', 'rm' ],        '11 mr/reg',   ),
    'VEX_VM_MEM':   ( 'VEX.ModR/M', [ 'vvvv', 'rm' ],        '!11 mr/reg',  ),

    'FIXED':        ( 'fixed',      None,                    '',            ),
};

## \@oppfx values.
g_kdPrefixes = {
    'none': [],
    '0x66': [],
    '0xf3': [],
    '0xf2': [],
};

## Special \@opcode tag values.
g_kdSpecialOpcodes = {
    '/reg':         [],
    'mr/reg':       [],
    '11 /reg':      [],
    '!11 /reg':     [],
    '11 mr/reg':    [],
    '!11 mr/reg':   [],
};

## Special \@opcodesub tag values.
## The first value is the real value for aliases.
## The second value is for bs3cg1.
g_kdSubOpcodes = {
    'none':                 [ None,                 '',         ],
    '11 mr/reg':            [ '11 mr/reg',          '',         ],
    '11':                   [ '11 mr/reg',          '',         ], ##< alias
    '!11 mr/reg':           [ '!11 mr/reg',         '',         ],
    '!11':                  [ '!11 mr/reg',         '',         ], ##< alias
    'rex.w=0':              [ 'rex.w=0',            'WZ',       ],
    'w=0':                  [ 'rex.w=0',            '',         ], ##< alias
    'rex.w=1':              [ 'rex.w=1',            'WNZ',      ],
    'w=1':                  [ 'rex.w=1',            '',         ], ##< alias
    'vex.l=0':              [ 'vex.l=0',            'L0',       ],
    'vex.l=1':              [ 'vex.l=0',            'L1',       ],
    '11 mr/reg vex.l=0':    [ '11 mr/reg vex.l=0',  'L0',       ],
    '11 mr/reg vex.l=1':    [ '11 mr/reg vex.l=1',  'L1',       ],
    '!11 mr/reg vex.l=0':   [ '!11 mr/reg vex.l=0', 'L0',       ],
    '!11 mr/reg vex.l=1':   [ '!11 mr/reg vex.l=1', 'L1',       ],
};

## Valid values for \@openc
g_kdEncodings = {
    'ModR/M':       [ 'BS3CG1ENC_MODRM', ],     ##< ModR/M
    'VEX.ModR/M':   [ 'BS3CG1ENC_VEX_MODRM', ], ##< VEX...ModR/M
    'fixed':        [ 'BS3CG1ENC_FIXED', ],     ##< Fixed encoding (address, registers, unused, etc).
    'VEX.fixed':    [ 'BS3CG1ENC_VEX_FIXED', ], ##< VEX + fixed encoding (address, registers, unused, etc).
    'prefix':       [ None, ],                  ##< Prefix
};

## \@opunused, \@opinvalid, \@opinvlstyle
g_kdInvalidStyles = {
    'immediate':                [], ##< CPU stops decoding immediately after the opcode.
    'vex.modrm':                [], ##< VEX+ModR/M, everyone.
    'intel-modrm':              [], ##< Intel decodes ModR/M.
    'intel-modrm-imm8':         [], ##< Intel decodes ModR/M and an 8-byte immediate.
    'intel-opcode-modrm':       [], ##< Intel decodes another opcode byte followed by ModR/M. (Unused extension tables.)
    'intel-opcode-modrm-imm8':  [], ##< Intel decodes another opcode byte followed by ModR/M and an 8-byte immediate.
};

g_kdCpuNames = {
    '8086':     (),
    '80186':    (),
    '80286':    (),
    '80386':    (),
    '80486':    (),
};

## \@opcpuid
g_kdCpuIdFlags = {
    'vme':          'X86_CPUID_FEATURE_EDX_VME',
    'tsc':          'X86_CPUID_FEATURE_EDX_TSC',
    'msr':          'X86_CPUID_FEATURE_EDX_MSR',
    'cx8':          'X86_CPUID_FEATURE_EDX_CX8',
    'sep':          'X86_CPUID_FEATURE_EDX_SEP',
    'cmov':         'X86_CPUID_FEATURE_EDX_CMOV',
    'clfsh':        'X86_CPUID_FEATURE_EDX_CLFSH',
    'clflushopt':   'X86_CPUID_STEXT_FEATURE_EBX_CLFLUSHOPT',
    'mmx':          'X86_CPUID_FEATURE_EDX_MMX',
    'fxsr':         'X86_CPUID_FEATURE_EDX_FXSR',
    'sse':          'X86_CPUID_FEATURE_EDX_SSE',
    'sse2':         'X86_CPUID_FEATURE_EDX_SSE2',
    'sse3':         'X86_CPUID_FEATURE_ECX_SSE3',
    'pclmul':       'X86_CPUID_FEATURE_ECX_DTES64',
    'monitor':      'X86_CPUID_FEATURE_ECX_CPLDS',
    'vmx':          'X86_CPUID_FEATURE_ECX_VMX',
    'smx':          'X86_CPUID_FEATURE_ECX_TM2',
    'ssse3':        'X86_CPUID_FEATURE_ECX_SSSE3',
    'fma':          'X86_CPUID_FEATURE_ECX_FMA',
    'cx16':         'X86_CPUID_FEATURE_ECX_CX16',
    'pcid':         'X86_CPUID_FEATURE_ECX_PCID',
    'sse4.1':       'X86_CPUID_FEATURE_ECX_SSE4_1',
    'sse4.2':       'X86_CPUID_FEATURE_ECX_SSE4_2',
    'movbe':        'X86_CPUID_FEATURE_ECX_MOVBE',
    'popcnt':       'X86_CPUID_FEATURE_ECX_POPCNT',
    'aes':          'X86_CPUID_FEATURE_ECX_AES',
    'xsave':        'X86_CPUID_FEATURE_ECX_XSAVE',
    'avx':          'X86_CPUID_FEATURE_ECX_AVX',
    'avx2':         'X86_CPUID_STEXT_FEATURE_EBX_AVX2',
    'f16c':         'X86_CPUID_FEATURE_ECX_F16C',
    'rdrand':       'X86_CPUID_FEATURE_ECX_RDRAND',

    'axmmx':        'X86_CPUID_AMD_FEATURE_EDX_AXMMX',
    '3dnowext':     'X86_CPUID_AMD_FEATURE_EDX_3DNOW_EX',
    '3dnow':        'X86_CPUID_AMD_FEATURE_EDX_3DNOW',
    'svm':          'X86_CPUID_AMD_FEATURE_ECX_SVM',
    'cr8l':         'X86_CPUID_AMD_FEATURE_ECX_CR8L',
    'abm':          'X86_CPUID_AMD_FEATURE_ECX_ABM',
    'sse4a':        'X86_CPUID_AMD_FEATURE_ECX_SSE4A',
    '3dnowprf':     'X86_CPUID_AMD_FEATURE_ECX_3DNOWPRF',
    'xop':          'X86_CPUID_AMD_FEATURE_ECX_XOP',
    'fma4':         'X86_CPUID_AMD_FEATURE_ECX_FMA4',
};

## \@ophints values.
g_kdHints = {
    'invalid':               'DISOPTYPE_INVALID',               ##<
    'harmless':              'DISOPTYPE_HARMLESS',              ##<
    'controlflow':           'DISOPTYPE_CONTROLFLOW',           ##<
    'potentially_dangerous': 'DISOPTYPE_POTENTIALLY_DANGEROUS', ##<
    'dangerous':             'DISOPTYPE_DANGEROUS',             ##<
    'portio':                'DISOPTYPE_PORTIO',                ##<
    'privileged':            'DISOPTYPE_PRIVILEGED',            ##<
    'privileged_notrap':     'DISOPTYPE_PRIVILEGED_NOTRAP',     ##<
    'uncond_controlflow':    'DISOPTYPE_UNCOND_CONTROLFLOW',    ##<
    'relative_controlflow':  'DISOPTYPE_RELATIVE_CONTROLFLOW',  ##<
    'cond_controlflow':      'DISOPTYPE_COND_CONTROLFLOW',      ##<
    'interrupt':             'DISOPTYPE_INTERRUPT',             ##<
    'illegal':               'DISOPTYPE_ILLEGAL',               ##<
    'rrm_dangerous':         'DISOPTYPE_RRM_DANGEROUS',         ##< Some additional dangerous ones when recompiling raw r0.
    'rrm_dangerous_16':      'DISOPTYPE_RRM_DANGEROUS_16',      ##< Some additional dangerous ones when recompiling 16-bit raw r0.
    'inhibit_irqs':          'DISOPTYPE_INHIBIT_IRQS',          ##< Will or can inhibit irqs (sti, pop ss, mov ss) */
    'portio_read':           'DISOPTYPE_PORTIO_READ',           ##<
    'portio_write':          'DISOPTYPE_PORTIO_WRITE',          ##<
    'invalid_64':            'DISOPTYPE_INVALID_64',            ##< Invalid in 64 bits mode
    'only_64':               'DISOPTYPE_ONLY_64',               ##< Only valid in 64 bits mode
    'default_64_op_size':    'DISOPTYPE_DEFAULT_64_OP_SIZE',    ##< Default 64 bits operand size
    'forced_64_op_size':     'DISOPTYPE_FORCED_64_OP_SIZE',     ##< Forced 64 bits operand size; regardless of prefix bytes
    'rexb_extends_opreg':    'DISOPTYPE_REXB_EXTENDS_OPREG',    ##< REX.B extends the register field in the opcode byte
    'mod_fixed_11':          'DISOPTYPE_MOD_FIXED_11',          ##< modrm.mod is always 11b
    'forced_32_op_size_x86': 'DISOPTYPE_FORCED_32_OP_SIZE_X86', ##< Forced 32 bits operand size; regardless of prefix bytes
                                                                ##  (only in 16 & 32 bits mode!)
    'avx':                   'DISOPTYPE_AVX',                   ##< AVX,AVX2,++ instruction. Not implemented yet!
    'sse':                   'DISOPTYPE_SSE',                   ##< SSE,SSE2,SSE3,++ instruction. Not implemented yet!
    'mmx':                   'DISOPTYPE_MMX',                   ##< MMX,MMXExt,3DNow,++ instruction. Not implemented yet!
    'fpu':                   'DISOPTYPE_FPU',                   ##< FPU instruction. Not implemented yet!
    'ignores_oz_pfx':        '',                                ##< Ignores operand size prefix 66h.
    'ignores_rexw':          '',                                ##< Ignores REX.W.
    'ignores_op_sizes':      '',                                ##< Shorthand for "ignores_oz_pfx | ignores_op_sizes".
    'vex_l_zero':            '',                                ##< VEX.L must be 0.
    'vex_l_ignored':         '',                                ##< VEX.L is ignored.
    'vex_v_zero':            '',                                ##< VEX.V must be 0. (generate sub-table?)
    'lock_allowed':          '',                                ##< Lock prefix allowed.
};

## \@opxcpttype values (see SDMv2 2.4, 2.7).
g_kdXcptTypes = {
    'none':     [],
    '1':        [],
    '2':        [],
    '3':        [],
    '4':        [],
    '4UA':      [],
    '5':        [],
    '5LZ':      [], # LZ = VEX.L must be zero.
    '6':        [],
    '7':        [],
    '7LZ':      [],
    '8':        [],
    '11':       [],
    '12':       [],
    'E1':       [],
    'E1NF':     [],
    'E2':       [],
    'E3':       [],
    'E3NF':     [],
    'E4':       [],
    'E4NF':     [],
    'E5':       [],
    'E5NF':     [],
    'E6':       [],
    'E6NF':     [],
    'E7NF':     [],
    'E9':       [],
    'E9NF':     [],
    'E10':      [],
    'E11':      [],
    'E12':      [],
    'E12NF':    [],
};


def _isValidOpcodeByte(sOpcode):
    """
    Checks if sOpcode is a valid lower case opcode byte.
    Returns true/false.
    """
    if len(sOpcode) == 4:
        if sOpcode[:2] == '0x':
            if sOpcode[2] in '0123456789abcdef':
                if sOpcode[3] in '0123456789abcdef':
                    return True;
    return False;


class InstructionMap(object):
    """
    Instruction map.

    The opcode map provides the lead opcode bytes (empty for the one byte
    opcode map).  An instruction can be member of multiple opcode maps as long
    as it uses the same opcode value within the map (because of VEX).
    """

    kdEncodings = {
        'legacy':   [],
        'vex1':     [], ##< VEX or EVEX prefix with vvvvv = 1
        'vex2':     [], ##< VEX or EVEX prefix with vvvvv = 2
        'vex3':     [], ##< VEX or EVEX prefix with vvvvv = 3
        'xop8':     [], ##< XOP prefix with vvvvv = 8
        'xop9':     [], ##< XOP prefix with vvvvv = 9
        'xop10':    [], ##< XOP prefix with vvvvv = 10
    };
    ## Selectors.
    ## 1. The first value is the number of table entries required by a
    ## decoder or disassembler for this type of selector.
    ## 2. The second value is how many entries per opcode byte if applicable.
    kdSelectors = {
        'byte':     [  256, 1, ], ##< next opcode byte selects the instruction (default).
        'byte+pfx': [ 1024, 4, ], ##< next opcode byte selects the instruction together with the 0x66, 0xf2 and 0xf3 prefixes.
        '/r':       [    8, 1, ], ##< modrm.reg selects the instruction.
        'memreg /r':[   16, 1, ], ##< modrm.reg and (modrm.mod == 3) selects the instruction.
        'mod /r':   [   32, 1, ], ##< modrm.reg and modrm.mod selects the instruction.
        '!11 /r':   [    8, 1, ], ##< modrm.reg selects the instruction with modrm.mod != 0y11.
        '11 /r':    [    8, 1, ], ##< modrm.reg select the instruction with modrm.mod == 0y11.
        '11':       [   64, 1, ], ##< modrm.reg and modrm.rm select the instruction with modrm.mod == 0y11.
    };

    ## Define the subentry number according to the Instruction::sPrefix
    ## value for 'byte+pfx' selected tables.
    kiPrefixOrder = {
        'none': 0,
        '0x66': 1,
        '0xf3': 2,
        '0xf2': 3,
    };

    def __init__(self, sName, sIemName = None, asLeadOpcodes = None, sSelector = 'byte+pfx',
                 sEncoding = 'legacy', sDisParse = None):
        assert sSelector in self.kdSelectors;
        assert sEncoding in self.kdEncodings;
        if asLeadOpcodes is None:
            asLeadOpcodes = [];
        else:
            for sOpcode in asLeadOpcodes:
                assert _isValidOpcodeByte(sOpcode);
        assert sDisParse is None or sDisParse.startswith('IDX_Parse');

        self.sName          = sName;
        self.sIemName       = sIemName;
        self.asLeadOpcodes  = asLeadOpcodes;    ##< Lead opcode bytes formatted as hex strings like '0x0f'.
        self.sSelector      = sSelector;        ##< The member selector, see kdSelectors.
        self.sEncoding      = sEncoding;        ##< The encoding, see kdSelectors.
        self.aoInstructions = []                # type: Instruction
        self.sDisParse      = sDisParse;        ##< IDX_ParseXXX.

    def copy(self, sNewName, sPrefixFilter = None):
        """
        Copies the table with filtering instruction by sPrefix if not None.
        """
        oCopy = InstructionMap(sNewName, sIemName = self.sIemName, asLeadOpcodes = self.asLeadOpcodes,
                               sSelector = 'byte' if sPrefixFilter is not None and self.sSelector == 'byte+pfx'
                               else self.sSelector,
                               sEncoding = self.sEncoding, sDisParse = self.sDisParse);
        if sPrefixFilter is None:
            oCopy.aoInstructions = list(self.aoInstructions);
        else:
            oCopy.aoInstructions = [oInstr for oInstr in self.aoInstructions if oInstr.sPrefix == sPrefixFilter];
        return oCopy;

    def getTableSize(self):
        """
        Number of table entries.   This corresponds directly to the selector.
        """
        return self.kdSelectors[self.sSelector][0];

    def getEntriesPerByte(self):
        """
        Number of table entries per opcode bytes.

        This only really makes sense for the 'byte' and 'byte+pfx' selectors, for
        the others it will just return 1.
        """
        return self.kdSelectors[self.sSelector][1];

    def getInstructionIndex(self, oInstr):
        """
        Returns the table index for the instruction.
        """
        bOpcode = oInstr.getOpcodeByte();

        # The byte selectors are simple.  We need a full opcode byte and need just return it.
        if self.sSelector == 'byte':
            assert oInstr.sOpcode[:2] == '0x' and len(oInstr.sOpcode) == 4, str(oInstr);
            return bOpcode;

        # The byte + prefix selector is similarly simple, though requires a prefix as well as the full opcode.
        if self.sSelector  == 'byte+pfx':
            assert oInstr.sOpcode[:2] == '0x' and len(oInstr.sOpcode) == 4, str(oInstr);
            assert self.kiPrefixOrder.get(oInstr.sPrefix, -16384) >= 0;
            return bOpcode * 4 + self.kiPrefixOrder.get(oInstr.sPrefix, -16384);

        # The other selectors needs masking and shifting.
        if self.sSelector == '/r':
            return (bOpcode >> 3) & 0x7;

        if self.sSelector == 'mod /r':
            return (bOpcode >> 3) & 0x1f;

        if self.sSelector == 'memreg /r':
            return ((bOpcode >> 3) & 0x7) | (int((bOpcode >> 6) == 3) << 3);

        if self.sSelector == '!11 /r':
            assert (bOpcode & 0xc0) != 0xc, str(oInstr);
            return (bOpcode >> 3) & 0x7;

        if self.sSelector == '11 /r':
            assert (bOpcode & 0xc0) == 0xc, str(oInstr);
            return (bOpcode >> 3) & 0x7;

        if self.sSelector == '11':
            assert (bOpcode & 0xc0) == 0xc, str(oInstr);
            return bOpcode & 0x3f;

        assert False, self.sSelector;
        return -1;

    def getInstructionsInTableOrder(self):
        """
        Get instructions in table order.

        Returns array of instructions.  Normally there is exactly one
        instruction per entry.  However the entry could also be None if
        not instruction was specified for that opcode value.  Or there
        could be a list of instructions to deal with special encodings
        where for instance prefix (e.g. REX.W) encodes a different
        instruction or different CPUs have different instructions or
        prefixes in the same place.
        """
        # Start with empty table.
        cTable  = self.getTableSize();
        aoTable = [None] * cTable;

        # Insert the instructions.
        for oInstr in self.aoInstructions:
            if oInstr.sOpcode:
                idxOpcode = self.getInstructionIndex(oInstr);
                assert idxOpcode < cTable, str(idxOpcode);

                oExisting = aoTable[idxOpcode];
                if oExisting is None:
                    aoTable[idxOpcode] = oInstr;
                elif not isinstance(oExisting, list):
                    aoTable[idxOpcode] = list([oExisting, oInstr]);
                else:
                    oExisting.append(oInstr);

        return aoTable;


    def getDisasTableName(self):
        """
        Returns the disassembler table name for this map.
        """
        sName = 'g_aDisas';
        for sWord in self.sName.split('_'):
            if sWord == 'm':            # suffix indicating modrm.mod==mem
                sName += '_m';
            elif sWord == 'r':          # suffix indicating modrm.mod==reg
                sName += '_r';
            elif len(sWord) == 2 and re.match('^[a-f0-9][a-f0-9]$', sWord):
                sName += '_' + sWord;
            else:
                sWord  = sWord.replace('grp', 'Grp');
                sWord  = sWord.replace('map', 'Map');
                sName += sWord[0].upper() + sWord[1:];
        return sName;

    def getDisasRangeName(self):
        """
        Returns the disassembler table range name for this map.
        """
        return self.getDisasTableName().replace('g_aDisas', 'g_Disas') + 'Range';

    def isVexMap(self):
        """ Returns True if a VEX map. """
        return self.sEncoding.startswith('vex');


class TestType(object):
    """
    Test value type.

    This base class deals with integer like values.  The fUnsigned constructor
    parameter indicates the default stance on zero vs sign extending.  It is
    possible to override fUnsigned=True by prefixing the value with '+' or '-'.
    """
    def __init__(self, sName, acbSizes = None, fUnsigned = True):
        self.sName = sName;
        self.acbSizes = [1, 2, 4, 8, 16, 32] if acbSizes is None else acbSizes;  # Normal sizes.
        self.fUnsigned = fUnsigned;

    class BadValue(Exception):
        """ Bad value exception. """
        def __init__(self, sMessage):
            Exception.__init__(self, sMessage);
            self.sMessage = sMessage;

    ## For ascii ~ operator.
    kdHexInv = {
        '0': 'f',
        '1': 'e',
        '2': 'd',
        '3': 'c',
        '4': 'b',
        '5': 'a',
        '6': '9',
        '7': '8',
        '8': '7',
        '9': '6',
        'a': '5',
        'b': '4',
        'c': '3',
        'd': '2',
        'e': '1',
        'f': '0',
    };

    def get(self, sValue):
        """
        Get the shortest normal sized byte representation of oValue.

        Returns ((fSignExtend, bytearray), ) or ((fSignExtend, bytearray), (fSignExtend, bytearray), ).
        The latter form is for AND+OR pairs where the first entry is what to
        AND with the field and the second the one or OR with.

        Raises BadValue if invalid value.
        """
        if not sValue:
            raise TestType.BadValue('empty value');

        # Deal with sign and detect hexadecimal or decimal.
        fSignExtend = not self.fUnsigned;
        if sValue[0] == '-' or sValue[0] == '+':
            fSignExtend = True;
            fHex = len(sValue) > 3 and sValue[1:3].lower() == '0x';
        else:
            fHex = len(sValue) > 2 and sValue[0:2].lower() == '0x';

        # try convert it to long integer.
        try:
            iValue = long(sValue, 16 if fHex else 10);
        except Exception as oXcpt:
            raise TestType.BadValue('failed to convert "%s" to integer (%s)' % (sValue, oXcpt));

        # Convert the hex string and pad it to a decent value.  Negative values
        # needs to be manually converted to something non-negative (~-n + 1).
        if iValue >= 0:
            sHex = hex(iValue);
            if sys.version_info[0] < 3:
                assert sHex[-1] == 'L';
                sHex = sHex[:-1];
            assert sHex[:2] == '0x';
            sHex = sHex[2:];
        else:
            sHex = hex(-iValue - 1);
            if sys.version_info[0] < 3:
                assert sHex[-1] == 'L';
                sHex = sHex[:-1];
            assert sHex[:2] == '0x';
            sHex = ''.join([self.kdHexInv[sDigit] for sDigit in sHex[2:]]);
            if fSignExtend and sHex[0] not in [ '8', '9', 'a', 'b', 'c', 'd', 'e', 'f']:
                sHex = 'f' + sHex;

        cDigits = len(sHex);
        if cDigits <= self.acbSizes[-1] * 2:
            for cb in self.acbSizes:
                cNaturalDigits = cb * 2;
                if cDigits <= cNaturalDigits:
                    break;
        else:
            cNaturalDigits = self.acbSizes[-1] * 2;
            cNaturalDigits = int((cDigits + cNaturalDigits - 1) / cNaturalDigits) * cNaturalDigits;
            assert isinstance(cNaturalDigits, int)

        if cNaturalDigits != cDigits:
            cNeeded = cNaturalDigits - cDigits;
            if iValue >= 0:
                sHex = ('0' * cNeeded) + sHex;
            else:
                sHex = ('f' * cNeeded) + sHex;

        # Invert and convert to bytearray and return it.
        abValue = bytearray([int(sHex[offHex - 2 : offHex], 16) for offHex in range(len(sHex), 0, -2)]);

        return ((fSignExtend, abValue),);

    def validate(self, sValue):
        """
        Returns True if value is okay, error message on failure.
        """
        try:
            self.get(sValue);
        except TestType.BadValue as oXcpt:
            return oXcpt.sMessage;
        return True;

    def isAndOrPair(self, sValue):
        """
        Checks if sValue is a pair.
        """
        _ = sValue;
        return False;


class TestTypeEflags(TestType):
    """
    Special value parsing for EFLAGS/RFLAGS/FLAGS.
    """

    kdZeroValueFlags = { 'nv': 0, 'pl': 0, 'nz': 0, 'na': 0, 'pe': 0, 'nc': 0, 'di': 0, 'up': 0 };

    def __init__(self, sName):
        TestType.__init__(self, sName, acbSizes = [1, 2, 4, 8], fUnsigned = True);

    def get(self, sValue):
        fClear = 0;
        fSet   = 0;
        for sFlag in sValue.split(','):
            sConstant = g_kdEFlagsMnemonics.get(sFlag, None);
            if sConstant is None:
                raise self.BadValue('Unknown flag "%s" in "%s"' % (sFlag, sValue))
            if sConstant[0] == '!':
                fClear |= g_kdX86EFlagsConstants[sConstant[1:]];
            else:
                fSet   |= g_kdX86EFlagsConstants[sConstant];

        aoSet = TestType.get(self, '0x%x' % (fSet,));
        if fClear != 0:
            aoClear = TestType.get(self, '%#x' % (fClear,))
            assert self.isAndOrPair(sValue) is True;
            return (aoClear[0], aoSet[0]);
        assert self.isAndOrPair(sValue) is False;
        return aoSet;

    def isAndOrPair(self, sValue):
        for sZeroFlag in self.kdZeroValueFlags:
            if sValue.find(sZeroFlag) >= 0:
                return True;
        return False;

class TestTypeFromDict(TestType):
    """
    Special value parsing for CR0.
    """

    kdZeroValueFlags = { 'nv': 0, 'pl': 0, 'nz': 0, 'na': 0, 'pe': 0, 'nc': 0, 'di': 0, 'up': 0 };

    def __init__(self, sName, kdConstantsAndValues, sConstantPrefix):
        TestType.__init__(self, sName, acbSizes = [1, 2, 4, 8], fUnsigned = True);
        self.kdConstantsAndValues = kdConstantsAndValues;
        self.sConstantPrefix      = sConstantPrefix;

    def get(self, sValue):
        fValue = 0;
        for sFlag in sValue.split(','):
            fFlagValue = self.kdConstantsAndValues.get(self.sConstantPrefix + sFlag.upper(), None);
            if fFlagValue is None:
                raise self.BadValue('Unknown flag "%s" in "%s"' % (sFlag, sValue))
            fValue |= fFlagValue;
        return TestType.get(self, '0x%x' % (fValue,));


class TestInOut(object):
    """
    One input or output state modifier.

    This should be thought as values to modify BS3REGCTX and extended (needs
    to be structured) state.
    """
    ## Assigned operators.
    kasOperators = [
        '&|=',  # Special AND(INV)+OR operator for use with EFLAGS.
        '&~=',
        '&=',
        '|=',
        '='
    ];
    ## Types
    kdTypes = {
        'uint':  TestType('uint', fUnsigned = True),
        'int':   TestType('int'),
        'efl':   TestTypeEflags('efl'),
        'cr0':   TestTypeFromDict('cr0', g_kdX86Cr0Constants, 'X86_CR0_'),
        'cr4':   TestTypeFromDict('cr4', g_kdX86Cr4Constants, 'X86_CR4_'),
        'xcr0':  TestTypeFromDict('xcr0', g_kdX86XSaveCConstants, 'XSAVE_C_'),
    };
    ## CPU context fields.
    kdFields = {
        # name:         ( default type, [both|input|output], )
        # Operands.
        'op1':          ( 'uint', 'both',   ), ## \@op1
        'op2':          ( 'uint', 'both',   ), ## \@op2
        'op3':          ( 'uint', 'both',   ), ## \@op3
        'op4':          ( 'uint', 'both',   ), ## \@op4
        # Flags.
        'efl':          ( 'efl',  'both',   ),
        'efl_undef':    ( 'uint', 'output', ),
        # 8-bit GPRs.
        'al':           ( 'uint', 'both',   ),
        'cl':           ( 'uint', 'both',   ),
        'dl':           ( 'uint', 'both',   ),
        'bl':           ( 'uint', 'both',   ),
        'ah':           ( 'uint', 'both',   ),
        'ch':           ( 'uint', 'both',   ),
        'dh':           ( 'uint', 'both',   ),
        'bh':           ( 'uint', 'both',   ),
        'r8l':          ( 'uint', 'both',   ),
        'r9l':          ( 'uint', 'both',   ),
        'r10l':         ( 'uint', 'both',   ),
        'r11l':         ( 'uint', 'both',   ),
        'r12l':         ( 'uint', 'both',   ),
        'r13l':         ( 'uint', 'both',   ),
        'r14l':         ( 'uint', 'both',   ),
        'r15l':         ( 'uint', 'both',   ),
        # 16-bit GPRs.
        'ax':           ( 'uint', 'both',   ),
        'dx':           ( 'uint', 'both',   ),
        'cx':           ( 'uint', 'both',   ),
        'bx':           ( 'uint', 'both',   ),
        'sp':           ( 'uint', 'both',   ),
        'bp':           ( 'uint', 'both',   ),
        'si':           ( 'uint', 'both',   ),
        'di':           ( 'uint', 'both',   ),
        'r8w':          ( 'uint', 'both',   ),
        'r9w':          ( 'uint', 'both',   ),
        'r10w':         ( 'uint', 'both',   ),
        'r11w':         ( 'uint', 'both',   ),
        'r12w':         ( 'uint', 'both',   ),
        'r13w':         ( 'uint', 'both',   ),
        'r14w':         ( 'uint', 'both',   ),
        'r15w':         ( 'uint', 'both',   ),
        # 32-bit GPRs.
        'eax':          ( 'uint', 'both',   ),
        'edx':          ( 'uint', 'both',   ),
        'ecx':          ( 'uint', 'both',   ),
        'ebx':          ( 'uint', 'both',   ),
        'esp':          ( 'uint', 'both',   ),
        'ebp':          ( 'uint', 'both',   ),
        'esi':          ( 'uint', 'both',   ),
        'edi':          ( 'uint', 'both',   ),
        'r8d':          ( 'uint', 'both',   ),
        'r9d':          ( 'uint', 'both',   ),
        'r10d':         ( 'uint', 'both',   ),
        'r11d':         ( 'uint', 'both',   ),
        'r12d':         ( 'uint', 'both',   ),
        'r13d':         ( 'uint', 'both',   ),
        'r14d':         ( 'uint', 'both',   ),
        'r15d':         ( 'uint', 'both',   ),
        # 64-bit GPRs.
        'rax':          ( 'uint', 'both',   ),
        'rdx':          ( 'uint', 'both',   ),
        'rcx':          ( 'uint', 'both',   ),
        'rbx':          ( 'uint', 'both',   ),
        'rsp':          ( 'uint', 'both',   ),
        'rbp':          ( 'uint', 'both',   ),
        'rsi':          ( 'uint', 'both',   ),
        'rdi':          ( 'uint', 'both',   ),
        'r8':           ( 'uint', 'both',   ),
        'r9':           ( 'uint', 'both',   ),
        'r10':          ( 'uint', 'both',   ),
        'r11':          ( 'uint', 'both',   ),
        'r12':          ( 'uint', 'both',   ),
        'r13':          ( 'uint', 'both',   ),
        'r14':          ( 'uint', 'both',   ),
        'r15':          ( 'uint', 'both',   ),
        # 16-bit, 32-bit or 64-bit registers according to operand size.
        'oz.rax':       ( 'uint', 'both',   ),
        'oz.rdx':       ( 'uint', 'both',   ),
        'oz.rcx':       ( 'uint', 'both',   ),
        'oz.rbx':       ( 'uint', 'both',   ),
        'oz.rsp':       ( 'uint', 'both',   ),
        'oz.rbp':       ( 'uint', 'both',   ),
        'oz.rsi':       ( 'uint', 'both',   ),
        'oz.rdi':       ( 'uint', 'both',   ),
        'oz.r8':        ( 'uint', 'both',   ),
        'oz.r9':        ( 'uint', 'both',   ),
        'oz.r10':       ( 'uint', 'both',   ),
        'oz.r11':       ( 'uint', 'both',   ),
        'oz.r12':       ( 'uint', 'both',   ),
        'oz.r13':       ( 'uint', 'both',   ),
        'oz.r14':       ( 'uint', 'both',   ),
        'oz.r15':       ( 'uint', 'both',   ),
        # Control registers.
        'cr0':          ( 'cr0',  'both',   ),
        'cr4':          ( 'cr4',  'both',   ),
        'xcr0':         ( 'xcr0', 'both',   ),
        # FPU Registers
        'fcw':          ( 'uint', 'both',   ),
        'fsw':          ( 'uint', 'both',   ),
        'ftw':          ( 'uint', 'both',   ),
        'fop':          ( 'uint', 'both',   ),
        'fpuip':        ( 'uint', 'both',   ),
        'fpucs':        ( 'uint', 'both',   ),
        'fpudp':        ( 'uint', 'both',   ),
        'fpuds':        ( 'uint', 'both',   ),
        'mxcsr':        ( 'uint', 'both',   ),
        'st0':          ( 'uint', 'both',   ),
        'st1':          ( 'uint', 'both',   ),
        'st2':          ( 'uint', 'both',   ),
        'st3':          ( 'uint', 'both',   ),
        'st4':          ( 'uint', 'both',   ),
        'st5':          ( 'uint', 'both',   ),
        'st6':          ( 'uint', 'both',   ),
        'st7':          ( 'uint', 'both',   ),
        # MMX registers.
        'mm0':          ( 'uint', 'both',   ),
        'mm1':          ( 'uint', 'both',   ),
        'mm2':          ( 'uint', 'both',   ),
        'mm3':          ( 'uint', 'both',   ),
        'mm4':          ( 'uint', 'both',   ),
        'mm5':          ( 'uint', 'both',   ),
        'mm6':          ( 'uint', 'both',   ),
        'mm7':          ( 'uint', 'both',   ),
        # SSE registers.
        'xmm0':         ( 'uint', 'both',   ),
        'xmm1':         ( 'uint', 'both',   ),
        'xmm2':         ( 'uint', 'both',   ),
        'xmm3':         ( 'uint', 'both',   ),
        'xmm4':         ( 'uint', 'both',   ),
        'xmm5':         ( 'uint', 'both',   ),
        'xmm6':         ( 'uint', 'both',   ),
        'xmm7':         ( 'uint', 'both',   ),
        'xmm8':         ( 'uint', 'both',   ),
        'xmm9':         ( 'uint', 'both',   ),
        'xmm10':        ( 'uint', 'both',   ),
        'xmm11':        ( 'uint', 'both',   ),
        'xmm12':        ( 'uint', 'both',   ),
        'xmm13':        ( 'uint', 'both',   ),
        'xmm14':        ( 'uint', 'both',   ),
        'xmm15':        ( 'uint', 'both',   ),
        'xmm0.lo':      ( 'uint', 'both',   ),
        'xmm1.lo':      ( 'uint', 'both',   ),
        'xmm2.lo':      ( 'uint', 'both',   ),
        'xmm3.lo':      ( 'uint', 'both',   ),
        'xmm4.lo':      ( 'uint', 'both',   ),
        'xmm5.lo':      ( 'uint', 'both',   ),
        'xmm6.lo':      ( 'uint', 'both',   ),
        'xmm7.lo':      ( 'uint', 'both',   ),
        'xmm8.lo':      ( 'uint', 'both',   ),
        'xmm9.lo':      ( 'uint', 'both',   ),
        'xmm10.lo':     ( 'uint', 'both',   ),
        'xmm11.lo':     ( 'uint', 'both',   ),
        'xmm12.lo':     ( 'uint', 'both',   ),
        'xmm13.lo':     ( 'uint', 'both',   ),
        'xmm14.lo':     ( 'uint', 'both',   ),
        'xmm15.lo':     ( 'uint', 'both',   ),
        'xmm0.hi':      ( 'uint', 'both',   ),
        'xmm1.hi':      ( 'uint', 'both',   ),
        'xmm2.hi':      ( 'uint', 'both',   ),
        'xmm3.hi':      ( 'uint', 'both',   ),
        'xmm4.hi':      ( 'uint', 'both',   ),
        'xmm5.hi':      ( 'uint', 'both',   ),
        'xmm6.hi':      ( 'uint', 'both',   ),
        'xmm7.hi':      ( 'uint', 'both',   ),
        'xmm8.hi':      ( 'uint', 'both',   ),
        'xmm9.hi':      ( 'uint', 'both',   ),
        'xmm10.hi':     ( 'uint', 'both',   ),
        'xmm11.hi':     ( 'uint', 'both',   ),
        'xmm12.hi':     ( 'uint', 'both',   ),
        'xmm13.hi':     ( 'uint', 'both',   ),
        'xmm14.hi':     ( 'uint', 'both',   ),
        'xmm15.hi':     ( 'uint', 'both',   ),
        'xmm0.lo.zx':   ( 'uint', 'both',   ),
        'xmm1.lo.zx':   ( 'uint', 'both',   ),
        'xmm2.lo.zx':   ( 'uint', 'both',   ),
        'xmm3.lo.zx':   ( 'uint', 'both',   ),
        'xmm4.lo.zx':   ( 'uint', 'both',   ),
        'xmm5.lo.zx':   ( 'uint', 'both',   ),
        'xmm6.lo.zx':   ( 'uint', 'both',   ),
        'xmm7.lo.zx':   ( 'uint', 'both',   ),
        'xmm8.lo.zx':   ( 'uint', 'both',   ),
        'xmm9.lo.zx':   ( 'uint', 'both',   ),
        'xmm10.lo.zx':  ( 'uint', 'both',   ),
        'xmm11.lo.zx':  ( 'uint', 'both',   ),
        'xmm12.lo.zx':  ( 'uint', 'both',   ),
        'xmm13.lo.zx':  ( 'uint', 'both',   ),
        'xmm14.lo.zx':  ( 'uint', 'both',   ),
        'xmm15.lo.zx':  ( 'uint', 'both',   ),
        'xmm0.dw0':     ( 'uint', 'both',   ),
        'xmm1.dw0':     ( 'uint', 'both',   ),
        'xmm2.dw0':     ( 'uint', 'both',   ),
        'xmm3.dw0':     ( 'uint', 'both',   ),
        'xmm4.dw0':     ( 'uint', 'both',   ),
        'xmm5.dw0':     ( 'uint', 'both',   ),
        'xmm6.dw0':     ( 'uint', 'both',   ),
        'xmm7.dw0':     ( 'uint', 'both',   ),
        'xmm8.dw0':     ( 'uint', 'both',   ),
        'xmm9.dw0':     ( 'uint', 'both',   ),
        'xmm10.dw0':    ( 'uint', 'both',   ),
        'xmm11.dw0':    ( 'uint', 'both',   ),
        'xmm12.dw0':    ( 'uint', 'both',   ),
        'xmm13.dw0':    ( 'uint', 'both',   ),
        'xmm14.dw0':    ( 'uint', 'both',   ),
        'xmm15_dw0':    ( 'uint', 'both',   ),
        # AVX registers.
        'ymm0':         ( 'uint', 'both',   ),
        'ymm1':         ( 'uint', 'both',   ),
        'ymm2':         ( 'uint', 'both',   ),
        'ymm3':         ( 'uint', 'both',   ),
        'ymm4':         ( 'uint', 'both',   ),
        'ymm5':         ( 'uint', 'both',   ),
        'ymm6':         ( 'uint', 'both',   ),
        'ymm7':         ( 'uint', 'both',   ),
        'ymm8':         ( 'uint', 'both',   ),
        'ymm9':         ( 'uint', 'both',   ),
        'ymm10':        ( 'uint', 'both',   ),
        'ymm11':        ( 'uint', 'both',   ),
        'ymm12':        ( 'uint', 'both',   ),
        'ymm13':        ( 'uint', 'both',   ),
        'ymm14':        ( 'uint', 'both',   ),
        'ymm15':        ( 'uint', 'both',   ),

        # Special ones.
        'value.xcpt':   ( 'uint', 'output', ),
    };

    def __init__(self, sField, sOp, sValue, sType):
        assert sField in self.kdFields;
        assert sOp in self.kasOperators;
        self.sField = sField;
        self.sOp    = sOp;
        self.sValue = sValue;
        self.sType  = sType;
        assert isinstance(sField, str);
        assert isinstance(sOp, str);
        assert isinstance(sType, str);
        assert isinstance(sValue, str);


class TestSelector(object):
    """
    One selector for an instruction test.
    """
    ## Selector compare operators.
    kasCompareOps = [ '==', '!=' ];
    ## Selector variables and their valid values.
    kdVariables = {
        # Operand size.
        'size': {
            'o16':  'size_o16',
            'o32':  'size_o32',
            'o64':  'size_o64',
        },
        # VEX.L value.
        'vex.l': {
            '0':    'vexl_0',
            '1':    'vexl_1',
        },
        # Execution ring.
        'ring': {
            '0':    'ring_0',
            '1':    'ring_1',
            '2':    'ring_2',
            '3':    'ring_3',
            '0..2': 'ring_0_thru_2',
            '1..3': 'ring_1_thru_3',
        },
        # Basic code mode.
        'codebits': {
            '64':   'code_64bit',
            '32':   'code_32bit',
            '16':   'code_16bit',
        },
        # cpu modes.
        'mode': {
            'real': 'mode_real',
            'prot': 'mode_prot',
            'long': 'mode_long',
            'v86':  'mode_v86',
            'smm':  'mode_smm',
            'vmx':  'mode_vmx',
            'svm':  'mode_svm',
        },
        # paging on/off
        'paging': {
            'on':       'paging_on',
            'off':      'paging_off',
        },
        # CPU vendor
        'vendor': {
            'amd':      'vendor_amd',
            'intel':    'vendor_intel',
            'via':      'vendor_via',
        },
    };
    ## Selector shorthand predicates.
    ## These translates into variable expressions.
    kdPredicates = {
        'o16':          'size==o16',
        'o32':          'size==o32',
        'o64':          'size==o64',
        'ring0':        'ring==0',
        '!ring0':       'ring==1..3',
        'ring1':        'ring==1',
        'ring2':        'ring==2',
        'ring3':        'ring==3',
        'user':         'ring==3',
        'supervisor':   'ring==0..2',
        '16-bit':       'codebits==16',
        '32-bit':       'codebits==32',
        '64-bit':       'codebits==64',
        'real':         'mode==real',
        'prot':         'mode==prot',
        'long':         'mode==long',
        'v86':          'mode==v86',
        'smm':          'mode==smm',
        'vmx':          'mode==vmx',
        'svm':          'mode==svm',
        'paging':       'paging==on',
        '!paging':      'paging==off',
        'amd':          'vendor==amd',
        '!amd':         'vendor!=amd',
        'intel':        'vendor==intel',
        '!intel':       'vendor!=intel',
        'via':          'vendor==via',
        '!via':         'vendor!=via',
    };

    def __init__(self, sVariable, sOp, sValue):
        assert sVariable in self.kdVariables;
        assert sOp in self.kasCompareOps;
        assert sValue in self.kdVariables[sVariable];
        self.sVariable  = sVariable;
        self.sOp        = sOp;
        self.sValue     = sValue;


class InstructionTest(object):
    """
    Instruction test.
    """

    def __init__(self, oInstr): # type: (InstructionTest, Instruction)
        self.oInstr         = oInstr    # type: InstructionTest
        self.aoInputs       = []        # type: list(TestInOut)
        self.aoOutputs      = []        # type: list(TestInOut)
        self.aoSelectors    = []        # type: list(TestSelector)

    def toString(self, fRepr = False):
        """
        Converts it to string representation.
        """
        asWords = [];
        if self.aoSelectors:
            for oSelector in self.aoSelectors:
                asWords.append('%s%s%s' % (oSelector.sVariable, oSelector.sOp, oSelector.sValue,));
            asWords.append('/');

        for oModifier in self.aoInputs:
            asWords.append('%s%s%s:%s' % (oModifier.sField, oModifier.sOp, oModifier.sValue, oModifier.sType,));

        asWords.append('->');

        for oModifier in self.aoOutputs:
            asWords.append('%s%s%s:%s' % (oModifier.sField, oModifier.sOp, oModifier.sValue, oModifier.sType,));

        if fRepr:
            return '<' + ' '.join(asWords) + '>';
        return '  '.join(asWords);

    def __str__(self):
        """ Provide string represenation. """
        return self.toString(False);

    def __repr__(self):
        """ Provide unambigious string representation. """
        return self.toString(True);

class Operand(object):
    """
    Instruction operand.
    """

    def __init__(self, sWhere, sType):
        assert sWhere in g_kdOpLocations, sWhere;
        assert sType  in g_kdOpTypes, sType;
        self.sWhere = sWhere;           ##< g_kdOpLocations
        self.sType  = sType;            ##< g_kdOpTypes

    def usesModRM(self):
        """ Returns True if using some form of ModR/M encoding. """
        return self.sType[0] in ['E', 'G', 'M'];



class Instruction(object): # pylint: disable=too-many-instance-attributes
    """
    Instruction.
    """

    def __init__(self, sSrcFile, iLine):
        ## @name Core attributes.
        ## @{
        self.oParent        = None      # type: Instruction
        self.sMnemonic      = None;
        self.sBrief         = None;
        self.asDescSections = []        # type: list(str)
        self.aoMaps         = []        # type: list(InstructionMap)
        self.aoOperands     = []        # type: list(Operand)
        self.sPrefix        = None;     ##< Single prefix: None, 'none', 0x66, 0xf3, 0xf2
        self.sOpcode        = None      # type: str
        self.sSubOpcode     = None      # type: str
        self.sEncoding      = None;
        self.asFlTest       = None;
        self.asFlModify     = None;
        self.asFlUndefined  = None;
        self.asFlSet        = None;
        self.asFlClear      = None;
        self.dHints         = {};       ##< Dictionary of instruction hints, flags, whatnot. (Dictionary for speed; dummy value).
        self.sDisEnum       = None;     ##< OP_XXXX value.  Default is based on the uppercased mnemonic.
        self.asCpuIds       = [];       ##< The CPUID feature bit names for this instruction. If multiple, assume AND.
        self.asReqFeatures  = [];       ##< Which features are required to be enabled to run this instruction.
        self.aoTests        = []        # type: list(InstructionTest)
        self.sMinCpu        = None;     ##< Indicates the minimum CPU required for the instruction. Not set when oCpuExpr is.
        self.oCpuExpr       = None;     ##< Some CPU restriction expression...
        self.sGroup         = None;
        self.fUnused        = False;    ##< Unused instruction.
        self.fInvalid       = False;    ##< Invalid instruction (like UD2).
        self.sInvalidStyle  = None;     ##< Invalid behviour style (g_kdInvalidStyles),
        self.sXcptType      = None;     ##< Exception type (g_kdXcptTypes).
        ## @}

        ## @name Implementation attributes.
        ## @{
        self.sStats         = None;
        self.sFunction      = None;
        self.fStub          = False;
        self.fUdStub        = False;
        ## @}

        ## @name Decoding info
        ## @{
        self.sSrcFile       = sSrcFile;
        self.iLineCreated   = iLine;
        self.iLineCompleted = None;
        self.cOpTags        = 0;
        self.iLineFnIemOpMacro  = -1;
        self.iLineMnemonicMacro = -1;
        ## @}

        ## @name Intermediate input fields.
        ## @{
        self.sRawDisOpNo    = None;
        self.asRawDisParams = [];
        self.sRawIemOpFlags = None;
        self.sRawOldOpcodes = None;
        self.asCopyTests    = [];
        ## @}

    def toString(self, fRepr = False):
        """ Turn object into a string. """
        aasFields = [];

        aasFields.append(['opcode',    self.sOpcode]);
        if self.sPrefix:
            aasFields.append(['prefix', self.sPrefix]);
        aasFields.append(['mnemonic',  self.sMnemonic]);
        for iOperand, oOperand in enumerate(self.aoOperands):
            aasFields.append(['op%u' % (iOperand + 1,), '%s:%s' % (oOperand.sWhere, oOperand.sType,)]);
        if self.aoMaps:         aasFields.append(['maps', ','.join([oMap.sName for oMap in self.aoMaps])]);
        aasFields.append(['encoding',  self.sEncoding]);
        if self.dHints:         aasFields.append(['hints', ','.join(self.dHints.keys())]);
        aasFields.append(['disenum',   self.sDisEnum]);
        if self.asCpuIds:       aasFields.append(['cpuid', ','.join(self.asCpuIds)]);
        aasFields.append(['group',     self.sGroup]);
        if self.fUnused:        aasFields.append(['unused', 'True']);
        if self.fInvalid:       aasFields.append(['invalid', 'True']);
        aasFields.append(['invlstyle', self.sInvalidStyle]);
        aasFields.append(['fltest',    self.asFlTest]);
        aasFields.append(['flmodify',  self.asFlModify]);
        aasFields.append(['flundef',   self.asFlUndefined]);
        aasFields.append(['flset',     self.asFlSet]);
        aasFields.append(['flclear',   self.asFlClear]);
        aasFields.append(['mincpu',    self.sMinCpu]);
        aasFields.append(['stats',     self.sStats]);
        aasFields.append(['sFunction', self.sFunction]);
        if self.fStub:          aasFields.append(['fStub', 'True']);
        if self.fUdStub:        aasFields.append(['fUdStub', 'True']);
        if self.cOpTags:        aasFields.append(['optags', str(self.cOpTags)]);
        if self.iLineFnIemOpMacro  != -1: aasFields.append(['FNIEMOP_XXX', str(self.iLineFnIemOpMacro)]);
        if self.iLineMnemonicMacro != -1: aasFields.append(['IEMOP_MNEMMONICn', str(self.iLineMnemonicMacro)]);

        sRet = '<' if fRepr else '';
        for sField, sValue in aasFields:
            if sValue is not None:
                if len(sRet) > 1:
                    sRet += '; ';
                sRet += '%s=%s' % (sField, sValue,);
        if fRepr:
            sRet += '>';

        return sRet;

    def __str__(self):
        """ Provide string represenation. """
        return self.toString(False);

    def __repr__(self):
        """ Provide unambigious string representation. """
        return self.toString(True);

    def copy(self, oMap = None, sOpcode = None, sSubOpcode = None, sPrefix = None):
        """
        Makes a copy of the object for the purpose of putting in a different map
        or a different place in the current map.
        """
        oCopy = Instruction(self.sSrcFile, self.iLineCreated);

        oCopy.oParent           = self;
        oCopy.sMnemonic         = self.sMnemonic;
        oCopy.sBrief            = self.sBrief;
        oCopy.asDescSections    = list(self.asDescSections);
        oCopy.aoMaps            = [oMap,]    if oMap       else list(self.aoMaps);
        oCopy.aoOperands        = list(self.aoOperands); ## Deeper copy?
        oCopy.sPrefix           = sPrefix    if sPrefix    else self.sPrefix;
        oCopy.sOpcode           = sOpcode    if sOpcode    else self.sOpcode;
        oCopy.sSubOpcode        = sSubOpcode if sSubOpcode else self.sSubOpcode;
        oCopy.sEncoding         = self.sEncoding;
        oCopy.asFlTest          = self.asFlTest;
        oCopy.asFlModify        = self.asFlModify;
        oCopy.asFlUndefined     = self.asFlUndefined;
        oCopy.asFlSet           = self.asFlSet;
        oCopy.asFlClear         = self.asFlClear;
        oCopy.dHints            = dict(self.dHints);
        oCopy.sDisEnum          = self.sDisEnum;
        oCopy.asCpuIds          = list(self.asCpuIds);
        oCopy.asReqFeatures     = list(self.asReqFeatures);
        oCopy.aoTests           = list(self.aoTests); ## Deeper copy?
        oCopy.sMinCpu           = self.sMinCpu;
        oCopy.oCpuExpr          = self.oCpuExpr;
        oCopy.sGroup            = self.sGroup;
        oCopy.fUnused           = self.fUnused;
        oCopy.fInvalid          = self.fInvalid;
        oCopy.sInvalidStyle     = self.sInvalidStyle;
        oCopy.sXcptType         = self.sXcptType;

        oCopy.sStats            = self.sStats;
        oCopy.sFunction         = self.sFunction;
        oCopy.fStub             = self.fStub;
        oCopy.fUdStub           = self.fUdStub;

        oCopy.iLineCompleted    = self.iLineCompleted;
        oCopy.cOpTags           = self.cOpTags;
        oCopy.iLineFnIemOpMacro = self.iLineFnIemOpMacro;
        oCopy.iLineMnemonicMacro = self.iLineMnemonicMacro;

        oCopy.sRawDisOpNo       = self.sRawDisOpNo;
        oCopy.asRawDisParams    = list(self.asRawDisParams);
        oCopy.sRawIemOpFlags    = self.sRawIemOpFlags;
        oCopy.sRawOldOpcodes    = self.sRawOldOpcodes;
        oCopy.asCopyTests       = list(self.asCopyTests);

        return oCopy;

    def getOpcodeByte(self):
        """
        Decodes sOpcode into a byte range integer value.
        Raises exception if sOpcode is None or invalid.
        """
        if self.sOpcode is None:
            raise Exception('No opcode byte for %s!' % (self,));
        sOpcode = str(self.sOpcode);    # pylint type confusion workaround.

        # Full hex byte form.
        if sOpcode[:2] == '0x':
            return int(sOpcode, 16);

        # The /r form:
        if len(sOpcode) == 4 and sOpcode.startswith('/') and sOpcode[-1].isdigit():
            return int(sOpcode[-1:]) << 3;

        # The 11/r form:
        if len(sOpcode) == 4 and sOpcode.startswith('11/') and sOpcode[-1].isdigit():
            return (int(sOpcode[-1:]) << 3) | 0xc0;

        # The !11/r form (returns mod=1):
        ## @todo this doesn't really work...
        if len(sOpcode) == 5 and sOpcode.startswith('!11/') and sOpcode[-1].isdigit():
            return (int(sOpcode[-1:]) << 3) | 0x80;

        raise Exception('unsupported opcode byte spec "%s" for %s' % (sOpcode, self,));

    @staticmethod
    def _flagsToIntegerMask(asFlags):
        """
        Returns the integer mask value for asFlags.
        """
        uRet = 0;
        if asFlags:
            for sFlag in asFlags:
                sConstant = g_kdEFlagsMnemonics[sFlag];
                assert sConstant[0] != '!', sConstant
                uRet |= g_kdX86EFlagsConstants[sConstant];
        return uRet;

    def getTestedFlagsMask(self):
        """ Returns asFlTest into a integer mask value """
        return self._flagsToIntegerMask(self.asFlTest);

    def getModifiedFlagsMask(self):
        """ Returns asFlModify into a integer mask value """
        return self._flagsToIntegerMask(self.asFlModify);

    def getUndefinedFlagsMask(self):
        """ Returns asFlUndefined into a integer mask value """
        return self._flagsToIntegerMask(self.asFlUndefined);

    def getSetFlagsMask(self):
        """ Returns asFlSet into a integer mask value """
        return self._flagsToIntegerMask(self.asFlSet);

    def getClearedFlagsMask(self):
        """ Returns asFlClear into a integer mask value """
        return self._flagsToIntegerMask(self.asFlClear);

    def onlyInVexMaps(self):
        """ Returns True if only in VEX maps, otherwise False.  (No maps -> False) """
        if not self.aoMaps:
            return False;
        for oMap in self.aoMaps:
            if not oMap.isVexMap():
                return False;
        return True;



## All the instructions.
g_aoAllInstructions = []            # type: list(Instruction)

## All the instructions indexed by statistics name (opstat).
g_dAllInstructionsByStat = {}       # type: dict(Instruction)

## All the instructions indexed by function name (opfunction).
g_dAllInstructionsByFunction = {}   # type: dict(list(Instruction))

## Instructions tagged by oponlytest
g_aoOnlyTestInstructions = []       # type: list(Instruction)

## Instruction maps.
g_aoInstructionMaps = [
    InstructionMap('one',        'g_apfnOneByteMap',        sSelector = 'byte'),
    InstructionMap('grp1_80',    asLeadOpcodes = ['0x80',], sSelector = '/r'),
    InstructionMap('grp1_81',    asLeadOpcodes = ['0x81',], sSelector = '/r'),
    InstructionMap('grp1_82',    asLeadOpcodes = ['0x82',], sSelector = '/r'),
    InstructionMap('grp1_83',    asLeadOpcodes = ['0x83',], sSelector = '/r'),
    InstructionMap('grp1a',      asLeadOpcodes = ['0x8f',], sSelector = '/r'),
    InstructionMap('grp2_c0',    asLeadOpcodes = ['0xc0',], sSelector = '/r'),
    InstructionMap('grp2_c1',    asLeadOpcodes = ['0xc1',], sSelector = '/r'),
    InstructionMap('grp2_d0',    asLeadOpcodes = ['0xd0',], sSelector = '/r'),
    InstructionMap('grp2_d1',    asLeadOpcodes = ['0xd1',], sSelector = '/r'),
    InstructionMap('grp2_d2',    asLeadOpcodes = ['0xd2',], sSelector = '/r'),
    InstructionMap('grp2_d3',    asLeadOpcodes = ['0xd3',], sSelector = '/r'),
    ## @todo g_apfnEscF1_E0toFF
    InstructionMap('grp3_f6',    asLeadOpcodes = ['0xf6',], sSelector = '/r'),
    InstructionMap('grp3_f7',    asLeadOpcodes = ['0xf7',], sSelector = '/r'),
    InstructionMap('grp4',       asLeadOpcodes = ['0xfe',], sSelector = '/r'),
    InstructionMap('grp5',       asLeadOpcodes = ['0xff',], sSelector = '/r'),
    InstructionMap('grp11_c6_m', asLeadOpcodes = ['0xc6',], sSelector = '!11 /r'),
    InstructionMap('grp11_c6_r', asLeadOpcodes = ['0xc6',], sSelector = '11'),    # xabort
    InstructionMap('grp11_c7_m', asLeadOpcodes = ['0xc7',], sSelector = '!11 /r'),
    InstructionMap('grp11_c7_r', asLeadOpcodes = ['0xc7',], sSelector = '11'),    # xbegin

    InstructionMap('two0f',      'g_apfnTwoByteMap',    asLeadOpcodes = ['0x0f',], sDisParse = 'IDX_ParseTwoByteEsc'),
    InstructionMap('grp6',       'g_apfnGroup6',        asLeadOpcodes = ['0x0f', '0x00',], sSelector = '/r'),
    InstructionMap('grp7_m',     'g_apfnGroup7Mem',     asLeadOpcodes = ['0x0f', '0x01',], sSelector = '!11 /r'),
    InstructionMap('grp7_r',                            asLeadOpcodes = ['0x0f', '0x01',], sSelector = '11'),
    InstructionMap('grp8',                              asLeadOpcodes = ['0x0f', '0xba',], sSelector = '/r'),
    InstructionMap('grp9',       'g_apfnGroup9RegReg',  asLeadOpcodes = ['0x0f', '0xc7',], sSelector = 'mod /r'),
    ## @todo What about g_apfnGroup9MemReg?
    InstructionMap('grp10',      None,                  asLeadOpcodes = ['0x0f', '0xb9',], sSelector = '/r'), # UD1 /w modr/m
    InstructionMap('grp12',      'g_apfnGroup12RegReg', asLeadOpcodes = ['0x0f', '0x71',], sSelector = 'mod /r'),
    InstructionMap('grp13',      'g_apfnGroup13RegReg', asLeadOpcodes = ['0x0f', '0x72',], sSelector = 'mod /r'),
    InstructionMap('grp14',      'g_apfnGroup14RegReg', asLeadOpcodes = ['0x0f', '0x73',], sSelector = 'mod /r'),
    InstructionMap('grp15',      'g_apfnGroup15MemReg', asLeadOpcodes = ['0x0f', '0xae',], sSelector = 'memreg /r'),
    ## @todo What about g_apfnGroup15RegReg?
    InstructionMap('grp16',       asLeadOpcodes = ['0x0f', '0x18',], sSelector = 'mod /r'),
    InstructionMap('grpA17',      asLeadOpcodes = ['0x0f', '0x78',], sSelector = '/r'), # AMD: EXTRQ weirdness
    InstructionMap('grpP',        asLeadOpcodes = ['0x0f', '0x0d',], sSelector = '/r'), # AMD: prefetch

    InstructionMap('three0f38',  'g_apfnThreeByte0f38',    asLeadOpcodes = ['0x0f', '0x38',]),
    InstructionMap('three0f3a',  'g_apfnThreeByte0f3a',    asLeadOpcodes = ['0x0f', '0x3a',]),

    InstructionMap('vexmap1',  'g_apfnVexMap1',          sEncoding = 'vex1'),
    InstructionMap('vexgrp12', 'g_apfnVexGroup12RegReg', sEncoding = 'vex1', asLeadOpcodes = ['0x71',], sSelector = 'mod /r'),
    InstructionMap('vexgrp13', 'g_apfnVexGroup13RegReg', sEncoding = 'vex1', asLeadOpcodes = ['0x72',], sSelector = 'mod /r'),
    InstructionMap('vexgrp14', 'g_apfnVexGroup14RegReg', sEncoding = 'vex1', asLeadOpcodes = ['0x73',], sSelector = 'mod /r'),
    InstructionMap('vexgrp15', 'g_apfnVexGroup15MemReg', sEncoding = 'vex1', asLeadOpcodes = ['0xae',], sSelector = 'memreg /r'),
    InstructionMap('vexgrp17', 'g_apfnVexGroup17_f3',    sEncoding = 'vex1', asLeadOpcodes = ['0xf3',], sSelector = '/r'),

    InstructionMap('vexmap2',  'g_apfnVexMap2',          sEncoding = 'vex2'),
    InstructionMap('vexmap3',  'g_apfnVexMap3',          sEncoding = 'vex3'),

    InstructionMap('3dnow',    asLeadOpcodes = ['0x0f', '0x0f',]),
    InstructionMap('xopmap8',  sEncoding = 'xop8'),
    InstructionMap('xopmap9',  sEncoding = 'xop9'),
    InstructionMap('xopgrp1',  sEncoding = 'xop9',  asLeadOpcodes = ['0x01'], sSelector = '/r'),
    InstructionMap('xopgrp2',  sEncoding = 'xop9',  asLeadOpcodes = ['0x02'], sSelector = '/r'),
    InstructionMap('xopgrp3',  sEncoding = 'xop9',  asLeadOpcodes = ['0x12'], sSelector = '/r'),
    InstructionMap('xopmap10', sEncoding = 'xop10'),
    InstructionMap('xopgrp4',  sEncoding = 'xop10', asLeadOpcodes = ['0x12'], sSelector = '/r'),
];
g_dInstructionMaps          = { oMap.sName:    oMap for oMap in g_aoInstructionMaps };
g_dInstructionMapsByIemName = { oMap.sIemName: oMap for oMap in g_aoInstructionMaps };



class ParserException(Exception):
    """ Parser exception """
    def __init__(self, sMessage):
        Exception.__init__(self, sMessage);


class SimpleParser(object):
    """
    Parser of IEMAllInstruction*.cpp.h instruction specifications.
    """

    ## @name Parser state.
    ## @{
    kiCode              = 0;
    kiCommentMulti      = 1;
    ## @}

    def __init__(self, sSrcFile, asLines, sDefaultMap):
        self.sSrcFile       = sSrcFile;
        self.asLines        = asLines;
        self.iLine          = 0;
        self.iState         = self.kiCode;
        self.sComment       = '';
        self.iCommentLine   = 0;
        self.aoCurInstrs    = [];

        assert sDefaultMap in g_dInstructionMaps;
        self.oDefaultMap    = g_dInstructionMaps[sDefaultMap];

        self.cTotalInstr    = 0;
        self.cTotalStubs    = 0;
        self.cTotalTagged   = 0;

        self.oReMacroName   = re.compile('^[A-Za-z_][A-Za-z0-9_]*$');
        self.oReMnemonic    = re.compile('^[A-Za-z_][A-Za-z0-9_]*$');
        self.oReStatsName   = re.compile('^[A-Za-z_][A-Za-z0-9_]*$');
        self.oReFunctionName= re.compile('^iemOp_[A-Za-z_][A-Za-z0-9_]*$');
        self.oReGroupName   = re.compile('^og_[a-z0-9]+(|_[a-z0-9]+|_[a-z0-9]+_[a-z0-9]+)$');
        self.oReDisEnum     = re.compile('^OP_[A-Z0-9_]+$');
        self.oReFunTable    = re.compile('^(IEM_STATIC|static) +const +PFNIEMOP +g_apfn[A-Za-z0-9_]+ *\[ *\d* *\] *= *$');
        self.oReComment     = re.compile('//.*?$|/\*.*?\*/'); ## Full comments.
        self.fDebug         = True;

        self.dTagHandlers   = {
            '@opbrief':     self.parseTagOpBrief,
            '@opdesc':      self.parseTagOpDesc,
            '@opmnemonic':  self.parseTagOpMnemonic,
            '@op1':         self.parseTagOpOperandN,
            '@op2':         self.parseTagOpOperandN,
            '@op3':         self.parseTagOpOperandN,
            '@op4':         self.parseTagOpOperandN,
            '@oppfx':       self.parseTagOpPfx,
            '@opmaps':      self.parseTagOpMaps,
            '@opcode':      self.parseTagOpcode,
            '@opcodesub':   self.parseTagOpcodeSub,
            '@openc':       self.parseTagOpEnc,
            '@opfltest':    self.parseTagOpEFlags,
            '@opflmodify':  self.parseTagOpEFlags,
            '@opflundef':   self.parseTagOpEFlags,
            '@opflset':     self.parseTagOpEFlags,
            '@opflclear':   self.parseTagOpEFlags,
            '@ophints':     self.parseTagOpHints,
            '@opdisenum':   self.parseTagOpDisEnum,
            '@opmincpu':    self.parseTagOpMinCpu,
            '@opcpuid':     self.parseTagOpCpuId,
            '@opgroup':     self.parseTagOpGroup,
            '@opunused':    self.parseTagOpUnusedInvalid,
            '@opinvalid':   self.parseTagOpUnusedInvalid,
            '@opinvlstyle': self.parseTagOpUnusedInvalid,
            '@optest':      self.parseTagOpTest,
            '@optestign':   self.parseTagOpTestIgnore,
            '@optestignore': self.parseTagOpTestIgnore,
            '@opcopytests': self.parseTagOpCopyTests,
            '@oponly':      self.parseTagOpOnlyTest,
            '@oponlytest':  self.parseTagOpOnlyTest,
            '@opxcpttype':  self.parseTagOpXcptType,
            '@opstats':     self.parseTagOpStats,
            '@opfunction':  self.parseTagOpFunction,
            '@opdone':      self.parseTagOpDone,
        };
        for i in range(48):
            self.dTagHandlers['@optest%u' % (i,)]   = self.parseTagOpTestNum;
            self.dTagHandlers['@optest[%u]' % (i,)] = self.parseTagOpTestNum;

        self.asErrors = [];

    def raiseError(self, sMessage):
        """
        Raise error prefixed with the source and line number.
        """
        raise ParserException("%s:%d: error: %s" % (self.sSrcFile, self.iLine, sMessage,));

    def raiseCommentError(self, iLineInComment, sMessage):
        """
        Similar to raiseError, but the line number is iLineInComment + self.iCommentLine.
        """
        raise ParserException("%s:%d: error: %s" % (self.sSrcFile, self.iCommentLine + iLineInComment, sMessage,));

    def error(self, sMessage):
        """
        Adds an error.
        returns False;
        """
        self.asErrors.append(u'%s:%d: error: %s\n' % (self.sSrcFile, self.iLine, sMessage,));
        return False;

    def errorOnLine(self, iLine, sMessage):
        """
        Adds an error.
        returns False;
        """
        self.asErrors.append(u'%s:%d: error: %s\n' % (self.sSrcFile, iLine, sMessage,));
        return False;

    def errorComment(self, iLineInComment, sMessage):
        """
        Adds a comment error.
        returns False;
        """
        self.asErrors.append(u'%s:%d: error: %s\n' % (self.sSrcFile, self.iCommentLine + iLineInComment, sMessage,));
        return False;

    def printErrors(self):
        """
        Print the errors to stderr.
        Returns number of errors.
        """
        if self.asErrors:
            sys.stderr.write(u''.join(self.asErrors));
        return len(self.asErrors);

    def debug(self, sMessage):
        """
        For debugging.
        """
        if self.fDebug:
            print('debug: %s' % (sMessage,));

    def stripComments(self, sLine):
        """
        Returns sLine with comments stripped.

        Complains if traces of incomplete multi-line comments are encountered.
        """
        sLine = self.oReComment.sub(" ", sLine);
        if sLine.find('/*') >= 0 or sLine.find('*/') >= 0:
            self.error('Unexpected multi-line comment will not be handled correctly. Please simplify.');
        return sLine;

    def parseFunctionTable(self, sLine):
        """
        Parses a PFNIEMOP table, updating/checking the @oppfx value.

        Note! Updates iLine as it consumes the whole table.
        """

        #
        # Extract the table name.
        #
        sName = re.search(' *([a-zA-Z_0-9]+) *\[', sLine).group(1);
        oMap  = g_dInstructionMapsByIemName.get(sName);
        if not oMap:
            self.debug('No map for PFNIEMOP table: %s' % (sName,));
            oMap = self.oDefaultMap; # This is wrong wrong wrong.

        #
        # All but the g_apfnOneByteMap & g_apfnEscF1_E0toFF tables uses four
        # entries per byte:
        #       no prefix, 066h prefix, f3h prefix, f2h prefix
        # Those tables has 256 & 32 entries respectively.
        #
        cEntriesPerByte   = 4;
        cValidTableLength = 1024;
        asPrefixes        = ('none', '0x66', '0xf3', '0xf2');

        oEntriesMatch = re.search('\[ *(256|32) *\]', sLine);
        if oEntriesMatch:
            cEntriesPerByte   = 1;
            cValidTableLength = int(oEntriesMatch.group(1));
            asPrefixes        = (None,);

        #
        # The next line should be '{' and nothing else.
        #
        if self.iLine >= len(self.asLines) or not re.match('^ *{ *$', self.asLines[self.iLine]):
            return self.errorOnLine(self.iLine + 1, 'Expected lone "{" on line following PFNIEMOP table %s start' % (sName, ));
        self.iLine += 1;

        #
        # Parse till we find the end of the table.
        #
        iEntry = 0;
        while self.iLine < len(self.asLines):
            # Get the next line and strip comments and spaces (assumes no
            # multi-line comments).
            sLine = self.asLines[self.iLine];
            self.iLine += 1;
            sLine = self.stripComments(sLine).strip();

            # Split the line up into entries, expanding IEMOP_X4 usage.
            asEntries = sLine.split(',');
            for i in range(len(asEntries) - 1, -1, -1):
                sEntry = asEntries[i].strip();
                if sEntry.startswith('IEMOP_X4(') and sEntry[-1] == ')':
                    sEntry = (sEntry[len('IEMOP_X4('):-1]).strip();
                    asEntries.insert(i + 1, sEntry);
                    asEntries.insert(i + 1, sEntry);
                    asEntries.insert(i + 1, sEntry);
                if sEntry:
                    asEntries[i] = sEntry;
                else:
                    del asEntries[i];

            # Process the entries.
            for sEntry in asEntries:
                if sEntry in ('};', '}'):
                    if iEntry != cValidTableLength:
                        return self.error('Wrong table length for %s: %#x, expected %#x' % (sName, iEntry, cValidTableLength, ));
                    return True;
                if sEntry.startswith('iemOp_Invalid'):
                    pass; # skip
                else:
                    # Look up matching instruction by function.
                    sPrefix = asPrefixes[iEntry % cEntriesPerByte];
                    sOpcode = '%#04x' % (iEntry // cEntriesPerByte);
                    aoInstr  = g_dAllInstructionsByFunction.get(sEntry);
                    if aoInstr:
                        if not isinstance(aoInstr, list):
                            aoInstr = [aoInstr,];
                        oInstr = None;
                        for oCurInstr in aoInstr:
                            if oCurInstr.sOpcode == sOpcode and oCurInstr.sPrefix == sPrefix:
                                pass;
                            elif oCurInstr.sOpcode == sOpcode and oCurInstr.sPrefix is None:
                                oCurInstr.sPrefix = sPrefix;
                            elif oCurInstr.sOpcode is None and oCurInstr.sPrefix is None:
                                oCurInstr.sOpcode = sOpcode;
                                oCurInstr.sPrefix = sPrefix;
                            else:
                                continue;
                            oInstr = oCurInstr;
                            break;
                        if not oInstr:
                            oInstr = aoInstr[0].copy(oMap = oMap, sOpcode = sOpcode, sPrefix = sPrefix);
                            aoInstr.append(oInstr);
                            g_dAllInstructionsByFunction[sEntry] = aoInstr;
                            g_aoAllInstructions.append(oInstr);
                            oMap.aoInstructions.append(oInstr);
                    else:
                        self.debug('Function "%s", entry %#04x / byte %#04x in %s, is not associated with an instruction.'
                                   % (sEntry, iEntry, iEntry // cEntriesPerByte, sName,));
                iEntry += 1;

        return self.error('Unexpected end of file in PFNIEMOP table');

    def addInstruction(self, iLine = None):
        """
        Adds an instruction.
        """
        oInstr = Instruction(self.sSrcFile, self.iLine if iLine is None else iLine);
        g_aoAllInstructions.append(oInstr);
        self.aoCurInstrs.append(oInstr);
        return oInstr;

    def deriveMnemonicAndOperandsFromStats(self, oInstr, sStats):
        """
        Derives the mnemonic and operands from a IEM stats base name like string.
        """
        if oInstr.sMnemonic is None:
            asWords = sStats.split('_');
            oInstr.sMnemonic = asWords[0].lower();
            if len(asWords) > 1 and not oInstr.aoOperands:
                for sType in asWords[1:]:
                    if sType in g_kdOpTypes:
                        oInstr.aoOperands.append(Operand(g_kdOpTypes[sType][1], sType));
                    else:
                        #return self.error('unknown operand type: %s (instruction: %s)' % (sType, oInstr))
                        return False;
        return True;

    def doneInstructionOne(self, oInstr, iLine):
        """
        Complete the parsing by processing, validating and expanding raw inputs.
        """
        assert oInstr.iLineCompleted is None;
        oInstr.iLineCompleted = iLine;

        #
        # Specified instructions.
        #
        if oInstr.cOpTags > 0:
            if oInstr.sStats is None:
                pass;

        #
        # Unspecified legacy stuff.  We generally only got a few things to go on here.
        #   /** Opcode 0x0f 0x00 /0. */
        #   FNIEMOPRM_DEF(iemOp_Grp6_sldt)
        #
        else:
            #if oInstr.sRawOldOpcodes:
            #
            #if oInstr.sMnemonic:
            pass;

        #
        # Common defaults.
        #

        # Guess mnemonic and operands from stats if the former is missing.
        if oInstr.sMnemonic is None:
            if oInstr.sStats is not None:
                self.deriveMnemonicAndOperandsFromStats(oInstr, oInstr.sStats);
            elif oInstr.sFunction is not None:
                self.deriveMnemonicAndOperandsFromStats(oInstr, oInstr.sFunction.replace('iemOp_', ''));

        # Derive the disassembler op enum constant from the mnemonic.
        if oInstr.sDisEnum is None and oInstr.sMnemonic is not None:
            oInstr.sDisEnum = 'OP_' + oInstr.sMnemonic.upper();

        # Derive the IEM statistics base name from mnemonic and operand types.
        if oInstr.sStats is None:
            if oInstr.sFunction is not None:
                oInstr.sStats = oInstr.sFunction.replace('iemOp_', '');
            elif oInstr.sMnemonic is not None:
                oInstr.sStats = oInstr.sMnemonic;
                for oOperand in oInstr.aoOperands:
                    if oOperand.sType:
                        oInstr.sStats += '_' + oOperand.sType;

        # Derive the IEM function name from mnemonic and operand types.
        if oInstr.sFunction is None:
            if oInstr.sMnemonic is not None:
                oInstr.sFunction = 'iemOp_' + oInstr.sMnemonic;
                for oOperand in oInstr.aoOperands:
                    if oOperand.sType:
                        oInstr.sFunction += '_' + oOperand.sType;
            elif oInstr.sStats:
                oInstr.sFunction = 'iemOp_' + oInstr.sStats;

        #
        # Apply default map and then add the instruction to all it's groups.
        #
        if not oInstr.aoMaps:
            oInstr.aoMaps = [ self.oDefaultMap, ];
        for oMap in oInstr.aoMaps:
            oMap.aoInstructions.append(oInstr);

        #
        # Derive encoding from operands and maps.
        #
        if oInstr.sEncoding is None:
            if not oInstr.aoOperands:
                if oInstr.fUnused and oInstr.sSubOpcode:
                    oInstr.sEncoding = 'VEX.ModR/M' if oInstr.onlyInVexMaps() else 'ModR/M';
                else:
                    oInstr.sEncoding = 'VEX.fixed' if oInstr.onlyInVexMaps() else 'fixed';
            elif oInstr.aoOperands[0].usesModRM():
                if     (len(oInstr.aoOperands) >= 2 and oInstr.aoOperands[1].sWhere == 'vvvv') \
                    or oInstr.onlyInVexMaps():
                    oInstr.sEncoding = 'VEX.ModR/M';
                else:
                    oInstr.sEncoding = 'ModR/M';

        #
        # Check the opstat value and add it to the opstat indexed dictionary.
        #
        if oInstr.sStats:
            if oInstr.sStats not in g_dAllInstructionsByStat:
                g_dAllInstructionsByStat[oInstr.sStats] = oInstr;
            else:
                self.error('Duplicate opstat value "%s"\nnew: %s\nold: %s'
                           % (oInstr.sStats, oInstr, g_dAllInstructionsByStat[oInstr.sStats],));

        #
        # Add to function indexed dictionary.  We allow multiple instructions per function.
        #
        if oInstr.sFunction:
            if oInstr.sFunction not in g_dAllInstructionsByFunction:
                g_dAllInstructionsByFunction[oInstr.sFunction] = [oInstr,];
            else:
                g_dAllInstructionsByFunction[oInstr.sFunction].append(oInstr);

        #self.debug('%d..%d: %s; %d @op tags' % (oInstr.iLineCreated, oInstr.iLineCompleted, oInstr.sFunction, oInstr.cOpTags));
        return True;

    def doneInstructions(self, iLineInComment = None):
        """
        Done with current instruction.
        """
        for oInstr in self.aoCurInstrs:
            self.doneInstructionOne(oInstr, self.iLine if iLineInComment is None else self.iCommentLine + iLineInComment);
            if oInstr.fStub:
                self.cTotalStubs += 1;

        self.cTotalInstr += len(self.aoCurInstrs);

        self.sComment     = '';
        self.aoCurInstrs  = [];
        return True;

    def setInstrunctionAttrib(self, sAttrib, oValue, fOverwrite = False):
        """
        Sets the sAttrib of all current instruction to oValue.  If fOverwrite
        is False, only None values and empty strings are replaced.
        """
        for oInstr in self.aoCurInstrs:
            if fOverwrite is not True:
                oOldValue = getattr(oInstr, sAttrib);
                if oOldValue is not None:
                    continue;
            setattr(oInstr, sAttrib, oValue);

    def setInstrunctionArrayAttrib(self, sAttrib, iEntry, oValue, fOverwrite = False):
        """
        Sets the iEntry of the array sAttrib of all current instruction to oValue.
        If fOverwrite is False, only None values and empty strings are replaced.
        """
        for oInstr in self.aoCurInstrs:
            aoArray = getattr(oInstr, sAttrib);
            while len(aoArray) <= iEntry:
                aoArray.append(None);
            if fOverwrite is True or aoArray[iEntry] is None:
                aoArray[iEntry] = oValue;

    def parseCommentOldOpcode(self, asLines):
        """ Deals with 'Opcode 0xff /4' like comments """
        asWords = asLines[0].split();
        if    len(asWords) >= 2  \
          and asWords[0] == 'Opcode'  \
          and (   asWords[1].startswith('0x')
               or asWords[1].startswith('0X')):
            asWords = asWords[:1];
            for iWord, sWord in enumerate(asWords):
                if sWord.startswith('0X'):
                    sWord = '0x' + sWord[:2];
                    asWords[iWord] = asWords;
            self.setInstrunctionAttrib('sRawOldOpcodes', ' '.join(asWords));

        return False;

    def ensureInstructionForOpTag(self, iTagLine):
        """ Ensure there is an instruction for the op-tag being parsed. """
        if not self.aoCurInstrs:
            self.addInstruction(self.iCommentLine + iTagLine);
        for oInstr in self.aoCurInstrs:
            oInstr.cOpTags += 1;
            if oInstr.cOpTags == 1:
                self.cTotalTagged += 1;
        return self.aoCurInstrs[-1];

    @staticmethod
    def flattenSections(aasSections):
        """
        Flattens multiline sections into stripped single strings.
        Returns list of strings, on section per string.
        """
        asRet = [];
        for asLines in aasSections:
            if asLines:
                asRet.append(' '.join([sLine.strip() for sLine in asLines]));
        return asRet;

    @staticmethod
    def flattenAllSections(aasSections, sLineSep = ' ', sSectionSep = '\n'):
        """
        Flattens sections into a simple stripped string with newlines as
        section breaks.  The final section does not sport a trailing newline.
        """
        # Typical: One section with a single line.
        if len(aasSections) == 1 and len(aasSections[0]) == 1:
            return aasSections[0][0].strip();

        sRet = '';
        for iSection, asLines in enumerate(aasSections):
            if asLines:
                if iSection > 0:
                    sRet += sSectionSep;
                sRet += sLineSep.join([sLine.strip() for sLine in asLines]);
        return sRet;



    ## @name Tag parsers
    ## @{

    def parseTagOpBrief(self, sTag, aasSections, iTagLine, iEndLine):
        """
        Tag:    \@opbrief
        Value:  Text description, multiple sections, appended.

        Brief description.  If not given, it's the first sentence from @opdesc.
        """
        oInstr = self.ensureInstructionForOpTag(iTagLine);

        # Flatten and validate the value.
        sBrief = self.flattenAllSections(aasSections);
        if not sBrief:
            return self.errorComment(iTagLine, '%s: value required' % (sTag,));
        if sBrief[-1] != '.':
            sBrief = sBrief + '.';
        if len(sBrief) > 180:
            return self.errorComment(iTagLine, '%s: value too long (max 180 chars): %s' % (sTag, sBrief));
        offDot = sBrief.find('.');
        while 0 <= offDot < len(sBrief) - 1 and sBrief[offDot + 1] != ' ':
            offDot = sBrief.find('.', offDot + 1);
        if offDot >= 0 and offDot != len(sBrief) - 1:
            return self.errorComment(iTagLine, '%s: only one sentence: %s' % (sTag, sBrief));

        # Update the instruction.
        if oInstr.sBrief is not None:
            return self.errorComment(iTagLine, '%s: attempting to overwrite brief "%s" with "%s"'
                                               % (sTag, oInstr.sBrief, sBrief,));
        _ = iEndLine;
        return True;

    def parseTagOpDesc(self, sTag, aasSections, iTagLine, iEndLine):
        """
        Tag:    \@opdesc
        Value:  Text description, multiple sections, appended.

        It is used to describe instructions.
        """
        oInstr = self.ensureInstructionForOpTag(iTagLine);
        if aasSections:
            oInstr.asDescSections.extend(self.flattenSections(aasSections));
            return True;

        _ = sTag; _ = iEndLine;
        return True;

    def parseTagOpMnemonic(self, sTag, aasSections, iTagLine, iEndLine):
        """
        Tag:    @opmenmonic
        Value:  mnemonic

        The 'mnemonic' value must be a valid C identifier string.  Because of
        prefixes, groups and whatnot, there times when the mnemonic isn't that
        of an actual assembler mnemonic.
        """
        oInstr = self.ensureInstructionForOpTag(iTagLine);

        # Flatten and validate the value.
        sMnemonic = self.flattenAllSections(aasSections);
        if not self.oReMnemonic.match(sMnemonic):
            return self.errorComment(iTagLine, '%s: invalid menmonic name: "%s"' % (sTag, sMnemonic,));
        if oInstr.sMnemonic is not None:
            return self.errorComment(iTagLine, '%s: attempting to overwrite menmonic "%s" with "%s"'
                                     % (sTag, oInstr.sMnemonic, sMnemonic,));
        oInstr.sMnemonic = sMnemonic

        _ = iEndLine;
        return True;

    def parseTagOpOperandN(self, sTag, aasSections, iTagLine, iEndLine):
        """
        Tags:  \@op1, \@op2, \@op3, \@op4
        Value: [where:]type

        The 'where' value indicates where the operand is found, like the 'reg'
        part of the ModR/M encoding. See Instruction.kdOperandLocations for
        a list.

        The 'type' value indicates the operand type.  These follow the types
        given in the opcode tables in the CPU reference manuals.
        See Instruction.kdOperandTypes for a list.

        """
        oInstr = self.ensureInstructionForOpTag(iTagLine);
        idxOp = int(sTag[-1]) - 1;
        assert 0 <= idxOp < 4;

        # flatten, split up, and validate the "where:type" value.
        sFlattened = self.flattenAllSections(aasSections);
        asSplit = sFlattened.split(':');
        if len(asSplit) == 1:
            sType  = asSplit[0];
            sWhere = None;
        elif len(asSplit) == 2:
            (sWhere, sType) = asSplit;
        else:
            return self.errorComment(iTagLine, 'expected %s value on format "[<where>:]<type>" not "%s"' % (sTag, sFlattened,));

        if sType not in g_kdOpTypes:
            return self.errorComment(iTagLine, '%s: invalid where value "%s", valid: %s'
                                               % (sTag, sType, ', '.join(g_kdOpTypes.keys()),));
        if sWhere is None:
            sWhere = g_kdOpTypes[sType][1];
        elif sWhere not in g_kdOpLocations:
            return self.errorComment(iTagLine, '%s: invalid where value "%s", valid: %s'
                                               % (sTag, sWhere, ', '.join(g_kdOpLocations.keys()),));

        # Insert the operand, refusing to overwrite an existing one.
        while idxOp >= len(oInstr.aoOperands):
            oInstr.aoOperands.append(None);
        if oInstr.aoOperands[idxOp] is not None:
            return self.errorComment(iTagLine, '%s: attempting to overwrite "%s:%s" with "%s:%s"'
                                               % ( sTag, oInstr.aoOperands[idxOp].sWhere, oInstr.aoOperands[idxOp].sType,
                                                   sWhere, sType,));
        oInstr.aoOperands[idxOp] = Operand(sWhere, sType);

        _ = iEndLine;
        return True;

    def parseTagOpMaps(self, sTag, aasSections, iTagLine, iEndLine):
        """
        Tag:    \@opmaps
        Value:  map[,map2]

        Indicates which maps the instruction is in.  There is a default map
        associated with each input file.
        """
        oInstr = self.ensureInstructionForOpTag(iTagLine);

        # Flatten, split up and validate the value.
        sFlattened = self.flattenAllSections(aasSections, sLineSep = ',', sSectionSep = ',');
        asMaps = sFlattened.split(',');
        if not asMaps:
            return self.errorComment(iTagLine, '%s: value required' % (sTag,));
        for sMap in asMaps:
            if sMap not in g_dInstructionMaps:
                return self.errorComment(iTagLine, '%s: invalid map value: %s  (valid values: %s)'
                                                   % (sTag, sMap, ', '.join(g_dInstructionMaps.keys()),));

        # Add the maps to the current list.  Throw errors on duplicates.
        for oMap in oInstr.aoMaps:
            if oMap.sName in asMaps:
                return self.errorComment(iTagLine, '%s: duplicate map assignment: %s' % (sTag, oMap.sName));

        for sMap in asMaps:
            oMap = g_dInstructionMaps[sMap];
            if oMap not in oInstr.aoMaps:
                oInstr.aoMaps.append(oMap);
            else:
                self.errorComment(iTagLine, '%s: duplicate map assignment (input): %s' % (sTag, sMap));

        _ = iEndLine;
        return True;

    def parseTagOpPfx(self, sTag, aasSections, iTagLine, iEndLine):
        """
        Tag:        \@oppfx
        Value:      n/a|none|0x66|0xf3|0xf2

        Required prefix for the instruction.  (In a (E)VEX context this is the
        value of the 'pp' field rather than an actual prefix.)
        """
        oInstr = self.ensureInstructionForOpTag(iTagLine);

        # Flatten and validate the value.
        sFlattened = self.flattenAllSections(aasSections);
        asPrefixes = sFlattened.split();
        if len(asPrefixes) > 1:
            return self.errorComment(iTagLine, '%s: max one prefix: %s' % (sTag, asPrefixes,));

        sPrefix = asPrefixes[0].lower();
        if sPrefix == 'none':
            sPrefix = 'none';
        elif sPrefix == 'n/a':
            sPrefix = None;
        else:
            if len(sPrefix) == 2:
                sPrefix = '0x' + sPrefix;
            if not _isValidOpcodeByte(sPrefix):
                return self.errorComment(iTagLine, '%s: invalid prefix: %s' % (sTag, sPrefix,));

        if sPrefix is not None and sPrefix not in g_kdPrefixes:
            return self.errorComment(iTagLine, '%s: invalid prefix: %s (valid %s)' % (sTag, sPrefix, g_kdPrefixes,));

        # Set it.
        if oInstr.sPrefix is not None:
            return self.errorComment(iTagLine, '%s: attempting to overwrite "%s" with "%s"' % ( sTag, oInstr.sPrefix, sPrefix,));
        oInstr.sPrefix = sPrefix;

        _ = iEndLine;
        return True;

    def parseTagOpcode(self, sTag, aasSections, iTagLine, iEndLine):
        """
        Tag:        \@opcode
        Value:      0x?? | /reg (TODO: | mr/reg | 11 /reg | !11 /reg | 11 mr/reg | !11 mr/reg)

        The opcode byte or sub-byte for the instruction in the context of a map.
        """
        oInstr = self.ensureInstructionForOpTag(iTagLine);

        # Flatten and validate the value.
        sOpcode = self.flattenAllSections(aasSections);
        if _isValidOpcodeByte(sOpcode):
            pass;
        elif len(sOpcode) == 2 and sOpcode.startswith('/') and sOpcode[-1] in '012345678':
            pass;
        elif len(sOpcode) == 4 and sOpcode.startswith('11/') and sOpcode[-1] in '012345678':
            pass;
        elif len(sOpcode) == 5 and sOpcode.startswith('!11/') and sOpcode[-1] in '012345678':
            pass;
        else:
            return self.errorComment(iTagLine, '%s: invalid opcode: %s' % (sTag, sOpcode,));

        # Set it.
        if oInstr.sOpcode is not None:
            return self.errorComment(iTagLine, '%s: attempting to overwrite "%s" with "%s"' % ( sTag, oInstr.sOpcode, sOpcode,));
        oInstr.sOpcode = sOpcode;

        _ = iEndLine;
        return True;

    def parseTagOpcodeSub(self, sTag, aasSections, iTagLine, iEndLine):
        """
        Tag:        \@opcodesub
        Value:      none | 11 mr/reg | !11 mr/reg | rex.w=0 | rex.w=1 | vex.l=0 | vex.l=1
                    | 11 mr/reg vex.l=0 | 11 mr/reg vex.l=1 | !11 mr/reg vex.l=0 | !11 mr/reg vex.l=1

        This is a simple way of dealing with encodings where the mod=3 and mod!=3
        represents exactly two different instructions.  The more proper way would
        be to go via maps with two members, but this is faster.
        """
        oInstr = self.ensureInstructionForOpTag(iTagLine);

        # Flatten and validate the value.
        sSubOpcode = self.flattenAllSections(aasSections);
        if sSubOpcode not in g_kdSubOpcodes:
            return self.errorComment(iTagLine, '%s: invalid sub opcode: %s  (valid: 11, !11, none)' % (sTag, sSubOpcode,));
        sSubOpcode = g_kdSubOpcodes[sSubOpcode][0];

        # Set it.
        if oInstr.sSubOpcode is not None:
            return self.errorComment(iTagLine, '%s: attempting to overwrite "%s" with "%s"'
                                               % ( sTag, oInstr.sSubOpcode, sSubOpcode,));
        oInstr.sSubOpcode = sSubOpcode;

        _ = iEndLine;
        return True;

    def parseTagOpEnc(self, sTag, aasSections, iTagLine, iEndLine):
        """
        Tag:        \@openc
        Value:      ModR/M|fixed|prefix|<map name>

        The instruction operand encoding style.
        """
        oInstr = self.ensureInstructionForOpTag(iTagLine);

        # Flatten and validate the value.
        sEncoding = self.flattenAllSections(aasSections);
        if sEncoding in g_kdEncodings:
            pass;
        elif sEncoding in g_dInstructionMaps:
            pass;
        elif not _isValidOpcodeByte(sEncoding):
            return self.errorComment(iTagLine, '%s: invalid encoding: %s' % (sTag, sEncoding,));

        # Set it.
        if oInstr.sEncoding is not None:
            return self.errorComment(iTagLine, '%s: attempting to overwrite "%s" with "%s"'
                                               % ( sTag, oInstr.sEncoding, sEncoding,));
        oInstr.sEncoding = sEncoding;

        _ = iEndLine;
        return True;

    ## EFlags tag to Instruction attribute name.
    kdOpFlagToAttr = {
        '@opfltest':    'asFlTest',
        '@opflmodify':  'asFlModify',
        '@opflundef':   'asFlUndefined',
        '@opflset':     'asFlSet',
        '@opflclear':   'asFlClear',
    };

    def parseTagOpEFlags(self, sTag, aasSections, iTagLine, iEndLine):
        """
        Tags:   \@opfltest, \@opflmodify, \@opflundef, \@opflset, \@opflclear
        Value:  <eflags specifier>

        """
        oInstr = self.ensureInstructionForOpTag(iTagLine);

        # Flatten, split up and validate the values.
        asFlags = self.flattenAllSections(aasSections, sLineSep = ',', sSectionSep = ',').split(',');
        if len(asFlags) == 1 and asFlags[0].lower() == 'none':
            asFlags = [];
        else:
            fRc = True;
            for iFlag, sFlag in enumerate(asFlags):
                if sFlag not in g_kdEFlagsMnemonics:
                    if sFlag.strip() in g_kdEFlagsMnemonics:
                        asFlags[iFlag] = sFlag.strip();
                    else:
                        fRc = self.errorComment(iTagLine, '%s: invalid EFLAGS value: %s' % (sTag, sFlag,));
            if not fRc:
                return False;

        # Set them.
        asOld = getattr(oInstr, self.kdOpFlagToAttr[sTag]);
        if asOld is not None:
            return self.errorComment(iTagLine, '%s: attempting to overwrite "%s" with "%s"' % ( sTag, asOld, asFlags,));
        setattr(oInstr, self.kdOpFlagToAttr[sTag], asFlags);

        _ = iEndLine;
        return True;

    def parseTagOpHints(self, sTag, aasSections, iTagLine, iEndLine):
        """
        Tag:        \@ophints
        Value:      Comma or space separated list of flags and hints.

        This covers the disassembler flags table and more.
        """
        oInstr = self.ensureInstructionForOpTag(iTagLine);

        # Flatten as a space separated list, split it up and validate the values.
        asHints = self.flattenAllSections(aasSections, sLineSep = ' ', sSectionSep = ' ').replace(',', ' ').split();
        if len(asHints) == 1 and asHints[0].lower() == 'none':
            asHints = [];
        else:
            fRc = True;
            for iHint, sHint in enumerate(asHints):
                if sHint not in g_kdHints:
                    if sHint.strip() in g_kdHints:
                        sHint[iHint] = sHint.strip();
                    else:
                        fRc = self.errorComment(iTagLine, '%s: invalid hint value: %s' % (sTag, sHint,));
            if not fRc:
                return False;

        # Append them.
        for sHint in asHints:
            if sHint not in oInstr.dHints:
                oInstr.dHints[sHint] = True; # (dummy value, using dictionary for speed)
            else:
                self.errorComment(iTagLine, '%s: duplicate hint: %s' % ( sTag, sHint,));

        _ = iEndLine;
        return True;

    def parseTagOpDisEnum(self, sTag, aasSections, iTagLine, iEndLine):
        """
        Tag:        \@opdisenum
        Value:      OP_XXXX

        This is for select a specific (legacy) disassembler enum value for the
        instruction.
        """
        oInstr = self.ensureInstructionForOpTag(iTagLine);

        # Flatten and split.
        asWords = self.flattenAllSections(aasSections).split();
        if len(asWords) != 1:
            self.errorComment(iTagLine, '%s: expected exactly one value: %s' % (sTag, asWords,));
            if not asWords:
                return False;
        sDisEnum = asWords[0];
        if not self.oReDisEnum.match(sDisEnum):
            return self.errorComment(iTagLine, '%s: invalid disassembler OP_XXXX enum: %s (pattern: %s)'
                                               % (sTag, sDisEnum, self.oReDisEnum.pattern));

        # Set it.
        if oInstr.sDisEnum is not None:
            return self.errorComment(iTagLine, '%s: attempting to overwrite "%s" with "%s"' % (sTag, oInstr.sDisEnum, sDisEnum,));
        oInstr.sDisEnum = sDisEnum;

        _ = iEndLine;
        return True;

    def parseTagOpMinCpu(self, sTag, aasSections, iTagLine, iEndLine):
        """
        Tag:        \@opmincpu
        Value:      <simple CPU name>

        Indicates when this instruction was introduced.
        """
        oInstr = self.ensureInstructionForOpTag(iTagLine);

        # Flatten the value, split into words, make sure there's just one, valid it.
        asCpus = self.flattenAllSections(aasSections).split();
        if len(asCpus) > 1:
            self.errorComment(iTagLine, '%s: exactly one CPU name, please: %s' % (sTag, ' '.join(asCpus),));

        sMinCpu = asCpus[0];
        if sMinCpu in g_kdCpuNames:
            oInstr.sMinCpu = sMinCpu;
        else:
            return self.errorComment(iTagLine, '%s: invalid CPU name: %s  (names: %s)'
                                               % (sTag, sMinCpu, ','.join(sorted(g_kdCpuNames)),));

        # Set it.
        if oInstr.sMinCpu is None:
            oInstr.sMinCpu = sMinCpu;
        elif oInstr.sMinCpu != sMinCpu:
            self.errorComment(iTagLine, '%s: attemting to overwrite "%s" with "%s"' % (sTag, oInstr.sMinCpu, sMinCpu,));

        _ = iEndLine;
        return True;

    def parseTagOpCpuId(self, sTag, aasSections, iTagLine, iEndLine):
        """
        Tag:        \@opcpuid
        Value:      none | <CPUID flag specifier>

        CPUID feature bit which is required for the instruction to be present.
        """
        oInstr = self.ensureInstructionForOpTag(iTagLine);

        # Flatten as a space separated list, split it up and validate the values.
        asCpuIds = self.flattenAllSections(aasSections, sLineSep = ' ', sSectionSep = ' ').replace(',', ' ').split();
        if len(asCpuIds) == 1 and asCpuIds[0].lower() == 'none':
            asCpuIds = [];
        else:
            fRc = True;
            for iCpuId, sCpuId in enumerate(asCpuIds):
                if sCpuId not in g_kdCpuIdFlags:
                    if sCpuId.strip() in g_kdCpuIdFlags:
                        sCpuId[iCpuId] = sCpuId.strip();
                    else:
                        fRc = self.errorComment(iTagLine, '%s: invalid CPUID value: %s' % (sTag, sCpuId,));
            if not fRc:
                return False;

        # Append them.
        for sCpuId in asCpuIds:
            if sCpuId not in oInstr.asCpuIds:
                oInstr.asCpuIds.append(sCpuId);
            else:
                self.errorComment(iTagLine, '%s: duplicate CPUID: %s' % ( sTag, sCpuId,));

        _ = iEndLine;
        return True;

    def parseTagOpGroup(self, sTag, aasSections, iTagLine, iEndLine):
        """
        Tag:        \@opgroup
        Value:      op_grp1[_subgrp2[_subsubgrp3]]

        Instruction grouping.
        """
        oInstr = self.ensureInstructionForOpTag(iTagLine);

        # Flatten as a space separated list, split it up and validate the values.
        asGroups = self.flattenAllSections(aasSections).split();
        if len(asGroups) != 1:
            return self.errorComment(iTagLine, '%s: exactly one group, please: %s' % (sTag, asGroups,));
        sGroup = asGroups[0];
        if not self.oReGroupName.match(sGroup):
            return self.errorComment(iTagLine, '%s: invalid group name: %s (valid: %s)'
                                               % (sTag, sGroup, self.oReGroupName.pattern));

        # Set it.
        if oInstr.sGroup is not None:
            return self.errorComment(iTagLine, '%s: attempting to overwrite "%s" with "%s"' % ( sTag, oInstr.sGroup, sGroup,));
        oInstr.sGroup = sGroup;

        _ = iEndLine;
        return True;

    def parseTagOpUnusedInvalid(self, sTag, aasSections, iTagLine, iEndLine):
        """
        Tag:    \@opunused, \@opinvalid, \@opinvlstyle
        Value:  <invalid opcode behaviour style>

        The \@opunused indicates the specification is for a currently unused
        instruction encoding.

        The \@opinvalid indicates the specification is for an invalid currently
        instruction encoding (like UD2).

        The \@opinvlstyle just indicates how CPUs decode the instruction when
        not supported (\@opcpuid, \@opmincpu) or disabled.
        """
        oInstr = self.ensureInstructionForOpTag(iTagLine);

        # Flatten as a space separated list, split it up and validate the values.
        asStyles = self.flattenAllSections(aasSections).split();
        if len(asStyles) != 1:
            return self.errorComment(iTagLine, '%s: exactly one invalid behviour style, please: %s' % (sTag, asStyles,));
        sStyle = asStyles[0];
        if sStyle not in g_kdInvalidStyles:
            return self.errorComment(iTagLine, '%s: invalid invalid behaviour style: %s (valid: %s)'
                                               % (sTag, sStyle, g_kdInvalidStyles.keys(),));
        # Set it.
        if oInstr.sInvalidStyle is not None:
            return self.errorComment(iTagLine,
                                     '%s: attempting to overwrite "%s" with "%s" (only one @opunused, @opinvalid, @opinvlstyle)'
                                     % ( sTag, oInstr.sInvalidStyle, sStyle,));
        oInstr.sInvalidStyle = sStyle;
        if sTag == '@opunused':
            oInstr.fUnused = True;
        elif sTag == '@opinvalid':
            oInstr.fInvalid = True;

        _ = iEndLine;
        return True;

    def parseTagOpTest(self, sTag, aasSections, iTagLine, iEndLine): # pylint: disable=too-many-locals
        """
        Tag:        \@optest
        Value:      [<selectors>[ ]?] <inputs> -> <outputs>
        Example:    mode==64bit / in1=0xfffffffe:dw in2=1:dw -> out1=0xffffffff:dw outfl=a?,p?

        The main idea here is to generate basic instruction tests.

        The probably simplest way of handling the diverse input, would be to use
        it to produce size optimized byte code for a simple interpreter that
        modifies the register input and output states.

        An alternative to the interpreter would be creating multiple tables,
        but that becomes rather complicated wrt what goes where and then to use
        them in an efficient manner.
        """
        oInstr = self.ensureInstructionForOpTag(iTagLine);

        #
        # Do it section by section.
        #
        for asSectionLines in aasSections:
            #
            # Sort the input into outputs, inputs and selector conditions.
            #
            sFlatSection = self.flattenAllSections([asSectionLines,]);
            if not sFlatSection:
                self.errorComment(iTagLine, '%s: missing value (dbg: aasSections=%s)' % ( sTag, aasSections));
                continue;
            oTest = InstructionTest(oInstr);

            asSelectors = [];
            asInputs    = [];
            asOutputs   = [];
            asCur   = asOutputs;
            fRc     = True;
            asWords = sFlatSection.split();
            for iWord in range(len(asWords) - 1, -1, -1):
                sWord = asWords[iWord];
                # Check for array switchers.
                if sWord == '->':
                    if asCur != asOutputs:
                        fRc = self.errorComment(iTagLine, '%s: "->" shall only occure once: %s' % (sTag, sFlatSection,));
                        break;
                    asCur = asInputs;
                elif sWord == '/':
                    if asCur != asInputs:
                        fRc = self.errorComment(iTagLine, '%s: "/" shall only occure once: %s' % (sTag, sFlatSection,));
                        break;
                    asCur = asSelectors;
                else:
                    asCur.insert(0, sWord);

            #
            # Validate and add selectors.
            #
            for sCond in asSelectors:
                sCondExp = TestSelector.kdPredicates.get(sCond, sCond);
                oSelector = None;
                for sOp in TestSelector.kasCompareOps:
                    off = sCondExp.find(sOp);
                    if off >= 0:
                        sVariable = sCondExp[:off];
                        sValue    = sCondExp[off + len(sOp):];
                        if sVariable in TestSelector.kdVariables:
                            if sValue in TestSelector.kdVariables[sVariable]:
                                oSelector = TestSelector(sVariable, sOp, sValue);
                            else:
                                self.errorComment(iTagLine, '%s: invalid condition value "%s" in "%s" (valid: %s)'
                                                             % ( sTag, sValue, sCond,
                                                                 TestSelector.kdVariables[sVariable].keys(),));
                        else:
                            self.errorComment(iTagLine, '%s: invalid condition variable "%s" in "%s" (valid: %s)'
                                                         % ( sTag, sVariable, sCond, TestSelector.kdVariables.keys(),));
                        break;
                if oSelector is not None:
                    for oExisting in oTest.aoSelectors:
                        if oExisting.sVariable == oSelector.sVariable:
                            self.errorComment(iTagLine, '%s: already have a selector for variable "%s" (existing: %s, new: %s)'
                                                         % ( sTag, oSelector.sVariable, oExisting, oSelector,));
                    oTest.aoSelectors.append(oSelector);
                else:
                    fRc = self.errorComment(iTagLine, '%s: failed to parse selector: %s' % ( sTag, sCond,));

            #
            # Validate outputs and inputs, adding them to the test as we go along.
            #
            for asItems, sDesc, aoDst in [ (asInputs, 'input', oTest.aoInputs), (asOutputs, 'output', oTest.aoOutputs)]:
                asValidFieldKinds = [ 'both', sDesc, ];
                for sItem in asItems:
                    oItem = None;
                    for sOp in TestInOut.kasOperators:
                        off = sItem.find(sOp);
                        if off < 0:
                            continue;
                        sField     = sItem[:off];
                        sValueType = sItem[off + len(sOp):];
                        if     sField in TestInOut.kdFields \
                           and TestInOut.kdFields[sField][1] in asValidFieldKinds:
                            asSplit = sValueType.split(':', 1);
                            sValue  = asSplit[0];
                            sType   = asSplit[1] if len(asSplit) > 1 else TestInOut.kdFields[sField][0];
                            if sType in TestInOut.kdTypes:
                                oValid = TestInOut.kdTypes[sType].validate(sValue);
                                if oValid is True:
                                    if not TestInOut.kdTypes[sType].isAndOrPair(sValue) or sOp == '&|=':
                                        oItem = TestInOut(sField, sOp, sValue, sType);
                                    else:
                                        self.errorComment(iTagLine, '%s: and-or %s value "%s" can only be used with "&|="'
                                                                    % ( sTag, sDesc, sItem, ));
                                else:
                                    self.errorComment(iTagLine, '%s: invalid %s value "%s" in "%s" (type: %s): %s'
                                                                % ( sTag, sDesc, sValue, sItem, sType, oValid, ));
                            else:
                                self.errorComment(iTagLine, '%s: invalid %s type "%s" in "%s" (valid types: %s)'
                                                             % ( sTag, sDesc, sType, sItem, TestInOut.kdTypes.keys(),));
                        else:
                            self.errorComment(iTagLine, '%s: invalid %s field "%s" in "%s"\nvalid fields: %s'
                                                         % ( sTag, sDesc, sField, sItem,
                                                             ', '.join([sKey for sKey, asVal in TestInOut.kdFields.items()
                                                                        if asVal[1] in asValidFieldKinds]),));
                        break;
                    if oItem is not None:
                        for oExisting in aoDst:
                            if oExisting.sField == oItem.sField and oExisting.sOp == oItem.sOp:
                                self.errorComment(iTagLine,
                                                  '%s: already have a "%s" assignment for field "%s" (existing: %s, new: %s)'
                                                  % ( sTag, oItem.sOp, oItem.sField, oExisting, oItem,));
                        aoDst.append(oItem);
                    else:
                        fRc = self.errorComment(iTagLine, '%s: failed to parse assignment: %s' % ( sTag, sItem,));

            #
            # .
            #
            if fRc:
                oInstr.aoTests.append(oTest);
            else:
                self.errorComment(iTagLine, '%s: failed to parse test: %s' % (sTag, ' '.join(asWords),));
                self.errorComment(iTagLine, '%s: asSelectors=%s / asInputs=%s -> asOutputs=%s'
                                            % (sTag, asSelectors, asInputs, asOutputs,));

        _ = iEndLine;
        return True;

    def parseTagOpTestNum(self, sTag, aasSections, iTagLine, iEndLine):
        """
        Numbered \@optest tag.  Either \@optest42 or \@optest[42].
        """
        oInstr = self.ensureInstructionForOpTag(iTagLine);

        iTest = 0;
        if sTag[-1] == ']':
            iTest = int(sTag[8:-1]);
        else:
            iTest = int(sTag[7:]);

        if iTest != len(oInstr.aoTests):
            self.errorComment(iTagLine, '%s: incorrect test number: %u, actual %u' % (sTag, iTest, len(oInstr.aoTests),));
        return self.parseTagOpTest(sTag, aasSections, iTagLine, iEndLine);

    def parseTagOpTestIgnore(self, sTag, aasSections, iTagLine, iEndLine):
        """
        Tag:        \@optestign | \@optestignore
        Value:      <value is ignored>

        This is a simple trick to ignore a test while debugging another.

        See also \@oponlytest.
        """
        _ = sTag; _ = aasSections; _ = iTagLine; _ = iEndLine;
        return True;

    def parseTagOpCopyTests(self, sTag, aasSections, iTagLine, iEndLine):
        """
        Tag:        \@opcopytests
        Value:      <opstat | function> [..]
        Example:    \@opcopytests add_Eb_Gb

        Trick to avoid duplicating tests for different encodings of the same
        operation.
        """
        oInstr = self.ensureInstructionForOpTag(iTagLine);

        # Flatten, validate and append the copy job to the instruction.  We execute
        # them after parsing all the input so we can handle forward references.
        asToCopy = self.flattenAllSections(aasSections).split();
        if not asToCopy:
            return self.errorComment(iTagLine, '%s: requires at least on reference value' % (sTag,));
        for sToCopy in asToCopy:
            if sToCopy not in oInstr.asCopyTests:
                if self.oReStatsName.match(sToCopy) or self.oReFunctionName.match(sToCopy):
                    oInstr.asCopyTests.append(sToCopy);
                else:
                    self.errorComment(iTagLine, '%s: invalid instruction reference (opstat or function) "%s" (valid: %s or %s)'
                                                % (sTag, sToCopy, self.oReStatsName.pattern, self.oReFunctionName.pattern));
            else:
                self.errorComment(iTagLine, '%s: ignoring duplicate "%s"' % (sTag, sToCopy,));

        _ = iEndLine;
        return True;

    def parseTagOpOnlyTest(self, sTag, aasSections, iTagLine, iEndLine):
        """
        Tag:        \@oponlytest | \@oponly
        Value:      none

        Only test instructions with this tag.  This is a trick that is handy
        for singling out one or two new instructions or tests.

        See also \@optestignore.
        """
        oInstr = self.ensureInstructionForOpTag(iTagLine);

        # Validate and add instruction to only test dictionary.
        sValue = self.flattenAllSections(aasSections).strip();
        if sValue:
            return self.errorComment(iTagLine, '%s: does not take any value: %s' % (sTag, sValue));

        if oInstr not in g_aoOnlyTestInstructions:
            g_aoOnlyTestInstructions.append(oInstr);

        _ = iEndLine;
        return True;

    def parseTagOpXcptType(self, sTag, aasSections, iTagLine, iEndLine):
        """
        Tag:        \@opxcpttype
        Value:      [none|1|2|3|4|4UA|5|6|7|8|11|12|E1|E1NF|E2|E3|E3NF|E4|E4NF|E5|E5NF|E6|E6NF|E7NF|E9|E9NF|E10|E11|E12|E12NF]

        Sets the SSE or AVX exception type (see SDMv2 2.4, 2.7).
        """
        oInstr = self.ensureInstructionForOpTag(iTagLine);

        # Flatten as a space separated list, split it up and validate the values.
        asTypes = self.flattenAllSections(aasSections).split();
        if len(asTypes) != 1:
            return self.errorComment(iTagLine, '%s: exactly one invalid exception type, please: %s' % (sTag, asTypes,));
        sType = asTypes[0];
        if sType not in g_kdXcptTypes:
            return self.errorComment(iTagLine, '%s: invalid invalid exception type: %s (valid: %s)'
                                               % (sTag, sType, sorted(g_kdXcptTypes.keys()),));
        # Set it.
        if oInstr.sXcptType is not None:
            return self.errorComment(iTagLine,
                                     '%s: attempting to overwrite "%s" with "%s" (only one @opxcpttype)'
                                     % ( sTag, oInstr.sXcptType, sType,));
        oInstr.sXcptType = sType;

        _ = iEndLine;
        return True;

    def parseTagOpFunction(self, sTag, aasSections, iTagLine, iEndLine):
        """
        Tag:        \@opfunction
        Value:      <VMM function name>

        This is for explicitly setting the IEM function name.  Normally we pick
        this up from the FNIEMOP_XXX macro invocation after the description, or
        generate it from the mnemonic and operands.

        It it thought it maybe necessary to set it when specifying instructions
        which implementation isn't following immediately or aren't implemented yet.
        """
        oInstr = self.ensureInstructionForOpTag(iTagLine);

        # Flatten and validate the value.
        sFunction = self.flattenAllSections(aasSections);
        if not self.oReFunctionName.match(sFunction):
            return self.errorComment(iTagLine, '%s: invalid VMM function name: "%s" (valid: %s)'
                                               % (sTag, sFunction, self.oReFunctionName.pattern));

        if oInstr.sFunction is not None:
            return self.errorComment(iTagLine, '%s: attempting to overwrite VMM function name "%s" with "%s"'
                                     % (sTag, oInstr.sFunction, sFunction,));
        oInstr.sFunction = sFunction;

        _ = iEndLine;
        return True;

    def parseTagOpStats(self, sTag, aasSections, iTagLine, iEndLine):
        """
        Tag:        \@opstats
        Value:      <VMM statistics base name>

        This is for explicitly setting the statistics name.  Normally we pick
        this up from the IEMOP_MNEMONIC macro invocation, or generate it from
        the mnemonic and operands.

        It it thought it maybe necessary to set it when specifying instructions
        which implementation isn't following immediately or aren't implemented yet.
        """
        oInstr = self.ensureInstructionForOpTag(iTagLine);

        # Flatten and validate the value.
        sStats = self.flattenAllSections(aasSections);
        if not self.oReStatsName.match(sStats):
            return self.errorComment(iTagLine, '%s: invalid VMM statistics name: "%s" (valid: %s)'
                                               % (sTag, sStats, self.oReStatsName.pattern));

        if oInstr.sStats is not None:
            return self.errorComment(iTagLine, '%s: attempting to overwrite VMM statistics base name "%s" with "%s"'
                                     % (sTag, oInstr.sStats, sStats,));
        oInstr.sStats = sStats;

        _ = iEndLine;
        return True;

    def parseTagOpDone(self, sTag, aasSections, iTagLine, iEndLine):
        """
        Tag:    \@opdone
        Value:  none

        Used to explictily flush the instructions that have been specified.
        """
        sFlattened = self.flattenAllSections(aasSections);
        if sFlattened != '':
            return self.errorComment(iTagLine, '%s: takes no value, found: "%s"' % (sTag, sFlattened,));
        _ = sTag; _ = iEndLine;
        return self.doneInstructions();

    ## @}


    def parseComment(self):
        """
        Parse the current comment (self.sComment).

        If it's a opcode specifiying comment, we reset the macro stuff.
        """
        #
        # Reject if comment doesn't seem to contain anything interesting.
        #
        if    self.sComment.find('Opcode') < 0 \
          and self.sComment.find('@') < 0:
            return False;

        #
        # Split the comment into lines, removing leading asterisks and spaces.
        # Also remove leading and trailing empty lines.
        #
        asLines = self.sComment.split('\n');
        for iLine, sLine in enumerate(asLines):
            asLines[iLine] = sLine.lstrip().lstrip('*').lstrip();

        while asLines and not asLines[0]:
            self.iCommentLine += 1;
            asLines.pop(0);

        while asLines and not asLines[-1]:
            asLines.pop(len(asLines) - 1);

        #
        # Check for old style: Opcode 0x0f 0x12
        #
        if asLines[0].startswith('Opcode '):
            self.parseCommentOldOpcode(asLines);

        #
        # Look for @op* tagged data.
        #
        cOpTags      = 0;
        sFlatDefault = None;
        sCurTag      = '@default';
        iCurTagLine  = 0;
        asCurSection = [];
        aasSections  = [ asCurSection, ];
        for iLine, sLine in enumerate(asLines):
            if not sLine.startswith('@'):
                if sLine:
                    asCurSection.append(sLine);
                elif asCurSection:
                    asCurSection = [];
                    aasSections.append(asCurSection);
            else:
                #
                # Process the previous tag.
                #
                if not asCurSection and len(aasSections) > 1:
                    aasSections.pop(-1);
                if sCurTag in self.dTagHandlers:
                    self.dTagHandlers[sCurTag](sCurTag, aasSections, iCurTagLine, iLine);
                    cOpTags += 1;
                elif sCurTag.startswith('@op'):
                    self.errorComment(iCurTagLine, 'Unknown tag: %s' % (sCurTag));
                elif sCurTag == '@default':
                    sFlatDefault = self.flattenAllSections(aasSections);
                elif '@op' + sCurTag[1:] in self.dTagHandlers:
                    self.errorComment(iCurTagLine, 'Did you mean "@op%s" rather than "%s"?' % (sCurTag[1:], sCurTag));
                elif sCurTag in ['@encoding', '@opencoding']:
                    self.errorComment(iCurTagLine, 'Did you mean "@openc" rather than "%s"?' % (sCurTag,));

                #
                # New tag.
                #
                asSplit = sLine.split(None, 1);
                sCurTag = asSplit[0].lower();
                if len(asSplit) > 1:
                    asCurSection = [asSplit[1],];
                else:
                    asCurSection = [];
                aasSections = [asCurSection, ];
                iCurTagLine = iLine;

        #
        # Process the final tag.
        #
        if not asCurSection and len(aasSections) > 1:
            aasSections.pop(-1);
        if sCurTag in self.dTagHandlers:
            self.dTagHandlers[sCurTag](sCurTag, aasSections, iCurTagLine, iLine);
            cOpTags += 1;
        elif sCurTag.startswith('@op'):
            self.errorComment(iCurTagLine, 'Unknown tag: %s' % (sCurTag));
        elif sCurTag == '@default':
            sFlatDefault = self.flattenAllSections(aasSections);

        #
        # Don't allow default text in blocks containing @op*.
        #
        if cOpTags > 0 and sFlatDefault:
            self.errorComment(0, 'Untagged comment text is not allowed with @op*: %s' % (sFlatDefault,));

        return True;

    def parseMacroInvocation(self, sInvocation):
        """
        Parses a macro invocation.

        Returns a tuple, first element is the offset following the macro
        invocation. The second element is a list of macro arguments, where the
        zero'th is the macro name.
        """
        # First the name.
        offOpen = sInvocation.find('(');
        if offOpen <= 0:
            self.raiseError("macro invocation open parenthesis not found");
        sName = sInvocation[:offOpen].strip();
        if not self.oReMacroName.match(sName):
            return self.error("invalid macro name '%s'" % (sName,));
        asRet = [sName, ];

        # Arguments.
        iLine    = self.iLine;
        cDepth   = 1;
        off      = offOpen + 1;
        offStart = off;
        chQuote  = None;
        while cDepth > 0:
            if off >= len(sInvocation):
                if iLine >= len(self.asLines):
                    self.error('macro invocation beyond end of file');
                    return (off, asRet);
                sInvocation += self.asLines[iLine];
                iLine += 1;
            ch = sInvocation[off];

            if chQuote:
                if ch == '\\' and off + 1 < len(sInvocation):
                    off += 1;
                elif ch == chQuote:
                    chQuote = None;
            elif ch in ('"', '\'',):
                chQuote = ch;
            elif ch in (',', ')',):
                if cDepth == 1:
                    asRet.append(sInvocation[offStart:off].strip());
                    offStart = off + 1;
                if ch == ')':
                    cDepth -= 1;
            elif ch == '(':
                cDepth += 1;
            off += 1;

        return (off, asRet);

    def findAndParseMacroInvocationEx(self, sCode, sMacro):
        """
        Returns (len(sCode), None) if not found, parseMacroInvocation result if found.
        """
        offHit = sCode.find(sMacro);
        if offHit >= 0 and sCode[offHit + len(sMacro):].strip()[0] == '(':
            offAfter, asRet = self.parseMacroInvocation(sCode[offHit:])
            return (offHit + offAfter, asRet);
        return (len(sCode), None);

    def findAndParseMacroInvocation(self, sCode, sMacro):
        """
        Returns None if not found, arguments as per parseMacroInvocation if found.
        """
        return self.findAndParseMacroInvocationEx(sCode, sMacro)[1];

    def findAndParseFirstMacroInvocation(self, sCode, asMacro):
        """
        Returns same as findAndParseMacroInvocation.
        """
        for sMacro in asMacro:
            asRet = self.findAndParseMacroInvocation(sCode, sMacro);
            if asRet is not None:
                return asRet;
        return None;

    def workerIemOpMnemonicEx(self, sMacro, sStats, sAsm, sForm, sUpper, sLower,  # pylint: disable=too-many-arguments
                              sDisHints, sIemHints, asOperands):
        """
        Processes one of the a IEMOP_MNEMONIC0EX, IEMOP_MNEMONIC1EX, IEMOP_MNEMONIC2EX,
        IEMOP_MNEMONIC3EX, and IEMOP_MNEMONIC4EX macros.
        """
        #
        # Some invocation checks.
        #
        if sUpper != sUpper.upper():
            self.error('%s: bad a_Upper parameter: %s' % (sMacro, sUpper,));
        if sLower != sLower.lower():
            self.error('%s: bad a_Lower parameter: %s' % (sMacro, sLower,));
        if sUpper.lower() != sLower:
            self.error('%s: a_Upper and a_Lower parameters does not match: %s vs %s' % (sMacro, sUpper, sLower,));
        if not self.oReMnemonic.match(sLower):
            self.error('%s: invalid a_Lower: %s  (valid: %s)' % (sMacro, sLower, self.oReMnemonic.pattern,));

        #
        # Check if sIemHints tells us to not consider this macro invocation.
        #
        if sIemHints.find('IEMOPHINT_SKIP_PYTHON') >= 0:
            return True;

        # Apply to the last instruction only for now.
        if not self.aoCurInstrs:
            self.addInstruction();
        oInstr = self.aoCurInstrs[-1];
        if oInstr.iLineMnemonicMacro == -1:
            oInstr.iLineMnemonicMacro = self.iLine;
        else:
            self.error('%s: already saw a IEMOP_MNEMONIC* macro on line %u for this instruction'
                       % (sMacro, oInstr.iLineMnemonicMacro,));

        # Mnemonic
        if oInstr.sMnemonic is None:
            oInstr.sMnemonic = sLower;
        elif oInstr.sMnemonic != sLower:
            self.error('%s: current instruction and a_Lower does not match: %s vs %s' % (sMacro, oInstr.sMnemonic, sLower,));

        # Process operands.
        if len(oInstr.aoOperands) not in [0, len(asOperands)]:
            self.error('%s: number of operands given by @opN does not match macro: %s vs %s'
                       % (sMacro, len(oInstr.aoOperands), len(asOperands),));
        for iOperand, sType in enumerate(asOperands):
            sWhere = g_kdOpTypes.get(sType, [None, None])[1];
            if sWhere is None:
                self.error('%s: unknown a_Op%u value: %s' % (sMacro, iOperand + 1, sType));
                if iOperand < len(oInstr.aoOperands): # error recovery.
                    sWhere = oInstr.aoOperands[iOperand].sWhere;
                    sType  = oInstr.aoOperands[iOperand].sType;
                else:
                    sWhere = 'reg';
                    sType  = 'Gb';
            if iOperand == len(oInstr.aoOperands):
                oInstr.aoOperands.append(Operand(sWhere, sType))
            elif oInstr.aoOperands[iOperand].sWhere != sWhere or oInstr.aoOperands[iOperand].sType != sType:
                self.error('%s: @op%u and a_Op%u mismatch: %s:%s vs %s:%s'
                           % (sMacro, iOperand + 1, iOperand + 1, oInstr.aoOperands[iOperand].sWhere,
                              oInstr.aoOperands[iOperand].sType, sWhere, sType,));

        # Encoding.
        if sForm not in g_kdIemForms:
            self.error('%s: unknown a_Form value: %s' % (sMacro, sForm,));
        else:
            if oInstr.sEncoding is None:
                oInstr.sEncoding = g_kdIemForms[sForm][0];
            elif g_kdIemForms[sForm][0] != oInstr.sEncoding:
                self.error('%s: current instruction @openc and a_Form does not match: %s vs %s (%s)'
                           % (sMacro, oInstr.sEncoding, g_kdIemForms[sForm], sForm));

            # Check the parameter locations for the encoding.
            if g_kdIemForms[sForm][1] is not None:
                if len(g_kdIemForms[sForm][1]) > len(oInstr.aoOperands):
                    self.error('%s: The a_Form=%s has a different operand count: %s (form) vs %s'
                               % (sMacro, sForm, len(g_kdIemForms[sForm][1]), len(oInstr.aoOperands) ));
                else:
                    for iOperand, sWhere in enumerate(g_kdIemForms[sForm][1]):
                        if oInstr.aoOperands[iOperand].sWhere != sWhere:
                            self.error('%s: current instruction @op%u and a_Form location does not match: %s vs %s (%s)'
                                       % (sMacro, iOperand + 1, oInstr.aoOperands[iOperand].sWhere, sWhere, sForm,));
                        sOpFormMatch = g_kdOpTypes[oInstr.aoOperands[iOperand].sType][4];
                        if    (sOpFormMatch in [ 'REG', 'MEM', ] and sForm.find('_' + sOpFormMatch) < 0) \
                           or (sOpFormMatch in [ 'FIXED', ]      and sForm.find(sOpFormMatch) < 0) \
                           or (sOpFormMatch == 'RM' and (sForm.find('_MEM') > 0 or sForm.find('_REG') > 0) ) \
                           or (sOpFormMatch == 'V'  and (   not (sForm.find('VEX') > 0 or sForm.find('XOP')) \
                                                         or sForm.replace('VEX','').find('V') < 0) ):
                            self.error('%s: current instruction @op%u and a_Form type does not match: %s/%s vs %s'
                                       % (sMacro, iOperand + 1, oInstr.aoOperands[iOperand].sType, sOpFormMatch, sForm, ));
                    if len(g_kdIemForms[sForm][1]) < len(oInstr.aoOperands):
                        for iOperand in range(len(g_kdIemForms[sForm][1]), len(oInstr.aoOperands)):
                            if    oInstr.aoOperands[iOperand].sType != 'FIXED' \
                              and g_kdOpTypes[oInstr.aoOperands[iOperand].sType][0] != 'IDX_ParseFixedReg':
                                self.error('%s: Expected FIXED type operand #%u following operands given by a_Form=%s: %s (%s)'
                                           % (sMacro, iOperand, sForm, oInstr.aoOperands[iOperand].sType,
                                              oInstr.aoOperands[iOperand].sWhere));


            # Check @opcodesub
            if oInstr.sSubOpcode \
              and g_kdIemForms[sForm][2] \
              and oInstr.sSubOpcode.find(g_kdIemForms[sForm][2]) < 0:
                self.error('%s: current instruction @opcodesub and a_Form does not match: %s vs %s (%s)'
                            % (sMacro, oInstr.sSubOpcode, g_kdIemForms[sForm][2], sForm,));

        # Stats.
        if not self.oReStatsName.match(sStats):
            self.error('%s: invalid a_Stats value: %s' % (sMacro, sStats,));
        elif oInstr.sStats is None:
            oInstr.sStats = sStats;
        elif oInstr.sStats != sStats:
            self.error('%s: mismatching @opstats and a_Stats value: %s vs %s'
                       % (sMacro, oInstr.sStats, sStats,));

        # Process the hints (simply merge with @ophints w/o checking anything).
        for sHint in sDisHints.split('|'):
            sHint = sHint.strip();
            if sHint.startswith('DISOPTYPE_'):
                sShortHint = sHint[len('DISOPTYPE_'):].lower();
                if sShortHint in g_kdHints:
                    oInstr.dHints[sShortHint] = True; # (dummy value, using dictionary for speed)
                else:
                    self.error('%s: unknown a_fDisHints value: %s' % (sMacro, sHint,));
            elif sHint != '0':
                self.error('%s: expected a_fDisHints value: %s' % (sMacro, sHint,));

        for sHint in sIemHints.split('|'):
            sHint = sHint.strip();
            if sHint.startswith('IEMOPHINT_'):
                sShortHint = sHint[len('IEMOPHINT_'):].lower();
                if sShortHint in g_kdHints:
                    oInstr.dHints[sShortHint] = True; # (dummy value, using dictionary for speed)
                else:
                    self.error('%s: unknown a_fIemHints value: %s' % (sMacro, sHint,));
            elif sHint != '0':
                self.error('%s: expected a_fIemHints value: %s' % (sMacro, sHint,));

        _ = sAsm;
        return True;

    def workerIemOpMnemonic(self, sMacro, sForm, sUpper, sLower, sDisHints, sIemHints, asOperands):
        """
        Processes one of the a IEMOP_MNEMONIC0, IEMOP_MNEMONIC1, IEMOP_MNEMONIC2,
        IEMOP_MNEMONIC3, and IEMOP_MNEMONIC4 macros.
        """
        if not asOperands:
            return self.workerIemOpMnemonicEx(sMacro, sLower, sLower, sForm, sUpper, sLower, sDisHints, sIemHints, asOperands);
        return self.workerIemOpMnemonicEx(sMacro, sLower + '_' + '_'.join(asOperands), sLower + ' ' + ','.join(asOperands),
                                          sForm, sUpper, sLower, sDisHints, sIemHints, asOperands);

    def checkCodeForMacro(self, sCode):
        """
        Checks code for relevant macro invocation.
        """
        #
        # Scan macro invocations.
        #
        if sCode.find('(') > 0:
            # Look for instruction decoder function definitions. ASSUME single line.
            asArgs = self.findAndParseFirstMacroInvocation(sCode,
                                                           [ 'FNIEMOP_DEF',
                                                             'FNIEMOP_STUB',
                                                             'FNIEMOP_STUB_1',
                                                             'FNIEMOP_UD_STUB',
                                                             'FNIEMOP_UD_STUB_1' ]);
            if asArgs is not None:
                sFunction = asArgs[1];

                if not self.aoCurInstrs:
                    self.addInstruction();
                for oInstr in self.aoCurInstrs:
                    if oInstr.iLineFnIemOpMacro == -1:
                        oInstr.iLineFnIemOpMacro = self.iLine;
                    else:
                        self.error('%s: already seen a FNIEMOP_XXX macro for %s' % (asArgs[0], oInstr,) );
                self.setInstrunctionAttrib('sFunction', sFunction);
                self.setInstrunctionAttrib('fStub', asArgs[0].find('STUB') > 0, fOverwrite = True);
                self.setInstrunctionAttrib('fUdStub', asArgs[0].find('UD_STUB') > 0, fOverwrite = True);
                if asArgs[0].find('STUB') > 0:
                    self.doneInstructions();
                return True;

            # IEMOP_HLP_DONE_VEX_DECODING_*
            asArgs = self.findAndParseFirstMacroInvocation(sCode,
                                                           [ 'IEMOP_HLP_DONE_VEX_DECODING',
                                                             'IEMOP_HLP_DONE_VEX_DECODING_L0',
                                                             'IEMOP_HLP_DONE_VEX_DECODING_NO_VVVV',
                                                             'IEMOP_HLP_DONE_VEX_DECODING_L0_AND_NO_VVVV',
                                                             ]);
            if asArgs is not None:
                sMacro = asArgs[0];
                if sMacro in ('IEMOP_HLP_DONE_VEX_DECODING_L0', 'IEMOP_HLP_DONE_VEX_DECODING_L0_AND_NO_VVVV', ):
                    for oInstr in self.aoCurInstrs:
                        if 'vex_l_zero' not in oInstr.dHints:
                            if oInstr.iLineMnemonicMacro >= 0:
                                self.errorOnLine(oInstr.iLineMnemonicMacro,
                                                 'Missing IEMOPHINT_VEX_L_ZERO! (%s on line %d)' % (sMacro, self.iLine,));
                            oInstr.dHints['vex_l_zero'] = True;
                return True;

            #
            # IEMOP_MNEMONIC*
            #

            # IEMOP_MNEMONIC(a_Stats, a_szMnemonic) IEMOP_INC_STATS(a_Stats)
            asArgs = self.findAndParseMacroInvocation(sCode, 'IEMOP_MNEMONIC');
            if asArgs is not None:
                if len(self.aoCurInstrs) == 1:
                    oInstr = self.aoCurInstrs[0];
                    if oInstr.sStats is None:
                        oInstr.sStats = asArgs[1];
                    self.deriveMnemonicAndOperandsFromStats(oInstr, asArgs[1]);

            # IEMOP_MNEMONIC0EX(a_Stats, a_szMnemonic, a_Form, a_Upper, a_Lower, a_fDisHints, a_fIemHints)
            asArgs = self.findAndParseMacroInvocation(sCode, 'IEMOP_MNEMONIC0EX');
            if asArgs is not None:
                self.workerIemOpMnemonicEx(asArgs[0], asArgs[1], asArgs[2], asArgs[3], asArgs[4], asArgs[5], asArgs[6], asArgs[7],
                                           []);
            # IEMOP_MNEMONIC1EX(a_Stats, a_szMnemonic, a_Form, a_Upper, a_Lower, a_Op1, a_fDisHints, a_fIemHints)
            asArgs = self.findAndParseMacroInvocation(sCode, 'IEMOP_MNEMONIC1EX');
            if asArgs is not None:
                self.workerIemOpMnemonicEx(asArgs[0], asArgs[1], asArgs[2], asArgs[3], asArgs[4], asArgs[5], asArgs[7], asArgs[8],
                                           [asArgs[6],]);
            # IEMOP_MNEMONIC2EX(a_Stats, a_szMnemonic, a_Form, a_Upper, a_Lower, a_Op1, a_Op2, a_fDisHints, a_fIemHints)
            asArgs = self.findAndParseMacroInvocation(sCode, 'IEMOP_MNEMONIC2EX');
            if asArgs is not None:
                self.workerIemOpMnemonicEx(asArgs[0], asArgs[1], asArgs[2], asArgs[3], asArgs[4], asArgs[5], asArgs[8], asArgs[9],
                                           [asArgs[6], asArgs[7]]);
            # IEMOP_MNEMONIC3EX(a_Stats, a_szMnemonic, a_Form, a_Upper, a_Lower, a_Op1, a_Op2, a_Op3, a_fDisHints, a_fIemHints)
            asArgs = self.findAndParseMacroInvocation(sCode, 'IEMOP_MNEMONIC3EX');
            if asArgs is not None:
                self.workerIemOpMnemonicEx(asArgs[0], asArgs[1], asArgs[2], asArgs[3], asArgs[4], asArgs[5], asArgs[9],
                                           asArgs[10], [asArgs[6], asArgs[7], asArgs[8],]);
            # IEMOP_MNEMONIC4EX(a_Stats, a_szMnemonic, a_Form, a_Upper, a_Lower, a_Op1, a_Op2, a_Op3, a_Op4, a_fDisHints,
            #                   a_fIemHints)
            asArgs = self.findAndParseMacroInvocation(sCode, 'IEMOP_MNEMONIC4EX');
            if asArgs is not None:
                self.workerIemOpMnemonicEx(asArgs[0], asArgs[1], asArgs[2], asArgs[3], asArgs[4], asArgs[5], asArgs[10],
                                           asArgs[11], [asArgs[6], asArgs[7], asArgs[8], asArgs[9],]);

            # IEMOP_MNEMONIC0(a_Form, a_Upper, a_Lower, a_fDisHints, a_fIemHints)
            asArgs = self.findAndParseMacroInvocation(sCode, 'IEMOP_MNEMONIC0');
            if asArgs is not None:
                self.workerIemOpMnemonic(asArgs[0], asArgs[1], asArgs[2], asArgs[3], asArgs[4], asArgs[5], []);
            # IEMOP_MNEMONIC1(a_Form, a_Upper, a_Lower, a_Op1, a_fDisHints, a_fIemHints)
            asArgs = self.findAndParseMacroInvocation(sCode, 'IEMOP_MNEMONIC1');
            if asArgs is not None:
                self.workerIemOpMnemonic(asArgs[0], asArgs[1], asArgs[2], asArgs[3], asArgs[5], asArgs[6], [asArgs[4],]);
            # IEMOP_MNEMONIC2(a_Form, a_Upper, a_Lower, a_Op1, a_Op2, a_fDisHints, a_fIemHints)
            asArgs = self.findAndParseMacroInvocation(sCode, 'IEMOP_MNEMONIC2');
            if asArgs is not None:
                self.workerIemOpMnemonic(asArgs[0], asArgs[1], asArgs[2], asArgs[3], asArgs[6], asArgs[7],
                                         [asArgs[4], asArgs[5],]);
            # IEMOP_MNEMONIC3(a_Form, a_Upper, a_Lower, a_Op1, a_Op2, a_Op3, a_fDisHints, a_fIemHints)
            asArgs = self.findAndParseMacroInvocation(sCode, 'IEMOP_MNEMONIC3');
            if asArgs is not None:
                self.workerIemOpMnemonic(asArgs[0], asArgs[1], asArgs[2], asArgs[3], asArgs[7], asArgs[8],
                                         [asArgs[4], asArgs[5], asArgs[6],]);
            # IEMOP_MNEMONIC4(a_Form, a_Upper, a_Lower, a_Op1, a_Op2, a_Op3, a_Op4, a_fDisHints, a_fIemHints)
            asArgs = self.findAndParseMacroInvocation(sCode, 'IEMOP_MNEMONIC4');
            if asArgs is not None:
                self.workerIemOpMnemonic(asArgs[0], asArgs[1], asArgs[2], asArgs[3], asArgs[8], asArgs[9],
                                         [asArgs[4], asArgs[5], asArgs[6], asArgs[7],]);

        return False;


    def parse(self):
        """
        Parses the given file.
        Returns number or errors.
        Raises exception on fatal trouble.
        """
        #self.debug('Parsing %s' % (self.sSrcFile,));

        while self.iLine < len(self.asLines):
            sLine = self.asLines[self.iLine];
            self.iLine  += 1;

            # We only look for comments, so only lines with a slash might possibly
            # influence the parser state.
            offSlash = sLine.find('/');
            if offSlash >= 0:
                if offSlash + 1 >= len(sLine)  or  sLine[offSlash + 1] != '/'  or  self.iState != self.kiCode:
                    offLine = 0;
                    while offLine < len(sLine):
                        if self.iState == self.kiCode:
                            offHit = sLine.find('/*', offLine); # only multiline comments for now.
                            if offHit >= 0:
                                self.checkCodeForMacro(sLine[offLine:offHit]);
                                self.sComment     = '';
                                self.iCommentLine = self.iLine;
                                self.iState       = self.kiCommentMulti;
                                offLine = offHit + 2;
                            else:
                                self.checkCodeForMacro(sLine[offLine:]);
                                offLine = len(sLine);

                        elif self.iState == self.kiCommentMulti:
                            offHit = sLine.find('*/', offLine);
                            if offHit >= 0:
                                self.sComment += sLine[offLine:offHit];
                                self.iState    = self.kiCode;
                                offLine = offHit + 2;
                                self.parseComment();
                            else:
                                self.sComment += sLine[offLine:];
                                offLine = len(sLine);
                        else:
                            assert False;
                # C++ line comment.
                elif offSlash > 0:
                    self.checkCodeForMacro(sLine[:offSlash]);

            # No slash, but append the line if in multi-line comment.
            elif self.iState == self.kiCommentMulti:
                #self.debug('line %d: multi' % (self.iLine,));
                self.sComment += sLine;

            # No slash, but check code line for relevant macro.
            elif self.iState == self.kiCode and sLine.find('IEMOP_') >= 0:
                #self.debug('line %d: macro' % (self.iLine,));
                self.checkCodeForMacro(sLine);

            # If the line is a '}' in the first position, complete the instructions.
            elif self.iState == self.kiCode and sLine[0] == '}':
                #self.debug('line %d: }' % (self.iLine,));
                self.doneInstructions();

            # Look for instruction table on the form 'IEM_STATIC const PFNIEMOP g_apfnVexMap3'
            # so we can check/add @oppfx info from it.
            elif self.iState == self.kiCode and sLine.find('PFNIEMOP') > 0 and self.oReFunTable.match(sLine):
                self.parseFunctionTable(sLine);

        self.doneInstructions();
        self.debug('%3s%% / %3s stubs out of %4s instructions in %s'
                   % (self.cTotalStubs * 100 // self.cTotalInstr, self.cTotalStubs, self.cTotalInstr,
                      os.path.basename(self.sSrcFile),));
        return self.printErrors();


def __parseFileByName(sSrcFile, sDefaultMap):
    """
    Parses one source file for instruction specfications.
    """
    #
    # Read sSrcFile into a line array.
    #
    try:
        oFile = open(sSrcFile, "r");    # pylint: disable=consider-using-with
    except Exception as oXcpt:
        raise Exception("failed to open %s for reading: %s" % (sSrcFile, oXcpt,));
    try:
        asLines = oFile.readlines();
    except Exception as oXcpt:
        raise Exception("failed to read %s: %s" % (sSrcFile, oXcpt,));
    finally:
        oFile.close();

    #
    # Do the parsing.
    #
    try:
        cErrors = SimpleParser(sSrcFile, asLines, sDefaultMap).parse();
    except ParserException as oXcpt:
        print(str(oXcpt));
        raise;

    return cErrors;


def __doTestCopying():
    """
    Executes the asCopyTests instructions.
    """
    asErrors = [];
    for oDstInstr in g_aoAllInstructions:
        if oDstInstr.asCopyTests:
            for sSrcInstr in oDstInstr.asCopyTests:
                oSrcInstr = g_dAllInstructionsByStat.get(sSrcInstr, None);
                if oSrcInstr:
                    aoSrcInstrs = [oSrcInstr,];
                else:
                    aoSrcInstrs = g_dAllInstructionsByFunction.get(sSrcInstr, []);
                if aoSrcInstrs:
                    for oSrcInstr in aoSrcInstrs:
                        if oSrcInstr != oDstInstr:
                            oDstInstr.aoTests.extend(oSrcInstr.aoTests);
                        else:
                            asErrors.append('%s:%s: error: @opcopytests reference "%s" matches the destination\n'
                                            % ( oDstInstr.sSrcFile, oDstInstr.iLineCreated, sSrcInstr));
                else:
                    asErrors.append('%s:%s: error: @opcopytests reference "%s" not found\n'
                                    % ( oDstInstr.sSrcFile, oDstInstr.iLineCreated, sSrcInstr));

    if asErrors:
        sys.stderr.write(u''.join(asErrors));
    return len(asErrors);


def __applyOnlyTest():
    """
    If g_aoOnlyTestInstructions contains any instructions, drop aoTests from
    all other instructions so that only these get tested.
    """
    if g_aoOnlyTestInstructions:
        for oInstr in g_aoAllInstructions:
            if oInstr.aoTests:
                if oInstr not in g_aoOnlyTestInstructions:
                    oInstr.aoTests = [];
    return 0;

def __parseAll():
    """
    Parses all the IEMAllInstruction*.cpp.h files.

    Raises exception on failure.
    """
    sSrcDir = os.path.dirname(os.path.abspath(__file__));
    cErrors = 0;
    for sDefaultMap, sName in [
        ( 'one',        'IEMAllInstructionsOneByte.cpp.h'),
        ( 'two0f',      'IEMAllInstructionsTwoByte0f.cpp.h'),
        ( 'three0f38',  'IEMAllInstructionsThree0f38.cpp.h'),
        ( 'three0f3a',  'IEMAllInstructionsThree0f3a.cpp.h'),
        ( 'vexmap1',    'IEMAllInstructionsVexMap1.cpp.h'),
        ( 'vexmap2',    'IEMAllInstructionsVexMap2.cpp.h'),
        ( 'vexmap3',    'IEMAllInstructionsVexMap3.cpp.h'),
        ( '3dnow',      'IEMAllInstructions3DNow.cpp.h'),
    ]:
        cErrors += __parseFileByName(os.path.join(sSrcDir, sName), sDefaultMap);
    cErrors += __doTestCopying();
    cErrors += __applyOnlyTest();

    # Total stub stats:
    cTotalStubs = 0;
    for oInstr in g_aoAllInstructions:
        cTotalStubs += oInstr.fStub;
    print('debug: %3s%% / %3s stubs out of %4s instructions in total'
          % (cTotalStubs * 100 // len(g_aoAllInstructions), cTotalStubs, len(g_aoAllInstructions),));

    if cErrors != 0:
        #raise Exception('%d parse errors' % (cErrors,));
        sys.exit(1);
    return True;



__parseAll();


#
# Generators (may perhaps move later).
#
def __formatDisassemblerTableEntry(oInstr):
    """
    """
    sMacro = 'OP';
    cMaxOperands = 3;
    if len(oInstr.aoOperands) > 3:
        sMacro = 'OPVEX'
        cMaxOperands = 4;
        assert len(oInstr.aoOperands) <= cMaxOperands;

    #
    # Format string.
    #
    sTmp = '%s("%s' % (sMacro, oInstr.sMnemonic,);
    for iOperand, oOperand in enumerate(oInstr.aoOperands):
        sTmp += ' ' if iOperand == 0 else ',';
        if g_kdOpTypes[oOperand.sType][2][0] != '%':        ## @todo remove upper() later.
            sTmp += g_kdOpTypes[oOperand.sType][2].upper(); ## @todo remove upper() later.
        else:
            sTmp += g_kdOpTypes[oOperand.sType][2];
    sTmp += '",';
    asColumns = [ sTmp, ];

    #
    # Decoders.
    #
    iStart = len(asColumns);
    if oInstr.sEncoding is None:
        pass;
    elif oInstr.sEncoding == 'ModR/M':
        # ASSUME the first operand is using the ModR/M encoding
        assert len(oInstr.aoOperands) >= 1 and oInstr.aoOperands[0].usesModRM();
        asColumns.append('IDX_ParseModRM,');
    elif oInstr.sEncoding in [ 'prefix', ]:
        for oOperand in oInstr.aoOperands:
            asColumns.append('0,');
    elif oInstr.sEncoding in [ 'fixed', 'VEX.fixed' ]:
        pass;
    elif oInstr.sEncoding == 'VEX.ModR/M':
        asColumns.append('IDX_ParseModRM,');
    elif oInstr.sEncoding == 'vex2':
        asColumns.append('IDX_ParseVex2b,')
    elif oInstr.sEncoding == 'vex3':
        asColumns.append('IDX_ParseVex3b,')
    elif oInstr.sEncoding in g_dInstructionMaps:
        asColumns.append(g_dInstructionMaps[oInstr.sEncoding].sDisParse + ',');
    else:
        ## @todo
        #IDX_ParseTwoByteEsc,
        #IDX_ParseGrp1,
        #IDX_ParseShiftGrp2,
        #IDX_ParseGrp3,
        #IDX_ParseGrp4,
        #IDX_ParseGrp5,
        #IDX_Parse3DNow,
        #IDX_ParseGrp6,
        #IDX_ParseGrp7,
        #IDX_ParseGrp8,
        #IDX_ParseGrp9,
        #IDX_ParseGrp10,
        #IDX_ParseGrp12,
        #IDX_ParseGrp13,
        #IDX_ParseGrp14,
        #IDX_ParseGrp15,
        #IDX_ParseGrp16,
        #IDX_ParseThreeByteEsc4,
        #IDX_ParseThreeByteEsc5,
        #IDX_ParseModFence,
        #IDX_ParseEscFP,
        #IDX_ParseNopPause,
        #IDX_ParseInvOpModRM,
        assert False, str(oInstr);

    # Check for immediates and stuff in the remaining operands.
    for oOperand in oInstr.aoOperands[len(asColumns) - iStart:]:
        sIdx = g_kdOpTypes[oOperand.sType][0];
        #if sIdx != 'IDX_UseModRM':
        asColumns.append(sIdx + ',');
    asColumns.extend(['0,'] * (cMaxOperands - (len(asColumns) - iStart)));

    #
    # Opcode and operands.
    #
    assert oInstr.sDisEnum, str(oInstr);
    asColumns.append(oInstr.sDisEnum + ',');
    iStart = len(asColumns)
    for oOperand in oInstr.aoOperands:
        asColumns.append('OP_PARM_' + g_kdOpTypes[oOperand.sType][3] + ',');
    asColumns.extend(['OP_PARM_NONE,'] * (cMaxOperands - (len(asColumns) - iStart)));

    #
    # Flags.
    #
    sTmp = '';
    for sHint in sorted(oInstr.dHints.keys()):
        sDefine = g_kdHints[sHint];
        if sDefine.startswith('DISOPTYPE_'):
            if sTmp:
                sTmp += ' | ' + sDefine;
            else:
                sTmp += sDefine;
    if sTmp:
        sTmp += '),';
    else:
        sTmp += '0),';
    asColumns.append(sTmp);

    #
    # Format the columns into a line.
    #
    aoffColumns = [4, 29, 49, 65, 77, 89, 109, 125, 141, 157, 183, 199];
    sLine = '';
    for i, s in enumerate(asColumns):
        if len(sLine) < aoffColumns[i]:
            sLine += ' ' * (aoffColumns[i] - len(sLine));
        else:
            sLine += ' ';
        sLine += s;

    # OP("psrlw %Vdq,%Wdq", IDX_ParseModRM, IDX_UseModRM, 0, OP_PSRLW, OP_PARM_Vdq, OP_PARM_Wdq, OP_PARM_NONE,
    # DISOPTYPE_HARMLESS),
    # define OP(pszOpcode, idxParse1, idxParse2, idxParse3, opcode, param1, param2, param3, optype) \
    # { pszOpcode, idxParse1, idxParse2, idxParse3, 0, opcode, param1, param2, param3, 0, 0, optype }
    return sLine;

def __checkIfShortTable(aoTableOrdered, oMap):
    """
    Returns (iInstr, cInstructions, fShortTable)
    """

    # Determin how much we can trim off.
    cInstructions = len(aoTableOrdered);
    while cInstructions > 0 and aoTableOrdered[cInstructions - 1] is None:
        cInstructions -= 1;

    iInstr = 0;
    while iInstr < cInstructions and aoTableOrdered[iInstr] is None:
        iInstr += 1;

    # If we can save more than 30%, we go for the short table version.
    if iInstr + len(aoTableOrdered) - cInstructions >= len(aoTableOrdered) // 30:
        return (iInstr, cInstructions, True);
    _ = oMap; # Use this for overriding.

    # Output the full table.
    return (0, len(aoTableOrdered), False);

def generateDisassemblerTables(oDstFile = sys.stdout):
    """
    Generates disassembler tables.
    """

    #
    # The disassembler uses a slightly different table layout to save space,
    # since several of the prefix varia
    #
    aoDisasmMaps = [];
    for sName, oMap in sorted(iter(g_dInstructionMaps.items()),
                              key = lambda aKV: aKV[1].sEncoding + ''.join(aKV[1].asLeadOpcodes)):
        if oMap.sSelector != 'byte+pfx':
            aoDisasmMaps.append(oMap);
        else:
            # Split the map by prefix.
            aoDisasmMaps.append(oMap.copy(oMap.sName,         'none'));
            aoDisasmMaps.append(oMap.copy(oMap.sName + '_66', '0x66'));
            aoDisasmMaps.append(oMap.copy(oMap.sName + '_F3', '0xf3'));
            aoDisasmMaps.append(oMap.copy(oMap.sName + '_F2', '0xf2'));

    #
    # Dump each map.
    #
    asHeaderLines = [];
    print("debug: maps=%s\n" % (', '.join([oMap.sName for oMap in aoDisasmMaps]),));
    for oMap in aoDisasmMaps:
        sName = oMap.sName;

        if not sName.startswith("vex"): continue; # only looking at the vex maps at the moment.

        #
        # Get the instructions for the map and see if we can do a short version or not.
        #
        aoTableOrder    = oMap.getInstructionsInTableOrder();
        cEntriesPerByte = oMap.getEntriesPerByte();
        (iInstrStart, iInstrEnd, fShortTable) = __checkIfShortTable(aoTableOrder, oMap);

        #
        # Output the table start.
        # Note! Short tables are static and only accessible via the map range record.
        #
        asLines = [];
        asLines.append('/* Generated from: %-11s  Selector: %-7s  Encoding: %-7s  Lead bytes opcodes: %s */'
                       % ( oMap.sName, oMap.sSelector, oMap.sEncoding, ' '.join(oMap.asLeadOpcodes), ));
        if fShortTable:
            asLines.append('%sconst DISOPCODE %s[] =' % ('static ' if fShortTable else '', oMap.getDisasTableName(),));
        else:
            asHeaderLines.append('extern const DISOPCODE %s[%d];'  % (oMap.getDisasTableName(), iInstrEnd - iInstrStart,));
            asLines.append(             'const DISOPCODE %s[%d] =' % (oMap.getDisasTableName(), iInstrEnd - iInstrStart,));
        asLines.append('{');

        if fShortTable and (iInstrStart & ((0x10 * cEntriesPerByte) - 1)) != 0:
            asLines.append('    /* %#04x: */' % (iInstrStart,));

        #
        # Output the instructions.
        #
        iInstr = iInstrStart;
        while iInstr < iInstrEnd:
            oInstr = aoTableOrder[iInstr];
            if (iInstr & ((0x10 * cEntriesPerByte) - 1)) == 0:
                if iInstr != iInstrStart:
                    asLines.append('');
                asLines.append('    /* %x */' % ((iInstr // cEntriesPerByte) >> 4,));

            if oInstr is None:
                # Invalid. Optimize blocks of invalid instructions.
                cInvalidInstrs = 1;
                while iInstr + cInvalidInstrs < len(aoTableOrder) and aoTableOrder[iInstr + cInvalidInstrs] is None:
                    cInvalidInstrs += 1;
                if (iInstr & (0x10 * cEntriesPerByte - 1)) == 0 and cInvalidInstrs >= 0x10 * cEntriesPerByte:
                    asLines.append('    INVALID_OPCODE_BLOCK_%u,' % (0x10 * cEntriesPerByte,));
                    iInstr += 0x10 * cEntriesPerByte - 1;
                elif cEntriesPerByte > 1:
                    if (iInstr & (cEntriesPerByte - 1)) == 0 and cInvalidInstrs >= cEntriesPerByte:
                        asLines.append('    INVALID_OPCODE_BLOCK_%u,' % (cEntriesPerByte,));
                        iInstr += 3;
                    else:
                        asLines.append('    /* %#04x/%d */ INVALID_OPCODE,'
                                       % (iInstr // cEntriesPerByte, iInstr % cEntriesPerByte));
                else:
                    asLines.append('    /* %#04x */ INVALID_OPCODE,' % (iInstr));
            elif isinstance(oInstr, list):
                if len(oInstr) != 0:
                    asLines.append('    /* %#04x */ ComplicatedListStuffNeedingWrapper, /* \n -- %s */'
                                   % (iInstr, '\n -- '.join([str(oItem) for oItem in oInstr]),));
                else:
                    asLines.append(__formatDisassemblerTableEntry(oInstr));
            else:
                asLines.append(__formatDisassemblerTableEntry(oInstr));

            iInstr += 1;

        if iInstrStart >= iInstrEnd:
            asLines.append('    /* dummy */ INVALID_OPCODE');

        asLines.append('};');
        asLines.append('AssertCompile(RT_ELEMENTS(%s) == %s);' % (oMap.getDisasTableName(), iInstrEnd - iInstrStart,));

        #
        # We always emit a map range record, assuming the linker will eliminate the unnecessary ones.
        #
        asHeaderLines.append('extern const DISOPMAPDESC %sRange;'  % (oMap.getDisasRangeName()));
        asLines.append('const DISOPMAPDESC %s = { &%s[0], %#04x, RT_ELEMENTS(%s) };'
                       % (oMap.getDisasRangeName(), oMap.getDisasTableName(), iInstrStart, oMap.getDisasTableName(),));

        #
        # Write out the lines.
        #
        oDstFile.write('\n'.join(asLines));
        oDstFile.write('\n');
        oDstFile.write('\n');
        #break; #for now

if __name__ == '__main__':
    generateDisassemblerTables();


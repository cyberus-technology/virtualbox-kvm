/* $Id: post.c $ */
/** @file
 * BIOS POST routines. Used only during initialization.
 */

/*
 * Copyright (C) 2004-2023 Oracle and/or its affiliates.
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

#include <stdint.h>
#include <string.h>
#include "biosint.h"
#include "inlines.h"

#if DEBUG_POST  || 0
#  define DPRINT(...) BX_DEBUG(__VA_ARGS__)
#else
#  define DPRINT(...)
#endif

/* In general, checksumming ROMs in a VM just wastes time. */
//#define CHECKSUM_ROMS

/* The format of a ROM is as follows:
 *
 *     ------------------------------
 *   0 | AA55h signature (word)     |
 *     ------------------------------
 *   2 | Size in 512B blocks (byte) |
 *     ------------------------------
 *   3 | Start of executable code   |
 *     |          .......           |
 * end |                            |
 *     ------------------------------
 */

typedef struct rom_hdr_tag {
    uint16_t    signature;
    uint8_t     num_blks;
    uint8_t     code;
} rom_hdr;


/* Calculate the checksum of a ROM. Note that the ROM might be
 * larger than 64K.
 */
static inline uint8_t rom_checksum(uint8_t __far *rom, uint8_t blocks)
{
    uint8_t     sum = 0;

#ifdef CHECKSUM_ROMS
    while (blocks--) {
        int     i;

        for (i = 0; i < 512; ++i)
            sum += rom[i];
        /* Add 512 bytes (32 paragraphs) to segment. */
        rom = MK_FP(FP_SEG(rom) + (512 >> 4), 0);
    }
#endif
    return sum;
}

/* The ROM init routine might trash register. Give the compiler a heads-up. */
typedef void __far (rom_init_rtn)(void);
#pragma aux rom_init_rtn modify [ax bx cx dx si di es /*ignored:*/ bp] /*ignored:*/ loadds;

/* The loadds bit is ignored for rom_init_rtn, the impression from the code
   generator is that this is due to using a memory model where DS is fixed.
   If we add DS as a modified register, we'll get run into compiler error
   E1122 (BAD_REG) because FixedRegs() in cg/intel/386/c/386rgtbl.c returns
   HW_DS as part of the fixed register set that cannot be modified.

   The problem of the vga bios trashing DS isn't a biggie, except when
   something goes sideways before we can reload it.  Setting a I/O port
   breakpoint on port 80h and wait a while before resuming execution
   (ba i 1 80 "sleep 400 ; g") will usually trigger a keyboard init panic and
   showcase the issue.  The panic message is garbage because it's read from
   segment 0c000h instead of 0f000h. */
static void restore_ds_as_dgroup(void);
#pragma aux restore_ds_as_dgroup = \
    "mov ax, 0f000h" \
    "mov ds, ax" \
    modify exact [ax];


/* Scan for ROMs in the given range and execute their POST code. */
void rom_scan(uint16_t start_seg, uint16_t end_seg)
{
    rom_hdr __far   *rom;
    uint8_t         rom_blks;

    DPRINT("Scanning for ROMs in %04X-%04X range\n", start_seg, end_seg);

    while (start_seg < end_seg) {
        rom = MK_FP(start_seg, 0);
        /* Check for the ROM signature. */
        if (rom->signature == 0xAA55) {
            DPRINT("Found ROM at segment %04X\n", start_seg);
            if (!rom_checksum((void __far *)rom, rom->num_blks)) {
                rom_init_rtn *rom_init;

                /* Checksum good, initialize ROM. */
                rom_init = (rom_init_rtn *)&rom->code;
                rom_init();
                int_disable();
                restore_ds_as_dgroup();
                /** @todo BP is not restored. */
                DPRINT("ROM initialized\n");

                /* Continue scanning past the end of this ROM. */
                rom_blks   = (rom->num_blks + 3) & ~3;  /* 4 blocks = 2K */
                start_seg += rom_blks / 4;
            }
        } else {
            /* Scanning is done in 2K steps. */
            start_seg += 2048 >> 4;
        }
    }
}

#if VBOX_BIOS_CPU >= 80386

/* NB: The CPUID detection is generic but currently not used elsewhere. */

/* Check CPUID availability. */
int is_cpuid_supported( void )
{
    uint32_t    old_flags, new_flags;

    old_flags = eflags_read();
    new_flags = old_flags ^ (1L << 21); /* Toggle CPUID bit. */
    eflags_write( new_flags );
    new_flags = eflags_read();
    return( old_flags != new_flags );   /* Supported if bit changed. */
}

#define APICMODE_DISABLED   0
#define APICMODE_APIC       1
#define APICMODE_X2APIC     2

#define APIC_BASE_MSR       0x1B
#define APICBASE_X2APIC     0x400   /* bit 10 */
#define APICBASE_ENABLE     0x800   /* bit 11 */

/*
 * Set up APIC/x2APIC. See also DevPcBios.cpp.
 *
 * NB: Virtual wire compatibility is set up earlier in 32-bit protected
 * mode assembler (because it needs to access MMIO just under 4GB).
 * Switching to x2APIC mode or disabling the APIC is done through an MSR
 * and needs no 32-bit addressing. Going to x2APIC mode does not lose the
 * existing virtual wire setup.
 *
 * NB: This code does not assume that there is a local APIC. It is necessary
 * to check CPUID whether APIC is present; the CPUID instruction might not be
 * available either.
 *
 * NB: Destroys high bits of 32-bit registers.
 */
void BIOSCALL apic_setup(void)
{
    uint64_t    base_msr;
    uint16_t    mask_set;
    uint16_t    mask_clr;
    uint8_t     apic_mode;
    uint32_t    cpu_id[4];

    /* If there's no CPUID, there's certainly no APIC. */
    if (!is_cpuid_supported()) {
        return;
    }

    /* Check EDX bit 9 */
    cpuid(&cpu_id, 1);
    BX_DEBUG("CPUID EDX: 0x%lx\n", cpu_id[3]);
    if ((cpu_id[3] & (1 << 9)) == 0) {
        return; /* No local APIC, nothing to do. */
    }

    /* APIC mode at offset 78h in CMOS NVRAM. */
    apic_mode = inb_cmos(0x78);

    mask_set = mask_clr = 0;
    if (apic_mode == APICMODE_X2APIC)
        mask_set = APICBASE_X2APIC;
    else if (apic_mode == APICMODE_DISABLED)
        mask_clr = APICBASE_ENABLE;
    else
        ;   /* Any other setting leaves things alone. */

    if (mask_set || mask_clr) {
        base_msr = msr_read(APIC_BASE_MSR);
        base_msr &= ~(uint64_t)mask_clr;
        base_msr |= mask_set;
        msr_write(base_msr, APIC_BASE_MSR);
    }
}

#endif

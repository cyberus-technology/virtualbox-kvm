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
 * SPDX-License-Identifier: GPL-3.0-only
 * --------------------------------------------------------------------
 *
 * This code is based on:
 *
 *  ROM BIOS for use with Bochs/Plex86/QEMU emulation environment
 *
 *  Copyright (C) 2002  MandrakeSoft S.A.
 *
 *    MandrakeSoft S.A.
 *    43, rue d'Aboukir
 *    75002 Paris - France
 *    http://www.linux-mandrake.com/
 *    http://www.mandrakesoft.com/
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
 *
 */

/*
 * Oracle LGPL Disclaimer: For the avoidance of doubt, except that if any license choice
 * other than GPL or LGPL is available it will apply instead, Oracle elects to use only
 * the Lesser General Public License version 2.1 (LGPLv2) at this time for any software where
 * a choice of LGPL license versions is made available with the language indicating
 * that LGPLv2 or any later version may be used, or where a choice of which version
 * of the LGPL is applied is otherwise unspecified.
 */


#include <stdint.h>
#include "biosint.h"
#include "inlines.h"

#if DEBUG_INT1A
#  define BX_DEBUG_INT1A(...)   BX_DEBUG(__VA_ARGS__)
#else
#  define BX_DEBUG_INT1A(...)
#endif

// for access to RAM area which is used by interrupt vectors
// and BIOS Data Area

typedef struct {
    uint8_t     filler1[0x400];
    uint8_t     filler2[0x6c];
    uint16_t    ticks_low;
    uint16_t    ticks_high;
    uint8_t     midnight_flag;
} bios_data_t;

#define BiosData ((bios_data_t __far *) 0)

void init_rtc(void)
{
    outb_cmos(0x0a, 0x26);
    outb_cmos(0x0b, 0x02);
    inb_cmos(0x0c);
    inb_cmos(0x0d);
}

bx_bool rtc_updating(void)
{
    // This function checks to see if the update-in-progress bit
    // is set in CMOS Status Register A.  If not, it returns 0.
    // If it is set, it tries to wait until there is a transition
    // to 0, and will return 0 if such a transition occurs.  A 1
    // is returned only after timing out.  The maximum period
    // that this bit should be set is constrained to 244useconds.
    // The count I use below guarantees coverage or more than
    // this time, with any reasonable IPS setting.

    uint16_t    iter;

    iter = 25000;
    while (--iter != 0) {
    if ( (inb_cmos(0x0a) & 0x80) == 0 )
        return 0;
    }
    return 1;   // update-in-progress never transitioned to 0
}


extern void eoi_both_pics(void);    /* in assembly code */
#pragma aux eoi_both_pics "*";

void call_int_4a(void);
#pragma aux call_int_4a = "int 4Ah";

void BIOSCALL int70_function(pusha_regs_t regs, uint16_t ds, uint16_t es, iret_addr_t iret_addr)
{
    // INT 70h: IRQ 8 - CMOS RTC interrupt from periodic or alarm modes
    uint8_t   registerB = 0, registerC = 0;

    // Check which modes are enabled and have occurred.
    registerB = inb_cmos( 0xB );
    registerC = inb_cmos( 0xC );

    if( ( registerB & 0x60 ) != 0 ) {
        if( ( registerC & 0x20 ) != 0 ) {
            // Handle Alarm Interrupt.
            int_enable();
            call_int_4a();
            int_disable();
        }
        if( ( registerC & 0x40 ) != 0 ) {
            // Handle Periodic Interrupt.

            if( read_byte( 0x40, 0xA0 ) != 0 ) {
                // Wait Interval (Int 15, AH=83 or AH=86) active.
                uint32_t    time;

                time = read_dword( 0x40, 0x9C );  // Time left in microseconds.
                if( time < 0x3D1 ) {
                    // Done waiting.
                    uint16_t    segment, offset;

                    segment = read_word( 0x40, 0x98 );
                    offset  = read_word( 0x40, 0x9A );
                    write_byte( 0x40, 0xA0, 0 );  // Turn off status byte.
                    outb_cmos( 0xB, registerB & 0x37 ); // Clear the Periodic Interrupt.
                    write_byte( segment, offset, read_byte(segment, offset) | 0x80 );  // Write to specified flag byte.
                } else {
                    // Continue waiting.
                    time -= 0x3D1;
                    write_dword( 0x40, 0x9C, time );
                }
            }
        }
    }
    eoi_both_pics();
}

/// @todo the coding style WRT register access is totally inconsistent
// in the following routines

void BIOSCALL int1a_function(pusha_regs_t regs, uint16_t ds, uint16_t es, iret_addr_t iret_addr)
{
    uint8_t     val8;

    BX_DEBUG_INT1A("int1a: AX=%04x BX=%04x CX=%04x DX=%04x DS=%04x\n",
                   regs.u.r16.ax, regs.u.r16.bx, regs.u.r16.cx, regs.u.r16.dx, ds);
    int_enable();

    switch (regs.u.r8.ah) {
    case 0: // get current clock count
        int_disable();
        regs.u.r16.cx = BiosData->ticks_high;
        regs.u.r16.dx = BiosData->ticks_low;
        regs.u.r8.al  = BiosData->midnight_flag;
        BiosData->midnight_flag = 0; // reset flag
        int_enable();
        // AH already 0
        ClearCF(iret_addr.flags); // OK
        break;

    case 1: // Set Current Clock Count
        int_disable();
        BiosData->ticks_high = regs.u.r16.cx;
        BiosData->ticks_low  = regs.u.r16.dx;
        BiosData->midnight_flag = 0; // reset flag
        int_enable();
        regs.u.r8.ah = 0;
        ClearCF(iret_addr.flags); // OK
        break;

    case 2: // Read CMOS Time
        if (rtc_updating()) {
            SetCF(iret_addr.flags);
            break;
        }

        regs.u.r8.dh = inb_cmos(0x00); // Seconds
        regs.u.r8.cl = inb_cmos(0x02); // Minutes
        regs.u.r8.ch = inb_cmos(0x04); // Hours
        regs.u.r8.dl = inb_cmos(0x0b) & 0x01; // Stat Reg B
        regs.u.r8.ah = 0;
        regs.u.r8.al = regs.u.r8.ch;
        ClearCF(iret_addr.flags); // OK
        break;

    case 3: // Set CMOS Time
        // Using a debugger, I notice the following masking/setting
        // of bits in Status Register B, by setting Reg B to
        // a few values and getting its value after INT 1A was called.
        //
        //        try#1       try#2       try#3
        // before 1111 1101   0111 1101   0000 0000
        // after  0110 0010   0110 0010   0000 0010
        //
        // Bit4 in try#1 flipped in hardware (forced low) due to bit7=1
        // My assumption: RegB = ((RegB & 01100000b) | 00000010b)
        if (rtc_updating()) {
            init_rtc();
            // fall through as if an update were not in progress
        }
        outb_cmos(0x00, regs.u.r8.dh); // Seconds
        outb_cmos(0x02, regs.u.r8.cl); // Minutes
        outb_cmos(0x04, regs.u.r8.ch); // Hours
        // Set Daylight Savings time enabled bit to requested value
        val8 = (inb_cmos(0x0b) & 0x60) | 0x02 | (regs.u.r8.dl & 0x01);
        // (reg B already selected)
        outb_cmos(0x0b, val8);
        regs.u.r8.ah = 0;
        regs.u.r8.al = val8; // val last written to Reg B
        ClearCF(iret_addr.flags); // OK
        break;

    case 4: // Read CMOS Date
        regs.u.r8.ah = 0;
        if (rtc_updating()) {
            SetCF(iret_addr.flags);
            break;
        }
        regs.u.r8.cl = inb_cmos(0x09); // Year
        regs.u.r8.dh = inb_cmos(0x08); // Month
        regs.u.r8.dl = inb_cmos(0x07); // Day of Month
        regs.u.r8.ch = inb_cmos(0x32); // Century
        regs.u.r8.al = regs.u.r8.ch;
        ClearCF(iret_addr.flags); // OK
        break;

    case 5: // Set CMOS Date
        // Using a debugger, I notice the following masking/setting
        // of bits in Status Register B, by setting Reg B to
        // a few values and getting its value after INT 1A was called.
        //
        //        try#1       try#2       try#3       try#4
        // before 1111 1101   0111 1101   0000 0010   0000 0000
        // after  0110 1101   0111 1101   0000 0010   0000 0000
        //
        // Bit4 in try#1 flipped in hardware (forced low) due to bit7=1
        // My assumption: RegB = (RegB & 01111111b)
        if (rtc_updating()) {
            init_rtc();
            SetCF(iret_addr.flags);
            break;
        }
        outb_cmos(0x09, regs.u.r8.cl); // Year
        outb_cmos(0x08, regs.u.r8.dh); // Month
        outb_cmos(0x07, regs.u.r8.dl); // Day of Month
        outb_cmos(0x32, regs.u.r8.ch); // Century
        val8 = inb_cmos(0x0b) & 0x7f; // clear halt-clock bit
        outb_cmos(0x0b, val8);
        regs.u.r8.ah = 0;
        regs.u.r8.al = val8; // AL = val last written to Reg B
        ClearCF(iret_addr.flags); // OK
        break;

    case 6: // Set Alarm Time in CMOS
        // Using a debugger, I notice the following masking/setting
        // of bits in Status Register B, by setting Reg B to
        // a few values and getting its value after INT 1A was called.
        //
        //        try#1       try#2       try#3
        // before 1101 1111   0101 1111   0000 0000
        // after  0110 1111   0111 1111   0010 0000
        //
        // Bit4 in try#1 flipped in hardware (forced low) due to bit7=1
        // My assumption: RegB = ((RegB & 01111111b) | 00100000b)
        val8 = inb_cmos(0x0b); // Get Status Reg B
        regs.u.r16.ax = 0;
        if (val8 & 0x20) {
            // Alarm interrupt enabled already
            SetCF(iret_addr.flags); // Error: alarm in use
            break;
        }
        if (rtc_updating()) {
            init_rtc();
            // fall through as if an update were not in progress
        }
        outb_cmos(0x01, regs.u.r8.dh); // Seconds alarm
        outb_cmos(0x03, regs.u.r8.cl); // Minutes alarm
        outb_cmos(0x05, regs.u.r8.ch); // Hours alarm
        outb(0xa1, inb(0xa1) & 0xfe); // enable IRQ 8
        // enable Status Reg B alarm bit, clear halt clock bit
        outb_cmos(0x0b, (val8 & 0x7f) | 0x20);
        ClearCF(iret_addr.flags); // OK
        break;

    case 7: // Turn off Alarm
        // Using a debugger, I notice the following masking/setting
        // of bits in Status Register B, by setting Reg B to
        // a few values and getting its value after INT 1A was called.
        //
        //        try#1       try#2       try#3       try#4
        // before 1111 1101   0111 1101   0010 0000   0010 0010
        // after  0100 0101   0101 0101   0000 0000   0000 0010
        //
        // Bit4 in try#1 flipped in hardware (forced low) due to bit7=1
        // My assumption: RegB = (RegB & 01010111b)
        val8 = inb_cmos(0x0b); // Get Status Reg B
        // clear clock-halt bit, disable alarm bit
        outb_cmos(0x0b, val8 & 0x57); // disable alarm bit
        regs.u.r8.ah = 0;
        regs.u.r8.al = val8; // val last written to Reg B
        ClearCF(iret_addr.flags); // OK
        break;

    default:
        BX_DEBUG_INT1A("int1a: AX=%04x unsupported\n", regs.u.r16.ax);
        SetCF(iret_addr.flags); // Unsupported
    }
}

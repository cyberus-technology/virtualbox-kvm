/** @file
 * Inline routines for Watcom C.
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
 * SPDX-License-Identifier: GPL-3.0-only
 */

#ifndef VBOX_INCLUDED_SRC_Graphics_BIOS_inlines_h
#define VBOX_INCLUDED_SRC_Graphics_BIOS_inlines_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

extern unsigned inp(unsigned port);
extern unsigned outp(unsigned port, unsigned value);
extern unsigned inpw(unsigned port);
extern unsigned outpw(unsigned port, unsigned value);
#pragma intrinsic(inp,outp,inpw,outpw)
#define inb(p)      inp(p)
#define outb(p, v)  outp(p, v)
#define inw(p)      inpw(p)
#define outw(p, v)  outpw(p, v)

/* Far byte/word/dword access routines. */

inline uint8_t read_byte(uint16_t seg, uint16_t offset)
{
    return( *(seg:>(uint8_t *)offset) );
}

inline void write_byte(uint16_t seg, uint16_t offset, uint8_t data)
{
    *(seg:>(uint8_t *)offset) = data;
}

inline uint16_t read_word(uint16_t seg, uint16_t offset)
{
    return( *(seg:>(uint16_t *)offset) );
}

inline void write_word(uint16_t seg, uint16_t offset, uint16_t data)
{
    *(seg:>(uint16_t *)offset) = data;
}

inline uint32_t read_dword(uint16_t seg, uint16_t offset)
{
    return( *(seg:>(uint32_t *)offset) );
}

inline void write_dword(uint16_t seg, uint16_t offset, uint32_t data)
{
    *(seg:>(uint32_t *)offset) = data;
}

void int_enable(void);
#pragma aux int_enable = "sti" modify exact [] nomemory;

void int_disable(void);
#pragma aux int_disable = "cli" modify exact [] nomemory;

uint16_t int_query(void);
#pragma aux int_query =     \
    "pushf"                 \
    "pop    ax"             \
    value [ax] modify exact [ax] nomemory;

void int_restore(uint16_t old_flags);
#pragma aux int_restore =   \
    "push   ax"             \
    "popf"                  \
    parm [ax] modify exact [] nomemory;

void halt(void);
#pragma aux halt = "hlt" modify exact [] nomemory;

void halt_forever(void);
#pragma aux halt_forever =  \
    "forever:"              \
    "hlt"                   \
    "jmp forever"           \
    modify exact [] nomemory aborts;

void rep_movsw(void __far *d, void __far *s, int nwords);
#pragma aux rep_movsw =     \
    "push   ds"             \
    "mov    ds, dx"         \
    "rep    movsw"          \
    "pop    ds"             \
    parm [es di] [dx si] [cx];

int repe_cmpsb(void __far *d, void __far *s, int nbytes);
#pragma aux repe_cmpsb =    \
    "push   ds"             \
    "mov    ds, dx"         \
    "repe   cmpsb"          \
    "pop    ds"             \
    "mov    ax, 0"          \
    "jz     match"          \
    "inc    al"             \
    "match:"                \
    parm [es di] [dx si] [cx] value [ax] modify nomemory;

char __far *rep_insb(char __far *buffer, unsigned nbytes, unsigned port);
#pragma aux rep_insb = ".286" "rep insb" parm [es di] [cx] [dx] value [es di] modify exact [cx di];

char __far *rep_insw(char __far *buffer, unsigned nwords, unsigned port);
#pragma aux rep_insw = ".286" "rep insw" parm [es di] [cx] [dx] value [es di] modify exact [cx di];

char __far *rep_outsb(char __far *buffer, unsigned nbytes, unsigned port);
#pragma aux rep_outsb = ".286" "rep outs dx,byte ptr es:[si]" parm [es si] [cx] [dx] value [es si] modify exact [cx si];

char __far *rep_outsw(char __far *buffer, unsigned nwords, unsigned port);
#pragma aux rep_outsw = ".286" "rep outs dx,word ptr es:[si]" parm [es si] [cx] [dx] value [es si] modify exact [cx si];

uint16_t __far swap_16(uint16_t val);
#pragma aux swap_16 = "xchg ah,al" parm [ax] value [ax] modify exact [ax] nomemory;

uint32_t __far swap_32(uint32_t val);
#pragma aux swap_32 =   \
    "xchg   ah, al"     \
    "xchg   dh, dl"     \
    "xchg   ax, dx"     \
    parm [dx ax] value [dx ax] modify exact [dx ax] nomemory;

extern void memsetb(uint16_t seg, uint16_t offset, uint16_t value, uint16_t count);
#pragma aux memsetb =   \
    "jcxz no_copy"      \
    "rep stosb"         \
    "no_copy:"          \
    parm [es] [di] [ax] [cx];

extern void memsetw(uint16_t seg, uint16_t offset, uint16_t value, uint16_t count);
#pragma aux memsetw =   \
    "jcxz no_copy"      \
    "rep stosw"         \
    "no_copy:"          \
    parm [es] [di] [ax] [cx];

extern void memcpyb(uint16_t dseg, uint16_t doffset, uint16_t sseg, uint16_t soffset, uint16_t count);
#pragma aux memcpyb =   \
    "jcxz   no_copy"    \
    "push   ds"         \
    "mov    ds, dx"     \
    "rep    movsb"      \
    "pop    ds"         \
    "no_copy:"          \
    parm [es] [di] [dx] [si] [cx];

extern void memcpyw(uint16_t dseg, uint16_t doffset, uint16_t sseg, uint16_t soffset, uint16_t count);
#pragma aux memcpyw =   \
    "jcxz   no_copy"    \
    "push   ds"         \
    "mov    ds, dx"     \
    "rep    movsw"      \
    "pop    ds"         \
    "no_copy:"          \
    parm [es] [di] [dx] [si] [cx];

#endif /* !VBOX_INCLUDED_SRC_Graphics_BIOS_inlines_h */


/* $XFree86: xc/programs/Xserver/hw/xfree86/loader/aout.h,v 1.7 2002/05/31 18:46:00 dawes Exp $ */

/*
 * Borrowed from NetBSD's exec_aout.h
 *
 * Copyright (c) 1993, 1994 Christopher G. Demetriou
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Christopher G. Demetriou.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifdef HAVE_XORG_CONFIG_H
#include <xorg-config.h>
#endif

#ifndef _AOUT_H
#define _AOUT_H

#include <X11/Xos.h>

/* Get prototype for ntohl, per SuSv3. */
#include <arpa/inet.h>

/* OS/2 EMX has ntohl in this file */
#ifdef __UNIXOS2__
#include <sys/param.h>
#endif

#define __LDPGSZ        4096U
#ifndef AOUT_PAGSIZ
#define AOUT_PAGSIZ(ex)    (__LDPGSZ)
#endif

/* 
 * a.out  header 
 */
typedef struct AOUT_exec {
    unsigned long a_midmag;	/* htonl(flags<<26 | mid<<16 | magic) */
    unsigned long a_text;	/* text segment size */
    unsigned long a_data;	/* initialized data size */
    unsigned long a_bss;	/* uninitialized data size */
    unsigned long a_syms;	/* symbol table size */
    unsigned long a_entry;	/* entry point */
    unsigned long a_trsize;	/* text relocation size */
    unsigned long a_drsize;	/* data relocation size */
} AOUTHDR;

/* a_magic */
#define OMAGIC          0407	/* old impure format */
#define NMAGIC          0410	/* read-only text */
#define ZMAGIC          0413	/* demand load format */
#define QMAGIC          0314	/* "compact" demand load format; deprecated */

/*
 * a_mid - keep sorted in numerical order for sanity's sake
 * ensure that: 0 < mid < 0x3ff
 */
#define MID_ZERO        0	/* unknown - implementation dependent */
#define MID_SUN010      1	/* sun 68010/68020 binary */
#define MID_SUN020      2	/* sun 68020-only binary */
#define MID_PC386       100	/* 386 PC binary. (so quoth BFD) */
#define MID_HP200       200	/* hp200 (68010) BSD binary */
#define MID_I386        134	/* i386 BSD binary */
#define MID_M68K        135	/* m68k BSD binary with 8K page sizes */
#define MID_M68K4K      136	/* m68k BSD binary with 4K page sizes */
#define MID_NS32532     137	/* ns32532 */
#define MID_SPARC       138	/* sparc */
#define MID_PMAX        139	/* pmax */
#define MID_VAX         140	/* vax */
#define MID_ALPHA       141	/* Alpha BSD binary */
#define MID_MIPS        142	/* big-endian MIPS */
#define MID_ARM6        143	/* ARM6 */
#define MID_HP300       300	/* hp300 (68020+68881) BSD binary */
#define MID_HPUX        0x20C	/* hp200/300 HP-UX binary */
#define MID_HPUX800     0x20B	/* hp800 HP-UX binary */

/*
 * a_flags
 */
#define EX_DYNAMIC      0x20
#define EX_PIC          0x10
#define EX_DPMASK       0x30
/*
 * Interpretation of the (a_flags & EX_DPMASK) bits:
 *
 *      00              traditional executable or object file
 *      01              object file contains PIC code (set by `as -k')
 *      10              dynamic executable
 *      11              position independent executable image
 *                      (eg. a shared library)
 *
 */

/*
 * The a.out structure's a_midmag field is a network-byteorder encoding
 * of this int
 *      FFFFFFmmmmmmmmmmMMMMMMMMMMMMMMMM
 * Where `F' is 6 bits of flag like EX_DYNAMIC,
 *       `m' is 10 bits of machine-id like MID_I386, and
 *       `M' is 16 bits worth of magic number, ie. ZMAGIC.
 * The macros below will set/get the needed fields.
 */
#define AOUT_GETMAGIC(ex) \
    ( (((ex)->a_midmag)&0xffff0000U) ? (ntohl(((ex)->a_midmag))&0xffffU) : ((ex)->a_midmag))
#define AOUT_GETMAGIC2(ex) \
    ( (((ex)->a_midmag)&0xffff0000U) ? (ntohl(((ex)->a_midmag))&0xffffU) : \
    (((ex)->a_midmag) | 0x10000) )
#define AOUT_GETMID(ex) \
    ( (((ex)->a_midmag)&0xffff0000U) ? ((ntohl(((ex)->a_midmag))>>16)&0x03ffU) : MID_ZERO )
#define AOUT_GETFLAG(ex) \
    ( (((ex)->a_midmag)&0xffff0000U) ? ((ntohl(((ex)->a_midmag))>>26)&0x3fU) : 0 )
#define AOUT_SETMAGIC(ex,mag,mid,flag) \
    ( (ex)->a_midmag = htonl( (((flag)&0x3fU)<<26) | (((mid)&0x03ffU)<<16) | \
    (((mag)&0xffffU)) ) )

#define AOUT_ALIGN(ex,x) \
        (AOUT_GETMAGIC(ex) == ZMAGIC || AOUT_GETMAGIC(ex) == QMAGIC ? \
        ((x) + __LDPGSZ - 1) & ~(__LDPGSZ - 1) : (x))

/* Valid magic number check. */
#define AOUT_BADMAG(ex) \
        (AOUT_GETMAGIC(ex) != NMAGIC && AOUT_GETMAGIC(ex) != OMAGIC && \
        AOUT_GETMAGIC(ex) != ZMAGIC && AOUT_GETMAGIC(ex) != QMAGIC)

/* Address of the bottom of the text segment. */
#define AOUT_TXTADDR(ex)   (AOUT_GETMAGIC2(ex) == (ZMAGIC|0x10000) ? 0 : __LDPGSZ)

/* Address of the bottom of the data segment. */
#define AOUT_DATADDR(ex) \
        (AOUT_GETMAGIC(ex) == OMAGIC ? AOUT_TXTADDR(ex) + (ex)->a_text : \
        (AOUT_TXTADDR(ex) + (ex)->a_text + __LDPGSZ - 1) & ~(__LDPGSZ - 1))

/* Address of the bottom of the bss segment. */
#define AOUT_BSSADDR(ex) \
        (AOUT_DATADDR(ex) + (ex)->a_data)

/* Text segment offset. */
#define AOUT_TXTOFF(ex) \
        ( AOUT_GETMAGIC2(ex)==ZMAGIC || AOUT_GETMAGIC2(ex)==(QMAGIC|0x10000) ? \
        0 : (AOUT_GETMAGIC2(ex)==(ZMAGIC|0x10000) ? __LDPGSZ : \
        sizeof(struct AOUT_exec)) )

/* Data segment offset. */
#define AOUT_DATOFF(ex) \
        AOUT_ALIGN(ex, AOUT_TXTOFF(ex) + (ex)->a_text)

/* Text relocation table offset. */
#define AOUT_TRELOFF(ex) \
        (AOUT_DATOFF(ex) + (ex)->a_data)

/* Data relocation table offset. */
#define AOUT_DRELOFF(ex) \
        (AOUT_TRELOFF(ex) + (ex)->a_trsize)

/* Symbol table offset. */
#define AOUT_SYMOFF(ex) \
        (AOUT_DRELOFF(ex) + (ex)->a_drsize)

/* String table offset. */
#define AOUT_STROFF(ex) \
        (AOUT_SYMOFF(ex) + (ex)->a_syms)

/* Relocation format. */
struct relocation_info_i386 {
    int r_address;		/* offset in text or data segment */
    unsigned int r_symbolnum:24,	/* ordinal number of add symbol */
        r_pcrel:1,		/* 1 if value should be pc-relative */
        r_length:2,		/* log base 2 of value's width */
        r_extern:1,		/* 1 if need to add symbol to value */
        r_baserel:1,		/* linkage table relative */
        r_jmptable:1,		/* relocate to jump table */
        r_relative:1,		/* load address relative */
        r_copy:1;		/* run time copy */
};

#define relocation_info relocation_info_i386

/*
 * Symbol table entry format.  The #ifdef's are so that programs including
 * nlist.h can initialize nlist structures statically.
 */
typedef struct AOUT_nlist {
    union {
	char *n_name;		/* symbol name (in memory) */
	long n_strx;		/* file string table offset (on disk) */
    } n_un;

#define AOUT_UNDF  0x00		/* undefined */
#define AOUT_ABS   0x02		/* absolute address */
#define AOUT_TEXT  0x04		/* text segment */
#define AOUT_DATA  0x06		/* data segment */
#define AOUT_BSS   0x08		/* bss segment */
#define AOUT_INDR  0x0a		/* alias definition */
#define AOUT_SIZE  0x0c		/* pseudo type, defines a symbol's size */
#define AOUT_COMM  0x12		/* common reference */
#define AOUT_FN    0x1e		/* file name (AOUT_EXT on) */
#define AOUT_WARN  0x1e		/* warning message (AOUT_EXT off) */

#define AOUT_EXT   0x01		/* external (global) bit, OR'ed in */
#define AOUT_TYPE  0x1e		/* mask for all the type bits */
    unsigned char n_type;	/* type defines */

    char n_other;		/* spare */
#define n_hash  n_desc		/* used internally by ld(1); XXX */
    short n_desc;		/* used by stab entries */
    unsigned long n_value;	/* address/value of the symbol */
} AOUT_nlist;

#define AOUT_FORMAT        "%08x"	/* namelist value format; XXX */
#define AOUT_STAB          0x0e0	/* mask for debugger symbols -- stab(5) */

#endif

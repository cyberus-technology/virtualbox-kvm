/* $XFree86: xc/programs/Xserver/hw/xfree86/loader/coff.h,v 1.5 1998/07/25 16:56:12 dawes Exp $ */

/* This file was implemented from the information in the book
   Understanding and Using COFF
   Gintaras R. Gircys
   O'Reilly, 1988
   and by looking at the Linux kernel code.

   It is therefore most likely free to use...

   If the file format changes in the COFF object, this file should be
   subsequently updated to reflect the changes.

   The actual loader module only uses a few of the COFF structures. 
   Only those are included here.  If you wish more information about 
   COFF, thein check out the book mentioned above.
*/

#ifdef HAVE_XORG_CONFIG_H
#include <xorg-config.h>
#endif

#ifndef _COFF_H
#define _COFF_H

#define  E_SYMNMLEN  8		/* Number of characters in a symbol name         */
/*
 * Intel 386/486  
 */

/*
 * FILE HEADER 
 */

typedef struct COFF_filehdr {
    unsigned short f_magic;	/* magic number                 */
    unsigned short f_nscns;	/* number of sections           */
    long f_timdat;		/* time & date stamp            */
    long f_symptr;		/* file pointer to symtab       */
    long f_nsyms;		/* number of symtab entries     */
    unsigned short f_opthdr;	/* sizeof(optional hdr)         */
    unsigned short f_flags;	/* flags                        */
} FILHDR;

#define	FILHSZ	sizeof(FILHDR)

/*
 * SECTION HEADER 
 */

typedef struct COFF_scnhdr {
    char s_name[8];		/* section name                 */
    long s_paddr;		/* physical address             */
    long s_vaddr;		/* virtual address              */
    long s_size;		/* section size                 */
    long s_scnptr;		/* raw data for section         */
    long s_relptr;		/* relocation                   */
    long s_lnnoptr;		/* line numbers                 */
    unsigned short s_nreloc;	/* number of relocation entries */
    unsigned short s_nlnno;	/* number of line number entries */
    long s_flags;		/* flags                        */
} SCNHDR;

#define	COFF_SCNHDR	struct COFF_scnhdr
#define	COFF_SCNHSZ	sizeof(COFF_SCNHDR)
#define SCNHSZ		COFF_SCNHSZ

/*
 * the optional COFF header as used by Linux COFF
 */

typedef struct {
    char magic[2];		/* type of file                  */
    char vstamp[2];		/* version stamp                 */
    char tsize[4];		/* text size in bytes            */
    char dsize[4];		/* initialized data              */
    char bsize[4];		/* uninitialized data            */
    char entry[4];		/* entry point                   */
    char text_start[4];		/* base of text                  */
    char data_start[4];		/* base of data                  */
} AOUTHDR;

/*
 * SYMBOLS 
 */

typedef struct COFF_syment {
    union {
	char _n_name[E_SYMNMLEN];	/* Symbol name (first 8 chars)  */
	struct {
	    long _n_zeroes;	/* Leading zeros               */
	    long _n_offset;	/* Offset for a header section */
	} _n_n;
	char *_n_nptr[2];	/* allows for overlaying       */
    } _n;

    long n_value;		/* address of the segment       */
    short n_scnum;		/* Section number               */
    unsigned short n_type;	/* Type of section              */
    char n_sclass;		/* Loader class                 */
    char n_numaux;		/* Number of aux entries following */
} SYMENT;

#define n_name		_n._n_name
#define n_nptr		_n._n_nptr[1]
#define n_zeroes	_n._n_n._n_zeroes
#define n_offset	_n._n_n._n_offset

#define COFF_E_SYMNMLEN	 8	/* characters in a short symbol name    */
#define COFF_E_FILNMLEN	14	/* characters in a file name            */
#define COFF_E_DIMNUM	 4	/* array dimensions in aux entry        */
#define SYMNMLEN	COFF_E_SYMNMLEN
#define SYMESZ		18	/* not really sizeof(SYMENT) due to padding */

/* Special section number found in the symbol section */
#define	N_UNDEF	0
#define	N_ABS	-1
#define	N_DEBUG	-2

/* Symbol storage class values */
#define C_NULL		0
#define C_EXT		2
#define C_FILE		103
#define C_HIDEXT	107

/*
 * AUX Entries
 */
typedef struct COFF_auxent {
    long x_scnlen;
    long x_parmhash;
    unsigned short x_snhash;
    unsigned char x_smtyp;
    unsigned char x_smclas;
    long x_stab;
    unsigned short x_snstab;
} AUXENT;

/* Auxillary Symbol type values */
#define XTY_ER	0		/* Enternal Reference */
#define XTY_SD	1		/* csect section definition */
#define XTY_LD	2		/* Label definition */
#define XTY_CM	3		/* common csect definition */

/* Auxillary Symbol storage mapping class values */
#define XMC_PR	0		/* Program code */
#define XMC_RO	1		/* Read-only constant */
#define XMC_DB	2		/* Debug dictionary */
#define XMC_TC	3		/* TOC entry */
#define XMC_UA	4		/* Unclassified */
#define XMC_RW	5		/* Read/write data */
#define XMC_GL	6		/* Global linkage */
#define XMC_XO	7		/* Extended operation */
#define XMC_SV	8		/* Supervisor call descriptor */
#define XMC_BS	9		/* BSS class */
#define XMC_DS	10		/* Function descriptor csect */
#define XMC_UC	11		/* Unnamed FORTRAN comon */
#define XMC_TI	12		/* Reserved */
#define XMC_TB	13		/* Reserved */
#define XMC_TC0	15		/* TOC anchor */
#define XMC_TD	16		/* Scalar data entry in TOC */

/*
 * RELOCATION DIRECTIVES
 */

typedef struct COFF_reloc {
    long r_vaddr;		/* Virtual address of item    */
    long r_symndx;		/* Symbol index in the symtab */
#if defined(__powerpc__)
    union {
	unsigned short _r_type;	/* old style coff relocation type */
	struct {
	    char _r_rsize;	/* sign and reloc bit len */
	    char _r_rtype;	/* toc relocation type */
	} _r_r;
    } _r;
#define r_otype  _r._r_type	/* old style reloc - original name */
#define r_rsize _r._r_r._r_rsize	/* extract sign and bit len    */
#define r_type _r._r_r._r_rtype	/* extract toc relocation type */
#else
    unsigned short r_type;	/* Relocation type             */
#endif
} RELOC;

#define COFF_RELOC	struct COFF_reloc
#define COFF_RELSZ	10
#define RELSZ		COFF_RELSZ

/*
 * x86 Relocation types 
 */
#define	R_ABS		000
#define	R_DIR32		006
#define	R_PCRLONG	024

#if defined(__powerpc__)
/*
 * Power PC
 */
#define R_LEN	0x1F		/* extract bit-length field */
#define R_SIGN	0x80		/* extract sign of relocation */
#define R_FIXUP	0x40		/* extract code-fixup bit */

#define RELOC_RLEN(x)	((x)._r._r_r._r_rsize & R_LEN)
#define RELOC_RSIGN(x)	((x)._r._r_r._r_rsize & R_SIGN)
#define RELOC_RFIXUP(x)	((x)._r._r_r._r_rsize & R_FIXUP)
#define RELOC_RTYPE(x)	((x)._r._r_r._r_rtype)

/*
 * POWER and PowerPC - relocation types
 */
#define R_POS	0x00	/* A(sym) Positive Relocation */
#define R_NEG	0x01	/* -A(sym) Negative Relocation */
#define R_REL	0x02	/* A(sym-*) Relative to self */
#define R_TOC	0x03	/* A(sym-TOC) Relative to TOC */
#define R_TRL	0x12	/* A(sym-TOC) TOC Relative indirect load. */
					/* modifiable instruction */
#define R_TRLA	0x13	/* A(sym-TOC) TOC Rel load address. modifiable inst */
#define R_GL	0x05	/* A(external TOC of sym) Global Linkage */
#define R_TCL	0x06	/* A(local TOC of sym) Local object TOC address */
#define R_RL	0x0C	/* A(sym) Pos indirect load. modifiable instruction */
#define R_RLA	0x0D	/* A(sym) Pos Load Address. modifiable instruction */
#define R_REF	0x0F	/* AL0(sym) Non relocating ref. No garbage collect */
#define R_BA	0x08	/* A(sym) Branch absolute. Cannot modify instruction */
#define R_RBA	0x18	/* A(sym) Branch absolute. modifiable instruction */
#define R_RBAC	0x19	/* A(sym) Branch absolute constant. modifiable instr */
#define R_BR	0x0A	/* A(sym-*) Branch rel to self. non modifiable */
#define R_RBR	0x1A	/* A(sym-*) Branch rel to self. modifiable instr */
#define R_RBRC	0x1B	/* A(sym-*) Branch absolute const. */
						/* modifiable to R_RBR */
#define R_RTB	0x04	/* A((sym-*)/2) RT IAR Rel Branch. non modifiable */
#define R_RRTBI	0x14	/* A((sym-*)/2) RT IAR Rel Br. modifiable to R_RRTBA */
#define R_RRTBA	0x15	/* A((sym-*)/2) RT absolute br. modifiable to R_RRTBI */
#endif /* __powerpc */

#endif /* _COFF_H */

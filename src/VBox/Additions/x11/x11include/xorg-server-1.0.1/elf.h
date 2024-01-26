/* $XFree86: xc/programs/Xserver/hw/xfree86/loader/elf.h,v 1.16 2003/06/12 14:12:34 eich Exp $ */

typedef unsigned int Elf32_Addr;
typedef unsigned short Elf32_Half;
typedef unsigned int Elf32_Off;
typedef long Elf32_Sword;
typedef unsigned int Elf32_Word;

typedef unsigned long Elf64_Addr;
typedef unsigned short Elf64_Half;
typedef unsigned long Elf64_Off;
typedef int Elf64_Sword;
typedef unsigned int Elf64_Word;
typedef unsigned long Elf64_Xword;
typedef long Elf64_Sxword;

/* These constants are for the segment types stored in the image headers */
#define PT_NULL    0
#define PT_LOAD    1
#define PT_DYNAMIC 2
#define PT_INTERP  3
#define PT_NOTE    4
#define PT_SHLIB   5
#define PT_PHDR    6
#define PT_LOPROC  0x70000000
#define PT_HIPROC  0x7fffffff

/* These constants define the different elf file types */
#define ET_NONE   0
#define ET_REL    1
#define ET_EXEC   2
#define ET_DYN    3
#define ET_CORE   4
#define ET_LOPROC 5
#define ET_HIPROC 6

/* These constants define the various ELF target machines */
#define EM_NONE  	0
#define EM_M32   	1
#define EM_SPARC 	2
#define EM_386   	3
#define EM_68K   	4
#define EM_88K   	5
#define EM_486   	6	/* Perhaps disused */
#define EM_860   	7
#define EM_MIPS		8
#define EM_MIPS_RS4_BE 10
#define EM_PARISC      15
#define EM_SPARC32PLUS 18
#define EM_PPC	       20
#define EM_SPARCV9     43
#define EM_IA_64       50
#define EM_ALPHA       0x9026

/* This is the info that is needed to parse the dynamic section of the file */
#define DT_NULL		0
#define DT_NEEDED	1
#define DT_PLTRELSZ	2
#define DT_PLTGOT	3
#define DT_HASH		4
#define DT_STRTAB	5
#define DT_SYMTAB	6
#define DT_RELA		7
#define DT_RELASZ	8
#define DT_RELAENT	9
#define DT_STRSZ	10
#define DT_SYMENT	11
#define DT_INIT		12
#define DT_FINI		13
#define DT_SONAME	14
#define DT_RPATH 	15
#define DT_SYMBOLIC	16
#define DT_REL	        17
#define DT_RELSZ	18
#define DT_RELENT	19
#define DT_PLTREL	20
#define DT_DEBUG	21
#define DT_TEXTREL	22
#define DT_JMPREL	23
#define DT_LOPROC	0x70000000
#define DT_HIPROC	0x7fffffff

/* This info is needed when parsing the symbol table */
#define STB_LOCAL  0
#define STB_GLOBAL 1
#define STB_WEAK   2

#define STT_NOTYPE  0
#define STT_OBJECT  1
#define STT_FUNC    2
#define STT_SECTION 3
#define STT_FILE    4
#define STT_LOPROC  13
#define STT_HIPROC  15

#define ELF32_ST_BIND(x) ((x) >> 4)
#define ELF32_ST_TYPE(x) (((unsigned int) x) & 0xf)

#define ELF64_ST_BIND(x) ELF32_ST_BIND (x)
#define ELF64_ST_TYPE(x) ELF32_ST_TYPE (x)

typedef struct dynamic32 {
    Elf32_Sword d_tag;
    union {
	Elf32_Sword d_val;
	Elf32_Addr d_ptr;
    } d_un;
} Elf32_Dyn;

typedef struct dynamic64 {
    Elf64_Sxword d_tag;
    union {
	Elf64_Xword d_val;
	Elf64_Addr d_ptr;
    } d_un;
} Elf64_Dyn;

#ifdef HAVE_XORG_CONFIG_H
#include <xorg-config.h>
#endif

#ifndef QNX4
extern Elf32_Dyn _DYNAMIC[];
#endif

/* The following are used with relocations */
#define ELF32_R_SYM(x) ((x) >> 8)
#define ELF32_R_TYPE(x) ((x) & 0xff)

#define ELF64_R_SYM(x)  ((x) >> 32)
#define ELF64_R_TYPE(x)  ((x) & 0xffffffff)

/* x86 Relocation Types */
#define R_386_NONE	0
#define R_386_32	1
#define R_386_PC32	2
#define R_386_GOT32	3
#define R_386_PLT32	4
#define R_386_COPY	5
#define R_386_GLOB_DAT	6
#define R_386_JMP_SLOT	7
#define R_386_RELATIVE	8
#define R_386_GOTOFF	9
#define R_386_GOTPC	10
#define R_386_NUM	11

/* AMD64 Relocation Types */
#define R_X86_64_NONE                   0
#define R_X86_64_64                     1
#define R_X86_64_PC32                   2
#define R_X86_64_GOT32                  3
#define R_X86_64_PLT32                  4
#define R_X86_64_COPY                   5
#define R_X86_64_GLOB_DAT               6
#define R_X86_64_JUMP_SLOT              7
#define R_X86_64_RELATIVE               8
#define R_X86_64_GOTPCREL               9
#define R_X86_64_32                    10
#define R_X86_64_32S                   11
#define R_X86_64_16                    12
#define R_X86_64_PC16                  13
#define R_X86_64_8                     14
#define R_X86_64_PC8                   15
#define R_X86_64_GNU_VTINHERIT         250
#define R_X86_64_GNU_VTENTRY           251

/* sparc Relocation Types */
#define	R_SPARC_NONE		0
#define	R_SPARC_8		1
#define	R_SPARC_16		2
#define	R_SPARC_32		3
#define	R_SPARC_DISP8		4
#define	R_SPARC_DISP16		5
#define	R_SPARC_DISP32		6
#define	R_SPARC_WDISP30		7
#define	R_SPARC_WDISP22		8
#define	R_SPARC_HI22		9
#define	R_SPARC_22		10
#define	R_SPARC_13		11
#define	R_SPARC_LO10		12
#define	R_SPARC_GOT10		13
#define	R_SPARC_GOT13		14
#define	R_SPARC_GOT22		15
#define	R_SPARC_PC10		16
#define	R_SPARC_PC22		17
#define	R_SPARC_WPLT30		18
#define	R_SPARC_COPY		19
#define	R_SPARC_GLOB_DAT	20
#define	R_SPARC_JMP_SLOT	21
#define	R_SPARC_RELATIVE	22
#define	R_SPARC_UA32		23
#define	R_SPARC_PLT32		24
#define	R_SPARC_HIPLT22		25
#define	R_SPARC_LOPLT10		26
#define	R_SPARC_PCPLT32		27
#define	R_SPARC_PCPLT22		28
#define	R_SPARC_PCPLT10		29
#define	R_SPARC_10		30
#define	R_SPARC_11		31
#define	R_SPARC_64		32
#define	R_SPARC_OLO10		33
#define	R_SPARC_HH22		34
#define	R_SPARC_HM10		35
#define	R_SPARC_LM22		36
#define	R_SPARC_PC_HH22		37
#define	R_SPARC_PC_HM10		38
#define	R_SPARC_PC_LM22		39
#define	R_SPARC_WDISP16		40
#define	R_SPARC_WDISP19		41
#define	R_SPARC_GLOB_JMP	42
#define	R_SPARC_7		43
#define	R_SPARC_5		44
#define	R_SPARC_6		45
#define	R_SPARC_DISP64		46
#define	R_SPARC_PLT64		47
#define	R_SPARC_HIX22		48
#define	R_SPARC_LOX10		49
#define	R_SPARC_H44		50
#define	R_SPARC_M44		51
#define	R_SPARC_L44		52
#define	R_SPARC_REGISTER	53
#define	R_SPARC_UA64		54
#define	R_SPARC_UA16		55
#define	R_SPARC_NUM		56

/* m68k Relocation Types */
#define R_68K_NONE	0	/* No reloc */
#define R_68K_32	1	/* Direct 32 bit  */
#define R_68K_16	2	/* Direct 16 bit  */
#define R_68K_8		3	/* Direct 8 bit  */
#define R_68K_PC32	4	/* PC relative 32 bit */
#define R_68K_PC16	5	/* PC relative 16 bit */
#define R_68K_PC8	6	/* PC relative 8 bit */
#define R_68K_GOT32	7	/* 32 bit PC relative GOT entry */
#define R_68K_GOT16	8	/* 16 bit PC relative GOT entry */
#define R_68K_GOT8	9	/* 8 bit PC relative GOT entry */
#define R_68K_GOT32O	10	/* 32 bit GOT offset */
#define R_68K_GOT16O	11	/* 16 bit GOT offset */
#define R_68K_GOT8O	12	/* 8 bit GOT offset */
#define R_68K_PLT32	13	/* 32 bit PC relative PLT address */
#define R_68K_PLT16	14	/* 16 bit PC relative PLT address */
#define R_68K_PLT8	15	/* 8 bit PC relative PLT address */
#define R_68K_PLT32O	16	/* 32 bit PLT offset */
#define R_68K_PLT16O	17	/* 16 bit PLT offset */
#define R_68K_PLT8O	18	/* 8 bit PLT offset */
#define R_68K_COPY	19	/* Copy symbol at runtime */
#define R_68K_GLOB_DAT	20	/* Create GOT entry */
#define R_68K_JMP_SLOT	21	/* Create PLT entry */
#define R_68K_RELATIVE	22	/* Adjust by program base */

/* Alpha Relocation Types */
#define R_ALPHA_NONE		0	/* No reloc */
#define R_ALPHA_REFLONG		1	/* Direct 32 bit */
#define R_ALPHA_REFQUAD		2	/* Direct 64 bit */
#define R_ALPHA_GPREL32		3	/* GP relative 32 bit */
#define R_ALPHA_LITERAL		4	/* GP relative 16 bit w/optimization */
#define R_ALPHA_LITUSE		5	/* Optimization hint for LITERAL */
#define R_ALPHA_GPDISP		6	/* Add displacement to GP */
#define R_ALPHA_BRADDR		7	/* PC+4 relative 23 bit shifted */
#define R_ALPHA_HINT		8	/* PC+4 relative 16 bit shifted */
#define R_ALPHA_SREL16		9	/* PC relative 16 bit */
#define R_ALPHA_SREL32		10	/* PC relative 32 bit */
#define R_ALPHA_SREL64		11	/* PC relative 64 bit */
#define R_ALPHA_OP_PUSH		12	/* OP stack push */
#define R_ALPHA_OP_STORE	13	/* OP stack pop and store */
#define R_ALPHA_OP_PSUB		14	/* OP stack subtract */
#define R_ALPHA_OP_PRSHIFT	15	/* OP stack right shift */
#define R_ALPHA_GPVALUE		16
#define R_ALPHA_GPRELHIGH	17
#define R_ALPHA_GPRELLOW	18
#define R_ALPHA_GPREL16		19
#define R_ALPHA_IMMED_GP_HI32	20
#define R_ALPHA_IMMED_SCN_HI32	21
#define R_ALPHA_IMMED_BR_HI32	22
#define R_ALPHA_IMMED_LO32	23
#define R_ALPHA_COPY		24	/* Copy symbol at runtime */
#define R_ALPHA_GLOB_DAT	25	/* Create GOT entry */
#define R_ALPHA_JMP_SLOT	26	/* Create PLT entry */
#define R_ALPHA_RELATIVE	27	/* Adjust by program base */
#define R_ALPHA_BRSGP		28	/* Calc displacement for BRS */   

/* IA-64 relocations.  */
#define R_IA64_NONE		0x00	/* none */
#define R_IA64_IMM14		0x21	/* symbol + addend, add imm14 */
#define R_IA64_IMM22		0x22	/* symbol + addend, add imm22 */
#define R_IA64_IMM64		0x23	/* symbol + addend, mov imm64 */
#define R_IA64_DIR32MSB		0x24	/* symbol + addend, data4 MSB */
#define R_IA64_DIR32LSB		0x25	/* symbol + addend, data4 LSB */
#define R_IA64_DIR64MSB		0x26	/* symbol + addend, data8 MSB */
#define R_IA64_DIR64LSB		0x27	/* symbol + addend, data8 LSB */
#define R_IA64_GPREL22		0x2a	/* @gprel(sym + add), add imm22 */
#define R_IA64_GPREL64I		0x2b	/* @gprel(sym + add), mov imm64 */
#define R_IA64_GPREL64MSB	0x2e	/* @gprel(sym + add), data8 MSB */
#define R_IA64_GPREL64LSB	0x2f	/* @gprel(sym + add), data8 LSB */
#define R_IA64_LTOFF22		0x32	/* @ltoff(sym + add), add imm22 */
#define R_IA64_LTOFF64I		0x33	/* @ltoff(sym + add), mov imm64 */
#define R_IA64_PLTOFF22		0x3a	/* @pltoff(sym + add), add imm22 */
#define R_IA64_PLTOFF64I	0x3b	/* @pltoff(sym + add), mov imm64 */
#define R_IA64_PLTOFF64MSB	0x3e	/* @pltoff(sym + add), data8 MSB */
#define R_IA64_PLTOFF64LSB	0x3f	/* @pltoff(sym + add), data8 LSB */
#define R_IA64_FPTR64I		0x43	/* @fptr(sym + add), mov imm64 */
#define R_IA64_FPTR32MSB	0x44	/* @fptr(sym + add), data4 MSB */
#define R_IA64_FPTR32LSB	0x45	/* @fptr(sym + add), data4 LSB */
#define R_IA64_FPTR64MSB	0x46	/* @fptr(sym + add), data8 MSB */
#define R_IA64_FPTR64LSB	0x47	/* @fptr(sym + add), data8 LSB */
#define R_IA64_PCREL21B		0x49	/* @pcrel(sym + add), ptb, call */
#define R_IA64_PCREL21M		0x4a	/* @pcrel(sym + add), chk.s */
#define R_IA64_PCREL21F		0x4b	/* @pcrel(sym + add), fchkf */
#define R_IA64_PCREL32MSB	0x4c	/* @pcrel(sym + add), data4 MSB */
#define R_IA64_PCREL32LSB	0x4d	/* @pcrel(sym + add), data4 LSB */
#define R_IA64_PCREL64MSB	0x4e	/* @pcrel(sym + add), data8 MSB */
#define R_IA64_PCREL64LSB	0x4f	/* @pcrel(sym + add), data8 LSB */
#define R_IA64_LTOFF_FPTR22	0x52	/* @ltoff(@fptr(s+a)), imm22 */
#define R_IA64_LTOFF_FPTR64I	0x53	/* @ltoff(@fptr(s+a)), imm64 */
#define R_IA64_SEGREL32MSB	0x5c	/* @segrel(sym + add), data4 MSB */
#define R_IA64_SEGREL32LSB	0x5d	/* @segrel(sym + add), data4 LSB */
#define R_IA64_SEGREL64MSB	0x5e	/* @segrel(sym + add), data8 MSB */
#define R_IA64_SEGREL64LSB	0x5f	/* @segrel(sym + add), data8 LSB */
#define R_IA64_SECREL32MSB	0x64	/* @secrel(sym + add), data4 MSB */
#define R_IA64_SECREL32LSB	0x65	/* @secrel(sym + add), data4 LSB */
#define R_IA64_SECREL64MSB	0x66	/* @secrel(sym + add), data8 MSB */
#define R_IA64_SECREL64LSB	0x67	/* @secrel(sym + add), data8 LSB */
#define R_IA64_REL32MSB		0x6c	/* data 4 + REL */
#define R_IA64_REL32LSB		0x6d	/* data 4 + REL */
#define R_IA64_REL64MSB		0x6e	/* data 8 + REL */
#define R_IA64_REL64LSB		0x6f	/* data 8 + REL */
#define R_IA64_LTV32MSB		0x70	/* symbol + addend, data4 MSB */
#define R_IA64_LTV32LSB		0x71	/* symbol + addend, data4 LSB */
#define R_IA64_LTV64MSB		0x72	/* symbol + addend, data8 MSB */
#define R_IA64_LTV64LSB		0x73	/* symbol + addend, data8 LSB */
#define R_IA64_IPLTMSB		0x80	/* dynamic reloc, imported PLT, MSB */
#define R_IA64_IPLTLSB		0x81	/* dynamic reloc, imported PLT, LSB */
#define R_IA64_LTOFF22X		0x86	/* LTOFF22, relaxable.  */
#define R_IA64_LDXMOV		0x87	/* Use of LTOFF22X.  */

#define R_IA64_TYPE(R)		((R) & -8)
#define R_IA64_FORMAT(R)	((R) & 7)

/*
 * Apparantly, Linux and PowerMAXOS use different version of ELF as the
 * Relocation types are very different.
 */
#if defined(PowerMAX_OS)
/* PPC Relocation Types */
#define R_PPC_NONE              0
#define R_PPC_COPY              1
#define R_PPC_GOTP_ENT          2
#define R_PPC_8                 4
#define R_PPC_8S                5
#define R_PPC_16S               7
#define R_PPC_14                8
#define R_PPC_DISP14            9
#define R_PPC_24                10
#define R_PPC_DISP24            11
#define R_PPC_PLT_DISP24        14
#define R_PPC_BBASED_16HU       15
#define R_PPC_BBASED_32         16
#define R_PPC_BBASED_32UA       17
#define R_PPC_BBASED_16H        18
#define R_PPC_BBASED_16L        19
#define R_PPC_ABDIFF_16HU       23
#define R_PPC_ABDIFF_32         24
#define R_PPC_ABDIFF_32UA       25
#define R_PPC_ABDIFF_16H        26
#define R_PPC_ABDIFF_16L        27
#define R_PPC_ABDIFF_16         28
#define R_PPC_16HU              31
#define R_PPC_32                32
#define R_PPC_32UA              33
#define R_PPC_16H               34
#define R_PPC_16L               35
#define R_PPC_16                36
#define R_PPC_GOT_16HU          39
#define R_PPC_GOT_32            40
#define R_PPC_GOT_32UA          41
#define R_PPC_GOT_16H           42
#define R_PPC_GOT_16L           43
#define R_PPC_GOT_16            44
#define R_PPC_GOTP_16HU         47
#define R_PPC_GOTP_32           48
#define R_PPC_GOTP_32UA         49
#define R_PPC_GOTP_16H          50
#define R_PPC_GOTP_16L          51
#define R_PPC_GOTP_16           52
#define R_PPC_PLT_16HU          55
#define R_PPC_PLT_32            56
#define R_PPC_PLT_32UA          57
#define R_PPC_PLT_16H           58
#define R_PPC_PLT_16L           59
#define R_PPC_PLT_16            60
#define R_PPC_ABREL_16HU        63
#define R_PPC_ABREL_32          64
#define R_PPC_ABREL_32UA        65
#define R_PPC_ABREL_16H         66
#define R_PPC_ABREL_16L         67
#define R_PPC_ABREL_16          68
#define R_PPC_GOT_ABREL_16HU    71
#define R_PPC_GOT_ABREL_32      72
#define R_PPC_GOT_ABREL_32UA    73
#define R_PPC_GOT_ABREL_16H     74
#define R_PPC_GOT_ABREL_16L     75
#define R_PPC_GOT_ABREL_16      76
#define R_PPC_GOTP_ABREL_16HU   79
#define R_PPC_GOTP_ABREL_32     80
#define R_PPC_GOTP_ABREL_32UA   81
#define R_PPC_GOTP_ABREL_16H    82
#define R_PPC_GOTP_ABREL_16L    83
#define R_PPC_GOTP_ABREL_16     84
#define R_PPC_PLT_ABREL_16HU    87
#define R_PPC_PLT_ABREL_32      88
#define R_PPC_PLT_ABREL_32UA    89
#define R_PPC_PLT_ABREL_16H     90
#define R_PPC_PLT_ABREL_16L     91
#define R_PPC_PLT_ABREL_16      92
#define R_PPC_SREL_16HU         95
#define R_PPC_SREL_32           96
#define R_PPC_SREL_32UA         97
#define R_PPC_SREL_16H          98
#define R_PPC_SREL_16L          99
#else
/*
 * The Linux version
 */
#define R_PPC_NONE		0
#define R_PPC_ADDR32		1
#define R_PPC_ADDR24		2
#define R_PPC_ADDR16		3
#define R_PPC_ADDR16_LO		4
#define R_PPC_ADDR16_HI		5
#define R_PPC_ADDR16_HA		6
#define R_PPC_ADDR14		7
#define R_PPC_ADDR14_BRTAKEN	8
#define R_PPC_ADDR14_BRNTAKEN	9
#define R_PPC_REL24		10
#define R_PPC_REL14		11
#define R_PPC_REL14_BRTAKEN	12
#define R_PPC_REL14_BRNTAKEN	13
#define R_PPC_GOT16		14
#define R_PPC_GOT16_LO		15
#define R_PPC_GOT16_HI		16
#define R_PPC_GOT16_HA		17
#define R_PPC_PLTREL24		18
#define R_PPC_COPY		19
#define R_PPC_GLOB_DAT		20
#define R_PPC_JMP_SLOT		21
#define R_PPC_RELATIVE		22
#define R_PPC_LOCAL24PC		23
#define R_PPC_UADDR32		24
#define R_PPC_UADDR16		25
#define R_PPC_REL32		26
#define R_PPC_PLT32		27
#define R_PPC_PLTREL32		28
#define R_PPC_PLT16_LO		29
#define R_PPC_PLT16_HI		30
#define R_PPC_PLT16_HA		31
#define R_PPC_SDAREL16		32
#define R_PPC_SECTOFF		33
#define R_PPC_SECTOFF_LO	34
#define R_PPC_SECTOFF_HI	35
#define R_PPC_SECTOFF_HA	36
#endif

/* ARM relocs.  */
#define R_ARM_NONE		0	/* No reloc */
#define R_ARM_PC24		1	/* PC relative 26 bit branch */
#define R_ARM_ABS32		2	/* Direct 32 bit  */
#define R_ARM_REL32		3	/* PC relative 32 bit */
#define R_ARM_PC13		4
#define R_ARM_ABS16		5	/* Direct 16 bit */
#define R_ARM_ABS12		6	/* Direct 12 bit */
#define R_ARM_THM_ABS5		7
#define R_ARM_ABS8		8	/* Direct 8 bit */
#define R_ARM_SBREL32		9
#define R_ARM_THM_PC22		10
#define R_ARM_THM_PC8		11
#define R_ARM_AMP_VCALL9	12
#define R_ARM_SWI24		13
#define R_ARM_THM_SWI8		14
#define R_ARM_XPC25		15
#define R_ARM_THM_XPC22		16
#define R_ARM_COPY		20	/* Copy symbol at runtime */
#define R_ARM_GLOB_DAT		21	/* Create GOT entry */
#define R_ARM_JUMP_SLOT		22	/* Create PLT entry */
#define R_ARM_RELATIVE		23	/* Adjust by program base */
#define R_ARM_GOTOFF		24	/* 32 bit offset to GOT */
#define R_ARM_GOTPC		25	/* 32 bit PC relative offset to GOT */
#define R_ARM_GOT32		26	/* 32 bit GOT entry */
#define R_ARM_PLT32		27	/* 32 bit PLT address */
#define R_ARM_GNU_VTENTRY	100
#define R_ARM_GNU_VTINHERIT	101
#define R_ARM_THM_PC11		102	/* thumb unconditional branch */
#define R_ARM_THM_PC9		103	/* thumb conditional branch */
#define R_ARM_RXPC25		249
#define R_ARM_RSBREL32		250
#define R_ARM_THM_RPC22		251
#define R_ARM_RREL32		252
#define R_ARM_RABS22		253
#define R_ARM_RPC24		254
#define R_ARM_RBASE		255

typedef struct elf32_rel {
    Elf32_Addr r_offset;
    Elf32_Word r_info;
} Elf32_Rel;

typedef struct elf64_rel {
    Elf64_Addr r_offset;
    Elf64_Xword r_info;
} Elf64_Rel;

typedef struct elf32_rela {
    Elf32_Addr r_offset;
    Elf32_Word r_info;
    Elf32_Sword r_addend;
} Elf32_Rela;

typedef struct elf64_rela {
    Elf64_Addr r_offset;
    Elf64_Xword r_info;
    Elf64_Sxword r_addend;
} Elf64_Rela;

typedef struct elf32_sym {
    Elf32_Word st_name;
    Elf32_Addr st_value;
    Elf32_Word st_size;
    unsigned char st_info;
    unsigned char st_other;
    Elf32_Half st_shndx;
} Elf32_Sym;

typedef struct elf64_sym {
    Elf64_Word st_name;
    unsigned char st_info;
    unsigned char st_other;
    Elf64_Half st_shndx;
    Elf64_Addr st_value;
    Elf64_Xword st_size;
} Elf64_Sym;

#define EI_NIDENT	16

typedef struct elf32hdr {
    unsigned char e_ident[EI_NIDENT];
    Elf32_Half e_type;
    Elf32_Half e_machine;
    Elf32_Word e_version;
    Elf32_Addr e_entry;		/* Entry point */
    Elf32_Off e_phoff;
    Elf32_Off e_shoff;
    Elf32_Word e_flags;
    Elf32_Half e_ehsize;
    Elf32_Half e_phentsize;
    Elf32_Half e_phnum;
    Elf32_Half e_shentsize;
    Elf32_Half e_shnum;
    Elf32_Half e_shstrndx;
} Elf32_Ehdr;

typedef struct elf64hdr {
    unsigned char e_ident[EI_NIDENT];
    Elf64_Half e_type;
    Elf64_Half e_machine;
    Elf64_Word e_version;
    Elf64_Addr e_entry;
    Elf64_Off e_phoff;
    Elf64_Off e_shoff;
    Elf64_Word e_flags;
    Elf64_Half e_ehsize;
    Elf64_Half e_phentsize;
    Elf64_Half e_phnum;
    Elf64_Half e_shentsize;
    Elf64_Half e_shnum;
    Elf64_Half e_shstrndx;
} Elf64_Ehdr;

/* These constants define the permissions on sections in the program
   header, p_flags. */
#define PF_R		0x4
#define PF_W		0x2
#define PF_X		0x1

typedef struct elf_phdr {
    Elf32_Word p_type;
    Elf32_Off p_offset;
    Elf32_Addr p_vaddr;
    Elf32_Addr p_paddr;
    Elf32_Word p_filesz;
    Elf32_Word p_memsz;
    Elf32_Word p_flags;
    Elf32_Word p_align;
} Elf32_Phdr;

typedef struct {
    Elf64_Word p_type;
    Elf64_Word p_flags;
    Elf64_Off p_offset;
    Elf64_Addr p_vaddr;
    Elf64_Addr p_paddr;
    Elf64_Xword p_filesz;
    Elf64_Xword p_memsz;
    Elf64_Xword p_align;
} Elf64_Phdr;

/* sh_type */
#define SHT_NULL	0
#define SHT_PROGBITS	1
#define SHT_SYMTAB	2
#define SHT_STRTAB	3
#define SHT_RELA	4
#define SHT_HASH	5
#define SHT_DYNAMIC	6
#define SHT_NOTE	7
#define SHT_NOBITS	8
#define SHT_REL		9
#define SHT_SHLIB	10
#define SHT_DYNSYM	11
#define SHT_NUM		12
#define SHT_LOPROC	0x70000000
#define SHT_HIPROC	0x7fffffff
#define SHT_LOUSER	0x80000000
#define SHT_HIUSER	0xffffffff

#define SHT_IA_64_UNWIND	(SHT_LOPROC + 1)	/* unwind bits */

/* sh_flags */
#define SHF_WRITE	0x1
#define SHF_ALLOC	0x2
#define SHF_EXECINSTR	0x4
#define SHF_MASKPROC	0xf0000000

/* special section indexes */
#define SHN_UNDEF	0
#define SHN_LORESERVE	0xff00
#define SHN_LOPROC	0xff00
#define SHN_HIPROC	0xff1f
#define SHN_ABS		0xfff1
#define SHN_COMMON	0xfff2
#define SHN_HIRESERVE	0xffff

typedef struct {
    Elf32_Word sh_name;
    Elf32_Word sh_type;
    Elf32_Word sh_flags;
    Elf32_Addr sh_addr;
    Elf32_Off sh_offset;
    Elf32_Word sh_size;
    Elf32_Word sh_link;
    Elf32_Word sh_info;
    Elf32_Word sh_addralign;
    Elf32_Word sh_entsize;
} Elf32_Shdr;

typedef struct {
    Elf64_Word sh_name;
    Elf64_Word sh_type;
    Elf64_Xword sh_flags;
    Elf64_Addr sh_addr;
    Elf64_Off sh_offset;
    Elf64_Xword sh_size;
    Elf64_Word sh_link;
    Elf64_Word sh_info;
    Elf64_Xword sh_addralign;
    Elf64_Xword sh_entsize;
} Elf64_Shdr;

#define	EI_MAG0		0	/* e_ident[] indexes */
#define	EI_MAG1		1
#define	EI_MAG2		2
#define	EI_MAG3		3
#define	EI_CLASS	4
#define	EI_DATA		5
#define	EI_VERSION	6
#define	EI_PAD		7

#define	ELFMAG0		0x7f	/* EI_MAG */
#define	ELFMAG1		'E'
#define	ELFMAG2		'L'
#define	ELFMAG3		'F'
#define	ELFMAG		"\177ELF"
#define	SELFMAG		4

#define	ELFDLMAG	3
#define	ELFDLOFF	16

#define	ELFCLASSNONE	0	/* EI_CLASS */
#define	ELFCLASS32	1
#define	ELFCLASS64	2
#define	ELFCLASSNUM	3

#define ELFDATANONE	0	/* e_ident[EI_DATA] */
#define ELFDATA2LSB	1
#define ELFDATA2MSB	2

#define EV_NONE		0	/* e_version, EI_VERSION */
#define EV_CURRENT	1
#define EV_NUM		2

/* Notes used in ET_CORE */
#define NT_PRSTATUS	1
#define NT_PRFPREG	2
#define NT_PRPSINFO	3
#define NT_TASKSTRUCT	4

/* Note header in a PT_NOTE section */
typedef struct elf_note {
    Elf32_Word n_namesz;	/* Name size */
    Elf32_Word n_descsz;	/* Content size */
    Elf32_Word n_type;		/* Content type */
} Elf32_Nhdr;

#define ELF_START_MMAP 0x80000000

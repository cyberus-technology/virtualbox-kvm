/*
 * as -arch x86_64 -mmacosx-version-min=10.7-o tstRTLdrMachORelocs-1-as.o tstRTLdrMachORelocs-1.S
 * ld -arch x86_64 -r -o tstRTLdrMachORelocs-1.o tstRTLdrMachORelocs-1-as.o
 * tstLdr-3 0x2000 tstRTLdrMachORelocs-1.o 0x2000 > tmp.disas
 * diff tmp.disas tstRTLdrMachORelocs-1.disas
 */
	.section	__TEXT,__text,regular,pure_instructions
	.globl	_myfunc6
	.align	4, 0x90
_myfunc6:
Leh_func_begin6:
       .byte 0x90
       .byte 0x48, 0xb8                 /* movabsq  */
       .quad   Lg_LocalData
       movb    Lg_LocalData(%rip), %al
	cmpb	$0x7f, Lg_LocalData(%rip)
	cmpw	$0x117f, Lg_LocalData(%rip)
	cmpl	$0x9988117f, Lg_LocalData(%rip)
       jmp     Lg_LocalData

       .byte 0x90
       .byte 0x48, 0xb8                 /* movabsq  */
       .quad   Leh_func_end6
       movb    Leh_func_end6(%rip), %al
	cmpb	$0x7e, Leh_func_end6(%rip)
	cmpw	$0x117e, Leh_func_end6(%rip)
	cmpl	$0x9988117e, Leh_func_end6(%rip)
       jmp     Leh_func_end6

       .byte 0x90
       .byte 0x48, 0xb8                 /* movabsq  */
       .quad   _g_Data.3-3
       movb    _g_Data.3-3(%rip), %al
	cmpb	$4, _g_Data.3-3(%rip)
       jmp     _g_Data.3-3

       .byte 0x90
       .byte 0x48, 0xb8                 /* movabsq  */
       .quad _g_Data.3
       movb    _g_Data.3(%rip), %al
	cmpb	$5, _g_Data.3(%rip)
	cmpw	$1105, _g_Data.3(%rip)
	cmpl	$99881105, _g_Data.3(%rip)
       jmp     _g_Data.3

       .byte 0x90
       .byte 0x48, 0xb8                 /* movabsq  */
       .quad abssym
       movb    abssym(%rip), %al
	cmpb	$6, abssym(%rip)
	cmpw	$1106, abssym(%rip)
	cmpl	$99881106, abssym(%rip)
       jmp     abssym

       .byte 0x90
	cmpb	$1, _g_Data.4.1(%rip)
	cmpb	$2, _g_Data.4.2(%rip)
	cmpb	$3, _g_Data.4.3(%rip)

       /* X86_64_RELOC_SUBTRACTOR */
       .byte 0x90
       .byte 0x48, 0xb8                 /* movabsq  */
       .quad   Leh_func_end6 - _g_Data.0
       .byte 0x48, 0xb8                 /* movabsq  */
       .quad   Leh_func_end6 - _g_Data.0 + 0x80  /* buggy assembler here? (clang-900.0.39.2) */
       .byte 0x48, 0xb8                 /* movabsq  */
       .quad   _myfunc6_end - _g_Data.0
       .byte 0x48, 0xb8                 /* movabsq  */
       .quad   _myfunc6_end - _g_Data.0 + 0x80
	.align	4, 0xcc
_myfunc6_end:

	.section	__TEXT,__othertext,regular,pure_instructions
       .align 3, 0xcc
Leh_func_end6:
        .byte 0x90
        .byte 0x90
        .byte 0xcc
        .byte 0xcc
        .byte 0x90
        .byte 0x90
        .byte 0xcc
        .byte 0xcc

	.section	__DATA,__data //,regular,pure_instructions
       .align 3, 0xcc
_g_Data.0:
       .byte 0x90
_g_Data.1:
       .byte 0xcc
_g_Data.2:
       .byte 0x90
_g_Data.3:
       .byte 0xcc
_g_Data.4.0:
       .byte 0x90
_g_Data.4.1:
       .byte 0xcc
_g_Data.4.2:
       .byte 0x90
_g_Data.4.3:
       .byte 0xcc
_g_EndOfData:

	.section	__GROG,__foobar,regular,pure_instructions
       .align 3, 0xcc
        .byte 0x90
Lg_LocalDataMinus3:
        .byte 0x90
Lg_LocalDataMinus2:
        .byte 0x90
Lg_LocalDataMinus1:
        .byte 0x90
Lg_LocalData:
        .byte 0xcc
        .byte 0xcc
        .byte 0xcc
        .byte 0xcc
        .byte 0xcc
        .byte 0xcc

.set abssym, 0x123456
.globl abssym

.subsections_via_symbols

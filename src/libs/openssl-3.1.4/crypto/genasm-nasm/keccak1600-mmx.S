%ifidn __OUTPUT_FORMAT__,obj
section	code	use32 class=code align=64
%elifidn __OUTPUT_FORMAT__,win32
section	.text	code align=64
%else
section	.text	code
%endif
align	16
__KeccakF1600:
	movq	mm0,[60+esi]
	movq	mm1,[68+esi]
	movq	mm2,[76+esi]
	movq	mm3,[84+esi]
	movq	mm4,[92+esi]
	mov	ecx,24
	jmp	NEAR L$000loop
align	16
L$000loop:
	pxor	mm0,[esi-100]
	pxor	mm1,[esi-92]
	pxor	mm2,[esi-84]
	pxor	mm3,[esi-76]
	pxor	mm4,[esi-68]
	pxor	mm0,[esi-60]
	pxor	mm1,[esi-52]
	pxor	mm2,[esi-44]
	pxor	mm3,[esi-36]
	pxor	mm4,[esi-28]
	pxor	mm0,[esi-20]
	pxor	mm1,[esi-12]
	pxor	mm2,[esi-4]
	pxor	mm3,[4+esi]
	pxor	mm4,[12+esi]
	pxor	mm2,[36+esi]
	pxor	mm0,[20+esi]
	pxor	mm1,[28+esi]
	pxor	mm3,[44+esi]
	movq	mm5,mm2
	pxor	mm4,[52+esi]
	movq	mm7,mm2
	psrlq	mm5,63
	movq	mm6,mm0
	psllq	mm7,1
	pxor	mm5,mm0
	psrlq	mm0,63
	pxor	mm5,mm7
	psllq	mm6,1
	movq	mm7,mm1
	movq	[12+esp],mm5
	pxor	mm6,mm0
	psrlq	mm7,63
	pxor	mm6,mm3
	movq	mm0,mm1
	movq	[36+esp],mm6
	psllq	mm0,1
	pxor	mm7,mm4
	pxor	mm0,mm7
	movq	mm7,mm3
	psrlq	mm3,63
	movq	[4+esp],mm0
	psllq	mm7,1
	movq	mm5,mm4
	psrlq	mm4,63
	pxor	mm1,mm3
	psllq	mm5,1
	pxor	mm1,mm7
	pxor	mm2,mm4
	movq	[20+esp],mm1
	pxor	mm2,mm5
	movq	mm3,[44+esi]
	movq	[28+esp],mm2
	pxor	mm3,mm2
	movq	mm4,[92+esi]
	movq	mm7,mm3
	psrlq	mm3,43
	pxor	mm4,mm6
	psllq	mm7,21
	movq	mm6,mm4
	psrlq	mm4,50
	por	mm3,mm7
	psllq	mm6,14
	movq	mm2,[esi-4]
	por	mm4,mm6
	pxor	mm2,mm1
	movq	mm1,[esi-52]
	movq	mm6,mm2
	psrlq	mm2,21
	pxor	mm1,[12+esp]
	psllq	mm6,43
	movq	mm7,mm1
	psrlq	mm1,20
	por	mm2,mm6
	psllq	mm7,44
	pxor	mm0,[esi-100]
	por	mm1,mm7
	movq	mm5,mm1
	movq	mm6,mm2
	pandn	mm5,mm2
	pandn	mm2,mm3
	pxor	mm5,mm0
	pxor	mm2,mm1
	pxor	mm5,[ebx]
	lea	ebx,[8+ebx]
	movq	mm7,mm3
	movq	[edi-100],mm5
	movq	mm5,mm4
	pandn	mm3,mm4
	pandn	mm4,mm0
	pxor	mm3,mm6
	movq	[edi-92],mm2
	pxor	mm4,mm7
	movq	mm7,[esi-76]
	movq	[edi-84],mm3
	pandn	mm0,mm1
	movq	[edi-76],mm4
	pxor	mm0,mm5
	pxor	mm7,[28+esp]
	movq	[edi-68],mm0
	movq	mm0,mm7
	psrlq	mm7,36
	movq	mm1,[esi-28]
	psllq	mm0,28
	pxor	mm1,[36+esp]
	por	mm0,mm7
	movq	mm6,mm1
	psrlq	mm1,44
	movq	mm2,[esi-20]
	psllq	mm6,20
	pxor	mm2,[4+esp]
	por	mm1,mm6
	movq	mm7,mm2
	psrlq	mm2,61
	movq	mm3,[28+esi]
	psllq	mm7,3
	pxor	mm3,[12+esp]
	por	mm2,mm7
	movq	mm5,mm3
	psrlq	mm3,19
	movq	mm4,[76+esi]
	psllq	mm5,45
	pxor	mm4,[20+esp]
	por	mm3,mm5
	movq	mm6,mm4
	psrlq	mm4,3
	psllq	mm6,61
	por	mm4,mm6
	movq	mm5,mm1
	movq	mm6,mm2
	pandn	mm5,mm2
	pandn	mm2,mm3
	pxor	mm5,mm0
	pxor	mm2,mm1
	movq	mm7,mm3
	movq	[edi-60],mm5
	movq	mm5,mm4
	pandn	mm3,mm4
	pandn	mm4,mm0
	pxor	mm3,mm6
	movq	[edi-52],mm2
	pxor	mm4,mm7
	movq	mm7,[esi-92]
	movq	[edi-44],mm3
	pandn	mm0,mm1
	movq	[edi-36],mm4
	pxor	mm0,mm5
	pxor	mm7,[12+esp]
	movq	[edi-28],mm0
	movq	mm0,mm7
	psrlq	mm7,63
	movq	mm1,[esi-44]
	psllq	mm0,1
	pxor	mm1,[20+esp]
	por	mm0,mm7
	movq	mm6,mm1
	psrlq	mm1,58
	movq	mm2,[4+esi]
	psllq	mm6,6
	pxor	mm2,[28+esp]
	por	mm1,mm6
	movq	mm7,mm2
	psrlq	mm2,39
	movq	mm3,[52+esi]
	psllq	mm7,25
	pxor	mm3,[36+esp]
	por	mm2,mm7
	movq	mm5,mm3
	psrlq	mm3,56
	movq	mm4,[60+esi]
	psllq	mm5,8
	pxor	mm4,[4+esp]
	por	mm3,mm5
	movq	mm6,mm4
	psrlq	mm4,46
	psllq	mm6,18
	por	mm4,mm6
	movq	mm5,mm1
	movq	mm6,mm2
	pandn	mm5,mm2
	pandn	mm2,mm3
	pxor	mm5,mm0
	pxor	mm2,mm1
	movq	mm7,mm3
	movq	[edi-20],mm5
	movq	mm5,mm4
	pandn	mm3,mm4
	pandn	mm4,mm0
	pxor	mm3,mm6
	movq	[edi-12],mm2
	pxor	mm4,mm7
	movq	mm7,[esi-68]
	movq	[edi-4],mm3
	pandn	mm0,mm1
	movq	[4+edi],mm4
	pxor	mm0,mm5
	pxor	mm7,[36+esp]
	movq	[12+edi],mm0
	movq	mm0,mm7
	psrlq	mm7,37
	movq	mm1,[esi-60]
	psllq	mm0,27
	pxor	mm1,[4+esp]
	por	mm0,mm7
	movq	mm6,mm1
	psrlq	mm1,28
	movq	mm2,[esi-12]
	psllq	mm6,36
	pxor	mm2,[12+esp]
	por	mm1,mm6
	movq	mm7,mm2
	psrlq	mm2,54
	movq	mm3,[36+esi]
	psllq	mm7,10
	pxor	mm3,[20+esp]
	por	mm2,mm7
	movq	mm5,mm3
	psrlq	mm3,49
	movq	mm4,[84+esi]
	psllq	mm5,15
	pxor	mm4,[28+esp]
	por	mm3,mm5
	movq	mm6,mm4
	psrlq	mm4,8
	psllq	mm6,56
	por	mm4,mm6
	movq	mm5,mm1
	movq	mm6,mm2
	pandn	mm5,mm2
	pandn	mm2,mm3
	pxor	mm5,mm0
	pxor	mm2,mm1
	movq	mm7,mm3
	movq	[20+edi],mm5
	movq	mm5,mm4
	pandn	mm3,mm4
	pandn	mm4,mm0
	pxor	mm3,mm6
	movq	[28+edi],mm2
	pxor	mm4,mm7
	movq	mm7,[esi-84]
	movq	[36+edi],mm3
	pandn	mm0,mm1
	movq	[44+edi],mm4
	pxor	mm0,mm5
	pxor	mm7,[20+esp]
	movq	[52+edi],mm0
	movq	mm0,mm7
	psrlq	mm7,2
	movq	mm1,[esi-36]
	psllq	mm0,62
	pxor	mm1,[28+esp]
	por	mm0,mm7
	movq	mm6,mm1
	psrlq	mm1,9
	movq	mm2,[12+esi]
	psllq	mm6,55
	pxor	mm2,[36+esp]
	por	mm1,mm6
	movq	mm7,mm2
	psrlq	mm2,25
	movq	mm3,[20+esi]
	psllq	mm7,39
	pxor	mm3,[4+esp]
	por	mm2,mm7
	movq	mm5,mm3
	psrlq	mm3,23
	movq	mm4,[68+esi]
	psllq	mm5,41
	pxor	mm4,[12+esp]
	por	mm3,mm5
	movq	mm6,mm4
	psrlq	mm4,62
	psllq	mm6,2
	por	mm4,mm6
	movq	mm5,mm0
	xor	edi,esi
	movq	[12+esp],mm1
	xor	esi,edi
	xor	edi,esi
	movq	mm6,mm1
	movq	mm7,mm2
	pandn	mm6,mm2
	pandn	mm7,mm3
	pxor	mm0,mm6
	pxor	mm1,mm7
	movq	mm6,mm3
	movq	[60+esi],mm0
	pandn	mm6,mm4
	movq	[68+esi],mm1
	pxor	mm2,mm6
	movq	mm7,mm4
	movq	[76+esi],mm2
	pandn	mm7,mm5
	pandn	mm5,[12+esp]
	pxor	mm3,mm7
	pxor	mm4,mm5
	movq	[84+esi],mm3
	sub	ecx,1
	movq	[92+esi],mm4
	jnz	NEAR L$000loop
	lea	ebx,[ebx-192]
	ret
global	_KeccakF1600
align	16
_KeccakF1600:
L$_KeccakF1600_begin:
	push	ebp
	push	ebx
	push	esi
	push	edi
	mov	esi,DWORD [20+esp]
	mov	ebp,esp
	sub	esp,240
	call	L$001pic_point
L$001pic_point:
	pop	ebx
	lea	ebx,[(L$iotas-L$001pic_point)+ebx]
	and	esp,-8
	lea	esi,[100+esi]
	lea	edi,[140+esp]
	call	__KeccakF1600
	mov	esp,ebp
	emms
	pop	edi
	pop	esi
	pop	ebx
	pop	ebp
	ret
global	_SHA3_absorb
align	16
_SHA3_absorb:
L$_SHA3_absorb_begin:
	push	ebp
	push	ebx
	push	esi
	push	edi
	mov	esi,DWORD [20+esp]
	mov	eax,DWORD [24+esp]
	mov	ecx,DWORD [28+esp]
	mov	edx,DWORD [32+esp]
	mov	ebp,esp
	sub	esp,248
	call	L$002pic_point
L$002pic_point:
	pop	ebx
	lea	ebx,[(L$iotas-L$002pic_point)+ebx]
	and	esp,-8
	mov	edi,esi
	lea	esi,[100+esi]
	mov	DWORD [ebp-4],edx
	jmp	NEAR L$003loop
align	16
L$003loop:
	cmp	ecx,edx
	jc	NEAR L$004absorbed
	shr	edx,3
L$005block:
	movq	mm0,[eax]
	lea	eax,[8+eax]
	pxor	mm0,[edi]
	lea	edi,[8+edi]
	sub	ecx,8
	movq	[edi-8],mm0
	dec	edx
	jnz	NEAR L$005block
	lea	edi,[140+esp]
	mov	DWORD [ebp-8],ecx
	call	__KeccakF1600
	mov	ecx,DWORD [ebp-8]
	mov	edx,DWORD [ebp-4]
	lea	edi,[esi-100]
	jmp	NEAR L$003loop
align	16
L$004absorbed:
	mov	eax,ecx
	mov	esp,ebp
	emms
	pop	edi
	pop	esi
	pop	ebx
	pop	ebp
	ret
global	_SHA3_squeeze
align	16
_SHA3_squeeze:
L$_SHA3_squeeze_begin:
	push	ebp
	push	ebx
	push	esi
	push	edi
	mov	esi,DWORD [20+esp]
	mov	eax,DWORD [24+esp]
	mov	ecx,DWORD [28+esp]
	mov	edx,DWORD [32+esp]
	mov	ebp,esp
	sub	esp,248
	call	L$006pic_point
L$006pic_point:
	pop	ebx
	lea	ebx,[(L$iotas-L$006pic_point)+ebx]
	and	esp,-8
	shr	edx,3
	mov	edi,esi
	lea	esi,[100+esi]
	mov	DWORD [ebp-4],edx
	jmp	NEAR L$007loop
align	16
L$007loop:
	cmp	ecx,8
	jc	NEAR L$008tail
	movq	mm0,[edi]
	lea	edi,[8+edi]
	movq	[eax],mm0
	lea	eax,[8+eax]
	sub	ecx,8
	jz	NEAR L$009done
	dec	edx
	jnz	NEAR L$007loop
	lea	edi,[140+esp]
	mov	DWORD [ebp-8],ecx
	call	__KeccakF1600
	mov	ecx,DWORD [ebp-8]
	mov	edx,DWORD [ebp-4]
	lea	edi,[esi-100]
	jmp	NEAR L$007loop
align	16
L$008tail:
	mov	esi,edi
	mov	edi,eax
dd	0xA4F39066
L$009done:
	mov	esp,ebp
	emms
	pop	edi
	pop	esi
	pop	ebx
	pop	ebp
	ret
align	32
L$iotas:
dd	1,0
dd	32898,0
dd	32906,2147483648
dd	2147516416,2147483648
dd	32907,0
dd	2147483649,0
dd	2147516545,2147483648
dd	32777,2147483648
dd	138,0
dd	136,0
dd	2147516425,0
dd	2147483658,0
dd	2147516555,0
dd	139,2147483648
dd	32905,2147483648
dd	32771,2147483648
dd	32770,2147483648
dd	128,2147483648
dd	32778,0
dd	2147483658,2147483648
dd	2147516545,2147483648
dd	32896,2147483648
dd	2147483649,0
dd	2147516424,2147483648
db	75,101,99,99,97,107,45,49,54,48,48,32,97,98,115,111
db	114,98,32,97,110,100,32,115,113,117,101,101,122,101,32,102
db	111,114,32,77,77,88,44,32,67,82,89,80,84,79,71,65
db	77,83,32,98,121,32,60,97,112,112,114,111,64,111,112,101
db	110,115,115,108,46,111,114,103,62,0

/*
 *
Copyright 1989, 1998  The Open Group

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from The Open Group.
 *
 * Author:  Keith Packard, MIT X Consortium
 */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#ifndef _MERGEROP_H_
#define _MERGEROP_H_

#ifndef GXcopy
#include <X11/X.h>
#endif

typedef struct _mergeRopBits {
    MfbBits   ca1, cx1, ca2, cx2;
} mergeRopRec, *mergeRopPtr;

extern mergeRopRec	mergeRopBits[16];
extern mergeRopPtr	mergeGetRopBits(int i);

#if defined(PPW) && defined(PGSZ) && (PPW != PGSZ)	/* cfb */
#define DeclareMergeRop() MfbBits   _ca1 = 0, _cx1 = 0, _ca2 = 0, _cx2 = 0;
#define DeclarePrebuiltMergeRop()	MfbBits	_cca, _ccx;
#if PSZ == 24  /* both for PGSZ == 32 and 64 */
#define DeclareMergeRop24() \
    MfbBits   _ca1u[4], _cx1u[4], _ca2u[4], _cx2u[4];
    /*    int _unrollidx[3]={0,0,1,2};*/
#define DeclarePrebuiltMergeRop24()	MfbBits	_ccau[4], _ccxu[4];
#endif /* PSZ == 24 */
#else /* mfb */
#define DeclareMergeRop() MfbBits   _ca1 = 0, _cx1 = 0, _ca2 = 0, _cx2 = 0;
#define DeclarePrebuiltMergeRop()	MfbBits	_cca, _ccx;
#endif

#if defined(PPW) && defined(PGSZ) && (PPW != PGSZ)	/* cfb */
#define InitializeMergeRop(alu,pm) {\
    MfbBits   _pm; \
    mergeRopPtr  _bits; \
    _pm = PFILL(pm); \
    _bits = mergeGetRopBits(alu); \
    _ca1 = _bits->ca1 &  _pm; \
    _cx1 = _bits->cx1 | ~_pm; \
    _ca2 = _bits->ca2 &  _pm; \
    _cx2 = _bits->cx2 &  _pm; \
}
#if PSZ == 24
#if	(BITMAP_BIT_ORDER == MSBFirst)
#define InitializeMergeRop24(alu,pm) {\
    register int i; \
    register MfbBits _pm = (pm) & 0xFFFFFF; \
    mergeRopPtr  _bits = mergeGetRopBits(alu); \
    MfbBits _bits_ca1 = _bits->ca1; \
    MfbBits _bits_cx1 = _bits->cx1; \
    MfbBits _bits_ca2 = _bits->ca2; \
    MfbBits _bits_cx2 = _bits->cx2; \
    _pm = (_pm << 8) | (_pm >> 16); \
    for(i = 0; i < 4; i++){ \
      _ca1u[i] = _bits_ca1 &  _pm; \
      _cx1u[i] = _bits_cx1 | ~_pm; \
      _ca2u[i] = _bits_ca2 &  _pm; \
      _cx2u[i] = _bits_cx2 &  _pm; \
      _pm = (_pm << 16)|(_pm >> 8); \
    } \
}
#else	/*(BITMAP_BIT_ORDER == LSBFirst)*/
#define InitializeMergeRop24(alu,pm) {\
    register int i; \
    register MfbBits _pm = (pm) & cfbmask[0]; \
    mergeRopPtr  _bits = mergeGetRopBits(alu); \
    MfbBits _bits_ca1 = _bits->ca1 & cfbmask[0]; \
    MfbBits _bits_cx1 = _bits->cx1 & cfbmask[0]; \
    MfbBits _bits_ca2 = _bits->ca2 & cfbmask[0]; \
    MfbBits _bits_cx2 = _bits->cx2 & cfbmask[0]; \
    _pm |= (_pm << 24); \
    _bits_ca1 |= (_bits->ca1 << 24); \
    _bits_cx1 |= (_bits->cx1 << 24); \
    _bits_ca2 |= (_bits->ca2 << 24); \
    _bits_cx2 |= (_bits->cx2 << 24); \
    for(i = 0; i < 4; i++){ \
      _ca1u[i] = _bits_ca1 &  _pm; \
      _cx1u[i] = _bits_cx1 | ~_pm; \
      _ca2u[i] = _bits_ca2 &  _pm; \
      _cx2u[i] = _bits_cx2 &  _pm; \
      _pm = (_pm << 16)|(_pm >> 8); \
    } \
}
#endif	/*(BITMAP_BIT_ORDER == MSBFirst)*/
#endif /* PSZ == 24 */
#else /* mfb */
#define InitializeMergeRop(alu,pm) {\
    mergeRopPtr  _bits; \
    _bits = mergeGetRopBits(alu); \
    _ca1 = _bits->ca1; \
    _cx1 = _bits->cx1; \
    _ca2 = _bits->ca2; \
    _cx2 = _bits->cx2; \
}
#endif

/* AND has higher precedence than XOR */

#define DoMergeRop(src, dst) \
    (((dst) & (((src) & _ca1) ^ _cx1)) ^ (((src) & _ca2) ^ _cx2))

#define DoMergeRop24u(src, dst, i)					\
(((dst) & (((src) & _ca1u[i]) ^ _cx1u[i])) ^ (((src) & _ca2u[i]) ^ _cx2u[i]))

#define DoMaskMergeRop24(src, dst, mask, index)  {\
	register int idx = ((index) & 3)<< 1; \
	MfbBits _src0 = (src);\
	MfbBits _src1 = (_src0 & _ca1) ^ _cx1; \
	MfbBits _src2 = (_src0 & _ca2) ^ _cx2; \
	*(dst) = (((*(dst)) & cfbrmask[idx]) | (((*(dst)) & cfbmask[idx]) & \
	(((( _src1 |(~mask))<<cfb24Shift[idx])&cfbmask[idx]) ^ \
	 ((( _src2&(mask))<<cfb24Shift[idx])&cfbmask[idx])))); \
	idx++; \
	(dst)++; \
	*(dst) = (((*(dst)) & cfbrmask[idx]) | (((*(dst)) & cfbmask[idx]) & \
	((((_src1 |(~mask))>>cfb24Shift[idx])&cfbmask[idx]) ^ \
	 (((_src2 &(mask))>>cfb24Shift[idx])&cfbmask[idx])))); \
	(dst)--; \
	}

#define DoMaskMergeRop(src, dst, mask) \
    (((dst) & ((((src) & _ca1) ^ _cx1) | ~(mask))) ^ ((((src) & _ca2) ^ _cx2) & (mask)))

#define DoMaskMergeRop24u(src, dst, mask, i)							\
(((dst) & ((((src) & _ca1u[(i)]) ^ _cx1u[(i)]) | ~(mask))) ^ ((((src) & _ca2u[(i)]) ^ _cx2u[(i)]) & (mask)))

#define DoMergeRop24(src,dst,index) {\
	register int idx = ((index) & 3)<< 1; \
	MfbBits _src0 = (src);\
	MfbBits _src1 = (_src0 & _ca1) ^ _cx1; \
	MfbBits _src2 = (_src0 & _ca2) ^ _cx2; \
	*(dst) = (((*(dst)) & cfbrmask[idx]) | ((((*(dst)) & cfbmask[idx]) & \
	((_src1 << cfb24Shift[idx])&cfbmask[idx])) ^ \
	((_src2 << cfb24Shift[idx])&cfbmask[idx]))); \
	idx++; \
	(dst)++; \
	*(dst) = (((*(dst)) & cfbrmask[idx]) | ((((*(dst)) & cfbmask[idx]) & \
	((_src1 >> cfb24Shift[idx])&cfbmask[idx])) ^ \
	((_src2 >> cfb24Shift[idx])&cfbmask[idx]))); \
	(dst)--; \
	}

#define DoPrebuiltMergeRop(dst) (((dst) & _cca) ^ _ccx)

#define DoPrebuiltMergeRop24(dst,index) { \
	register int idx = ((index) & 3)<< 1; \
	*(dst) = (((*(dst)) & cfbrmask[idx]) | ((((*(dst)) & cfbmask[idx]) &\
	(( _cca <<cfb24Shift[idx])&cfbmask[idx])) ^ \
	(( _ccx <<cfb24Shift[idx])&cfbmask[idx]))); \
	idx++; \
	(dst)++; \
	*(dst) = (((*(dst)) & cfbrmask[idx]) | ((((*(dst)) & cfbmask[idx]) &\
	(( _cca >>cfb24Shift[idx])&cfbmask[idx])) ^ \
	(( _ccx >>cfb24Shift[idx])&cfbmask[idx]))); \
	(dst)--; \
	}

#define DoMaskPrebuiltMergeRop(dst,mask) \
    (((dst) & (_cca | ~(mask))) ^ (_ccx & (mask)))

#define PrebuildMergeRop(src) ((_cca = ((src) & _ca1) ^ _cx1), \
			       (_ccx = ((src) & _ca2) ^ _cx2))

#ifndef MROP
#define MROP 0
#endif

#define Mclear		(1<<GXclear)
#define Mand		(1<<GXand)
#define MandReverse	(1<<GXandReverse)
#define Mcopy		(1<<GXcopy)
#define MandInverted	(1<<GXandInverted)
#define Mnoop		(1<<GXnoop)
#define Mxor		(1<<GXxor)
#define Mor		(1<<GXor)
#define Mnor		(1<<GXnor)
#define Mequiv		(1<<GXequiv)
#define Minvert		(1<<GXinvert)
#define MorReverse	(1<<GXorReverse)
#define McopyInverted	(1<<GXcopyInverted)
#define MorInverted	(1<<GXorInverted)
#define Mnand		(1<<GXnand)
#define Mset		(1<<GXset)

#define MROP_PIXEL24(pix, idx) \
	(((*(pix) & cfbmask[(idx)<<1]) >> cfb24Shift[(idx)<<1])| \
	((*((pix)+1) & cfbmask[((idx)<<1)+1]) << cfb24Shift[((idx)<<1)+1]))

#define MROP_SOLID24P(src,dst,sindex, index) \
	MROP_SOLID24(MROP_PIXEL24(src,sindex),dst,index)

#define MROP_MASK24P(src,dst,mask,sindex,index)	\
	MROP_MASK24(MROP_PIXEL24(src,sindex),dst,mask,index)

#if (MROP) == Mcopy
#define MROP_DECLARE()
#define MROP_DECLARE_REG()
#define MROP_INITIALIZE(alu,pm)
#define MROP_SOLID(src,dst)	(src)
#define MROP_SOLID24(src,dst,index)	    {\
	register int idx = ((index) & 3)<< 1; \
	MfbBits _src = (src); \
	*(dst) = (*(dst) & cfbrmask[idx])|((_src<<cfb24Shift[idx])&cfbmask[idx]); \
	idx++; \
	*((dst)+1) = (*((dst)+1) & cfbrmask[idx])|((_src>>cfb24Shift[idx])&cfbmask[idx]); \
	}
#define MROP_MASK(src,dst,mask)	(((dst) & ~(mask)) | ((src) & (mask)))
#define MROP_MASK24(src,dst,mask,index)	{\
	register int idx = ((index) & 3)<< 1; \
	MfbBits _src = (src); \
	*(dst) = (*(dst) & cfbrmask[idx] &(~(((mask)<< cfb24Shift[idx])&cfbmask[idx])) | \
		(((_src &(mask))<<cfb24Shift[idx])&cfbmask[idx])); \
	idx++; \
	*((dst)+1) = (*((dst)+1) & cfbrmask[idx] &(~(((mask)>>cfb24Shift[idx])&cfbmask[idx])) | \
		(((_src&(mask))>>cfb24Shift[idx])&cfbmask[idx])); \
	}
#define MROP_NAME(prefix)	MROP_NAME_CAT(prefix,Copy)
#endif

#if (MROP) == McopyInverted
#define MROP_DECLARE()
#define MROP_DECLARE_REG()
#define MROP_INITIALIZE(alu,pm)
#define MROP_SOLID(src,dst)	(~(src))
#define MROP_SOLID24(src,dst,index)	    {\
	register int idx = ((index) & 3)<< 1; \
	MfbBits _src = ~(src); \
	*(dst) = (*(dst) & cfbrmask[idx])|((_src << cfb24Shift[idx])&cfbmask[idx]); \
	idx++; \
	(dst)++; \
	*(dst) = (*(dst) & cfbrmask[idx])|((_src >>cfb24Shift[idx])&cfbmask[idx]); \
	(dst)--; \
	}
#define MROP_MASK(src,dst,mask)	(((dst) & ~(mask)) | ((~(src)) & (mask)))
#define MROP_MASK24(src,dst,mask,index)	{\
	register int idx = ((index) & 3)<< 1; \
	MfbBits _src = ~(src); \
	*(dst) = (*(dst) & cfbrmask[idx] &(~(((mask)<< cfb24Shift[idx])&cfbmask[idx])) | \
		(((_src &(mask))<<cfb24Shift[idx])&cfbmask[idx])); \
	idx++; \
	(dst)++; \
	*(dst) = (*(dst) & cfbrmask[idx] &(~(((mask)>>cfb24Shift[idx])&cfbmask[idx])) | \
		((((_src & (mask))>>cfb24Shift[idx])&cfbmask[idx])); \
	(dst)--; \
	}
#define MROP_NAME(prefix)	MROP_NAME_CAT(prefix,CopyInverted)
#endif

#if (MROP) == Mxor
#define MROP_DECLARE()
#define MROP_DECLARE_REG()
#define MROP_INITIALIZE(alu,pm)
#define MROP_SOLID(src,dst)	((src) ^ (dst))
#define MROP_SOLID24(src,dst,index)	    {\
	register int idx = ((index) & 3)<< 1; \
	MfbBits _src = (src); \
	*(dst) ^= ((_src << cfb24Shift[idx])&cfbmask[idx]); \
	idx++; \
	(dst)++; \
	*(dst) ^= ((_src >>cfb24Shift[idx])&cfbmask[idx]); \
	(dst)--; \
	}
#define MROP_MASK(src,dst,mask)	(((src) & (mask)) ^ (dst))
#define MROP_MASK24(src,dst,mask,index)	{\
	register int idx = ((index) & 3)<< 1; \
	*(dst) ^= ((((src)&(mask))<<cfb24Shift[idx])&cfbmask[idx]); \
	idx++; \
	(dst)++; \
	*(dst) ^= ((((src)&(mask))>>cfb24Shift[idx])&cfbmask[idx]); \
	(dst)--; \
	}
#define MROP_NAME(prefix)	MROP_NAME_CAT(prefix,Xor)
#endif

#if (MROP) == Mor
#define MROP_DECLARE()
#define MROP_DECLARE_REG()
#define MROP_INITIALIZE(alu,pm)
#define MROP_SOLID(src,dst)	((src) | (dst))
#define MROP_SOLID24(src,dst,index)	    {\
	register int idx = ((index) & 3)<< 1; \
	*(dst) |= (((src)<<cfb24Shift[idx])&cfbmask[idx]); \
	idx++; \
	(dst)++; \
	*(dst) |= (((src)>>cfb24Shift[idx])&cfbmask[idx]); \
	(dst)--; \
	}
#define MROP_MASK(src,dst,mask)	(((src) & (mask)) | (dst))
#define MROP_MASK24(src,dst,mask,index)	{\
	register int idx = ((index) & 3)<< 1; \
	MfbBits _src = (src); \
	*(dst) |= (((_src &(mask))<<cfb24Shift[idx])&cfbmask[idx]); \
	idx++; \
	(dst)++; \
	*(dst) |= (((_src &(mask))>>cfb24Shift[idx])&cfbmask[idx]); \
	(dst)--; \
	}
#define MROP_NAME(prefix)	MROP_NAME_CAT(prefix,Or)
#endif

#if (MROP) == (Mcopy|Mxor|MandReverse|Mor)
#define MROP_DECLARE()	MfbBits _ca1 = 0, _cx1 = 0;
#define MROP_DECLARE_REG()	register MROP_DECLARE()
#define MROP_INITIALIZE(alu,pm)	{ \
    mergeRopPtr  _bits; \
    _bits = mergeGetRopBits(alu); \
    _ca1 = _bits->ca1; \
    _cx1 = _bits->cx1; \
}
#define MROP_SOLID(src,dst) \
    (((dst) & (((src) & _ca1) ^ _cx1)) ^ (src))
#define MROP_MASK(src,dst,mask)	\
    (((dst) & ((((src) & _ca1) ^ _cx1)) | (~(mask)) ^ ((src) & (mask))))
#define MROP_NAME(prefix)	MROP_NAME_CAT(prefix,CopyXorAndReverseOr)
#define MROP_PREBUILD(src)	PrebuildMergeRop(src)
#define MROP_PREBUILT_DECLARE()	DeclarePrebuiltMergeRop()
#define MROP_PREBUILT_SOLID(src,dst)	DoPrebuiltMergeRop(dst)
#define MROP_PREBUILT_SOLID24(src,dst,index)	DoPrebuiltMergeRop24(dst,index)
#define MROP_PREBUILT_MASK(src,dst,mask)    DoMaskPrebuiltMergeRop(dst,mask)
#define MROP_PREBUILT_MASK24(src,dst,mask,index)    DoMaskPrebuiltMergeRop24(dst,mask,index)
#endif

#if (MROP) == 0
#if !defined(PSZ) || (PSZ != 24)
#define MROP_DECLARE()	DeclareMergeRop()
#define MROP_DECLARE_REG()	register DeclareMergeRop()
#define MROP_INITIALIZE(alu,pm)	InitializeMergeRop(alu,pm)
#define MROP_SOLID(src,dst)	DoMergeRop(src,dst)
#define MROP_MASK(src,dst,mask)	DoMaskMergeRop(src, dst, mask)
#else
#define MROP_DECLARE() \
        DeclareMergeRop() \
        DeclareMergeRop24()
#define MROP_DECLARE_REG() \
        register DeclareMergeRop()\
        DeclareMergeRop24()
#define MROP_INITIALIZE(alu,pm)	\
        InitializeMergeRop(alu,pm)\
        InitializeMergeRop24(alu,pm)
#define MROP_SOLID(src,dst)	DoMergeRop24u(src,dst,((int)(&(dst)-pdstBase) % 3))
#define MROP_MASK(src,dst,mask)	DoMaskMergeRop24u(src, dst, mask,((int)(&(dst) - pdstBase)%3))
#endif
#define MROP_SOLID24(src,dst,index)	DoMergeRop24(src,dst,index)
#define MROP_MASK24(src,dst,mask,index)	DoMaskMergeRop24(src, dst, mask,index)
#define MROP_NAME(prefix)	MROP_NAME_CAT(prefix,General)
#define MROP_PREBUILD(src)	PrebuildMergeRop(src)
#define MROP_PREBUILT_DECLARE()	DeclarePrebuiltMergeRop()
#define MROP_PREBUILT_SOLID(src,dst)	DoPrebuiltMergeRop(dst)
#define MROP_PREBUILT_SOLID24(src,dst,index)	DoPrebuiltMergeRop24(dst,index)
#define MROP_PREBUILT_MASK(src,dst,mask)    DoMaskPrebuiltMergeRop(dst,mask)
#define MROP_PREBUILT_MASK24(src,dst,mask,index) \
	DoMaskPrebuiltMergeRop24(dst,mask,index)
#endif

#ifndef MROP_PREBUILD
#define MROP_PREBUILD(src)
#define MROP_PREBUILT_DECLARE()
#define MROP_PREBUILT_SOLID(src,dst)	MROP_SOLID(src,dst)
#define MROP_PREBUILT_SOLID24(src,dst,index)	MROP_SOLID24(src,dst,index)
#define MROP_PREBUILT_MASK(src,dst,mask)    MROP_MASK(src,dst,mask)
#define MROP_PREBUILT_MASK24(src,dst,mask,index) MROP_MASK24(src,dst,mask,index)
#endif

#if !defined(UNIXCPP) || defined(ANSICPP)
#define MROP_NAME_CAT(prefix,suffix)	prefix##suffix
#else
#define MROP_NAME_CAT(prefix,suffix)	prefix/**/suffix
#endif

#endif

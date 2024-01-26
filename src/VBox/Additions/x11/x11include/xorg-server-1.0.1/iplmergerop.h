/* $XFree86: xc/programs/Xserver/iplan2p4/iplmergerop.h,v 3.0 1996/08/18 01:54:53 dawes Exp $ */
#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#ifndef _IPLANMERGEROP_H_
#define _IPLANMERGEROP_H_

/* Modified nov 94 by Martin Schaller (Martin_Schaller@maus.r.de) for use with
interleaved planes */

/* defines: 
   INTER_MROP_NAME
   INTER_MROP_DECLARE_REG()
   INTER_MROP_INITIALIZE(alu, pm)
   INTER_MROP_SOLID(src1, src2, dst)
   INTER_MROP_MASK(src1, src2, mask, dst)
   INTER_MROP_PREBUILD(src)
   INTER_MROP_PREBUILT_DECLARE()
   INTER_MROP_PREBUILT_SOLID(src,dst)
   INTER_MROP_PREBUILT_MASK(src,dst,mask)
*/
 
#ifndef GXcopy
#include <X11/X.h>
#endif

typedef struct _mergeRopBits {
    unsigned long   ca1, cx1, ca2, cx2;
} mergeRopRec, *mergeRopPtr;

extern mergeRopRec	mergeRopBits[16];

#define INTER_DeclareMergeRop() \
	INTER_DECLAREGP(_ca1);	\
	INTER_DECLAREGP(_cx1);	\
	INTER_DECLAREGP(_ca2);	\
	INTER_DECLAREGP(_cx2);	

#define INTER_DeclarePrebuiltMergeRop() \
	INTER_DECLAREGP(_cca);  	\
	INTER_DECLAREGP(_ccx);
	
#define INTER_InitializeMergeRop(alu,pm) {	\
    INTER_DECLAREGP(_pm);			\
    mergeRopPtr  _bits; 			\
    INTER_PFILL(pm, _pm);			\
    _bits = &mergeRopBits[alu];			\
    INTER_ANDMSK(_pm, _bits->ca1, _ca1);	\
    INTER_ANDMSK(_pm, _bits->ca2, _ca2);	\
    INTER_ANDMSK(_pm, _bits->cx2, _cx2);	\
    INTER_NOT(_pm, _pm);			\
    INTER_ORMSK(_pm, _bits->cx1, _cx1);		\
}

#define INTER_DoMergeRop(src1, src2, dst) \
	INTER_CPLX(src1, src2, _ca1, _cx1, _ca2, _cx2, dst)

#define INTER_DoMaskMergeRop(src1, src2, mask, dst) \
	INTER_CPLXM(src1, src2, _ca1, _cx1, _ca2, _cx2, mask, dst)

#define INTER_DoPrebuiltMergeRop(src, dst) \
	INTER_DoRRop(src, _cca, _ccx, dst)

#define INTER_DoMaskPrebuiltMergeRop(src, mask, dst) \
	INTER_DoMaskRRop(src, _cca, _ccx, mask, dst)

#define INTER_PrebuildMergeRop(src) 		\
	INTER_DoRRop(src, _ca1, _cx1, _cca);  	\
	INTER_DoRRop(src, _ca2, _cx2, _ccx);

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

#if (MROP) == Mcopy
#define INTER_MROP_NAME(prefix) INTER_MROP_NAME_CAT(prefix,Copy)
#define INTER_MROP_DECLARE_REG()
#define INTER_MROP_INITIALIZE(alu,pm)
#define INTER_MROP_SOLID(src,dst,dst2) INTER_COPY(src, dst2)
#define INTER_MROP_MASK(src,dst,mask, dst2) INTER_COPYM(src,dst,mask,dst2)
#endif

#if (MROP) == Mxor
#define INTER_MROP_NAME(prefix) INTER_MROP_NAME_CAT(prefix,Xor)
#define INTER_MROP_DECLARE_REG()
#define INTER_MROP_INITIALIZE(alu,pm)
#define INTER_MROP_SOLID(src,dst,dst2)	INTER_XOR(src,dst,dst2)
#define INTER_MROP_MASK(src,dst,mask,dst2) INTER_XORM(src,dst,mask,dst2)
#endif

#if (MROP) == Mor
#define INTER_MROP_NAME(prefix) INTER_MROP_NAME_CAT(prefix,Or)
#define INTER_MROP_DECLARE_REG()
#define INTER_MROP_INITIALIZE(alu,pm)
#define INTER_MROP_SOLID(src,dst,dst2)	INTER_OR(src,dst,dst2)
#define INTER_MROP_MASK(src,dst,mask,dst2) INTER_ORM(src,dst,mask,dst2)
#endif

#if (MROP) == 0
#define INTER_MROP_NAME(prefix) INTER_MROP_NAME_CAT(prefix,General)
#define INTER_MROP_DECLARE_REG() INTER_DeclareMergeRop()
#define INTER_MROP_INITIALIZE(alu,pm) INTER_InitializeMergeRop(alu,pm)
#define INTER_MROP_SOLID(src,dst,dst2) INTER_DoMergeRop(src, dst, dst2)
#define INTER_MROP_MASK(src,dst,mask,dst2) \
	INTER_DoMaskMergeRop(src, dst, mask, dst2)
#define INTER_MROP_PREBUILD(src) INTER_PrebuildMergeRop(src)
#define INTER_MROP_PREBUILT_DECLARE() INTER_DeclarePrebuiltMergeRop()
#define INTER_MROP_PREBUILT_SOLID(src,dst, dst2) \
	INTER_DoPrebuiltMergeRop(dst,dst2)
#define INTER_MROP_PREBUILT_MASK(src,dst,mask,dst2) \
	INTER_DoMaskPrebuiltMergeRop(dst,mask, dst2)
#endif

#ifndef INTER_MROP_PREBUILD
#define INTER_MROP_PREBUILD(src)
#define INTER_MROP_PREBUILT_DECLARE()
#define INTER_MROP_PREBUILT_SOLID(src,dst,dst2)	INTER_MROP_SOLID(src,dst,dst2)
#define INTER_MROP_PREBUILT_MASK(src,dst,mask,dst2) \
	INTER_MROP_MASK(src,dst,mask,dst2)
#endif

#if !defined(UNIXCPP) || defined(ANSICPP)
#define INTER_MROP_NAME_CAT(prefix,suffix)	prefix##suffix
#else
#define INTER_MROP_NAME_CAT(prefix,suffix)	prefix/**/suffix
#endif

#endif /* _IPLANMERGEROP_H_ */

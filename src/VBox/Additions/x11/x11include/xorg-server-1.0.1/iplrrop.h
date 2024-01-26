/* $XFree86: xc/programs/Xserver/iplan2p4/iplrrop.h,v 3.0 1996/08/18 01:55:04 dawes Exp $ */
/* Modified nov 94 by Martin Schaller (Martin_Schaller@maus.r.de) for use with
interleaved planes */

/* reduced raster ops */
/* INTER_RROP_DECLARE INTER_RROP_FETCH_GC, 
   INTER_RROP_SOLID_MASK, INTER_RROP_SPAN INTER_RROP_NAME */

#define INTER_RROP_FETCH_GC(gc) \
INTER_RROP_FETCH_GCPRIV(((iplPrivGCPtr)(gc)->devPrivates[iplGCPrivateIndex].ptr))

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#if RROP == GXcopy
#define INTER_RROP_DECLARE	register unsigned short *rrop_xor;
#define INTER_RROP_FETCH_GCPRIV(devPriv)  rrop_xor = (devPriv)->xorg;
#define INTER_RROP_SOLID(dst)	    	INTER_COPY(rrop_xor, dst)
#define INTER_RROP_SOLID_MASK(dst,mask) INTER_COPYM(rrop_xor, dst, mask, dst)
#define INTER_RROP_NAME(prefix) INTER_RROP_NAME_CAT(prefix,Copy)
#endif /* GXcopy */

#if RROP == GXxor
#define INTER_RROP_DECLARE	register unsigned short	*rrop_xor;
#define INTER_RROP_FETCH_GCPRIV(devPriv)  rrop_xor = (devPriv)->xorg;
#define INTER_RROP_SOLID(dst)		INTER_XOR(rrop_xor, dst, dst)
#define INTER_RROP_SOLID_MASK(dst,mask) INTER_XORM(rrop_xor, dst, mask, dst)
#define INTER_RROP_NAME(prefix) INTER_RROP_NAME_CAT(prefix,Xor)
#endif /* GXxor */

#if RROP == GXand
#define INTER_RROP_DECLARE	register unsigned short *rrop_and;
#define INTER_RROP_FETCH_GCPRIV(devPriv)  rrop_and = (devPriv)->andg;
#define INTER_RROP_SOLID(dst)	    	INTER_AND(rrop_and, dst, dst)
#define INTER_RROP_SOLID_MASK(dst,mask) INTER_ANDM(rrop_and, dst, mask, dst)
#define INTER_RROP_NAME(prefix) INTER_RROP_NAME_CAT(prefix,And)
#endif /* GXand */

#if RROP == GXor
#define INTER_RROP_DECLARE	register unsigned short *rrop_or;
#define INTER_RROP_FETCH_GCPRIV(devPriv)  rrop_or = (devPriv)->xorg;
#define INTER_RROP_SOLID(dst)	    	INTER_OR(rrop_or, dst, dst)
#define INTER_RROP_SOLID_MASK(dst,mask) INTER_ORM(mask, rrop_or, dst, dst)
#define INTER_RROP_NAME(prefix) INTER_RROP_NAME_CAT(prefix,Or)
#endif /* GXor */

#if RROP == GXnoop
#define INTER_RROP_DECLARE
#define INTER_RROP_FETCH_GCPRIV(devPriv)
#define INTER_RROP_SOLID(dst)
#define INTER_RROP_SOLID_MASK(dst,mask)
#define INTER_RROP_NAME(prefix) INTER_RROP_NAME_CAT(prefix,Noop)
#endif /* GXnoop */

#if RROP ==  GXset
#define INTER_RROP_DECLARE	    register unsigned short	*rrop_and, *rrop_xor;
#define INTER_RROP_FETCH_GCPRIV(devPriv)  rrop_and = (devPriv)->andg; \
				    	  rrop_xor = (devPriv)->xorg;
#define INTER_RROP_SOLID(dst)	INTER_DoRRop(dst, rrop_and, rrop_xor, dst)
#define INTER_RROP_SOLID_MASK(dst,mask) \
	INTER_DoMaskRRop(dst, rrop_and, rrop_xor, mask, dst)
#define INTER_RROP_NAME(prefix) INTER_RROP_NAME_CAT(prefix,General)
#endif /* GXset */

#ifndef INTER_RROP_SPAN
#define INTER_RROP_SPAN(pdst,nmiddle) 		\
    while (--(nmiddle) >= 0) { 			\
	INTER_RROP_SOLID(pdst); 		\
	(pdst) = INTER_NEXT(pdst); 		\
    }

#endif

#if !defined(UNIXCPP) || defined(ANSICPP)
#define INTER_RROP_NAME_CAT(prefix,suffix)	prefix##suffix
#else
#define INTER_RROP_NAME_CAT(prefix,suffix)	prefix/**/suffix
#endif


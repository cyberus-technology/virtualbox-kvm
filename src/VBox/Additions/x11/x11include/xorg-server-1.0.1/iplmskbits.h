/* $XFree86$ */
/* Modified nov 94 by Martin Schaller (Martin_Schaller@maus.r.de) for use with
interleaved planes */

#define INTER_PIXGRP unsigned short

#define INTER_PGSZ	   16
#define INTER_PGSZB	    2
#define INTER_PPG	   16
#define INTER_PPGMSK   0xffff
#define INTER_PLST	   15
#define INTER_PIM	   15
#define INTER_PGSH	    4
#define INTER_PMSK ((1 << (INTER_PLANES)) - 1)

extern INTER_PIXGRP iplmask[];
extern INTER_PIXGRP iplstarttab[];
extern INTER_PIXGRP iplendtab[];
extern INTER_PIXGRP iplstartpartial[];
extern INTER_PIXGRP iplendpartial[];

#define MFB_PSZ		    1

#define INTER_NEXT(x) ((x) + INTER_PLANES)
#define INTER_NEXT_GROUP(x) (x) += INTER_PLANES
#define INTER_PREV_GROUP(x) (x) -= INTER_PLANES

#define _I(x)	(((unsigned long *) (x))[_INDEX])
#define _IG(x)  ((x)[_INDEX])

#define INTER_DECLAREG(x) INTER_PIXGRP x
#define INTER_DECLAREGP(x) INTER_PIXGRP x[INTER_PLANES]

#define INTER_DECLARERRAX(x) INTER_PIXGRP *(x)
#define INTER_DECLARERRAXP(x) INTER_PIXGRP x[INTER_PLANES]

/* and |= PLANE_FILL(~fg), or &= PLANE_FILL(fg) */ 
#define INTER_ANDXOR_PM(pm, and, xor)					\
	PLANE_TIMESG(							\
	if (!(pm & INTER_PLANE(_INDEX))) {				\
		_IG(and) = INTER_PPGMSK;				\
		_IG(xor) = 0;						\
	})

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#if INTER_PLANES == 2

#define PLANE_TIMESCONDG(x)					\
	({	int _INDEX;					\
		int _ret;					\
		_ret=(_INDEX=0, (x)) &&				\
		     (_INDEX=1, (x)) &&				\
		_ret;						\
	})

#define PLANE_TIMESCOND(x)					\
	({	int _INDEX;					\
		(_INDEX=0, x)					\
	})

#define PLANE_TIMESG(x)						\
	{	int _INDEX;					\
		_INDEX=0; x;					\
		_INDEX=1; x;					\
	}

#define PLANE_TIMES(x)						\
	{	int _INDEX;					\
		_INDEX=0; x;					\
	}	

#elif INTER_PLANES == 4

#define PLANE_TIMESCONDG(x)					\
	({	int _INDEX;					\
		int _ret;					\
		_ret=(_INDEX=0, (x)) &&				\
		     (_INDEX=1, (x)) &&				\
		     (_INDEX=2, (x)) &&				\
		     (_INDEX=3, (x)); 				\
		_ret;						\
	})

#define PLANE_TIMESCOND(x)					\
	({	int _INDEX;					\
		((_INDEX=0, x) &&				\
		(_INDEX=1, x))					\
	})

#define PLANE_TIMESG(x)						\
	{	int _INDEX;					\
		_INDEX=0; x;					\
		_INDEX=1; x;					\
		_INDEX=2; x;					\
		_INDEX=3; x;					\
	}

#define PLANE_TIMES(x)						\
	{	int _INDEX;					\
		_INDEX=0; x;					\
		_INDEX=1; x;					\
	}	

#elif INTER_PLANES == 8

#define PLANE_TIMESCONDG(x)					\
	({	int _INDEX;					\
		int _ret;					\
		_ret=((_INDEX=0, (x)) &&				\
		     (_INDEX=1, (x)) &&				\
		     (_INDEX=2, (x)) &&				\
		     (_INDEX=3, (x)) &&				\
		     (_INDEX=4, (x)) &&				\
		     (_INDEX=5, (x)) &&				\
		     (_INDEX=6, (x)) &&				\
		     (_INDEX=7, (x))); 				\
		_ret;						\
	})

#define PLANE_TIMESCOND(x)					\
	({	int _INDEX;					\
		((_INDEX=0, x) &&				\
		 (_INDEX=1, x) &&				\
		 (_INDEX=2, x) &&				\
		 (_INDEX=3, x))					\
	})

#define PLANE_TIMESG(x)						\
	{	int _INDEX;					\
		_INDEX=0; x;					\
		_INDEX=1; x;					\
		_INDEX=2; x;					\
		_INDEX=3; x;					\
		_INDEX=4; x;					\
		_INDEX=5; x;					\
		_INDEX=6; x;					\
		_INDEX=7; x;					\
	}

#define PLANE_TIMES(x)						\
	{	int _INDEX;					\
		_INDEX=0; x;					\
		_INDEX=1; x;					\
		_INDEX=2; x;					\
		_INDEX=3; x;					\
	}	

#endif

/* src = 0 */
#define INTER_IS_CLR(src)					\
	PLANE_TIMESCONDG(_IG(src) == 0)	

/* src = PPGMSK ? */
#define INTER_IS_SET(src)					\
	PLANE_TIMESCONDG(_IG(src) == INTER_PPGMSK)

/* (src1 ^ scr2) = PPGMSK ? */
#define INTER_IS_XOR_SET(src1, src2)				\
	PLANE_TIMESCONDG((_IG(src1) ^ _IG(src2)) == INTER_PPGMSK) 

/* dst = ~src */
#define INTER_NOT(src, dst)					\
	PLANE_TIMES(_I(dst) = ~_I(src))

/* dst = 0 */
#define INTER_CLR(dst)						\
	PLANE_TIMES(_I(dst) = 0)

/* dst = PPGMSK */
#define INTER_SET(dst)						\
	PLANE_TIMESG(_IG(dst) = INTER_PPGMSK)

/* dst = src */
#define INTER_COPY(src,dst)					\
	PLANE_TIMES(_I(dst) = _I(src))

/* dst2 = (dst & ~mask) | (src & mask) */
#define INTER_COPYM(src,dst,mask,dst2)					\
	PLANE_TIMESG(							\
	    _IG(dst2) = (_IG(dst) & ~mask) | (_IG(src) & mask) 		\
	)

/* dst2 = dst ^ src */
#define INTER_XOR(src,dst,dst2)					\
	PLANE_TIMES(_I(dst2) = _I(dst) ^ _I(src))  

/* dst2 = dst ^ (src & mask) */
#define INTER_XORM(src,dst,mask,dst2)				\
	PLANE_TIMESG(_IG(dst2) = _IG(dst) ^ (_IG(src) & (mask))) 

/* dst2 = dst & src */
#define INTER_AND(src,dst,dst2)					\
	PLANE_TIMES(_I(dst2) = _I(dst) & _I(src))  

/* dst2 = dst & (src | ~mask) */
#define INTER_ANDM(mask,src,dst,dst2)				\
	PLANE_TIMESG(_IG(dst2) = _IG(dst) & (_IG(src) | ~(mask)))

/* dst2 = dst | src */
#define INTER_OR(src,dst,dst2) 					\
	PLANE_TIMES(_I(dst2) = _I(dst) | _I(src))  

/* dst2 = dst | (src & mask) */
#define INTER_ORM(src,dst,mask,dst2)				\
	PLANE_TIMESG(_IG(dst2) = _IG(dst) | (_IG(src)  & (mask)))

/* dst = src | msk */
#define INTER_ORMSK(src,msk,dst)				\
	PLANE_TIMESG(_IG(dst) = _IG(src) | (msk))

/* dst = src & msk */
#define INTER_ANDMSK(src,msk,dst)				\
	PLANE_TIMESG(_IG(dst) = _IG(src) & (msk))

/* dst = (src1 & msk1) | (src2 & msk2) */
#define INTER_ANDMSK2(src1,msk1,src2,msk2,dst)			\
	PLANE_TIMESG(_IG(dst) = (_IG(src1) & (msk1)) | (_IG(src2) & (msk2)))

#define INTER_PLANE(x)	(1<<(x))

#define INTER_PFILL(col, fill)					\
	PLANE_TIMESG(_IG(fill) = 				\
		((col) & INTER_PLANE(_INDEX)) ? INTER_PPGMSK : 0)

/* dst = src >> cnt */
#define INTER_SCRRIGHT(cnt, src, dst)				\
	PLANE_TIMESG(_IG(dst) = _IG(src) >> (cnt))
	
/* dst = src << cnt */
#define INTER_SCRLEFT(cnt, src, dst)				\
	PLANE_TIMESG(_IG(dst) = _IG(src) << (cnt))

/* bits1=(bits >> right) | (bits=psrc) << left) */
#define INTER_GETRLC(right, left, psrc, bits, bits1)		\
	PLANE_TIMESG( _IG(bits1)=(_IG(bits) >> (right)) | 	\
	((_IG(bits) = _IG(psrc)) << (left)))

/* bits1=(bits << left) | (bits=psrc) >> right) */
#define INTER_GETLRC(left, right, psrc, bits, bits1)		\
	PLANE_TIMESG( _IG(bits1)=(_IG(bits) << (left)) |	\
	((_IG(bits) = _IG(psrc)) >> (right))) 

/* dst=src2 & (src1 & a1 ^ x1) ^ (src1 & a2 ^ x2) */
#define INTER_CPLX(src1, src2, a1, x1, a2, x2, dst) 	\
	PLANE_TIMES( _I(dst) = (_I(src2) 		\
	& (_I(src1) & _I(a1) ^ _I(x1))			\
	^ (_I(src1) & _I(a2) ^ _I(x2))))		\

/* dst=src2 & ((src1 & a1 ^ x1) | ~mask) ^ ((src1 & a2 ^ x2) & mask) */
#define INTER_CPLXM(src1, src2, a1, x1, a2, x2, mask, dst) 	\
	PLANE_TIMESG( _IG(dst) = (_IG(src2)			\
	& ((_IG(src1) & _IG(a1) ^ _IG(x1)) | ~mask)		\
	^ ((_IG(src1) & _IG(a2) ^ _IG(x2)) & mask)))

/* dst = (src & ~(bitmask | planemask)) | (insert | (bitmask | planemask)) */
#define INTER_PMSKINS(bitmask, planemask, insert, src, dst)		\
	PLANE_TIMESG(							\
    	if (planemask & INTER_PLANE(_INDEX)) 				\
	    _IG(dst) = (_IG(src) & ~bitmask) | (_IG(insert) & bitmask) 	\
	)

/* dst = (src & ~bitmask) | ((insert >> shift) & bitmask) */
#define INTER_SCRRMSKINS(bitmask, planemask, insert, shift, src, dst)	\
	PLANE_TIMESG(							\
    	if (planemask & INTER_PLANE(_INDEX)) 				\
	    _IG(dst) = (_IG(src) & ~(bitmask)) | 			\
		((_IG(insert) >> shift) & (bitmask)) 			\
	)

/* dst = (src & ~bitmask) | ((insert << shift) & bitmask) */
#define INTER_SCRLMSKINS(bitmask, planemask, insert, shift, src, dst)	\
	PLANE_TIMESG(							\
    	if (planemask & INTER_PLANE(_INDEX)) 				\
	    _IG(dst) = (_IG(src) & ~bitmask) | 				\
		((_IG(insert) << shift) & bitmask) 			\
	)

/* dst = ((src1 << sl1) & bitmask1) | ((src2 >> sr2) & bitmask2) */  
#define INTER_MSKINSM(bitmask1, sl1, src1, bitmask2, sr2, src2, dst)	\
	PLANE_TIMESG(							\
	    _IG(dst) = ((_IG(src1) << sl1) & (bitmask1)) |		\
	    		    ((_IG(src2) >> sr2) & (bitmask2))		\
	)

/* dst = src & and ^ xor */
#define INTER_DoRRop(src, and, xor, dst)				\
	PLANE_TIMES(_I(dst) = (_I(src) & _I(and) ^ _I(xor)))		\

#define INTER_DoMaskRRop(src, and, xor, mask, dst)			\
	PLANE_TIMESG(							\
	_IG(dst) = (_IG(src) & ((_IG(and) | ~(mask)))			\
		^ (_IG(xor) & mask))) 
		
#define INTER_DoRop(result, alu, src, dst)				\
{									\
	if (alu == GXcopy) {						\
		PLANE_TIMES(						\
		_I(result) = fnCOPY (_I(src), _I(dst))); 		\
	} else if (alu == GXxor) {					\
		PLANE_TIMES(						\
		_I(result) = fnXOR (_I(src), _I(dst))); 		\
	}								\
	else {								\
	    switch (alu)						\
	    {								\
	      case GXclear:						\
		PLANE_TIMES(						\
		_I(result) = fnCLEAR (_I(src), _I(dst))); 		\
		break;							\
	      case GXand:						\
		PLANE_TIMES(						\
		_I(result) = fnAND (_I(src), _I(dst))); 		\
		break;							\
	      case GXandReverse:					\
		PLANE_TIMES(						\
		_I(result) = fnANDREVERSE (_I(src), _I(dst))); 		\
		break;							\
	      case GXandInverted:					\
		PLANE_TIMES(						\
		_I(result) = fnANDINVERTED (_I(src), _I(dst))); 	\
		break;							\
	      case GXnoop:						\
		PLANE_TIMES(						\
		_I(result) = fnNOOP (_I(src), _I(dst))); 		\
		break;							\
	      case GXor:						\
		PLANE_TIMES(						\
		_I(result) = fnOR (_I(src), _I(dst))); 			\
		break;							\
	      case GXnor:						\
		PLANE_TIMES(						\
		_I(result) = fnNOR (_I(src), _I(dst))); 		\
		break;							\
	      case GXequiv:						\
		PLANE_TIMES(						\
		_I(result) = fnEQUIV (_I(src), _I(dst))); 		\
		break;							\
	      case GXinvert:						\
		PLANE_TIMES(						\
		_I(result) = fnINVERT (_I(src), _I(dst))); 		\
		break;							\
	      case GXorReverse:						\
		PLANE_TIMES(						\
		_I(result) = fnORREVERSE (_I(src), _I(dst))); 		\
		break;							\
	      case GXcopyInverted:					\
		PLANE_TIMES(						\
		_I(result) = fnCOPYINVERTED (_I(src), _I(dst))); 	\
		break;							\
	      case GXorInverted:					\
		PLANE_TIMES(						\
		_I(result) = fnORINVERTED (_I(src), _I(dst))); 		\
		break;							\
	      case GXnand:						\
		PLANE_TIMES(						\
		_I(result) = fnNAND (_I(src), _I(dst))); 		\
		break;							\
	      case GXset:						\
		PLANE_TIMES(						\
		_I(result) = fnSET (_I(src), _I(dst))); 		\
		break;							\
	      }								\
	}								\
}

#define iplGetGroupWidthAndPointer(pDrawable, width, pointer) \
    iplGetTypedWidthAndPointer(pDrawable, width, pointer, INTER_PIXGRP, INTER_PIXGRP) 

#define INTER_getstipplepixels(psrcstip, x, w, ones, psrcpix, pdstpix) 	\
{									\
	unsigned long q;						\
	int m;								\
	if (ones) {							\
		if ((m = ((x) - ((MFB_PPW*MFB_PSZ)-MFB_PPW))) > 0) {	\
			q = (*(psrcstip)) << m;				\
			if ( (x)+(w) > (MFB_PPW*MFB_PSZ) )		\
			     q |= *((psrcstip)+1) >> ((MFB_PPW*MFB_PSZ)-m); \
		}							\
		else							\
			q = (*(psrcstip)) >> -m;			\
	}								\
	else {								\
		if ((m = ((x) - ((MFB_PPW*MFB_PSZ)-MFB_PPW))) > 0) {	\
			q = (~ *(psrcstip)) << m; 			\
		if ( (x)+(w) > (MFB_PPW*MFB_PSZ) )			\
			     q |= (~*((psrcstip)+1)) >> ((MFB_PPW*MFB_PSZ)-m); \
		}							\
		else							\
			q = (~ *(psrcstip)) >> -m;			\
	}								\
	q >>=16;							\
	INTER_ANDMSK(psrcpix,q,pdstpix);				\
}

#define INTER_getstipplepixelsb(psrcstip, x, w, psrcpix0, psrcpix1, pdstpix) \
{									\
	unsigned long q,qn;						\
	int m;								\
	if ((m = ((x) - ((MFB_PPW*MFB_PSZ)-MFB_PPW))) > 0) {		\
		q = (*(psrcstip)) << m;					\
		qn = (~ *(psrcstip)) << m;				\
		if ( (x)+(w) > (MFB_PPW*MFB_PSZ) ) {			\
		     q |= *((psrcstip)+1) >> ((MFB_PPW*MFB_PSZ)-m); \
		     qn |= (~ *((psrcstip)+1)) >> ((MFB_PPW*MFB_PSZ)-m); \
		}							\
	}								\
	else	{							\
		q = (*(psrcstip)) >> -m;				\
		qn = (~ *(psrcstip)) >> -m;				\
	}								\
	q >>=16;							\
	qn >>=16;							\
	INTER_ANDMSK2(psrcpix0,qn,psrcpix1,q,pdstpix);			\
}

#define INTER_maskbits(x, w, startmask, endmask, nlg)			\
	startmask = iplstarttab[(x) & INTER_PIM];			\
	endmask = iplendtab[((x)+(w)) & INTER_PIM];			\
	if (startmask)							\
		nlg = (((w) - (INTER_PPG - ((x) & INTER_PIM))) >> INTER_PGSH); \
	else								\
		nlg = (w) >> INTER_PGSH;

#define INTER_maskpartialbits(x, w, mask) 				\
    mask = iplstartpartial[(x) & INTER_PIM] & 			\
	   iplendpartial[((x) + (w)) & INTER_PIM];

#define INTER_mask32bits(x, w, startmask, endmask, nlw)			\
    	startmask = iplstarttab[(x) & INTER_PIM];			\
	endmask = iplendtab[((x)+(w)) & INTER_PIM];

#define INTER_getbits(psrc, x, w, pdst)					\
	if ( ((x) + (w)) <= INTER_PPG) 					\
	{ 								\
	    INTER_SCRLEFT((x), psrc, pdst); 				\
	} 								\
	else 								\
	{ 								\
 	    int m; 							\
	    m = INTER_PPG-(x);						\
	    INTER_MSKINSM(iplendtab[m], x, psrc, 			\
			  iplstarttab[m], m, INTER_NEXT(psrc), pdst);	\
	}

#define INTER_putbits(psrc, x, w, pdst, planemask)			\
	if ( ((x)+(w)) <= INTER_PPG) 					\
	{ 								\
	    INTER_DECLAREG(tmpmask);					\
	    INTER_maskpartialbits((x), (w), tmpmask); 			\
	    INTER_SCRRMSKINS(tmpmask, planemask, psrc, x, pdst, pdst);	\
	} 								\
	else 								\
	{ 								\
	    unsigned long m; 						\
	    unsigned long n; 						\
	    m = INTER_PPG-(x); 						\
	    n = (w) - m; 						\
	    INTER_SCRRMSKINS(iplstarttab[x], planemask, psrc, x, 	\
		pdst, pdst);						\
	    INTER_SCRLMSKINS(iplendtab[n], planemask, psrc, m, 	\
		INTER_NEXT(pdst), INTER_NEXT(pdst));			\
	}

#define INTER_putbitsrop(psrc, x, w, pdst, planemask, rop)		\
if ( ((x)+(w)) <= INTER_PPG)						\
{									\
	INTER_DECLAREG(tmpmask);					\
	INTER_DECLAREGP(t1); INTER_DECLAREGP(t2);			\
	INTER_maskpartialbits((x), (w), tmpmask);			\
	INTER_SCRRIGHT((x), (psrc), (t1));				\
	INTER_DoRop(t2, rop, t1, pdst);					\
	INTER_PMSKINS(tmpmask, planemask, t2, pdst, pdst);		\
}									\
else									\
{									\
	unsigned long m;						\
	unsigned long n;						\
	INTER_DECLAREGP(t1); INTER_DECLAREGP(t2);			\
	m = INTER_PPG-(x);						\
	n = (w) - m;							\
	INTER_SCRRIGHT((x), (psrc), (t1));				\
	INTER_DoRop(t2, rop, t1, pdst);					\
	INTER_PMSKINS(iplstarttab[x], planemask, t2, pdst, pdst);	\
	INTER_SCRLEFT(m, (psrc), (t1));					\
	INTER_DoRop(t2, rop, t1, pdst+1);				\
	INTER_PMSKINS(iplendtab[n], planemask, t2, pdst, pdst);	\
}

#define INTER_putbitsmropshort(src, x, w, pdst) {		\
	INTER_DECLAREG(_tmpmask);				\
	INTER_DECLAREGP(_t1);					\
	INTER_maskpartialbits((x), (w), _tmpmask);		\
	INTER_SCRRIGHT((x), (src), _t1);			\
	INTER_DoMaskMergeRop(_t1, pdst, _tmpmask, pdst);	\
}
 

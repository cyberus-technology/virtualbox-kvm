/* $XFree86: xc/programs/Xserver/cfb24/cfbrrop24.h,v 3.1tsi Exp $ */

#define RROP_DECLARE \
    unsigned long piQxelAnd[3], piQxelXor[3], spiQxelXor[8];

#define RROP_COPY_SETUP(ptn)  \
    spiQxelXor[0] = ptn & 0xFFFFFF; \
    spiQxelXor[2] = ptn << 24; \
    spiQxelXor[3] = (ptn & 0xFFFF00)>> 8; \
    spiQxelXor[4] = ptn << 16; \
    spiQxelXor[5] = (ptn & 0xFF0000)>> 16; \
    spiQxelXor[6] = ptn << 8; \
    spiQxelXor[1] = spiQxelXor[7] = 0;

#define RROP_SOLID24_COPY(dst,index)	    {\
	    register int idx = ((index) & 3)<< 1; \
	    *(dst) = (*(dst) & cfbrmask[idx])|spiQxelXor[idx]; \
	    idx++; \
	    *((dst)+1) = (*((dst)+1) & cfbrmask[idx])|spiQxelXor[idx]; \
	}

#define RROP_SET_SETUP(xor, and) \
    spiQxelXor[0] = xor & 0xFFFFFF; \
    spiQxelXor[1] = xor << 24; \
    spiQxelXor[2] = xor << 16; \
    spiQxelXor[3] = xor << 8; \
    spiQxelXor[4] = (xor >> 8) & 0xFFFF; \
    spiQxelXor[5] = (xor >> 16) & 0xFF; \
    piQxelAnd[0] = (and & 0xFFFFFF)|(and << 24); \
    piQxelAnd[1] = (and << 16)|((and >> 8) & 0xFFFF); \
    piQxelAnd[2] = (and << 8)|((and >> 16) & 0xFF); \
    piQxelXor[0] = (xor & 0xFFFFFF)|(xor << 24); \
    piQxelXor[1] = (xor << 16)|((xor >> 8) & 0xFFFF); \
    piQxelXor[2] = (xor << 8)|((xor >> 16) & 0xFF);


#define RROP_SOLID24_SET(dst,index)	     {\
	    switch((index) & 3){ \
	    case 0: \
	      *(dst) = ((*(dst) & (piQxelAnd[0] |0xFF000000))^(piQxelXor[0] & 0xFFFFFF)); \
	      break; \
	    case 3: \
	      *(dst) = ((*(dst) & (piQxelAnd[2]|0xFF))^(piQxelXor[2] & 0xFFFFFF00)); \
	      break; \
	    case 1: \
	      *(dst) = ((*(dst) & (piQxelAnd[0]|0xFFFFFF))^(piQxelXor[0] & 0xFF000000)); \
	      *((dst)+1) = ((*((dst)+1) & (piQxelAnd[1]|0xFFFF0000))^(piQxelXor[1] & 0xFFFF)); \
	      break; \
	    case 2: \
	      *(dst) = ((*(dst) & (piQxelAnd[1]|0xFFFF))^(piQxelXor[1] & 0xFFFF0000)); \
	      *((dst)+1) = ((*((dst)+1) & (piQxelAnd[2]|0xFFFFFF00))^(piQxelXor[2] & 0xFF)); \
	      break; \
	    } \
	    }

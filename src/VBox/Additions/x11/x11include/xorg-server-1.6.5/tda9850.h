#ifndef __TDA9850_H__
#define __TDA9850_H__

#include "xf86i2c.h"

typedef struct {
	I2CDevRec d;
	
	int mux;
	int stereo;
	int sap;
	Bool mute;
	Bool sap_mute;
	} TDA9850Rec, *TDA9850Ptr;

#define TDA9850_ADDR_1   0xB4

TDA9850Ptr Detect_tda9850(I2CBusPtr b, I2CSlaveAddr addr);
Bool tda9850_init(TDA9850Ptr t);
void tda9850_setaudio(TDA9850Ptr t);
void tda9850_mute(TDA9850Ptr t, Bool mute);
void tda9850_sap_mute(TDA9850Ptr t, Bool sap_mute);
CARD16 tda9850_getstatus(TDA9850Ptr t);

#define TDA9850SymbolsList  \
		"Detect_tda9850", \
		"tda9850_init", \
		"tda9850_setaudio", \
		"tda9850_mute", \
		"tda9850_sap_mute"

#define xf86_Detect_tda9850       ((TDA9850Ptr (*)(I2CBusPtr, I2CSlaveAddr))LoaderSymbol("Detect_tda9850"))
#define xf86_tda9850_init         ((Bool (*)(TDA9850Ptr))LoaderSymbol("tda9850_init"))
#define xf86_tda9850_setaudio     ((void (*)(TDA9850Ptr))LoaderSymbol("tda9850_setaudio"))
#define xf86_tda9850_mute         ((void (*)(TDA9850Ptr, Bool))LoaderSymbol("tda9850_mute"))
#define xf86_tda9850_sap_mute     ((void (*)(TDA9850Ptr, Bool))LoaderSymbol("tda9850_sap_mute"))
#define xf86_tda9850_getstatus    ((CARD16 (*)(TDA9850Ptr))LoaderSymbol("tda9850_getstatus"))

#endif

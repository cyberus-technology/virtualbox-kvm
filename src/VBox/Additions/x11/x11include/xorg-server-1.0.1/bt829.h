#ifndef __BT829_H__
#define __BT829_H__

#include "xf86i2c.h"

typedef struct {
	int 		tunertype;	/* Must be set before init */
        /* Private variables */
	I2CDevRec d;

    	CARD8		brightness;
    	CARD8 		ccmode;
        CARD8           code;
    	CARD16		contrast;
    	CARD8		format;
	int		height;
    	CARD8		hue;
        CARD8           len;
    	CARD8		mux;
    	CARD8           out_en;
        CARD8           p_io;
    	CARD16		sat_u;
    	CARD16		sat_v;
        CARD8           vbien;
        CARD8           vbifmt;
	int 		width;

    	CARD16		hdelay;
        CARD16		hscale;
    	CARD16		vactive;
    	CARD16		vdelay;
        CARD16		vscale;

        CARD16          htotal;
    	CARD8		id;
    	CARD8		svideo_mux;
} BT829Rec, *BT829Ptr;

BT829Ptr bt829_Detect(I2CBusPtr b, I2CSlaveAddr addr);

/* ATI card specific initialization */
#define BT829_ATI_ADDR_1	0x8A
#define BT829_ATI_ADDR_2	0x88
int bt829_ATIInit(BT829Ptr bt);

#define BT829_NTSC		1	/* NTSC-M */
#define BT829_NTSC_JAPAN	2	/* NTSC-Japan */
#define BT829_PAL		3	/* PAL-B,D,G,H,I */
#define BT829_PAL_M		4	/* PAL-M */
#define BT829_PAL_N		5	/* PAL-N */
#define BT829_SECAM		6	/* SECAM */
#define BT829_PAL_N_COMB	7	/* PAL-N combination */
int bt829_SetFormat(BT829Ptr bt, CARD8 format);

#define BT829_MUX2	1	/* ATI -> composite video */
#define BT829_MUX0	2	/* ATI -> tv tuner */
#define BT829_MUX1	3	/* ATI -> s-video */
int bt829_SetMux(BT829Ptr bt, CARD8 mux);

int bt829_SetCaptSize(BT829Ptr bt, int width, int height);

void bt829_SetBrightness(BT829Ptr bt, int brightness);
void bt829_SetContrast(BT829Ptr bt, int contrast);
void bt829_SetSaturation(BT829Ptr bt, int saturation);
void bt829_SetTint(BT829Ptr bt, int hue);	/* Hue */

void bt829_SetOUT_EN(BT829Ptr bt, BOOL out_en);	/* VPOLE register */
void bt829_SetP_IO(BT829Ptr bt, CARD8 p_io);	/* P_IO register */

int bt829_SetCC(BT829Ptr bt);

#define BT829SymbolsList   \
		"bt829_Detect", \
		"bt829_ATIInit", \
		"bt829_SetFormat", \
		"bt829_SetMux", \
		"bt829_SetBrightness", \
		"bt829_SetContrast", \
		"bt829_SetSaturation", \
		"bt829_SetTint", \
		"bt829_SetCaptSize", \
		"bt829_SetOUT_EN", \
		"bt829_SetP_IO"

#ifdef XFree86LOADER

#define xf86_bt829_Detect		((BT829Ptr (*)(I2CBusPtr, I2CSlaveAddr))LoaderSymbol("bt829_Detect"))
#define xf86_bt829_ATIInit		((int (*)(BT829Ptr))LoaderSymbol("bt829_ATIInit"))
#define xf86_bt829_SetFormat		((int (*)(BT829Ptr, CARD8))LoaderSymbol("bt829_SetFormat"))
#define xf86_bt829_SetMux		((int (*)(BT829Ptr, CARD8))LoaderSymbol("bt829_SetMux"))
#define xf86_bt829_SetCaptSize		((int (*)(BT829Ptr, int, int))LoaderSymbol("bt829_SetCaptSize"))
#define xf86_bt829_SetBrightness	((void (*)(BT829Ptr, int))LoaderSymbol("bt829_SetBrightness"))
#define xf86_bt829_SetContrast		((void (*)(BT829Ptr, int))LoaderSymbol("bt829_SetContrast"))
#define xf86_bt829_SetSaturation	((void (*)(BT829Ptr, int))LoaderSymbol("bt829_SetSaturation"))
#define xf86_bt829_SetTint		((void (*)(BT829Ptr, int))LoaderSymbol("bt829_SetTint"))
#define xf86_bt829_SetOUT_EN		((void (*)(BT829Ptr, Bool))LoaderSymbol("bt829_SetOUT_EN"))
#define xf86_bt829_SetP_IO		((void (*)(BT829Ptr, CARD8))LoaderSymbol("bt829_SetP_IO"))

#else

#define xf86_bt829_Detect		bt829_Detect
#define xf86_bt829_ATIInit		bt829_ATIInit
#define xf86_bt829_SetFormat		bt829_SetFormat
#define xf86_bt829_SetMux		bt829_SetMux
#define xf86_bt829_SetCaptSize		bt829_SetCaptSize
#define xf86_bt829_SetBrightness	bt829_SetBrightness
#define xf86_bt829_SetContrast		bt829_SetContrast
#define xf86_bt829_SetSaturation	bt829_SetSaturation
#define xf86_bt829_SetTint		bt829_SetTint
#define xf86_bt829_SetOUT_EN		bt829_SetOUT_EN
#define xf86_bt829_SetP_IO		bt829_SetP_IO

#endif

#endif

/*************************************************************************************
 * Copyright (C) 2005 Bogdan D. bogdand@users.sourceforge.net
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this 
 * software and associated documentation files (the "Software"), to deal in the Software 
 * without restriction, including without limitation the rights to use, copy, modify, 
 * merge, publish, distribute, sublicense, and/or sell copies of the Software, 
 * and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or 
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, 
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE 
 * AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY CLAIM, 
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, 
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name of the author shall not be used in advertising or 
 * otherwise to promote the sale, use or other dealings in this Software without prior written 
 * authorization from the author.
 *
 * Revision 1.3  2005/09/24 21:56:00  bogdand
 * Changed the license to a X/MIT one
 *
 * Revision 1.2  2005/07/01 22:43:11  daniels
 * Change all misc.h and os.h references to <X11/foo.h>.
 *
 *
 ************************************************************************************/

#ifndef __UDA1380_H__
#define __UDA1380_H__

#include "xf86i2c.h"

typedef struct {
	I2CDevRec d;
	
	CARD16 analog_mixer_settings;	/* register 0x03 */
	
	} UDA1380Rec, *UDA1380Ptr;

#define UDA1380_ADDR_1   0x30
#define UDA1380_ADDR_2   0x34

UDA1380Ptr Detect_uda1380(I2CBusPtr b, I2CSlaveAddr addr);
Bool uda1380_init(UDA1380Ptr t);
void uda1380_shutdown(UDA1380Ptr t);
void uda1380_setvolume(UDA1380Ptr t, INT32);
void uda1380_mute(UDA1380Ptr t, Bool);
void uda1380_setparameters(UDA1380Ptr t);
void uda1380_getstatus(UDA1380Ptr t);
void uda1380_dumpstatus(UDA1380Ptr t);

#define UDA1380SymbolsList  \
		"Detect_uda1380", \
		"uda1380_init", \
		"uda1380_shutdown", \
		"uda1380_setvolume", \
		"uda1380_mute", \
		"uda1380_setparameters", \
		"uda1380_getstatus", \
		"uda1380_dumpstatus"

#define xf86_Detect_uda1380       ((UDA1380Ptr (*)(I2CBusPtr, I2CSlaveAddr))LoaderSymbol("Detect_uda1380"))
#define xf86_uda1380_init         ((Bool (*)(UDA1380Ptr))LoaderSymbol("uda1380_init"))
#define xf86_uda1380_shutdown     ((void (*)(UDA1380Ptr))LoaderSymbol("uda1380_shutdown"))
#define xf86_uda1380_setvolume         ((void (*)(UDA1380Ptr, CARD16))LoaderSymbol("uda1380_setvolume"))
#define xf86_uda1380_mute         ((void (*)(UDA1380Ptr, Bool))LoaderSymbol("uda1380_mute"))
#define xf86_uda1380_setparameters     ((void (*)(UDA1380Ptr))LoaderSymbol("uda1380_setparameters"))
#define xf86_uda1380_getstatus    ((void (*)(UDA1380Ptr))LoaderSymbol("uda1380_getstatus"))
#define xf86_uda1380_dumpstatus    ((void (*)(UDA1380Ptr))LoaderSymbol("uda1380_dumpstatus"))

#endif

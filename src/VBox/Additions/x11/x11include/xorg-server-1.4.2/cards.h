/*
 * Copyright (c) 2000 by Conectiva S.A. (http://www.conectiva.com)
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *  
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * CONECTIVA LINUX BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 * 
 * Except as contained in this notice, the name of Conectiva Linux shall
 * not be used in advertising or otherwise to promote the sale, use or other
 * dealings in this Software without prior written authorization from
 * Conectiva Linux.
 *
 * Author: Paulo César Pereira de Andrade <pcpa@conectiva.com.br>
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <X11/Xfuncproto.h>
#include <X11/Xmd.h>
#include <X11/Intrinsic.h>
#include <X11/Xmu/SysUtil.h>

#ifndef _xf86cfg_cards_h
#define _xf86cfg_cards_h

#ifdef USE_MODULES
#ifdef CARDS_PRIVATE
#include "loader.h"

#include "xf86PciStr.h"
#include "xf86PciIds.h"
#endif		/* CARDS_PRIVATE */
#endif		/* USE_MODULES */

/* Flags in CardsEntry */
#define F_NOCLOCKPROBE	0x1	/* Never probe clocks of the card. */
#define F_UNSUPPORTED	0x2	/* Card is not supported (only VGA). */

/*
 * Types
 */
typedef struct {
    char *name;		/* Name of the card. */
    char *chipset;	/* Chipset (decriptive). */
    char *server;	/* Server identifier. */
    char *driver;	/* Driver identifier. */
    char *ramdac;	/* Ramdac identifier. */
    char *clockchip;	/* Clockchip identifier. */
    char *dacspeed;	/* DAC speed rating. */
    int flags;
    char *lines;	/* Additional Device section lines. */
    char *see;		/* Must resolve in a last step.
			 * Allow more than one SEE entry? */
} CardsEntry;

extern CardsEntry **CardsDB;
extern int NumCardsEntry;

/*
 * Prototypes
 */
void ReadCardsDatabase(void);
CardsEntry *LookupCard(char*);
char **GetCardNames(int*);
char **FilterCardNames(char*, int*);
#ifdef USE_MODULES
void InitializePciInfo(void);
typedef struct _xf86cfgModuleOptions *xf86cfgModuleOptionsPtr;
void CheckChipsets(xf86cfgModuleOptionsPtr, int*);
#endif

#endif /* _xf86cfg_cards_h */

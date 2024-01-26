/*
 * Copyright 1997 through 2004 by Marc Aurele La France (TSI @ UQV), tsi@xfree86.org
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that copyright
 * notice and this permission notice appear in supporting documentation, and
 * that the name of Marc Aurele La France not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  Marc Aurele La France makes no representations
 * about the suitability of this software for any purpose.  It is provided
 * "as-is" without express or implied warranty.
 *
 * MARC AURELE LA FRANCE DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS.  IN NO
 * EVENT SHALL MARC AURELE LA FRANCE BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */


#ifndef __MIBANK_H__
#define __MIBANK_H__ 1

#include "scrnintstr.h"

/*
 * Banking external interface.
 */

/*
 * This is the banking function type.  The return value is normally zero.
 * Non-zero returns can be used to implement the likes of scanline interleave,
 * etc.
 */
typedef int miBankProc(
    ScreenPtr /*pScreen*/,
    unsigned int /*iBank*/
);

typedef miBankProc *miBankProcPtr;

typedef struct _miBankInfo
{
    /*
     * Banking refers to the use of one or more apertures (in the server's
     * address space) to access various parts of a potentially larger hardware
     * frame buffer.
     *
     * Three different banking schemes are supported:
     *
     * Single banking is indicated when pBankA and pBankB are equal and all
     * three miBankProcPtr's point to the same function.  Here, both reads and
     * writes through the aperture access the same hardware location.
     *
     * Shared banking is indicated when pBankA and pBankB are equal but the
     * source and destination functions differ.  Here reads through the
     * aperture do not necessarily access the same hardware location as writes.
     *
     * Double banking is indicated when pBankA and pBankB differ.  Here two
     * independent apertures are used to provide read/write access to
     * potentially different hardware locations.
     *
     * Any other combination will result in no banking.
     */
    miBankProcPtr SetSourceBank;                /* Set pBankA bank number */
    miBankProcPtr SetDestinationBank;           /* Set pBankB bank number */
    miBankProcPtr SetSourceAndDestinationBanks; /* Set both bank numbers */

    pointer pBankA;     /* First aperture location */
    pointer pBankB;     /* First or second aperture location */

    /*
     * BankSize is in units of sizeof(char) and is the size of each bank.
     */
    unsigned long BankSize;

    /*
     * nBankDepth is the colour depth associated with the maximum number of a
     * pixel's bits that are simultaneously accessible through the frame buffer
     * aperture.
     */
    unsigned int nBankDepth;
} miBankInfoRec, *miBankInfoPtr;

extern _X_EXPORT Bool
miInitializeBanking(
    ScreenPtr /*pScreen*/,
    unsigned int /*xsize*/,
    unsigned int /*ysize*/,
    unsigned int /*width*/,
    miBankInfoPtr /*pBankInfo*/
);

/*
 * This function determines the minimum screen width, given a initial estimate
 * and various screen attributes.  DDX needs to determine this width before
 * initializing the screen.
 */
extern _X_EXPORT int
miScanLineWidth(
    unsigned int /*xsize*/,
    unsigned int /*ysize*/,
    unsigned int /*width*/,
    unsigned long /*BankSize*/,
    PixmapFormatRec * /*pBankFormat*/,
    unsigned int /*nWidthUnit*/
);

#endif /* __MIBANK_H__ */

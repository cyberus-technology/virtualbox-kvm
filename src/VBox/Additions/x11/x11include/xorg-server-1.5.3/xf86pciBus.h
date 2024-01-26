
/*
 * Copyright (c) 1999-2003 by The XFree86 Project, Inc.
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
 * THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name of the copyright holder(s)
 * and author(s) shall not be used in advertising or otherwise to promote
 * the sale, use or other dealings in this Software without prior written
 * authorization from the copyright holder(s) and author(s).
 */

#ifdef HAVE_XORG_CONFIG_H
#include <xorg-config.h>
#endif

#ifndef _XF86_PCI_BUS_H
#define _XF86_PCI_BUS_H

#define PCITAG_SPECIAL pciTag(0xFF,0xFF,0xFF)

typedef struct {
    CARD32 command;
    CARD32 base[6];
    CARD32 biosBase;
} pciSave, *pciSavePtr;

typedef struct {
    struct pci_device * dev;
    CARD32 ctrl;
} pciArg;

typedef struct {
    pciArg arg;
    xf86AccessRec ioAccess;
    xf86AccessRec io_memAccess;
    xf86AccessRec memAccess;
    pciSave save;
    pciSave restore;
    Bool ctrl;
} pciAccRec, *pciAccPtr;

typedef union {
    CARD16 control;
} pciBridgesSave, *pciBridgesSavePtr;

void xf86PciProbe(void);
void initPciState(void);
void initPciBusState(void);
void DisablePciAccess(void);
void DisablePciBusAccess(void);
void PciStateEnter(void);
void PciBusStateEnter(void);
void PciStateLeave(void);
void PciBusStateLeave(void);
void pciConvertRange2Host(int entityIndex, resRange *pRange);
void isaConvertRange2Host(resRange *pRange);

#endif /* _XF86_PCI_BUS_H */

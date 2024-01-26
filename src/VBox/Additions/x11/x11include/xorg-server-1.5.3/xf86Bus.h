
/*
 * Copyright (c) 1997-2003 by The XFree86 Project, Inc.
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

/*
 * This file contains definitions of the bus-related data structures/types.
 * Everything contained here is private to xf86Bus.c.  In particular the
 * video drivers must not include this file.
 */

#ifdef HAVE_XORG_CONFIG_H
#include <xorg-config.h>
#endif

#ifndef _XF86_BUS_H
#define _XF86_BUS_H

#include "xf86pciBus.h"
#if defined(__sparc__) || defined(__sparc)
#include "xf86sbusBus.h"
#endif

typedef struct racInfo {
    xf86AccessPtr mem_new;
    xf86AccessPtr io_new;
    xf86AccessPtr io_mem_new;
    xf86SetAccessFuncPtr old;
} AccessFuncRec, *AccessFuncPtr;


typedef struct {
    DriverPtr                   driver;
    int                         chipset;
    int                         entityProp;
    EntityProc                  entityInit;
    EntityProc                  entityEnter;
    EntityProc                  entityLeave;
    pointer                     private;
    resPtr                      resources;
    Bool                        active;
    Bool                        inUse;
    BusRec                      bus;
    EntityAccessPtr             access;
    AccessFuncPtr               rac;
    pointer                     busAcc;
    int                         lastScrnFlag;
    DevUnion *                  entityPrivates;
    int                         numInstances;
    GDevPtr *                   devices;   
    IOADDRESS                   domainIO;
} EntityRec, *EntityPtr;

#define NO_SEPARATE_IO_FROM_MEM 0x0001
#define NO_SEPARATE_MEM_FROM_IO 0x0002
#define NEED_VGA_ROUTED 0x0004
#define NEED_VGA_ROUTED_SETUP 0x0008
#define NEED_MEM 0x0010
#define NEED_IO  0x0020
#define NEED_MEM_SHARED 0x0040
#define NEED_IO_SHARED 0x0080
#define ACCEL_IS_SHARABLE 0x0100
#define IS_SHARED_ACCEL 0x0200
#define SA_PRIM_INIT_DONE 0x0400
#define NEED_VGA_MEM 0x1000
#define NEED_VGA_IO  0x2000

#define NEED_SHARED (NEED_MEM_SHARED | NEED_IO_SHARED)

#define busType bus.type
#define isaBusId bus.id.isa
#define sbusBusId bus.id.sbus

struct x_BusAccRec;
typedef void (*BusAccProcPtr)(struct x_BusAccRec *ptr);

typedef struct x_BusAccRec {
    BusAccProcPtr set_f;
    BusAccProcPtr enable_f;
    BusAccProcPtr disable_f;
    BusAccProcPtr save_f;
    BusAccProcPtr restore_f;
    struct x_BusAccRec *current; /* pointer to bridge open on this bus */
    struct x_BusAccRec *primary; /* pointer to the bus connecting to this */
    struct x_BusAccRec *next;    /* this links the different buses together */
    BusType type;
    BusType busdep_type;
    /* Bus-specific fields */
    union {
	struct {
	    int bus;
	    int primary_bus;
	    struct pci_device * dev;
	    pciBridgesSave save;
	} pci;
    } busdep;
} BusAccRec, *BusAccPtr;

/* state change notification callback */
typedef struct _stateChange {
    xf86StateChangeNotificationCallbackFunc func;
    pointer arg;
    struct _stateChange *next;
} StateChangeNotificationRec, *StateChangeNotificationPtr;


extern EntityPtr *xf86Entities;
extern int xf86NumEntities;
extern xf86AccessRec AccessNULL;
extern BusRec primaryBus;
extern resPtr Acc;
extern resPtr ResRange;
extern BusAccPtr xf86BusAccInfo;

int xf86AllocateEntity(void);
BusType StringToBusType(const char* busID, const char **retID);
memType ChkConflict(resRange *rgp, resPtr res, xf86State state);
Bool xf86IsSubsetOf(resRange range, resPtr list);
resPtr xf86ExtractTypeFromList(resPtr list, unsigned long type);
resPtr xf86FindIntersect(resRange Range, resPtr list);
void RemoveOverlaps(resPtr target, resPtr list, Bool pow2Alignment,
		    Bool useEstimated);

#endif /* _XF86_BUS_H */

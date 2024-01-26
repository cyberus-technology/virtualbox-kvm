
/* xf86DDC.h
 *
 * This file contains all information to interpret a standard EDIC block 
 * transmitted by a display device via DDC (Display Data Channel). So far 
 * there is no information to deal with optional EDID blocks.  
 * DDC is a Trademark of VESA (Video Electronics Standard Association).
 *
 * Copyright 1998 by Egbert Eich <Egbert.Eich@Physik.TU-Darmstadt.DE>
 */

#ifndef XF86_DDC_H
# define XF86_DDC_H

#include "edid.h"
#include "xf86i2c.h"
#include "xf86str.h"

/* speed up / slow down */
typedef enum {
  DDC_SLOW,
  DDC_FAST
} xf86ddcSpeed;

typedef void (* DDC1SetSpeedProc)(ScrnInfoPtr, xf86ddcSpeed);

extern xf86MonPtr xf86DoEDID_DDC1(
    int scrnIndex, 
    DDC1SetSpeedProc DDC1SetSpeed,
    unsigned int (*DDC1Read)(ScrnInfoPtr)
);

extern xf86MonPtr xf86DoEDID_DDC2(
   int scrnIndex,
   I2CBusPtr pBus
);

extern xf86MonPtr xf86DoEEDID(int scrnIndex, I2CBusPtr pBus, Bool);

extern xf86MonPtr xf86PrintEDID(
    xf86MonPtr monPtr
);

extern xf86MonPtr xf86InterpretEDID(
    int screenIndex, Uchar *block
);

extern xf86MonPtr xf86InterpretEEDID(
    int screenIndex, Uchar *block
);

extern void 
xf86DDCMonitorSet(int scrnIndex, MonPtr Monitor, xf86MonPtr DDC);

extern Bool xf86SetDDCproperties(
    ScrnInfoPtr pScreen,
    xf86MonPtr DDC
);

DisplayModePtr xf86DDCGetModes(int scrnIndex, xf86MonPtr DDC);

extern Bool
xf86MonitorIsHDMI(xf86MonPtr mon);

#endif

/* $XFree86: xc/programs/Xserver/hw/xfree86/ramdac/IBMPriv.h,v 1.1.2.2 1998/07/18 17:54:01 dawes Exp $ */

#ifdef HAVE_XORG_CONFIG_H
#include <xorg-config.h>
#endif

#include "IBM.h"

typedef struct {
	char *DeviceName;
} xf86IBMramdacInfo;

extern xf86IBMramdacInfo IBMramdacDeviceInfo[];

#ifdef INIT_IBM_RAMDAC_INFO
xf86IBMramdacInfo IBMramdacDeviceInfo[] = {
	{"IBM 524"},
	{"IBM 524A"},
	{"IBM 525"},
	{"IBM 526"},
	{"IBM 526DB(DoubleBuffer)"},
	{"IBM 528"},
	{"IBM 528A"},
	{"IBM 624"},
	{"IBM 624DB(DoubleBuffer)"},
	{"IBM 640"}
};
#endif

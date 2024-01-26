/*
 * SBUS bus-specific declarations
 *
 * Copyright (C) 2000 Jakub Jelinek (jakub@redhat.com)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * JAKUB JELINEK BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef _XF86_SBUSBUS_H
#define _XF86_SBUSBUS_H

#include "xf86str.h"

#define SBUS_DEVICE_BW2		0x0001
#define SBUS_DEVICE_CG2		0x0002
#define SBUS_DEVICE_CG3		0x0003
#define SBUS_DEVICE_CG4		0x0004
#define SBUS_DEVICE_CG6		0x0005
#define SBUS_DEVICE_CG8		0x0006
#define SBUS_DEVICE_CG12	0x0007
#define SBUS_DEVICE_CG14	0x0008
#define SBUS_DEVICE_LEO		0x0009
#define SBUS_DEVICE_TCX		0x000a
#define SBUS_DEVICE_FFB		0x000b
#define SBUS_DEVICE_GT		0x000c
#define SBUS_DEVICE_MGX		0x000d

typedef struct sbus_prom_node {
    int			node;
    /* Because of misdesigned openpromio */
    int			cookie[2];
} sbusPromNode, *sbusPromNodePtr;

typedef struct sbus_device {
    int			devId;
    int			fbNum;
    int			fd;
    int			width, height;
    sbusPromNode	node;
    char		*descr;
    char		*device;
} sbusDevice, *sbusDevicePtr;

extern struct sbus_devtable {
    int devId;
    int fbType;
    char *promName;
    char *descr;
} sbusDeviceTable[];

void xf86SbusProbe(void);
extern sbusDevicePtr *xf86SbusInfo;

int xf86MatchSbusInstances(const char *driverName, int sbusDevId, 
			   GDevPtr *devList, int numDevs, DriverPtr drvp,
			   int **foundEntities);
sbusDevicePtr xf86GetSbusInfoForEntity(int entityIndex);
int xf86GetEntityForSbusInfo(sbusDevicePtr psdp);
void xf86SbusUseBuiltinMode(ScrnInfoPtr pScrn, sbusDevicePtr psdp);
pointer xf86MapSbusMem(sbusDevicePtr psdp, unsigned long offset,
		       unsigned long size);
void xf86UnmapSbusMem(sbusDevicePtr psdp, pointer addr, unsigned long size);
void xf86SbusHideOsHwCursor(sbusDevicePtr psdp);
void xf86SbusSetOsHwCursorCmap(sbusDevicePtr psdp, int bg, int fg);
Bool xf86SbusHandleColormaps(ScreenPtr pScreen, sbusDevicePtr psdp);

extern int promRootNode;

int promGetSibling(int node);
int promGetChild(int node);
char * promGetProperty(const char *prop, int *lenp);
int promGetBool(const char *prop);

int sparcPromInit(void);
void sparcPromClose(void);
char * sparcPromGetProperty(sbusPromNodePtr pnode, const char *prop, int *lenp);
int sparcPromGetBool(sbusPromNodePtr pnode, const char *prop);
void sparcPromAssignNodes(void);
char * sparcPromNode2Pathname(sbusPromNodePtr pnode);
int sparcPromPathname2Node(const char *pathName);

#endif /* _XF86_SBUSBUS_H */

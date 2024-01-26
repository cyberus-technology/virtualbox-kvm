/* $XFree86: xc/programs/Xserver/hw/xfree86/scanpci/xf86PciData.h,v 1.3 2003/08/24 17:37:10 dawes Exp $ */

/*
 * Copyright (c) 2000-2002 by The XFree86 Project, Inc.
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

#ifndef PCI_DATA_H_
#define PCI_DATA_H_

#define NOVENDOR 0xFFFF
#define NODEVICE 0xFFFF
#define NOSUBSYS 0xFFFF

typedef Bool (*ScanPciSetupProcPtr)(void);
typedef void (*ScanPciCloseProcPtr)(void);
typedef int (*ScanPciFindByDeviceProcPtr)(
			unsigned short vendor, unsigned short device,
			unsigned short svendor, unsigned short subsys,
			const char **vname, const char **dname,
			const char **svname, const char **sname);
typedef int (*ScanPciFindBySubsysProcPtr)(
			unsigned short svendor, unsigned short subsys,
			const char **svname, const char **sname);
typedef CARD32 (*ScanPciFindClassBySubsysProcPtr)(
			unsigned short vendor, unsigned short subsystem);
typedef CARD32 (*ScanPciFindClassByDeviceProcPtr)(
			unsigned short vendor, unsigned short device);

/*
 * Whoever loads this module needs to define these and initialise them
 * after loading.
 */
extern ScanPciSetupProcPtr xf86SetupPciIds;
extern ScanPciCloseProcPtr xf86ClosePciIds;
extern ScanPciFindByDeviceProcPtr xf86FindPciNamesByDevice;
extern ScanPciFindBySubsysProcPtr xf86FindPciNamesBySubsys;
extern ScanPciFindClassBySubsysProcPtr xf86FindPciClassBySubsys;
extern ScanPciFindClassByDeviceProcPtr xf86FindPciClassByDevice;

Bool ScanPciSetupPciIds(void);
void ScanPciClosePciIds(void);
int ScanPciFindPciNamesByDevice(unsigned short vendor, unsigned short device,
				unsigned short svendor, unsigned short subsys,
				const char **vname, const char **dname,
				const char **svname, const char **sname);
int ScanPciFindPciNamesBySubsys(unsigned short svendor, unsigned short subsys,
				const char **svname, const char **sname);
CARD32 ScanPciFindPciClassBySubsys(unsigned short vendor,
				   unsigned short subsystem);
CARD32 ScanPciFindPciClassByDevice(unsigned short vendor,
				   unsigned short device);

#endif

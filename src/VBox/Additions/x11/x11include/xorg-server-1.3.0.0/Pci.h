/*
 * Copyright 1998 by Concurrent Computer Corporation
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without fee,
 * provided that the above copyright notice appear in all copies and that
 * both that copyright notice and this permission notice appear in
 * supporting documentation, and that the name of Concurrent Computer
 * Corporation not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior
 * permission.  Concurrent Computer Corporation makes no representations
 * about the suitability of this software for any purpose.  It is
 * provided "as is" without express or implied warranty.
 *
 * CONCURRENT COMPUTER CORPORATION DISCLAIMS ALL WARRANTIES WITH REGARD
 * TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS, IN NO EVENT SHALL CONCURRENT COMPUTER CORPORATION BE
 * LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY
 * DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 * WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
 * ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 *
 * Copyright 1998 by Metro Link Incorporated
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without fee,
 * provided that the above copyright notice appear in all copies and that
 * both that copyright notice and this permission notice appear in
 * supporting documentation, and that the name of Metro Link
 * Incorporated not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior
 * permission.  Metro Link Incorporated makes no representations
 * about the suitability of this software for any purpose.  It is
 * provided "as is" without express or implied warranty.
 *
 * METRO LINK INCORPORATED DISCLAIMS ALL WARRANTIES WITH REGARD
 * TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS, IN NO EVENT SHALL METRO LINK INCORPORATED BE
 * LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY
 * DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 * WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
 * ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 *
 * This file is derived in part from the original xf86_PCI.h that included
 * following copyright message:
 *
 * Copyright 1995 by Robin Cutshaw <robin@XFree86.Org>
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the names of the above listed copyright holder(s)
 * not be used in advertising or publicity pertaining to distribution of
 * the software without specific, written prior permission.  The above listed
 * copyright holder(s) make(s) no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without express or
 * implied warranty.
 *
 * THE ABOVE LISTED COPYRIGHT HOLDER(S) DISCLAIM(S) ALL WARRANTIES WITH REGARD
 * TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS, IN NO EVENT SHALL THE ABOVE LISTED COPYRIGHT HOLDER(S) BE
 * LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY
 * DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER
 * IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING
 * OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */
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


/*
 * This file has the private Pci definitions.  The public ones are imported
 * from xf86Pci.h.  Drivers should not use this file.
 */
#ifdef HAVE_XORG_CONFIG_H
#include <xorg-config.h>
#endif

#ifndef _PCI_H
#define _PCI_H 1

#include <X11/Xarch.h>
#include <X11/Xfuncproto.h>
#include "xf86Pci.h"
#include "xf86PciInfo.h"

/*
 * Global Definitions
 */
#define MAX_PCI_DEVICES 128	/* Max number of devices accomodated */
				/* by xf86scanpci		     */
#if defined(sun) && defined(SVR4) && defined(sparc)
# define MAX_PCI_BUSES   4096	/* Max number of PCI buses           */
#elif (defined(__alpha__) || defined(__ia64__)) && defined (linux)
# define MAX_PCI_DOMAINS	512
# define PCI_DOM_MASK	0x01fful
# define MAX_PCI_BUSES	(MAX_PCI_DOMAINS*256) /* 256 per domain      */
#else
# define MAX_PCI_BUSES   256	/* Max number of PCI buses           */
#endif

#define DEVID(vendor, device) \
    ((CARD32)((PCI_##device << 16) | PCI_##vendor))

#ifndef PCI_DOM_MASK
# define PCI_DOM_MASK 0x0ffu
#endif
#define PCI_DOMBUS_MASK (((PCI_DOM_MASK) << 8) | 0x0ffu)

/*
 * "b" contains an optional domain number.
 */
#define PCI_MAKE_TAG(b,d,f)  ((((b) & (PCI_DOMBUS_MASK)) << 16) | \
			      (((d) & 0x00001fu) << 11) | \
			      (((f) & 0x000007u) << 8))

#define PCI_MAKE_BUS(d,b)    ((((d) & (PCI_DOM_MASK)) << 8) | ((b) & 0xffu))

#define PCI_DOM_FROM_TAG(tag)  (((tag) >> 24) & (PCI_DOM_MASK))
#define PCI_BUS_FROM_TAG(tag)  (((tag) >> 16) & (PCI_DOMBUS_MASK))
#define PCI_DEV_FROM_TAG(tag)  (((tag) & 0x0000f800u) >> 11)
#define PCI_FUNC_FROM_TAG(tag) (((tag) & 0x00000700u) >> 8)

#define PCI_DFN_FROM_TAG(tag)  (((tag) & 0x0000ff00u) >> 8)
#define PCI_BDEV_FROM_TAG(tag) ((tag) & 0x00fff800u)

#define PCI_DOM_FROM_BUS(bus)  (((bus) >> 8) & (PCI_DOM_MASK))
#define PCI_BUS_NO_DOMAIN(bus) ((bus) & 0xffu)
#define PCI_TAG_NO_DOMAIN(tag) ((tag) & 0x00ffff00u)

/*
 * Macros for bus numbers found in P2P headers.
 */
#define PCI_PRIMARY_BUS_EXTRACT(x, tag)     \
    ((((x) & PCI_PRIMARY_BUS_MASK    ) >>  0) | (PCI_DOM_FROM_TAG(tag) << 8))
#define PCI_SECONDARY_BUS_EXTRACT(x, tag)   \
    ((((x) & PCI_SECONDARY_BUS_MASK  ) >>  8) | (PCI_DOM_FROM_TAG(tag) << 8))
#define PCI_SUBORDINATE_BUS_EXTRACT(x, tag) \
    ((((x) & PCI_SUBORDINATE_BUS_MASK) >> 16) | (PCI_DOM_FROM_TAG(tag) << 8))

#define PCI_PRIMARY_BUS_INSERT(x, y)     \
    (((x) & ~PCI_PRIMARY_BUS_MASK    ) | (((y) & 0xffu) <<  0))
#define PCI_SECONDARY_BUS_INSERT(x, y)   \
    (((x) & ~PCI_SECONDARY_BUS_MASK  ) | (((y) & 0xffu) <<  8))
#define PCI_SUBORDINATE_BUS_INSERT(x, y) \
    (((x) & ~PCI_SUBORDINATE_BUS_MASK) | (((y) & 0xffu) << 16))

/* Ditto for CardBus bridges */
#define PCI_CB_PRIMARY_BUS_EXTRACT(x, tag)     \
    PCI_PRIMARY_BUS_EXTRACT(x, tag)
#define PCI_CB_CARDBUS_BUS_EXTRACT(x, tag)     \
    PCI_SECONDARY_BUS_EXTRACT(x, tag)
#define PCI_CB_SUBORDINATE_BUS_EXTRACT(x, tag) \
    PCI_SUBORDINATE_BUS_EXTRACT(x, tag)

#define PCI_CB_PRIMARY_BUS_INSERT(x, tag)     \
    PCI_PRIMARY_BUS_INSERT(x, tag)
#define PCI_CB_CARDBUS_BUS_INSERT(x, tag)     \
    PCI_SECONDARY_BUS_INSERT(x, tag)
#define PCI_CB_SUBORDINATE_BUS_INSERT(x, tag) \
    PCI_SUBORDINATE_BUS_INSERT(x, tag)

#if X_BYTE_ORDER == X_BIG_ENDIAN
#define PCI_CPU(val)	(((val >> 24) & 0x000000ff) |	\
			 ((val >>  8) & 0x0000ff00) |	\
			 ((val <<  8) & 0x00ff0000) |	\
			 ((val << 24) & 0xff000000))
#define PCI_CPU16(val)	(((val >>  8) & 0x000000ff) |	\
			 ((val <<  8) & 0x0000ff00))
#else
#define PCI_CPU(val)	(val)
#define PCI_CPU16(val)	(val)
#endif

/*
 * Debug Macros/Definitions
 */
/* #define DEBUGPCI  2 */    /* Disable/enable trace in PCI code */

#if defined(DEBUGPCI)

# define PCITRACE(lvl,printfargs) \
	if (lvl > xf86Verbose) { \
		ErrorF printfargs; \
	}

#else /* !defined(DEBUGPCI) */

# define PCITRACE(lvl,printfargs)

#endif /* !defined(DEBUGPCI) */

/*
 * PCI Config mechanism definitions
 */
#define PCI_EN 0x80000000

#define	PCI_CFGMECH1_ADDRESS_REG	0xCF8
#define	PCI_CFGMECH1_DATA_REG		0xCFC

#define PCI_CFGMECH1_MAXDEV	32

/*
 * Select architecture specific PCI init function
 */
#if defined(__alpha__)
# if defined(linux)
#  define ARCH_PCI_INIT axpPciInit
#  define INCLUDE_XF86_MAP_PCI_MEM
# elif defined(__FreeBSD__) || defined(__OpenBSD__)
#  define ARCH_PCI_INIT freebsdPciInit
#  define INCLUDE_XF86_MAP_PCI_MEM
#  define INCLUDE_XF86_NO_DOMAIN
# elif defined(__NetBSD__)
#  define ARCH_PCI_INIT netbsdPciInit
#  define INCLUDE_XF86_MAP_PCI_MEM
#  define INCLUDE_XF86_NO_DOMAIN
# endif
#elif defined(__arm__)
# if defined(linux)
#  define ARCH_PCI_INIT linuxPciInit
#  define INCLUDE_XF86_MAP_PCI_MEM
#  define INCLUDE_XF86_NO_DOMAIN
# endif
#elif defined(__hppa__)
# if defined(linux)
#  define ARCH_PCI_INIT linuxPciInit
#  define INCLUDE_XF86_MAP_PCI_MEM
#  define INCLUDE_XF86_NO_DOMAIN
# endif
#elif defined(__ia64__)
# if defined(linux)
#  define ARCH_PCI_INIT ia64linuxPciInit
#  define INCLUDE_XF86_MAP_PCI_MEM
# elif defined(FreeBSD)
#  define ARCH_PCI_INIT freebsdPciInit
#  define INCLUDE_XF86_MAP_PCI_MEM
#  define INCLUDE_XF86_NO_DOMAIN
# endif
# define XF86SCANPCI_WRAPPER ia64ScanPCIWrapper
#elif defined(__i386__) || defined(i386)
# define ARCH_PCI_INIT ix86PciInit
# define INCLUDE_XF86_MAP_PCI_MEM
# define INCLUDE_XF86_NO_DOMAIN
# if defined(linux)
#  define ARCH_PCI_OS_INIT linuxPciInit
# endif
#elif defined(__mc68000__)
# if defined(linux)
#  define ARCH_PCI_INIT linuxPciInit
#  define INCLUDE_XF86_MAP_PCI_MEM
#  define INCLUDE_XF86_NO_DOMAIN
# endif
#elif defined(__mips__)
# if defined(linux)
#  define ARCH_PCI_INIT linuxPciInit
#  define INCLUDE_XF86_MAP_PCI_MEM
#  define INCLUDE_XF86_NO_DOMAIN
# endif
#elif defined(__powerpc__) || defined(__powerpc64__)
# if defined(linux)
#  define ARCH_PCI_INIT linuxPciInit
#  define INCLUDE_XF86_MAP_PCI_MEM
#  define INCLUDE_XF86_NO_DOMAIN	/* Needs kernel work to remove */
# elif defined(__FreeBSD__) || defined(__OpenBSD__)
#  define  ARCH_PCI_INIT freebsdPciInit
#  define INCLUDE_XF86_MAP_PCI_MEM
#  define INCLUDE_XF86_NO_DOMAIN
# elif defined(__NetBSD__)
#  define ARCH_PCI_INIT netbsdPciInit
#  define INCLUDE_XF86_MAP_PCI_MEM
#  define INCLUDE_XF86_NO_DOMAIN
# else
#  define ARCH_PCI_INIT ppcPciInit
#  define INCLUDE_XF86_MAP_PCI_MEM
#  define INCLUDE_XF86_NO_DOMAIN
# endif
#elif defined(__s390__)
# if defined(linux)
#  define ARCH_PCI_INIT linuxPciInit
#  define INCLUDE_XF86_MAP_PCI_MEM
#  define INCLUDE_XF86_NO_DOMAIN
# endif
#elif defined(__sh__)
# if defined(linux)
#  define ARCH_PCI_INIT linuxPciInit
#  define INCLUDE_XF86_MAP_PCI_MEM
#  define INCLUDE_XF86_NO_DOMAIN
# endif
#elif defined(__sparc__) || defined(sparc)
# if defined(linux)
#  define ARCH_PCI_INIT linuxPciInit
#  define INCLUDE_XF86_MAP_PCI_MEM
# elif defined(sun)
#  define ARCH_PCI_INIT sparcPciInit
#  define INCLUDE_XF86_MAP_PCI_MEM
# elif (defined(__OpenBSD__) || defined(__FreeBSD__)) && defined(__sparc64__)
#  define  ARCH_PCI_INIT freebsdPciInit
#  define INCLUDE_XF86_MAP_PCI_MEM
#  define INCLUDE_XF86_NO_DOMAIN
# endif
# if !defined(__FreeBSD__)
#  define ARCH_PCI_PCI_BRIDGE sparcPciPciBridge
# endif
#elif defined(__amd64__) || defined(__amd64)
# if defined(__FreeBSD__) || defined(__FreeBSD_kernel__)
#  define ARCH_PCI_INIT freebsdPciInit
# else
#  define ARCH_PCI_INIT ix86PciInit
# endif
# define INCLUDE_XF86_MAP_PCI_MEM
# define INCLUDE_XF86_NO_DOMAIN
# if defined(linux)
#  define ARCH_PCI_OS_INIT linuxPciInit
# endif
#endif

#ifndef ARCH_PCI_INIT
#error No PCI support available for this architecture/OS combination
#endif

extern void ARCH_PCI_INIT(void);
#if defined(ARCH_PCI_OS_INIT)
extern void ARCH_PCI_OS_INIT(void);
#endif

#if defined(ARCH_PCI_PCI_BRIDGE)
extern void ARCH_PCI_PCI_BRIDGE(pciConfigPtr pPCI);
#endif

#if defined(XF86SCANPCI_WRAPPER)
typedef enum {
    SCANPCI_INIT,
    SCANPCI_TERM
} scanpciWrapperOpt;
extern void XF86SCANPCI_WRAPPER(scanpciWrapperOpt flags);
#endif

/*
 * Table of functions used to access a specific PCI bus domain
 * (e.g. a primary PCI bus and all of its secondaries)
 */
typedef struct pci_bus_funcs {
	CARD32  (*pciReadLong)(PCITAG, int);
	void    (*pciWriteLong)(PCITAG, int, CARD32);
	void    (*pciSetBitsLong)(PCITAG, int, CARD32, CARD32);
	ADDRESS (*pciAddrHostToBus)(PCITAG, PciAddrType, ADDRESS);
	ADDRESS (*pciAddrBusToHost)(PCITAG, PciAddrType, ADDRESS);
	/*
	 * The next three are optional.  If NULL, the corresponding function is
	 * to be performed generically.
	 */
	CARD16  (*pciControlBridge)(int, CARD16, CARD16);
	void    (*pciGetBridgeBuses)(int, int *, int *, int *);
	/* Use pointer's to avoid #include recursion */
	void    (*pciGetBridgeResources)(int, pointer *, pointer *, pointer *);

	/* These are optional and will be implemented using read long
	 * if not present. */
	CARD8   (*pciReadByte)(PCITAG, int);
	void    (*pciWriteByte)(PCITAG, int, CARD8);
	CARD16  (*pciReadWord)(PCITAG, int);
	void    (*pciWriteWord)(PCITAG, int, CARD16);

} pciBusFuncs_t, *pciBusFuncs_p;

/*
 * pciBusInfo_t - One structure per defined PCI bus
 */
typedef struct pci_bus_info {
	unsigned char  configMech;   /* PCI config type to use      */
	unsigned char  numDevices;   /* Range of valid devnums      */
	unsigned char  secondary;    /* Boolean: bus is a secondary */
	int            primary_bus;  /* Parent bus                  */
	pciBusFuncs_p  funcs;        /* PCI access functions        */
	void          *pciBusPriv;   /* Implementation private data */
	pciConfigPtr   bridge;       /* bridge that opens this bus  */
} pciBusInfo_t;

#define HOST_NO_BUS ((pciBusInfo_t *)(-1))

/* configMech values */
#define PCI_CFG_MECH_UNKNOWN 0 /* Not yet known  */
#define PCI_CFG_MECH_1       1 /* Most machines  */
#define PCI_CFG_MECH_2       2 /* Older PC's     */
#define PCI_CFG_MECH_OTHER   3 /* Something else */

/* Generic PCI service functions and helpers */
PCITAG        pciGenFindFirst(void);
PCITAG        pciGenFindNext(void);
CARD32        pciCfgMech1Read(PCITAG tag, int offset);
void          pciCfgMech1Write(PCITAG tag, int offset, CARD32 val);
void          pciCfgMech1SetBits(PCITAG tag, int offset, CARD32 mask,
				 CARD32 val);
CARD32        pciByteSwap(CARD32);
Bool          pciMfDev(int, int);
ADDRESS       pciAddrNOOP(PCITAG tag, PciAddrType type, ADDRESS);

extern void pciSetOSBIOSPtr(int (*bios_fn)(PCITAG Tag, int basereg, unsigned char * buf, int len));
extern PCITAG (*pciFindFirstFP)(void);
extern PCITAG (*pciFindNextFP)(void);

extern CARD32 pciDevid;
extern CARD32 pciDevidMask;

extern int    pciMaxBusNum;

extern int    pciBusNum;
extern int    pciDevNum;
extern int    pciFuncNum;
extern PCITAG pciDeviceTag;

extern int    xf86MaxPciDevs;

extern pciBusInfo_t  *pciBusInfo[];

#endif /* _PCI_H */

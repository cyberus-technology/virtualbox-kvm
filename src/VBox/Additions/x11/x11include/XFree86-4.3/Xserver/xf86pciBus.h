/* $XFree86: xc/programs/Xserver/hw/xfree86/common/xf86pciBus.h,v 3.8 2002/09/16 16:55:33 tsi Exp $ */

#ifndef _XF86_PCI_BUS_H
#define _XF86_PCI_BUS_H

#define PCITAG_SPECIAL pciTag(0xFF,0xFF,0xFF)

typedef struct {
    CARD32 command;
    CARD32 base[6];
    CARD32 biosBase;
} pciSave, *pciSavePtr;

typedef void (*SetBitsProcPtr)(PCITAG, int, CARD32, CARD32);
typedef void (*WriteProcPtr)(PCITAG, int, CARD32);

typedef struct {
    PCITAG tag;
    WriteProcPtr func;
    CARD32 ctrl;
} pciArg;

typedef struct {
    int busnum;
    int devnum;
    int funcnum;
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

typedef struct pciBusRec {
    int brbus, brdev, brfunc;	/* ID of the bridge to this bus */
    int primary, secondary, subordinate;
    int subclass;		/* bridge type */
    int interface;
    resPtr preferred_io;	/* I/O range */
    resPtr preferred_mem;	/* non-prefetchable memory range */
    resPtr preferred_pmem;	/* prefetchable memory range */
    resPtr io;			/* for subtractive PCI-PCI bridges */
    resPtr mem;
    resPtr pmem;
    int brcontrol;		/* bridge_control byte */
    struct pciBusRec *next;
} PciBusRec, *PciBusPtr;

void xf86PciProbe(void);
void ValidatePci(void);
resList GetImplicitPciResources(int entityIndex);
void initPciState(void);
void initPciBusState(void);
void DisablePciAccess(void);
void DisablePciBusAccess(void);
void PciStateEnter(void);
void PciBusStateEnter(void);
void PciStateLeave(void);
void PciBusStateLeave(void);
resPtr ResourceBrokerInitPci(resPtr *osRes);
void pciConvertRange2Host(int entityIndex, resRange *pRange);
void isaConvertRange2Host(resRange *pRange);

extern pciAccPtr * xf86PciAccInfo;

#endif /* _XF86_PCI_BUS_H */

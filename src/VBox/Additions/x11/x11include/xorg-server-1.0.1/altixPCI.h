#ifdef HAVE_XORG_CONFIG_H
#include <xorg-config.h>
#endif

#ifndef PCI_ALTIX_H
#define PCI_ALTIX_H 1

#include <X11/Xdefs.h>
#include <Pci.h>

Bool xorgProbeAltix(scanpciWrapperOpt flags);
void xf86PreScanAltix(void);
void xf86PostScanAltix(void);

/* Some defines for PCI */
#define VENDOR_SGI 0x10A9
#define CHIP_TIO_CA 0x1010
#define CHIP_PIC_PCI 0x1011

#endif

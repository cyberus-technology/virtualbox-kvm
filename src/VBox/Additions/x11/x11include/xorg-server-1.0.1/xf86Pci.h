/* $XFree86: xc/programs/Xserver/hw/xfree86/os-support/bus/xf86Pci.h,v 1.39 2003/08/24 17:37:05 dawes Exp $ */
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
 * This file contains just the public interface to the PCI code.
 * Drivers should use this file rather than Pci.h.
 */

#ifndef _XF86PCI_H
#define _XF86PCI_H 1
#include <X11/Xarch.h>
#include <X11/Xfuncproto.h>
#include "misc.h"

#define PCI_NOT_FOUND	0xFFFFFFFFU

/*
 * PCI cfg space definitions (e.g. stuff right out of the PCI spec)
 */

/* Device identification register */
#define PCI_ID_REG			0x00

/* Command and status register */
#define PCI_CMD_STAT_REG		0x04
#define PCI_CMD_BASE_REG		0x10
#define PCI_CMD_BIOS_REG		0x30
#define PCI_CMD_MASK			0xffff
#define PCI_CMD_IO_ENABLE		0x01
#define PCI_CMD_MEM_ENABLE		0x02
#define PCI_CMD_MASTER_ENABLE		0x04
#define PCI_CMD_SPECIAL_ENABLE		0x08
#define PCI_CMD_INVALIDATE_ENABLE	0x10
#define PCI_CMD_PALETTE_ENABLE		0x20
#define PCI_CMD_PARITY_ENABLE		0x40
#define PCI_CMD_STEPPING_ENABLE		0x80
#define PCI_CMD_SERR_ENABLE		0x100
#define PCI_CMD_BACKTOBACK_ENABLE	0x200
#define PCI_CMD_BIOS_ENABLE		0x01

/* base class */
#define PCI_CLASS_REG		0x08
#define PCI_CLASS_MASK		0xff000000
#define PCI_CLASS_SHIFT		24
#define PCI_CLASS_EXTRACT(x)	\
	(((x) & PCI_CLASS_MASK) >> PCI_CLASS_SHIFT)

/* base class values */
#define PCI_CLASS_PREHISTORIC		0x00
#define PCI_CLASS_MASS_STORAGE		0x01
#define PCI_CLASS_NETWORK		0x02
#define PCI_CLASS_DISPLAY		0x03
#define PCI_CLASS_MULTIMEDIA		0x04
#define PCI_CLASS_MEMORY		0x05
#define PCI_CLASS_BRIDGE		0x06
#define PCI_CLASS_COMMUNICATIONS	0x07
#define PCI_CLASS_SYSPERIPH		0x08
#define PCI_CLASS_INPUT			0x09
#define PCI_CLASS_DOCKING		0x0a
#define PCI_CLASS_PROCESSOR		0x0b
#define PCI_CLASS_SERIALBUS		0x0c
#define PCI_CLASS_WIRELESS		0x0d
#define PCI_CLASS_I2O			0x0e
#define PCI_CLASS_SATELLITE		0x0f
#define PCI_CLASS_CRYPT			0x10
#define PCI_CLASS_DATA_ACQUISTION	0x11
#define PCI_CLASS_UNDEFINED		0xff

/* sub class */
#define PCI_SUBCLASS_MASK	0x00ff0000
#define PCI_SUBCLASS_SHIFT	16
#define PCI_SUBCLASS_EXTRACT(x)	\
	(((x) & PCI_SUBCLASS_MASK) >> PCI_SUBCLASS_SHIFT)

/* Sub class values */
/* 0x00 prehistoric subclasses */
#define PCI_SUBCLASS_PREHISTORIC_MISC	0x00
#define PCI_SUBCLASS_PREHISTORIC_VGA	0x01

/* 0x01 mass storage subclasses */
#define PCI_SUBCLASS_MASS_STORAGE_SCSI		0x00
#define PCI_SUBCLASS_MASS_STORAGE_IDE		0x01
#define PCI_SUBCLASS_MASS_STORAGE_FLOPPY	0x02
#define PCI_SUBCLASS_MASS_STORAGE_IPI		0x03
#define PCI_SUBCLASS_MASS_STORAGE_MISC		0x80

/* 0x02 network subclasses */
#define PCI_SUBCLASS_NETWORK_ETHERNET	0x00
#define PCI_SUBCLASS_NETWORK_TOKENRING	0x01
#define PCI_SUBCLASS_NETWORK_FDDI	0x02
#define PCI_SUBCLASS_NETWORK_MISC	0x80

/* 0x03 display subclasses */
#define PCI_SUBCLASS_DISPLAY_VGA	0x00
#define PCI_SUBCLASS_DISPLAY_XGA	0x01
#define PCI_SUBCLASS_DISPLAY_MISC	0x80

/* 0x04 multimedia subclasses */
#define PCI_SUBCLASS_MULTIMEDIA_VIDEO	0x00
#define PCI_SUBCLASS_MULTIMEDIA_AUDIO	0x01
#define PCI_SUBCLASS_MULTIMEDIA_MISC	0x80

/* 0x05 memory subclasses */
#define PCI_SUBCLASS_MEMORY_RAM		0x00
#define PCI_SUBCLASS_MEMORY_FLASH	0x01
#define PCI_SUBCLASS_MEMORY_MISC	0x80

/* 0x06 bridge subclasses */
#define PCI_SUBCLASS_BRIDGE_HOST	0x00
#define PCI_SUBCLASS_BRIDGE_ISA		0x01
#define PCI_SUBCLASS_BRIDGE_EISA	0x02
#define PCI_SUBCLASS_BRIDGE_MC		0x03
#define PCI_SUBCLASS_BRIDGE_PCI		0x04
#define PCI_SUBCLASS_BRIDGE_PCMCIA	0x05
#define PCI_SUBCLASS_BRIDGE_NUBUS	0x06
#define PCI_SUBCLASS_BRIDGE_CARDBUS	0x07
#define PCI_SUBCLASS_BRIDGE_RACEWAY	0x08
#define PCI_SUBCLASS_BRIDGE_MISC	0x80
#define PCI_IF_BRIDGE_PCI_SUBTRACTIVE	0x01

/* 0x07 communications controller subclasses */
#define PCI_SUBCLASS_COMMUNICATIONS_SERIAL	0x00
#define PCI_SUBCLASS_COMMUNICATIONS_PARALLEL	0x01
#define PCI_SUBCLASS_COMMUNICATIONS_MULTISERIAL	0x02
#define PCI_SUBCLASS_COMMUNICATIONS_MODEM	0x03
#define PCI_SUBCLASS_COMMUNICATIONS_MISC	0x80

/* 0x08 generic system peripherals subclasses */
#define PCI_SUBCLASS_SYSPERIPH_PIC	0x00
#define PCI_SUBCLASS_SYSPERIPH_DMA	0x01
#define PCI_SUBCLASS_SYSPERIPH_TIMER	0x02
#define PCI_SUBCLASS_SYSPERIPH_RTC	0x03
#define PCI_SUBCLASS_SYSPERIPH_HOTPCI	0x04
#define PCI_SUBCLASS_SYSPERIPH_MISC	0x80

/* 0x09 input device subclasses */
#define PCI_SUBCLASS_INPUT_KEYBOARD	0x00
#define PCI_SUBCLASS_INPUT_DIGITIZER	0x01
#define PCI_SUBCLASS_INPUT_MOUSE	0x02
#define PCI_SUBCLASS_INPUT_SCANNER	0x03
#define PCI_SUBCLASS_INPUT_GAMEPORT	0x04
#define PCI_SUBCLASS_INPUT_MISC		0x80

/* 0x0a docking station subclasses */
#define PCI_SUBCLASS_DOCKING_GENERIC	0x00
#define PCI_SUBCLASS_DOCKING_MISC	0x80

/* 0x0b processor subclasses */
#define PCI_SUBCLASS_PROCESSOR_386	0x00
#define PCI_SUBCLASS_PROCESSOR_486	0x01
#define PCI_SUBCLASS_PROCESSOR_PENTIUM	0x02
#define PCI_SUBCLASS_PROCESSOR_ALPHA	0x10
#define PCI_SUBCLASS_PROCESSOR_POWERPC	0x20
#define PCI_SUBCLASS_PROCESSOR_MIPS	0x30
#define PCI_SUBCLASS_PROCESSOR_COPROC	0x40

/* 0x0c serial bus controller subclasses */
#define PCI_SUBCLASS_SERIAL_FIREWIRE		0x00
#define PCI_SUBCLASS_SERIAL_ACCESS		0x01
#define PCI_SUBCLASS_SERIAL_SSA			0x02
#define PCI_SUBCLASS_SERIAL_USB			0x03
#define PCI_SUBCLASS_SERIAL_FIBRECHANNEL	0x04
#define PCI_SUBCLASS_SERIAL_SMBUS		0x05

/* 0x0d wireless controller subclasses */
#define PCI_SUBCLASS_WIRELESS_IRDA		0x00
#define PCI_SUBCLASS_WIRELESS_CONSUMER_IR	0x01
#define PCI_SUBCLASS_WIRELESS_RF		0x02
#define PCI_SUBCLASS_WIRELESS_MISC		0x80

/* 0x0e intelligent I/O controller subclasses */
#define PCI_SUBCLASS_I2O_I2O		0x00

/* 0x0f satellite communications controller subclasses */
#define PCI_SUBCLASS_SATELLITE_TV	0x01
#define PCI_SUBCLASS_SATELLITE_AUDIO	0x02
#define PCI_SUBCLASS_SATELLITE_VOICE	0x03
#define PCI_SUBCLASS_SATELLITE_DATA	0x04

/* 0x10 encryption/decryption controller subclasses */
#define PCI_SUBCLASS_CRYPT_NET_COMPUTING	0x00
#define PCI_SUBCLASS_CRYPT_ENTERTAINMENT	0x10
#define PCI_SUBCLASS_CRYPT_MISC			0x80

/* 0x11 data acquisition and signal processing controller subclasses */
#define PCI_SUBCLASS_DATAACQ_DPIO	0x00
#define PCI_SUBCLASS_DATAACQ_MISC	0x80


/* Header */
#define PCI_HEADER_MISC			0x0c
#define PCI_HEADER_MULTIFUNCTION	0x00800000

/* Interrupt configration register */
#define PCI_INTERRUPT_REG		0x3c
#define PCI_INTERRUPT_PIN_MASK		0x0000ff00
#define PCI_INTERRUPT_PIN_EXTRACT(x)	\
	((((x) & PCI_INTERRUPT_PIN_MASK) >> 8) & 0xff)
#define PCI_INTERRUPT_PIN_NONE		0x00
#define PCI_INTERRUPT_PIN_A		0x01
#define PCI_INTERRUPT_PIN_B		0x02
#define PCI_INTERRUPT_PIN_C		0x03
#define PCI_INTERRUPT_PIN_D		0x04

#define PCI_INTERRUPT_LINE_MASK		0x000000ff
#define PCI_INTERRUPT_LINE_EXTRACT(x)	\
	((((x) & PCI_INTERRUPT_LINE_MASK) >> 0) & 0xff)
#define PCI_INTERRUPT_LINE_INSERT(x,v)	\
	(((x) & ~PCI_INTERRUPT_LINE_MASK) | ((v) << 0))

/* Base registers */
#define PCI_MAP_REG_START		0x10
#define PCI_MAP_REG_END			0x28
#define PCI_MAP_ROM_REG			0x30

#define PCI_MAP_MEMORY			0x00000000
#define PCI_MAP_IO			0x00000001

#define PCI_MAP_MEMORY_TYPE		0x00000007
#define PCI_MAP_IO_TYPE			0x00000003

#define PCI_MAP_MEMORY_TYPE_32BIT	0x00000000
#define PCI_MAP_MEMORY_TYPE_32BIT_1M	0x00000002
#define PCI_MAP_MEMORY_TYPE_64BIT	0x00000004
#define PCI_MAP_MEMORY_TYPE_MASK	0x00000006
#define PCI_MAP_MEMORY_CACHABLE		0x00000008
#define PCI_MAP_MEMORY_ATTR_MASK	0x0000000e
#define PCI_MAP_MEMORY_ADDRESS_MASK	0xfffffff0

#define PCI_MAP_IO_ATTR_MASK		0x00000003

#define PCI_MAP_IS_IO(b)	((b) & PCI_MAP_IO)
#define PCI_MAP_IS_MEM(b)	(!PCI_MAP_IS_IO(b))

#define PCI_MAP_IS64BITMEM(b)	\
	(((b) & PCI_MAP_MEMORY_TYPE) == PCI_MAP_MEMORY_TYPE_64BIT)

#define PCIGETMEMORY(b)		((b) & PCI_MAP_MEMORY_ADDRESS_MASK)
#define PCIGETMEMORY64HIGH(b)	(*((CARD32*)&(b) + 1))
#define PCIGETMEMORY64(b)	\
	(PCIGETMEMORY(b) | ((CARD64)PCIGETMEMORY64HIGH(b) << 32))

#define PCI_MAP_IO_ADDRESS_MASK		0xfffffffc

#define PCIGETIO(b)		((b) & PCI_MAP_IO_ADDRESS_MASK)

#define PCI_MAP_ROM_DECODE_ENABLE	0x00000001
#define PCI_MAP_ROM_ADDRESS_MASK	0xfffff800

#define PCIGETROM(b)		((b) & PCI_MAP_ROM_ADDRESS_MASK)

/* PCI-PCI bridge mapping registers */
#define PCI_PCI_BRIDGE_BUS_REG		0x18
#define PCI_SUBORDINATE_BUS_MASK	0x00ff0000
#define PCI_SECONDARY_BUS_MASK		0x0000ff00
#define PCI_PRIMARY_BUS_MASK		0x000000ff

#define PCI_PCI_BRIDGE_IO_REG		0x1c
#define PCI_PCI_BRIDGE_MEM_REG		0x20
#define PCI_PCI_BRIDGE_PMEM_REG		0x24

#define PCI_PPB_IOBASE_EXTRACT(x)	(((x) << 8) & 0xFF00)
#define PCI_PPB_IOLIMIT_EXTRACT(x)	(((x) << 0) & 0xFF00)

#define PCI_PPB_MEMBASE_EXTRACT(x)	(((x) << 16) & 0xFFFF0000)
#define PCI_PPB_MEMLIMIT_EXTRACT(x)	(((x) <<  0) & 0xFFFF0000)

#define PCI_PCI_BRIDGE_CONTROL_REG	0x3E
#define PCI_PCI_BRIDGE_PARITY_EN	0x01
#define PCI_PCI_BRIDGE_SERR_EN		0x02
#define PCI_PCI_BRIDGE_ISA_EN		0x04
#define PCI_PCI_BRIDGE_VGA_EN		0x08
#define PCI_PCI_BRIDGE_MASTER_ABORT_EN	0x20
#define PCI_PCI_BRIDGE_SECONDARY_RESET	0x40
#define PCI_PCI_BRIDGE_FAST_B2B_EN	0x80
/* header type 2 extensions */
#define PCI_CB_BRIDGE_CTL_CB_RESET	0x40	/* CardBus reset */
#define PCI_CB_BRIDGE_CTL_16BIT_INT	0x80	/* Enable interrupt for 16-bit cards */
#define PCI_CB_BRIDGE_CTL_PREFETCH_MEM0	0x100
#define PCI_CB_BRIDGE_CTL_PREFETCH_MEM1	0x200
#define PCI_CB_BRIDGE_CTL_POST_WRITES	0x400

#define PCI_CB_SEC_STATUS_REG		0x16	/* Secondary status */
#define PCI_CB_PRIMARY_BUS_REG		0x18	/* PCI bus number */
#define PCI_CB_CARD_BUS_REG		0x19	/* CardBus bus number */
#define PCI_CB_SUBORDINATE_BUS_REG	0x1a	/* Subordinate bus number */
#define PCI_CB_LATENCY_TIMER_REG	0x1b	/* CardBus latency timer */
#define PCI_CB_MEM_BASE_0_REG		0x1c
#define PCI_CB_MEM_LIMIT_0_REG		0x20
#define PCI_CB_MEM_BASE_1_REG		0x24
#define PCI_CB_MEM_LIMIT_1_REG		0x28
#define PCI_CB_IO_BASE_0_REG		0x2c
#define PCI_CB_IO_LIMIT_0_REG		0x30
#define PCI_CB_IO_BASE_1_REG		0x34
#define PCI_CB_IO_LIMIT_1_REG		0x38
#define PCI_CB_BRIDGE_CONTROL_REG	0x3E

#define PCI_CB_IO_RANGE_MASK		~0x03
#define PCI_CB_IOBASE(x)		(x & PCI_CB_IO_RANGE_MASK)
#define PCI_CB_IOLIMIT(x)		((x & PCI_CB_IO_RANGE_MASK) + 3)

/* Subsystem identification register */
#define PCI_SUBSYSTEM_ID_REG		0x2c

/* User defined cfg space regs */
#define PCI_REG_USERCONFIG		0x40
#define PCI_OPTION_REG			0x40

/*
 * Typedefs, etc...
 */

/* Primitive Types */
typedef unsigned long ADDRESS;		/* Memory/PCI address */
typedef unsigned long IOADDRESS;	/* Must be large enough for a pointer */
typedef unsigned long PCITAG;

/*
 * PCI configuration space
 */
typedef struct pci_cfg_regs {
    /* start of official PCI config space header */
    union {				/* Offset 0x0 - 0x3 */
	CARD32 device_vendor;
	struct {
#if X_BYTE_ORDER == X_BIG_ENDIAN
	    CARD16 device;
	    CARD16 vendor;
#else
	    CARD16 vendor;
	    CARD16 device;
#endif
	} dv;
    } dv_id;

    union {				/* Offset 0x4 - 0x8 */
	CARD32 status_command;
	struct {
#if X_BYTE_ORDER == X_BIG_ENDIAN
	    CARD16 status;
	    CARD16 command;
#else
	    CARD16 command;
	    CARD16 status;
#endif
	} sc;
    } stat_cmd;

    union {				/* Offset 0x8 - 0xb */
	CARD32 class_revision;
	struct {
#if X_BYTE_ORDER == X_BIG_ENDIAN
	    CARD8 base_class;
	    CARD8 sub_class;
	    CARD8 prog_if;
	    CARD8 rev_id;
#else
	    CARD8 rev_id;
	    CARD8 prog_if;
	    CARD8 sub_class;
	    CARD8 base_class;
#endif
	} cr;
    } class_rev;

    union {				/* Offset 0xc - 0xf */
	CARD32 bist_header_latency_cache;
	struct {
#if X_BYTE_ORDER == X_BIG_ENDIAN
	    CARD8 bist;
	    CARD8 header_type;
	    CARD8 latency_timer;
	    CARD8 cache_line_size;
#else
	    CARD8 cache_line_size;
	    CARD8 latency_timer;
	    CARD8 header_type;
	    CARD8 bist;
#endif
	} bhlc;
    } bhlc;
    union {				/* Offset 0x10 - 0x3b */
	struct {				/* header type 2 */
	    CARD32 cg_rsrvd1;			/* 0x10 */
#if X_BYTE_ORDER == X_BIG_ENDIAN
	    CARD16 secondary_status;		/* 0x16 */
	    CARD16 cg_rsrvd2;			/* 0x14 */

	    union {
		CARD32 cg_bus_reg;
		struct {
		    CARD8 latency_timer;		/* 0x1b */
		    CARD8 subordinate_bus_number;	/* 0x1a */
		    CARD8 cardbus_bus_number;		/* 0x19 */
		    CARD8 primary_bus_number;		/* 0x18 */
		} cgbr;
	    } cgbr;
#else
	    CARD16 cg_rsrvd2;			/* 0x14 */
	    CARD16 secondary_status;		/* 0x16 */

	    union {
		CARD32 cg_bus_reg;
		struct {
		    CARD8  primary_bus_number;		/* 0x18 */
		    CARD8  cardbus_bus_number;		/* 0x19 */
		    CARD8  subordinate_bus_number;	/* 0x1a */
		    CARD8  latency_timer;		/* 0x1b */
		} cgbr;
	    } cgbr;
#endif
	    CARD32 mem_base0;			/* 0x1c */
	    CARD32 mem_limit0;			/* 0x20 */
	    CARD32 mem_base1;			/* 0x24 */
	    CARD32 mem_limit1;			/* 0x28 */
	    CARD32 io_base0;			/* 0x2c */
	    CARD32 io_limit0;			/* 0x30 */
	    CARD32 io_base1;			/* 0x34 */
	    CARD32 io_limit1;			/* 0x38 */
	} cg;
	struct {
	    union {			/* Offset 0x10 - 0x27 */
		struct {			/* header type 0 */
		    CARD32 dv_base0;
		    CARD32 dv_base1;
		    CARD32 dv_base2;
		    CARD32 dv_base3;
		    CARD32 dv_base4;
		    CARD32 dv_base5;
		} dv;
		struct {			/* header type 1 */
		    CARD32 bg_rsrvd[2];
#if X_BYTE_ORDER == X_BIG_ENDIAN
		    union {
			CARD32 pp_bus_reg;
			struct {
			    CARD8  secondary_latency_timer;
			    CARD8  subordinate_bus_number;
			    CARD8  secondary_bus_number;
			    CARD8  primary_bus_number;
			} ppbr;
		    } ppbr;

		    CARD16 secondary_status;
		    CARD8  io_limit;
		    CARD8  io_base;

		    CARD16 mem_limit;
		    CARD16 mem_base;

		    CARD16 prefetch_mem_limit;
		    CARD16 prefetch_mem_base;
#else
		    union {
			CARD32 pp_bus_reg;
			struct {
			    CARD8  primary_bus_number;
			    CARD8  secondary_bus_number;
			    CARD8  subordinate_bus_number;
			    CARD8  secondary_latency_timer;
			} ppbr;
		    } ppbr;

		    CARD8  io_base;
		    CARD8  io_limit;
		    CARD16 secondary_status;

		    CARD16 mem_base;
		    CARD16 mem_limit;

		    CARD16 prefetch_mem_base;
		    CARD16 prefetch_mem_limit;
#endif
		} bg;
	    } bc;
	    union {			/* Offset 0x28 - 0x2b */
		CARD32 rsvd1;
		CARD32 pftch_umem_base;
		CARD32 cardbus_cis_ptr;
	    } um_c_cis;
	    union {			/* Offset 0x2c - 0x2f */
		CARD32 subsys_card_vendor;
		CARD32 pftch_umem_limit;
		CARD32 rsvd2;
		struct {
#if X_BYTE_ORDER == X_BIG_ENDIAN
		    CARD16 subsys_card;
		    CARD16 subsys_vendor;
#else
		    CARD16 subsys_vendor;
		    CARD16 subsys_card;
#endif
		} ssys;
	    } um_ssys_id;
	    union {			/* Offset 0x30 - 0x33 */
		CARD32 baserom;
		struct {
#if X_BYTE_ORDER == X_BIG_ENDIAN
		    CARD16 io_ulimit;
		    CARD16 io_ubase;
#else
		    CARD16 io_ubase;
		    CARD16 io_ulimit;
#endif
		} b_u_io;
	    } uio_rom;
	    struct {
		CARD32 rsvd3;		/* Offset 0x34 - 0x37 */
		CARD32 rsvd4;		/* Offset 0x38 - 0x3b */
	    } rsvd;
	} cd;
    } cx;
    union {				/* Offset 0x3c - 0x3f */
	union {					/* header type 0 */
	    CARD32 max_min_ipin_iline;
	    struct {
#if X_BYTE_ORDER == X_BIG_ENDIAN
		CARD8 max_lat;
		CARD8 min_gnt;
		CARD8 int_pin;
		CARD8 int_line;
#else
		CARD8 int_line;
		CARD8 int_pin;
		CARD8 min_gnt;
		CARD8 max_lat;
#endif
	    } mmii;
	} mmii;
	struct {				/* header type 1 */
#if X_BYTE_ORDER == X_BIG_ENDIAN
	    CARD16 bridge_control;	/* upper 8 bits reserved */
	    CARD8  rsvd2;
	    CARD8  rsvd1;
#else
	    CARD8  rsvd1;
	    CARD8  rsvd2;
	    CARD16 bridge_control;	/* upper 8 bits reserved */
#endif
	} bctrl;
    } bm;
    union {				/* Offset 0x40 - 0xff */
	CARD32 dwords[48];
	CARD8  bytes[192];
    } devspf;
} pciCfgRegs;

typedef union pci_cfg_spc {
    pciCfgRegs regs;
    CARD32     dwords[256/sizeof(CARD32)];
    CARD8      bytes[256/sizeof(CARD8)];
} pciCfgSpc;

/*
 * Data structure returned by xf86scanpci including contents of
 * PCI config space header
 */
typedef struct pci_device {
    PCITAG    tag;
    int	      busnum;
    int	      devnum;
    int	      funcnum;
    pciCfgSpc cfgspc;
    int	      basesize[7];	/* number of bits in base addr allocations */
    Bool      minBasesize;
    CARD32    listed_class;
    pointer   businfo;		/* pointer to secondary's bus info structure */
    Bool      fakeDevice;	/* Device added by system chipset support */
} pciDevice, *pciConfigPtr;

typedef enum {
    PCI_MEM,
    PCI_MEM_SIZE,
    PCI_MEM_SPARSE_BASE,
    PCI_MEM_SPARSE_MASK,
    PCI_IO,
    PCI_IO_SIZE,
    PCI_IO_SPARSE_BASE,
    PCI_IO_SPARSE_MASK
} PciAddrType;

#define pci_device_vendor	      cfgspc.regs.dv_id.device_vendor
#define pci_vendor		      cfgspc.regs.dv_id.dv.vendor
#define pci_device		      cfgspc.regs.dv_id.dv.device
#define pci_status_command	      cfgspc.regs.stat_cmd.status_command
#define pci_command		      cfgspc.regs.stat_cmd.sc.command
#define pci_status		      cfgspc.regs.stat_cmd.sc.status
#define pci_class_revision	      cfgspc.regs.class_rev.class_revision
#define pci_rev_id		      cfgspc.regs.class_rev.cr.rev_id
#define pci_prog_if		      cfgspc.regs.class_rev.cr.prog_if
#define pci_sub_class		      cfgspc.regs.class_rev.cr.sub_class
#define pci_base_class		      cfgspc.regs.class_rev.cr.base_class
#define pci_bist_header_latency_cache cfgspc.regs.bhlc.bist_header_latency_cache
#define pci_cache_line_size	      cfgspc.regs.bhlc.bhlc.cache_line_size
#define pci_latency_timer	      cfgspc.regs.bhlc.bhlc.latency_timer
#define pci_header_type		      cfgspc.regs.bhlc.bhlc.header_type
#define pci_bist		      cfgspc.regs.bhlc.bhlc.bist
#define pci_cb_secondary_status	      cfgspc.regs.cx.cg.secondary_status
#define pci_cb_bus_register           cfgspc.regs.cx.cg.cgbr.cg_bus_reg
#define pci_cb_primary_bus_number     cfgspc.regs.cx.cg.cgbr.cgbr.primary_bus_number
#define pci_cb_cardbus_bus_number     cfgspc.regs.cx.cg.cgbr.cgbr.cardbus_bus_number
#define pci_cb_subordinate_bus_number cfgspc.regs.cx.cg.cgbr.cgbr.subordinate_bus_number
#define pci_cb_latency_timer	      cfgspc.regs.cx.cg.cgbr.cgbr.latency_timer
#define pci_cb_membase0		      cfgspc.regs.cx.cg.mem_base0
#define pci_cb_memlimit0	      cfgspc.regs.cx.cg.mem_limit0
#define pci_cb_membase1		      cfgspc.regs.cx.cg.mem_base1
#define pci_cb_memlimit1	      cfgspc.regs.cx.cg.mem_limit1
#define pci_cb_iobase0		      cfgspc.regs.cx.cg.io_base0
#define pci_cb_iolimit0		      cfgspc.regs.cx.cg.io_limit0
#define pci_cb_iobase1		      cfgspc.regs.cx.cg.io_base1
#define pci_cb_iolimit1		      cfgspc.regs.cx.cg.io_limit1
#define pci_base0		      cfgspc.regs.cx.cd.bc.dv.dv_base0
#define pci_base1		      cfgspc.regs.cx.cd.bc.dv.dv_base1
#define pci_base2		      cfgspc.regs.cx.cd.bc.dv.dv_base2
#define pci_base3		      cfgspc.regs.cx.cd.bc.dv.dv_base3
#define pci_base4		      cfgspc.regs.cx.cd.bc.dv.dv_base4
#define pci_base5		      cfgspc.regs.cx.cd.bc.dv.dv_base5
#define pci_cardbus_cis_ptr	      cfgspc.regs.cx.cd.umem_c_cis.cardbus_cis_ptr
#define pci_subsys_card_vendor	      cfgspc.regs.cx.cd.um_ssys_id.subsys_card_vendor
#define pci_subsys_vendor	      cfgspc.regs.cx.cd.um_ssys_id.ssys.subsys_vendor
#define pci_subsys_card		      cfgspc.regs.cx.cd.um_ssys_id.ssys.subsys_card
#define pci_baserom		      cfgspc.regs.cx.cd.uio_rom.baserom
#define pci_pp_bus_register           cfgspc.regs.cx.cd.bc.bg.ppbr.pp_bus_reg
#define pci_primary_bus_number	      cfgspc.regs.cx.cd.bc.bg.ppbr.ppbr.primary_bus_number
#define pci_secondary_bus_number      cfgspc.regs.cx.cd.bc.bg.ppbr.ppbr.secondary_bus_number
#define pci_subordinate_bus_number    cfgspc.regs.cx.cd.bc.bg.ppbr.ppbr.subordinate_bus_number
#define pci_secondary_latency_timer   cfgspc.regs.cx.cd.bc.bg.ppbr.ppbr.secondary_latency_timer
#define pci_io_base		      cfgspc.regs.cx.cd.bc.bg.io_base
#define pci_io_limit		      cfgspc.regs.cx.cd.bc.bg.io_limit
#define pci_secondary_status	      cfgspc.regs.cx.cd.bc.bg.secondary_status
#define pci_mem_base		      cfgspc.regs.cx.cd.bc.bg.mem_base
#define pci_mem_limit		      cfgspc.regs.cx.cd.bc.bg.mem_limit
#define pci_prefetch_mem_base	      cfgspc.regs.cx.cd.bc.bg.prefetch_mem_base
#define pci_prefetch_mem_limit	      cfgspc.regs.cx.cd.bc.bg.prefetch_mem_limit
#define pci_rsvd1		      cfgspc.regs.cx.cd.um_c_cis.rsvd1
#define pci_rsvd2		      cfgspc.regs.cx.cd.um_ssys_id.rsvd2
#define pci_prefetch_upper_mem_base   cfgspc.regs.cx.cd.um_c_cis.pftch_umem_base
#define pci_prefetch_upper_mem_limit  cfgspc.regs.cx.cd.um_ssys_id.pftch_umem_limit
#define pci_upper_io_base	      cfgspc.regs.cx.cd.uio_rom.b_u_io.io_ubase
#define pci_upper_io_limit	      cfgspc.regs.cx.cd.uio_rom.b_u_io.io_ulimit
#define pci_int_line		      cfgspc.regs.bm.mmii.mmii.int_line
#define pci_int_pin		      cfgspc.regs.bm.mmii.mmii.int_pin
#define pci_min_gnt		      cfgspc.regs.bm.mmii.mmii.min_gnt
#define pci_max_lat		      cfgspc.regs.bm.mmii.mmii.max_lat
#define pci_max_min_ipin_iline	      cfgspc.regs.bm.mmii.max_min_ipin_iline
#define pci_bridge_control	      cfgspc.regs.bm.bctrl.bridge_control
#define pci_user_config		      cfgspc.regs.devspf.dwords[0]
#define pci_user_config_0	      cfgspc.regs.devspf.bytes[0]
#define pci_user_config_1	      cfgspc.regs.devspf.bytes[1]
#define pci_user_config_2	      cfgspc.regs.devspf.bytes[2]
#define pci_user_config_3	      cfgspc.regs.devspf.bytes[3]

typedef enum {
  PCI_BIOS_PC = 0,
  PCI_BIOS_OPEN_FIRMARE,
  PCI_BIOS_HP_PA_RISC,
  PCI_BIOS_OTHER
} PciBiosType;

/* Public PCI access functions */
void	      pciInit(void);
PCITAG	      pciFindFirst(CARD32 id, CARD32 mask);
PCITAG	      pciFindNext(void);
CARD32	      pciReadLong(PCITAG tag, int offset);
CARD16	      pciReadWord(PCITAG tag, int offset);
CARD8	      pciReadByte(PCITAG tag, int offset);
void	      pciWriteLong(PCITAG tag, int offset, CARD32 val);
void	      pciWriteWord(PCITAG tag, int offset, CARD16 val);
void	      pciWriteByte(PCITAG tag, int offset, CARD8 val);
void	      pciSetBitsLong(PCITAG tag, int offset, CARD32 mask, CARD32 val);
void	      pciSetBitsByte(PCITAG tag, int offset, CARD8 mask, CARD8 val);
ADDRESS	      pciBusAddrToHostAddr(PCITAG tag, PciAddrType type, ADDRESS addr);
ADDRESS	      pciHostAddrToBusAddr(PCITAG tag, PciAddrType type, ADDRESS addr);
PCITAG	      pciTag(int busnum, int devnum, int funcnum);
int	      pciGetBaseSize(PCITAG tag, int indx, Bool destructive, Bool *min);
CARD32	      pciCheckForBrokenBase(PCITAG tag,int basereg);
pointer	      xf86MapPciMem(int ScreenNum, int Flags, PCITAG Tag,
				ADDRESS Base, unsigned long Size);
int	      xf86ReadPciBIOS(unsigned long Offset, PCITAG Tag, int basereg,
				unsigned char *Buf, int Len);
int	      xf86ReadPciBIOSByType(unsigned long Offset, PCITAG Tag,
				    int basereg, unsigned char *Buf,
				    int Len, PciBiosType Type);
int	      xf86GetAvailablePciBIOSTypes(PCITAG Tag, int basereg,
					   PciBiosType *Buf);
pciConfigPtr *xf86scanpci(int flags);

extern int pciNumBuses;

/* Domain access functions.  Some of these probably shouldn't be public */
int	      xf86GetPciDomain(PCITAG tag);
pointer	      xf86MapDomainMemory(int ScreenNum, int Flags, PCITAG Tag,
				  ADDRESS Base, unsigned long Size);
IOADDRESS     xf86MapDomainIO(int ScreenNum, int Flags, PCITAG Tag,
			      IOADDRESS Base, unsigned long Size);
int	      xf86ReadDomainMemory(PCITAG Tag, ADDRESS Base, int Len,
				   unsigned char *Buf);

typedef enum {
  ROM_BASE_PRESET = -2,
  ROM_BASE_BIOS,
  ROM_BASE_MEM0 = 0,
  ROM_BASE_MEM1,
  ROM_BASE_MEM2,
  ROM_BASE_MEM3,
  ROM_BASE_MEM4,
  ROM_BASE_MEM5,
  ROM_BASE_FIND
} romBaseSource;

#endif /* _XF86PCI_H */

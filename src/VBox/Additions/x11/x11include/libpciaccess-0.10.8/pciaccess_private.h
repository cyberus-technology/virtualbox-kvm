/*
 * (C) Copyright IBM Corporation 2006
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * on the rights to use, copy, modify, merge, publish, distribute, sub
 * license, and/or sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.  IN NO EVENT SHALL
 * IBM AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

/**
 * \file pciaccess_private.h
 * Functions and datastructures that are private to the pciaccess library.
 * 
 * \author Ian Romanick <idr@us.ibm.com>
 */

#if defined(__GNUC__) && (__GNUC__ >= 4)
# define _pci_hidden      __attribute__((visibility("hidden")))
#elif defined(__SUNPRO_C) && (__SUNPRO_C >= 0x550)
# define _pci_hidden      __hidden
#else /* not gcc >= 4 and not Sun Studio >= 8 */
# define _pci_hidden
#endif /* GNUC >= 4 */

struct pci_device_mapping;

int pci_fill_capabilities_generic( struct pci_device * dev );
int pci_device_generic_unmap_range(struct pci_device *dev,
    struct pci_device_mapping *map);

struct pci_system_methods {
    void (*destroy)( void );
    void (*destroy_device)( struct pci_device * dev );
    int (*read_rom)( struct pci_device * dev, void * buffer );    
    int (*probe)( struct pci_device * dev );
    int (*map_range)(struct pci_device *dev, struct pci_device_mapping *map);
    int (*unmap_range)(struct pci_device * dev,
		       struct pci_device_mapping *map);
    
    int (*read)(struct pci_device * dev, void * data, pciaddr_t offset,
		pciaddr_t size, pciaddr_t * bytes_read );

    int (*write)(struct pci_device * dev, const void * data, pciaddr_t offset,
		pciaddr_t size, pciaddr_t * bytes_written );

    int (*fill_capabilities)( struct pci_device * dev );
    void (*enable)( struct pci_device *dev );
    int (*boot_vga)( struct pci_device *dev );
    int (*has_kernel_driver)( struct pci_device *dev );
};

struct pci_device_mapping {
    pciaddr_t base;
    pciaddr_t size;
    unsigned region;
    unsigned flags;
    void *memory;
};

struct pci_device_private {
    struct pci_device  base;
    const char * device_string;
    
    uint8_t header_type;

    /**
     * \name PCI Capabilities
     */
    /*@{*/
    const struct pci_agp_info * agp;   /**< AGP capability information. */
    /*@}*/
    
    /**
     * Base address of the device's expansion ROM.
     */
    pciaddr_t rom_base;

    /**
     * \name Bridge information.
     */
    /*@{*/
    union {
	struct pci_bridge_info * pci;
	struct pci_pcmcia_bridge_info * pcmcia;
    } bridge;
    /*@}*/

    /**
     * \name Mappings active on this device.
     */
    /*@{*/
    struct pci_device_mapping *mappings;
    unsigned num_mappings;
    /*@}*/
};


/**
 * Base type for tracking PCI subsystem information.
 */
struct pci_system {
    /**
     * Platform dependent implementations of specific API routines.
     */
    const struct pci_system_methods * methods;

    /**
     * Number of known devices in the system.
     */
    size_t num_devices;

    /**
     * Array of known devices.
     */
    struct pci_device_private * devices;

#ifdef HAVE_MTRR
    int mtrr_fd;
#endif
    int vgaarb_fd;
    int vga_count;
    struct pci_device *vga_target;
    struct pci_device *vga_default_dev;
};

extern struct pci_system * pci_sys;

extern int pci_system_linux_sysfs_create( void );
extern int pci_system_freebsd_create( void );
extern int pci_system_netbsd_create( void );
extern int pci_system_openbsd_create( void );
extern void pci_system_openbsd_init_dev_mem( int );
extern int pci_system_solx_devfs_create( void );

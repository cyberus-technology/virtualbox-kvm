/* $XdotOrg: xc/programs/Xserver/hw/xfree86/os-support/sunos/agpgart.h,v 1.1 2005/06/09 03:11:58 alanc Exp $ */
/*
 * AGPGART module version 0.99
 * Copyright (C) 1999 Jeff Hartmann
 * Copyright (C) 1999 Precision Insight, Inc.
 * Copyright (C) 1999 Xi Graphics, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * JEFF HARTMANN, OR ANY OTHER CONTRIBUTORS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
 * OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
/*
 * Copyright 2005 Sun Microsystems, Inc.  All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, and/or sell copies of the Software, and to permit persons
 * to whom the Software is furnished to do so, provided that the above
 * copyright notice(s) and this permission notice appear in all copies of
 * the Software and that both the above copyright notice(s) and this
 * permission notice appear in supporting documentation.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT
 * OF THIRD PARTY RIGHTS. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * HOLDERS INCLUDED IN THIS NOTICE BE LIABLE FOR ANY CLAIM, OR ANY SPECIAL
 * INDIRECT OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING
 * FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION
 * WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Except as contained in this notice, the name of a copyright holder
 * shall not be used in advertising or otherwise to promote the sale, use
 * or other dealings in this Software without prior written authorization
 * of the copyright holder.
 */

#ifndef	_AGPGART_H
#define	_AGPGART_H

#pragma ident	"@(#)agpgart.h	1.1	05/04/04 SMI"

typedef struct _agp_version {
	uint16_t	agpv_major;
	uint16_t	agpv_minor;
} agp_version_t;

typedef struct	_agp_info {
	agp_version_t	agpi_version;
	uint32_t	agpi_devid;	/* bridge vendor + device */
	uint32_t	agpi_mode;	/* mode of bridge */
	ulong_t		agpi_aperbase;	/* base of aperture */
	size_t		agpi_apersize;	/* aperture range size */
	uint32_t	agpi_pgtotal;	/* max number of pages in aperture */
	uint32_t	agpi_pgsystem;	/* same as pg_total */
	uint32_t	agpi_pgused;	/* NUMBER of currently used pages */
} agp_info_t;

typedef struct _agp_setup {
	uint32_t	agps_mode;
} agp_setup_t;

typedef struct _agp_allocate {
	int32_t		agpa_key;
	uint32_t	agpa_pgcount;
	uint32_t	agpa_type;
	uint32_t	agpa_physical;	/* for i810/830 driver */
} agp_allocate_t;

typedef struct _agp_bind {
	int32_t		agpb_key;
	uint32_t	agpb_pgstart;
} agp_bind_t;

typedef struct _agp_unbind {
	int32_t		agpu_key;
	uint32_t	agpu_pri;	/* no use in solaris */
} agp_unbind_t;

#define	AGPIOC_BASE		'G'
#define	AGPIOC_INFO		_IOR(AGPIOC_BASE, 0, 100)
#define	AGPIOC_ACQUIRE		_IO(AGPIOC_BASE, 1)
#define	AGPIOC_RELEASE		_IO(AGPIOC_BASE, 2)
#define	AGPIOC_SETUP		_IOW(AGPIOC_BASE, 3, agp_setup_t)
#define	AGPIOC_ALLOCATE		_IOWR(AGPIOC_BASE, 4, agp_allocate_t)
#define	AGPIOC_DEALLOCATE	_IOW(AGPIOC_BASE, 5, int)
#define	AGPIOC_BIND		_IOW(AGPIOC_BASE, 6, agp_bind_t)
#define	AGPIOC_UNBIND		_IOW(AGPIOC_BASE, 7, agp_unbind_t)

#define	AGP_DEVICE	"/dev/agpgart"

#endif /* _AGPGART_H */

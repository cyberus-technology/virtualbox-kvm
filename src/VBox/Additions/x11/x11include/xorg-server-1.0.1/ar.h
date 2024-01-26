/* $XFree86: xc/programs/Xserver/hw/xfree86/loader/ar.h,v 1.3 1998/07/25 16:56:12 dawes Exp $ */

#ifdef HAVE_XORG_CONFIG_H
#include <xorg-config.h>
#endif

#ifndef _AR_H
#define _AR_H

#define ARMAG "!<arch>\n"
#define SARMAG 8
#define ARFMAG "`\n"

#if !(defined(__powerpc__) && defined(Lynx))
struct ar_hdr {
    char ar_name[16];
    char ar_date[12];
    char ar_uid[6];
    char ar_gid[6];
    char ar_mode[8];
    char ar_size[10];
    char ar_fmag[2];
};

#else

#define AIAMAG "<aiaff>\n"
#define SAIAMAG 8
#define AIAFMAG "`\n"

struct fl_hdr {			/* archive fixed length header - printable ascii */
    char fl_magic[SAIAMAG];	/* Archive file magic string */
    char fl_memoff[12];		/* Offset to member table */
    char fl_gstoff[12];		/* Offset to global symbol table */
    char fl_fstmoff[12];	/* Offset to first archive member */
    char fl_lstmoff[12];	/* Offset to last archive member */
    char fl_freeoff[12];	/* Offset to first mem on free list */
};

#define FL_HDR struct fl_hdr
#define FL_HSZ sizeof(FL_HDR)

struct ar_hdr {			/* archive file member header - printable ascii */
    char ar_size[12];		/* file member size - decimal */
    char ar_nxtmem[12];		/* pointer to next member -  decimal */
    char ar_prvmem[12];		/* pointer to previous member -  decimal */
    char ar_date[12];		/* file member date - decimal */
    char ar_uid[12];		/* file member user id - decimal */
    char ar_gid[12];		/* file member group id - decimal */
    char ar_mode[12];		/* file member mode - octal */
    char ar_namlen[4];		/* file member name length - decimal */
    union {
	char an_name[2];	/* variable length member name */
	char an_fmag[2];	/* AIAFMAG - string to end header */
    } _ar_name;			/*      and variable length name */
};

#define ar_name _ar_name.an_name

/*
 *	Note: 	'ar_namlen' contains the length of the member name which
 *		may be up to 255 chars.  The character string containing
 *		the name begins at '_ar_name.ar_name'.  The terminating
 *		string AIAFMAG, is only cosmetic. File member contents begin
 *		at the first even byte boundary past 'header position + 
 *		sizeof(struct ar_hdr) + ar_namlen',  and continue for
 *		'ar_size' bytes.
*/

#define AR_HDR struct ar_hdr
#define AR_HSZ sizeof(AR_HDR)

#endif /* !__powerpc__ && Lynx */

#endif /* _AR_H */

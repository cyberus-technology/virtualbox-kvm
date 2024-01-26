/* $XFree86: xc/programs/Xserver/hw/xfree86/os-support/xf86_OSlib.h,v 3.94 2003/11/03 05:11:51 tsi Exp $ */
/*
 * Copyright 1990, 1991 by Thomas Roell, Dinkelscherben, Germany
 * Copyright 1992 by David Dawes <dawes@XFree86.org>
 * Copyright 1992 by Jim Tsillas <jtsilla@damon.ccs.northeastern.edu>
 * Copyright 1992 by Rich Murphey <Rich@Rice.edu>
 * Copyright 1992 by Robert Baron <Robert.Baron@ernst.mach.cs.cmu.edu>
 * Copyright 1992 by Orest Zborowski <obz@eskimo.com>
 * Copyright 1993 by Vrije Universiteit, The Netherlands
 * Copyright 1993 by David Wexelblat <dwex@XFree86.org>
 * Copyright 1994, 1996 by Holger Veit <Holger.Veit@gmd.de>
 * Copyright 1997 by Takis Psarogiannakopoulos <takis@dpmms.cam.ac.uk>
 * Copyright 1994-2003 by The XFree86 Project, Inc
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the names of the above listed copyright holders 
 * not be used in advertising or publicity pertaining to distribution of 
 * the software without specific, written prior permission.  The above listed
 * copyright holders make no representations about the suitability of this 
 * software for any purpose.  It is provided "as is" without express or 
 * implied warranty.
 *
 * THE ABOVE LISTED COPYRIGHT HOLDERS DISCLAIM ALL WARRANTIES WITH REGARD 
 * TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY 
 * AND FITNESS, IN NO EVENT SHALL THE ABOVE LISTED COPYRIGHT HOLDERS BE 
 * LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY 
 * DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER 
 * IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING 
 * OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */

/*
 * The ARM32 code here carries the following copyright:
 *
 * Copyright 1997
 * Digital Equipment Corporation. All rights reserved.
 * This software is furnished under license and may be used and copied only in 
 * accordance with the following terms and conditions.  Subject to these
 * conditions, you may download, copy, install, use, modify and distribute
 * this software in source and/or binary form. No title or ownership is
 * transferred hereby.
 *
 * 1) Any source code used, modified or distributed must reproduce and retain
 *    this copyright notice and list of conditions as they appear in the
 *    source file.
 *
 * 2) No right is granted to use any trade name, trademark, or logo of Digital 
 *    Equipment Corporation. Neither the "Digital Equipment Corporation"
 *    name nor any trademark or logo of Digital Equipment Corporation may be
 *    used to endorse or promote products derived from this software without
 *    the prior written permission of Digital Equipment Corporation.
 *
 * 3) This software is provided "AS-IS" and any express or implied warranties,
 *    including but not limited to, any implied warranties of merchantability,
 *    fitness for a particular purpose, or non-infringement are disclaimed.
 *    In no event shall DIGITAL be liable for any damages whatsoever, and in
 *    particular, DIGITAL shall not be liable for special, indirect,
 *    consequential, or incidental damages or damages for lost profits, loss
 *    of revenue or loss of use, whether such damages arise in contract, 
 *    negligence, tort, under statute, in equity, at law or otherwise, even
 *    if advised of the possibility of such damage. 
 *
 */

/* $XConsortium: xf86_OSlib.h /main/22 1996/10/27 11:06:31 kaleb $ */
/* $XdotOrg: xserver/xorg/hw/xfree86/os-support/xf86_OSlib.h,v 1.15 2006/02/03 02:37:52 reed Exp $ */

/*
 * This is private, and should not be included by any drivers.  Drivers
 * may include xf86_OSproc.h to get prototypes for public interfaces.
 */

#ifndef _XF86_OSLIB_H
#define _XF86_OSLIB_H

#include <X11/Xos.h>
#include <X11/Xfuncproto.h>

/*
 * Define some things from the "ANSI" C wrappers that are needed in the
 * the core server.
 */
#ifndef HAVE_WRAPPER_DECLS
#define HAVE_WRAPPER_DECLS
#undef usleep
#define usleep(a) xf86usleep(a)
extern void xf86usleep(unsigned long);
extern int xf86getpagesize(void);
extern int xf86GetErrno(void);  
typedef unsigned long xf86size_t;
typedef signed long xf86ssize_t;
#endif

#include <stdio.h>
#include <ctype.h>
#include <stddef.h>

/**************************************************************************/
/* SYSV386 (SVR3, SVR4), including Solaris                                */
/**************************************************************************/
#if (defined(SYSV) || defined(SVR4)) && \
    !defined(DGUX) && !defined(sgi) && \
    (defined(sun) || defined(i386))
# ifdef SCO325
#  ifndef _SVID3
#   define _SVID3
#  endif
#  ifndef _NO_STATIC
#   define _NO_STATIC
#  endif
# endif
# include <sys/ioctl.h>
# include <signal.h>
# include <termio.h>
# include <sys/stat.h>
# include <sys/types.h>
# if defined(__SCO__) || defined(ISC)
# include <sys/param.h>
# endif

# ifdef ISC
#  define TIOCMSET (TIOC|26)	/* set all modem bits */
#  define TIOCMBIS (TIOC|27)	/* bis modem bits */
#  define TIOCMBIC (TIOC|28)	/* bic modem bits */
#  define TIOCMGET (TIOC|29)	/* get all modem bits */
# endif

# include <errno.h>

# if defined(_NEED_SYSI86)
#  include <sys/immu.h>
#  if !(defined (sun) && defined (SVR4))
#    include <sys/region.h>
#  endif
#  include <sys/proc.h>
#  include <sys/tss.h>
#  include <sys/sysi86.h>
#  if defined(SVR4) && !defined(sun)
#   include <sys/seg.h>
#  endif /* SVR4 && !sun */
/* V86SC_IOPL was moved to <sys/sysi86.h> on Solaris 7 and later */
#  if defined(sun) && defined (SVR4)		/* Solaris? */
#   if defined(i386) || defined(__x86)		/* on x86 or x64? */
#    if !defined(V86SC_IOPL)			/* Solaris 7 or later? */
#     include <sys/v86.h>			/* Nope */
#    endif
#   endif /* V86SC_IOPL */
#  else 
#   include <sys/v86.h>					/* Not solaris */
#  endif /* sun && i386 && SVR4 */
#  if defined(sun) && (defined (i386) || defined(__x86))  && defined (SVR4)
#    include <sys/psw.h>
#  endif
# endif /* _NEED_SYSI86 */

# if defined(HAS_SVR3_MMAPDRV)
#  include <sys/sysmacros.h>
#  if !defined(_NEED_SYSI86)
#   include <sys/immu.h>
#   include <sys/region.h>
#  endif
#  include <sys/mmap.h>		/* MMAP driver header */
# endif

# if !defined(sun) || (!defined(sparc) && !defined(__SOL8__))
#  define HAS_USL_VTS
# endif
# if !defined(sun)
#  include <sys/emap.h>
# endif
# if defined(SCO325)
#  include <sys/vtkd.h>
#  include <sys/console.h>
#  include <sys/scankbd.h>
#  include <sys/vid.h>
#  define LED_CAP CLKED
#  define LED_NUM NLKED
#  define LED_SCR SLKED
# elif defined(HAS_USL_VTS)
#  include <sys/at_ansi.h>
#  include <sys/kd.h>
#  include <sys/vt.h>
# elif defined(sun)
#  include <sys/fbio.h>
#  include <sys/kbd.h> 
#  include <sys/kbio.h>

/* undefine symbols from <sys/kbd.h> we don't need that conflict with enum
   definitions in parser/xf86tokens.h */
#  undef STRING
#  undef LEFTALT
#  undef RIGHTALT

#  define LED_CAP LED_CAPS_LOCK
#  define LED_NUM LED_NUM_LOCK
#  define LED_SCR LED_SCROLL_LOCK
#  define LED_COMP LED_COMPOSE
# endif /* sun */

# if !defined(VT_ACKACQ)
#  define VT_ACKACQ 2
# endif /* !VT_ACKACQ */

# if defined(__SCO__)
#  include <sys/sysmacros.h>
#  define POSIX_TTY
# endif /* __SCO__ */

# if defined(SVR4) || defined(SCO325)
#  include <sys/mman.h>
#  if !(defined(sun) && defined (SVR4))
#    define DEV_MEM "/dev/pmem"
#  endif
#  ifdef SCO325
#   undef DEV_MEM
#   define DEV_MEM "/dev/mem"
#  endif
#  define CLEARDTR_SUPPORT
#  define POSIX_TTY
# endif /* SVR4 */

# ifdef ISC
#  include <termios.h>
#  define POSIX_TTY
# endif

# if defined(sun) && defined (i386) && defined (SVR4) && !defined(__SOL8__)
#  define USE_VT_SYSREQ
#  define VT_SYSREQ_DEFAULT TRUE
# endif

# if defined(ATT) && !defined(i386)
#  define i386 /* not defined in ANSI C mode */
# endif /* ATT && !i386 */

# if (defined(ATT) || defined(SVR4)) && !defined(sun)
#  ifndef __UNIXWARE__
#   ifndef XQUEUE
#    define XQUEUE
#   endif
#  endif
#  include <sys/xque.h>
# endif /* ATT || SVR4 */

# ifdef SYSV
#  if !defined(ISC) || defined(ISC202) || defined(ISC22)
#   define NEED_STRERROR
#  endif
# endif

#endif /* (SYSV || SVR4) && !DGUX */



/**************************************************************************/
/* DG/ux R4.20MU03 Intel AViion Machines                                  */
/**************************************************************************/
#if defined(DGUX) && defined(SVR4)
#include <sys/ioctl.h>
#include <signal.h>
#include <ctype.h>
#include <termios.h>      /* Use termios for BSD Flavor ttys */
#include <sys/termios.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/param.h>
#include <errno.h>
#include <sys/sysi86.h>
#include <unistd.h>
#include <sys/proc.h>
#include <sys/map.h>
#include <sys/sysmacros.h>
#include <sys/mman.h>       /* Memory handling */
#include <sys/kd.h>       /* definitios for KDENABIO KDDISABIO needed for IOPL s */
#include <sys/kbd.h>
#include <fcntl.h>
#include <time.h>
#include <sys/stream.h>
#include <sys/ptms.h>

#include <sys/socket.h>
#include <sys/utsname.h>
#include <sys/stropts.h>
#include <sys/sockio.h>


#define POSIX_TTY

#undef HAS_USL_VTS
#undef USE_VT_SYSREQ
#undef VT_ACKACQ

#define LED_CAP KBD_LED_CAPS_LOCK
#define LED_NUM KBD_LED_NUM_LOCK
#define LED_SCR KBD_LED_SCROLL_LOCK

#define KDGKBTYPE KBD_GET_LANGUAGE


/* General keyboard types */
# define KB_84          2
# define KB_101         1  /* Because ioctl(dgkeybdFd,KBD_GET_LANGUAGE,&type) gives 1=US keyboard */
# define KB_OTHER       3

#define KDSETLED KBD_SET_LED
#define KDGETLED KBD_GET_STATE
#undef KDMKTONE
#define KDMKTONE KBD_TONE_HIGH


#undef DEV_MEM
#define DEV_MEM "/dev/mem"
#define CLEARDTR_SUPPORT

#undef  VT_SYSREQ_DEFAULT
#define VT_SYSREQ_DEFAULT FALSE        /* Make sure that we dont define any VTs since DG/ux has none */

#endif /* DGUX && SVR4 */

/**************************************************************************/
/* Linux or Glibc-based system                                            */
/**************************************************************************/
#if defined(__linux__) || defined(__GLIBC__)
# include <sys/ioctl.h>
# include <signal.h>
# include <stdlib.h>
# include <sys/types.h>
# include <assert.h>

#ifdef __GNU__ /* GNU/Hurd */
# define USE_OSMOUSE
#endif

# ifdef __linux__
#  include <termio.h>
# else /* __GLIBC__ */
#  include <termios.h>
# endif
# ifdef __sparc__
#  include <sys/param.h>
# endif

# include <errno.h>

# include <sys/stat.h>

# include <sys/mman.h>
# ifdef __linux__
#  define HAS_USL_VTS
#  include <sys/kd.h>
#  include <sys/vt.h>
#  define LDGMAP GIO_SCRNMAP
#  define LDSMAP PIO_SCRNMAP
#  define LDNMAP LDSMAP
#  define CLEARDTR_SUPPORT
#  define USE_VT_SYSREQ
# endif

# define POSIX_TTY

#endif /* __linux__ || __GLIBC__ */

/**************************************************************************/
/* LynxOS AT                                                              */
/**************************************************************************/
#if defined(Lynx)
 
# include <termio.h>
# include <sys/ioctl.h>
# include <param.h>
# include <signal.h>
# include <kd.h>
# include <vt.h>
# include <sys/stat.h>

# include <errno.h>
extern int errno;
 
/* smem_create et.al. to access physical memory */ 
# include <smem.h>
 
/* keyboard types */
# define KB_84		1
# define KB_101 	2
# define KB_OTHER	3

/* atc drivers ignores argument to VT_RELDISP ioctl */
# define VT_ACKACQ	2

# include <termios.h>
# define POSIX_TTY
# define CLEARDTR_SUPPORT

/* LynxOS 2.5.1 has these */
# ifdef LED_NUMLOCK
#  define LED_CAP	LED_CAPSLOCK
#  define LED_NUM	LED_NUMLOCK
#  define LED_SCR	LED_SCROLLOCK
# endif

#endif /* Lynx */

/**************************************************************************/
/* 386BSD and derivatives,  BSD/386                                       */
/**************************************************************************/

#if defined(__386BSD__) && (defined(__FreeBSD__) || defined(__NetBSD__))
# undef __386BSD__
#endif

#ifdef CSRG_BASED
# include <sys/ioctl.h>
# include <signal.h>

# include <termios.h>
# define termio termios
# define POSIX_TTY

# include <errno.h>

# include <sys/types.h>
# include <sys/mman.h>
# include <sys/stat.h>

# if defined(__bsdi__)
#  include <sys/param.h>
# if (_BSDI_VERSION < 199510)
#  include <i386/isa/vgaioctl.h>
# endif
# endif /* __bsdi__ */

#endif /* CSRG_BASED */

/**************************************************************************/
/* Kernel of *BSD                                                         */
/**************************************************************************/
#if defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || \
 defined(__NetBSD__) || defined(__OpenBSD__) || defined(__bsdi__) || defined(__DragonFly__)

# include <sys/param.h>
# if defined(__FreeBSD_version) && !defined(__FreeBSD_kernel_version)
#  define __FreeBSD_kernel_version __FreeBSD_version
# endif

# if !defined(LINKKIT)
  /* Don't need this stuff for the Link Kit */
#  if defined(__bsdi__)
#   include <i386/isa/pcconsioctl.h>
#   define CONSOLE_X_MODE_ON PCCONIOCRAW
#   define CONSOLE_X_MODE_OFF PCCONIOCCOOK
#   define CONSOLE_X_BELL PCCONIOCBEEP
#  else /* __bsdi__ */
#   if defined(__OpenBSD__)
#     ifdef PCCONS_SUPPORT
#       include <machine/pccons.h>
#       undef CONSOLE_X_MODE_ON
#       undef CONSOLE_X_MODE_OFF
#       undef CONSOLE_X_BELL
#     endif
#   endif
#   ifdef SYSCONS_SUPPORT
#    define COMPAT_SYSCONS
#    if defined(__NetBSD__) || defined(__OpenBSD__)
#     include <machine/console.h>
#    else
#     if defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || defined(__DragonFly__)
#        if defined(__DragonFly__)  || (__FreeBSD_kernel_version >= 410000)
#          include <sys/consio.h>
#          include <sys/kbio.h>
#        else
#          include <machine/console.h>
#        endif /* FreeBSD 4.1 RELEASE or lator */
#     else
#      include <sys/console.h>
#     endif
#    endif
#   endif /* SYSCONS_SUPPORT */
#   if defined(PCVT_SUPPORT)
#    if !defined(SYSCONS_SUPPORT)
      /* no syscons, so include pcvt specific header file */
#     if defined(__FreeBSD__) || defined(__FreeBSD_kernel__)
#      include <machine/pcvt_ioctl.h>
#     else
#      if defined(__NetBSD__) || defined(__OpenBSD__)
#       if !defined(WSCONS_SUPPORT)
#        include <machine/pcvt_ioctl.h>
#       endif /* WSCONS_SUPPORT */
#      else
#       include <sys/pcvt_ioctl.h>
#      endif /* __NetBSD__ */
#     endif /* __FreeBSD_kernel__ || __OpenBSD__ */
#    else /* pcvt and syscons: hard-code the ID magic */
#     define VGAPCVTID _IOWR('V',113, struct pcvtid)
      struct pcvtid {
	char name[16];
	int rmajor, rminor;
      };
#    endif /* PCVT_SUPPORT && SYSCONS_SUPPORT */
#   endif /* PCVT_SUPPORT */
#   ifdef WSCONS_SUPPORT
#    include <dev/wscons/wsconsio.h>
#    include <dev/wscons/wsdisplay_usl_io.h>
#   endif /* WSCONS_SUPPORT */
#   if defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || defined(__DragonFly__)
#    if defined(__FreeBSD_kernel_version) && (__FreeBSD_kernel_version >= 500013)
#     include <sys/mouse.h>
#    else
#     undef MOUSE_GETINFO
#     include <machine/mouse.h>
#    endif
#   endif
    /* Include these definitions in case ioctl_pc.h didn't get included */
#   ifndef CONSOLE_X_MODE_ON
#    define CONSOLE_X_MODE_ON _IO('t',121)
#   endif
#   ifndef CONSOLE_X_MODE_OFF
#    define CONSOLE_X_MODE_OFF _IO('t',122)
#   endif
#   ifndef CONSOLE_X_BELL
#    define CONSOLE_X_BELL _IOW('t',123,int[2])
#   endif
#   ifndef CONSOLE_X_TV_ON
#    define CONSOLE_X_TV_ON _IOW('t',155,int)
#    define XMODE_RGB   0
#    define XMODE_NTSC  1
#    define XMODE_PAL   2
#    define XMODE_SECAM 3
#   endif
#   ifndef CONSOLE_X_TV_OFF
#    define CONSOLE_X_TV_OFF _IO('t',156)
#   endif
#ifndef CONSOLE_GET_LINEAR_INFO
#    define CONSOLE_GET_LINEAR_INFO         _IOR('t',157,struct map_info)
#endif
#ifndef CONSOLE_GET_IO_INFO 
#    define CONSOLE_GET_IO_INFO             _IOR('t',158,struct map_info)
#endif
#ifndef CONSOLE_GET_MEM_INFO 
#    define CONSOLE_GET_MEM_INFO            _IOR('t',159,struct map_info)
#endif
#  endif /* __bsdi__ */
# endif /* !LINKKIT */

#if defined(USE_I386_IOPL) || defined(USE_AMD64_IOPL)
#include <machine/sysarch.h>
#endif

# define CLEARDTR_SUPPORT

# if defined(SYSCONS_SUPPORT) || defined(PCVT_SUPPORT) || defined(WSCONS_SUPPORT)
#  define USE_VT_SYSREQ
# endif

#endif
/* __FreeBSD_kernel__ || __NetBSD__ || __OpenBSD__ || __bsdi__ */

/**************************************************************************/
/* OS/2                                                                   */
/**************************************************************************/
/* currently OS/2 with a modified EMX/GCC compiler only */
#if defined(__UNIXOS2__) 
# include <signal.h>
# include <errno.h>
# include <sys/stat.h>

/* I would have liked to have this included here always, but
 * it causes clashes for BYTE and BOOL with Xmd.h, which is too dangerous. 
 * So I'll include it in place where I know it does no harm.
 */
#if defined(I_NEED_OS2_H)
# undef BOOL
# undef BYTE
# include <os2.h>
#endif

  /* keyboard types */
# define KB_84                   1
# define KB_101                  2
/* could detect more keyboards */
# define KB_OTHER                3

  /* LEDs */
#  define LED_CAP 0x40
#  define LED_NUM 0x20
#  define LED_SCR 0x10

  /* mouse driver */
# define OSMOUSE_ONLY
# define MOUSE_PROTOCOL_IN_KERNEL

extern char* __XOS2RedirRoot(char*);

#endif

/**************************************************************************/
/* QNX4                                                                   */
/**************************************************************************/
/* This is the QNX code for Watcom 10.6 and QNX 4.x */
#if defined(QNX4)
#include <signal.h>
#include <errno.h>
#include <sys/stat.h>
#include <termios.h>
#include <ioctl.h>
#include <sys/param.h>

/* Warning: by default, the fd_set size is 32 in QNX!  */
#define FD_SETSIZE 256
#include <sys/select.h>

  /* keyboard types */
# define KB_84                   1
# define KB_101                  2
# define KB_OTHER                3

  /* LEDs */
#  define LED_CAP 0x04
#  define LED_NUM 0x02
#  define LED_SCR 0x01

# define POSIX_TTY
# define OSMOUSE_ONLY
# define MOUSE_PROTOCOL_IN_KERNEL

#define TIOCM_DTR       0x0001            /* data terminal ready */
#define TIOCM_RTS       0x0002            /* request to send */
#define TIOCM_CTS       0x1000            /* clear to send */
#define TIOCM_DSR       0x2000            /* data set ready */
#define TIOCM_RI        0x4000            /* ring */
#define TIOCM_RNG       TIOCM_RI
#define TIOCM_CD        0x8000            /* carrier detect */
#define TIOCM_CAR       TIOCM_CD
#define TIOCM_LE        0x0100            /* line enable */
#define TIOCM_ST        0x0200            /* secondary transmit */
#define TIOCM_SR        0x0400            /* secondary receive */

#endif

/**************************************************************************/
/* QNX/Neutrino                                                           */
/**************************************************************************/
/* This is the Neutrino code for for NTO2.0 and GCC */
#if defined(__QNXNTO__)
#include <signal.h>
#include <errno.h>
#include <sys/stat.h>
#include <termios.h>
#include <ioctl.h>
#include <sys/param.h>

/* Warning: by default, the fd_set size is 32 in NTO!  */
#define FD_SETSIZE 256
#include <sys/select.h>

  /* keyboard types */
# define KB_84                   1
# define KB_101                  2
# define KB_OTHER                3

# define POSIX_TTY

#endif

/**************************************************************************/
/* IRIX                                                                   */
/**************************************************************************/
#if defined(sgi)

#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

#endif

/**************************************************************************/
/* Generic                                                                */
/**************************************************************************/

#include <sys/wait.h>	/* May need to adjust this for other OSs */

/* 
 * Hack originally for ISC 2.2 POSIX headers, but may apply elsewhere,
 * and it's safe, so just do it.
 */
#if !defined(O_NDELAY) && defined(O_NONBLOCK)
# define O_NDELAY O_NONBLOCK
#endif /* !O_NDELAY && O_NONBLOCK */

#if !defined(MAXHOSTNAMELEN)
# define MAXHOSTNAMELEN 32
#endif /* !MAXHOSTNAMELEN */

#if !defined(X_NOT_POSIX)
# if defined(_POSIX_SOURCE)
#  include <limits.h>
# else
#  define _POSIX_SOURCE
#  include <limits.h>
#  undef _POSIX_SOURCE
# endif /* _POSIX_SOURCE */
#endif /* !X_NOT_POSIX */
#if !defined(PATH_MAX)
# if defined(MAXPATHLEN)
#  define PATH_MAX MAXPATHLEN
# else
#  define PATH_MAX 1024
# endif /* MAXPATHLEN */
#endif /* !PATH_MAX */

#ifdef NEED_STRERROR
# ifndef strerror
extern char *sys_errlist[];
extern int sys_nerr;
#  define strerror(n) \
     ((n) >= 0 && (n) < sys_nerr) ? sys_errlist[n] : "unknown error"
# endif /* !strerror */
#endif /* NEED_STRERROR */

#if defined(ISC) || defined(Lynx)
#define rint(x) RInt(x)
double RInt(
	double x
);
#endif

#ifndef DEV_MEM
#define DEV_MEM "/dev/mem"
#endif

#ifndef VT_SYSREQ_DEFAULT
#define VT_SYSREQ_DEFAULT FALSE
#endif

#ifdef OSMOUSE_ONLY
# ifndef MOUSE_PROTOCOL_IN_KERNEL
#  define MOUSE_PROTOCOL_IN_KERNEL
# endif
#endif

#define SYSCALL(call) while(((call) == -1) && (errno == EINTR))

#define XF86_OS_PRIVS
#include "xf86_OSproc.h"

#ifndef NO_COMPILER_H
#include "compiler.h"
#endif

#endif /* _XF86_OSLIB_H */

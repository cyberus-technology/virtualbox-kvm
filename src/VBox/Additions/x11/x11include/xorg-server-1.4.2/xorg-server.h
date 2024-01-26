/* include/xorg-server.h.  Generated from xorg-server.h.in by configure.  */
/* xorg-server.h.in						-*- c -*-
 *
 * This file is the template file for the xorg-server.h file which gets
 * installed as part of the SDK.  The #defines in this file overlap
 * with those from config.h, but only for those options that we want
 * to export to external modules.  Boilerplate autotool #defines such
 * as HAVE_STUFF and PACKAGE_NAME is kept in config.h
 *
 * It is still possible to update config.h.in using autoheader, since
 * autoheader only creates a .h.in file for the first
 * AM_CONFIG_HEADER() line, and thus does not overwrite this file.
 *
 * However, it should be kept in sync with this file.
 */

#ifndef _XORG_SERVER_H_
#define _XORG_SERVER_H_

/* Support BigRequests extension */
#define BIGREQS 1

/* Default font path */
#define COMPILEDDEFAULTFONTPATH "/usr/local/lib/X11/fonts/misc/,/usr/local/lib/X11/fonts/TTF/,/usr/local/lib/X11/fonts/OTF,/usr/local/lib/X11/fonts/Type1/,/usr/local/lib/X11/fonts/100dpi/,/usr/local/lib/X11/fonts/75dpi/"

/* Support Composite Extension */
#define COMPOSITE 1

/* Use OsVendorInit */
#define DDXOSINIT 1

/* Build DPMS extension */
#define DPMSExtension 1

/* Built-in output drivers */
#define DRIVERS {}

/* Build GLX extension */
/* #undef GLXEXT */

/* Include handhelds.org h3600 touchscreen driver */
/* #undef H3600_TS */

/* Support XDM-AUTH*-1 */
#define HASXDMAUTH 1

/* Support SHM */
#define HAS_SHM 1

/* Built-in input drivers */
#define IDRIVERS {}

/* Support IPv6 for TCP connections */
#define IPv6 1

/* Support MIT Misc extension */
#define MITMISC 1

/* Support MIT-SHM Extension */
#define MITSHM 1

/* Disable some debugging code */
#define NDEBUG 1

/* Need XFree86 helper functions */
#define NEED_XF86_PROTOTYPES 1

/* Need XFree86 typedefs */
#define NEED_XF86_TYPES 1

/* Internal define for Xinerama */
#define PANORAMIX 1

/* Support pixmap privates */
#define PIXPRIV 1

/* Support RANDR extension */
#define RANDR 1

/* Support RENDER extension */
#define RENDER 1

/* Support X resource extension */
#define RES 1

/* Support MIT-SCREEN-SAVER extension */
#define SCREENSAVER 1

/* Use a lock to prevent multiple servers on a display */
#define SERVER_LOCK 1

/* Support SHAPE extension */
#define SHAPE 1

/* Include time-based scheduler */
#define SMART_SCHEDULE 1

/* Define to 1 on systems derived from System V Release 4 */
/* #undef SVR4 */

/* Support TCP socket connections */
#define TCPCONN 1

/* Enable touchscreen support */
/* #undef TOUCHSCREEN */

/* Support tslib touchscreen abstraction library */
/* #undef TSLIB */

/* Support UNIX socket connections */
#define UNIXCONN 1

/* Use builtin rgb color database */
/* #undef USE_RGB_BUILTIN */

/* Use rgb.txt directly */
#define USE_RGB_TXT 1

/* unaligned word accesses behave as expected */
/* #undef WORKING_UNALIGNED_INT */

/* Support XCMisc extension */
#define XCMISC 1

/* Support Xdmcp */
#define XDMCP 1

/* Build XFree86 BigFont extension */
#define XF86BIGFONT 1

/* Support XFree86 miscellaneous extensions */
#define XF86MISC 1

/* Support XFree86 Video Mode extension */
#define XF86VIDMODE 1

/* Build XDGA support */
#define XFreeXDGA 1

/* Support Xinerama extension */
#define XINERAMA 1

/* Support X Input extension */
#define XINPUT 1

/* Build XKB */
#define XKB 1

/* Enable XKB per default */
#define XKB_DFLT_DISABLED 0

/* Build XKB server */
#define XKB_IN_SERVER 1

/* Support loadable input and output drivers */
/* #undef XLOADABLE */

/* Build DRI extension */
#define XF86DRI 1

/* Build Xorg server */
#define XORGSERVER 1

/* Vendor release */
/* #undef XORG_RELEASE */

/* Current Xorg version */
#define XORG_VERSION_CURRENT (((1) * 10000000) + ((4) * 100000) + ((2) * 1000) + 0)

/* Build Xv Extension */
#define XvExtension 1

/* Build XvMC Extension */
#define XvMCExtension 1

/* Build XRes extension */
#define XResExtension 1

/* Support XSync extension */
#define XSYNC 1

/* Support XTest extension */
#define XTEST 1

/* Support XTrap extension */
#define XTRAP 1

/* Support Xv Extension */
#define XV 1

/* Vendor name */
#define XVENDORNAME "The X.Org Foundation"

/* Endian order */
#define _X_BYTE_ORDER X_LITTLE_ENDIAN
/* Deal with multiple architecture compiles on Mac OS X */
#ifndef __APPLE_CC__
#define X_BYTE_ORDER _X_BYTE_ORDER
#else
#ifdef __BIG_ENDIAN__
#define X_BYTE_ORDER X_BIG_ENDIAN
#else
#define X_BYTE_ORDER X_LITTLE_ENDIAN
#endif
#endif

/* BSD-compliant source */
/* #undef _BSD_SOURCE */

/* POSIX-compliant source */
/* #undef _POSIX_SOURCE */

/* X/Open-compliant source */
/* #undef _XOPEN_SOURCE */

/* Vendor web address for support */
#define __VENDORDWEBSUPPORT__ "http://wiki.x.org"

/* Location of configuration file */
#define __XCONFIGFILE__ "xorg.conf"

/* XKB default rules */
#define __XKBDEFRULES__ "xorg"

/* Name of X server */
#define __XSERVERNAME__ "Xorg"

/* Define to 1 if unsigned long is 64 bits. */
/* #undef _XSERVER64 */

/* Building vgahw module */
#define WITH_VGAHW 1

/* System is BSD-like */
/* #undef CSRG_BASED */

/* Solaris 8 or later? */
/* #undef __SOL8__ */

/* System has PC console */
/* #undef PCCONS_SUPPORT */

/* System has PCVT console */
/* #undef PCVT_SUPPORT */

/* System has syscons console */
/* #undef SYSCONS_SUPPORT */

/* System has wscons console */
/* #undef WSCONS_SUPPORT */

/* Loadable XFree86 server awesomeness */
#define XFree86LOADER 1

#endif /* _XORG_SERVER_H_ */

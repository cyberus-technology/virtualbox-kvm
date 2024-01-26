
#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#ifndef INITARGS
#define INITARGS void
#endif

#define _SHAPE_SERVER_  /* don't want Xlib structures */
#include <X11/extensions/shapestr.h>

#ifdef MULTIBUFFER
extern void MultibufferExtensionInit(INITARGS);
#define _MULTIBUF_SERVER_	/* don't want Xlib structures */
#include <X11/extensions/multibufst.h>
#endif

#ifdef XTEST
extern void XTestExtensionInit(INITARGS);
#define _XTEST_SERVER_
#include <X11/extensions/XTest.h>
#include <X11/extensions/xteststr.h>
#endif

#if 1
extern void XTestExtension1Init(INITARGS);
#endif

#ifdef SCREENSAVER
extern void ScreenSaverExtensionInit (INITARGS);
#include <X11/extensions/saver.h>
#endif

#ifdef XF86VIDMODE
extern void	XFree86VidModeExtensionInit(INITARGS);
#define _XF86VIDMODE_SERVER_
#include <X11/extensions/xf86vmstr.h>
#endif

#ifdef XFreeXDGA
extern void XFree86DGAExtensionInit(INITARGS);
extern void XFree86DGARegister(INITARGS);
#define _XF86DGA_SERVER_
#include <X11/extensions/xf86dgastr.h>
#endif

#ifdef DPMSExtension
extern void DPMSExtensionInit(INITARGS);
#include <X11/extensions/dpmsstr.h>
#endif

#ifdef XV
extern void XvExtensionInit(INITARGS);
extern void XvMCExtensionInit(INITARGS);
extern void XvRegister(INITARGS);
#include <X11/extensions/Xv.h>
#include <X11/extensions/XvMC.h>
#endif

#ifdef RES
extern void ResExtensionInit(INITARGS);
#include <X11/extensions/XResproto.h>
#endif

#ifdef SHM
extern void ShmExtensionInit(INITARGS);
#include <X11/extensions/shmstr.h>
extern void ShmRegisterFuncs(
    ScreenPtr pScreen,
    ShmFuncsPtr funcs);
#endif

#ifdef XSELINUX
extern void SELinuxExtensionInit(INITARGS);
#include "xselinux.h"
#endif

#ifdef XEVIE
extern void XevieExtensionInit(INITARGS);
#endif

#if 1
extern void SecurityExtensionInit(INITARGS);
#endif

#if 1
extern void PanoramiXExtensionInit(int argc, char *argv[]);
#endif

#if 1
extern void XkbExtensionInit(INITARGS);
#endif

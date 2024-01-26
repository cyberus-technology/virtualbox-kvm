/* $XFree86: xc/include/extensions/xf86rush.h,v 1.4 2000/02/29 03:09:00 dawes Exp $ */
/*

Copyright (c) 1998  Daryll Strauss

*/

#ifndef _XF86RUSH_H_
#define _XF86RUSH_H_

#include <X11/extensions/Xv.h>
#include <X11/Xfuncproto.h>

#define X_XF86RushQueryVersion		0
#define X_XF86RushLockPixmap		1
#define X_XF86RushUnlockPixmap		2
#define X_XF86RushUnlockAllPixmaps	3
#define X_XF86RushGetCopyMode		4
#define X_XF86RushSetCopyMode		5
#define X_XF86RushGetPixelStride	6
#define X_XF86RushSetPixelStride	7
#define X_XF86RushOverlayPixmap		8
#define X_XF86RushStatusRegOffset	9
#define X_XF86RushAT3DEnableRegs	10
#define X_XF86RushAT3DDisableRegs	11

#define XF86RushNumberEvents		0

#define XF86RushClientNotLocal		0
#define XF86RushNumberErrors		(XF86RushClientNotLocal + 1)

#ifndef _XF86RUSH_SERVER_

_XFUNCPROTOBEGIN

Bool XF86RushQueryVersion(
#if NeedFunctionPrototypes
    Display*		/* dpy */,
    int*		/* majorVersion */,
    int*		/* minorVersion */
#endif
);

Bool XF86RushQueryExtension(
#if NeedFunctionPrototypes
    Display*		/* dpy */,
    int*		/* event_base */,
    int*		/* error_base */
#endif
);

Bool XF86RushLockPixmap(
#if NeedFunctionPrototypes
    Display *		/* dpy */,
    int			/* screen */,
    Pixmap		/* Pixmap */,
    void **		/* Return address */
#endif
);

Bool XF86RushUnlockPixmap(
#if NeedFunctionPrototypes
    Display *		/* dpy */,
    int			/* screen */,
    Pixmap		/* Pixmap */
#endif
); 

Bool XF86RushUnlockAllPixmaps(
#if NeedFunctionPrototypes
    Display *		/* dpy */
#endif			    
);

Bool XF86RushSetCopyMode(
#if NeedFunctionPrototypes
    Display *		/* dpy */,
    int			/* screen */,
    int			/* copy mode */
#endif			    
);

Bool XF86RushSetPixelStride(
#if NeedFunctionPrototypes
    Display *		/* dpy */,
    int			/* screen */,
    int			/* pixel stride */
#endif			    
);

Bool XF86RushOverlayPixmap(
#if NeedFunctionPrototypes
    Display *		/* dpy */,
    XvPortID		/* port */,
    Drawable		/* d */,
    GC			/* gc */,
    Pixmap		/* pixmap */,
    int			/* src_x */,
    int			/* src_y */,
    unsigned int	/* src_w */,
    unsigned int	/* src_h */,
    int			/* dest_x */,
    int			/* dest_y */,
    unsigned int	/* dest_w */,
    unsigned int	/* dest_h */,
    unsigned int	/* id */
#endif			    
);

int XF86RushStatusRegOffset(
#if NeedFunctionPrototypes
    Display *		/* dpy */,
    int			/* screen */
#endif			    
);

Bool XF86RushAT3DEnableRegs(
#if NeedFunctionPrototypes
    Display *		/* dpy */,
    int			/* screen */
#endif			    
);

Bool XF86RushAT3DDisableRegs(
#if NeedFunctionPrototypes
    Display *		/* dpy */,
    int			/* screen */
#endif			    
);

_XFUNCPROTOEND

#endif /* _XF86RUSH_SERVER_ */

#endif /* _XF86RUSH_H_ */

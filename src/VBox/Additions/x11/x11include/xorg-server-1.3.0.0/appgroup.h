/*
Copyright 1996, 1998  The Open Group

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall
not be used in advertising or otherwise to promote the sale, use or
other dealings in this Software without prior written authorization
from The Open Group.
*/

#ifndef _APPGROUP_SRV_H_
#define _APPGROUP_SRV_H_

#include <X11/Xfuncproto.h>

_XFUNCPROTOBEGIN

extern void XagConnectionInfo(
    ClientPtr			/* client */,
    xConnSetupPrefix**		/* conn_prefix */,
    char**			/* conn_info */,
    int*			/* num_screens */
);

extern VisualID XagRootVisual(
    ClientPtr			/* client */
);

extern Colormap XagDefaultColormap(
    ClientPtr			/* client */
);

extern ClientPtr XagLeader(
    ClientPtr			/* client */
);

extern void XagCallClientStateChange(
    CallbackListPtr *		/* pcbl */,
    pointer 			/* nulldata */,
    pointer 			/* calldata */
);

extern Bool XagIsControlledRoot (
    ClientPtr			/* client */,
    WindowPtr			/* pParent */
);

extern XID XagId (
    ClientPtr			/* client */
);

extern void XagGetDeltaInfo (
    ClientPtr			/* client */,
    CARD32*			/* buf */
);

extern void XagClientStateChange(
    CallbackListPtr* pcbl,
    pointer nulldata,
    pointer calldata);

extern int ProcXagCreate (
    register ClientPtr client);

extern int ProcXagDestroy(
    register ClientPtr client);

_XFUNCPROTOEND

#endif /* _APPGROUP_SRV_H_ */




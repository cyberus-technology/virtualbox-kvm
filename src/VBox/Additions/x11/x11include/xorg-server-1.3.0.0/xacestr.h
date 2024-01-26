/************************************************************

Author: Eamon Walsh <ewalsh@epoch.ncsc.mil>

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
this permission notice appear in supporting documentation.  This permission
notice shall be included in all copies or substantial portions of the
Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHOR BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

********************************************************/

#ifndef _XACESTR_H
#define _XACESTR_H

#include <X11/Xdefs.h>
#include "dixstruct.h"
#include "resource.h"
#include "extnsionst.h"
#include "gcstruct.h"
#include "windowstr.h"
#include "inputstr.h"
#include "xace.h"

/* XACE_CORE_DISPATCH */
typedef struct {
    ClientPtr client;
    int rval;
} XaceCoreDispatchRec;

/* XACE_RESOURCE_ACCESS */
/* XACE_RESOURCE_CREATE */
typedef struct {
    ClientPtr client;
    XID id;
    RESTYPE rtype;
    Mask access_mode;
    pointer res;
    int rval;
} XaceResourceAccessRec;

/* XACE_DEVICE_ACCESS */
typedef struct {
    ClientPtr client;
    DeviceIntPtr dev;
    Bool fromRequest;
    int rval;
} XaceDeviceAccessRec;

/* XACE_PROPERTY_ACCESS */
typedef struct {
    ClientPtr client;
    WindowPtr pWin;
    Atom propertyName;
    Mask access_mode;
    int rval;
} XacePropertyAccessRec;

/* XACE_DRAWABLE_ACCESS */
typedef struct {
    ClientPtr client;
    DrawablePtr pDraw;
    int rval;
} XaceDrawableAccessRec;

/* XACE_MAP_ACCESS */
/* XACE_BACKGRND_ACCESS */
typedef struct {
    ClientPtr client;
    WindowPtr pWin;
    int rval;
} XaceMapAccessRec;

/* XACE_EXT_DISPATCH_ACCESS */
/* XACE_EXT_ACCESS */
typedef struct {
    ClientPtr client;
    ExtensionEntry *ext;
    int rval;
} XaceExtAccessRec;

/* XACE_HOSTLIST_ACCESS */
typedef struct {
    ClientPtr client;
    Mask access_mode;
    int rval;
} XaceHostlistAccessRec;

/* XACE_SITE_POLICY */
typedef struct {
    char *policyString;
    int len;
    int rval;
} XaceSitePolicyRec;

/* XACE_DECLARE_EXT_SECURE */
typedef struct {
    ExtensionEntry *ext;
    Bool secure;
} XaceDeclareExtSecureRec;

/* XACE_AUTH_AVAIL */
typedef struct {
    ClientPtr client;
    XID authId;
} XaceAuthAvailRec;

/* XACE_KEY_AVAIL */
typedef struct {
    xEventPtr event;
    DeviceIntPtr keybd;
    int count;
} XaceKeyAvailRec;

/* XACE_WINDOW_INIT */
typedef struct {
    ClientPtr client;
    WindowPtr pWin;
} XaceWindowRec;

/* XACE_AUDIT_BEGIN */
/* XACE_AUDIT_END */
typedef struct {
    ClientPtr client;
    int requestResult;
} XaceAuditRec;

#endif /* _XACESTR_H */

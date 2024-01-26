/************************************************************

Author: Eamon Walsh <ewalsh@tycho.nsa.gov>

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

#ifndef _XSELINUX_H
#define _XSELINUX_H

#include "dixaccess.h"

/* Extension info */
#define SELINUX_EXTENSION_NAME		"SELinux"
#define SELINUX_MAJOR_VERSION		1
#define SELINUX_MINOR_VERSION		1
#define SELinuxNumberEvents		0
#define SELinuxNumberErrors		0

/* Extension protocol */
#define X_SELinuxQueryVersion			0
#define X_SELinuxSetDeviceCreateContext		1
#define X_SELinuxGetDeviceCreateContext		2
#define X_SELinuxSetDeviceContext		3
#define X_SELinuxGetDeviceContext		4
#define X_SELinuxSetDrawableCreateContext	5
#define X_SELinuxGetDrawableCreateContext	6
#define X_SELinuxGetDrawableContext		7
#define X_SELinuxSetPropertyCreateContext	8
#define X_SELinuxGetPropertyCreateContext	9
#define X_SELinuxSetPropertyUseContext		10
#define X_SELinuxGetPropertyUseContext		11
#define X_SELinuxGetPropertyContext		12
#define X_SELinuxGetPropertyDataContext		13
#define X_SELinuxListProperties			14
#define X_SELinuxSetSelectionCreateContext	15
#define X_SELinuxGetSelectionCreateContext	16
#define X_SELinuxSetSelectionUseContext		17
#define X_SELinuxGetSelectionUseContext		18
#define X_SELinuxGetSelectionContext		19
#define X_SELinuxGetSelectionDataContext	20
#define X_SELinuxListSelections			21
#define X_SELinuxGetClientContext		22

typedef struct {
    CARD8   reqType;
    CARD8   SELinuxReqType;
    CARD16  length;
    CARD8   client_major;
    CARD8   client_minor;
} SELinuxQueryVersionReq;

typedef struct {
    CARD8   type;
    CARD8   pad1;
    CARD16  sequenceNumber;
    CARD32  length;
    CARD16  server_major;
    CARD16  server_minor;
    CARD32  pad2;
    CARD32  pad3;
    CARD32  pad4;
    CARD32  pad5;
    CARD32  pad6; 
} SELinuxQueryVersionReply;

typedef struct {
    CARD8   reqType;
    CARD8   SELinuxReqType;
    CARD16  length;
    CARD32  context_len;
} SELinuxSetCreateContextReq;

typedef struct {
    CARD8   reqType;
    CARD8   SELinuxReqType;
    CARD16  length;
} SELinuxGetCreateContextReq;

typedef struct {
    CARD8   reqType;
    CARD8   SELinuxReqType;
    CARD16  length;
    CARD32  id;
    CARD32  context_len;
} SELinuxSetContextReq;

typedef struct {
    CARD8   reqType;
    CARD8   SELinuxReqType;
    CARD16  length;
    CARD32  id;
} SELinuxGetContextReq;

typedef struct {
    CARD8   reqType;
    CARD8   SELinuxReqType;
    CARD16  length;
    CARD32  window;
    CARD32  property;
} SELinuxGetPropertyContextReq;

typedef struct {
    CARD8   type;
    CARD8   pad1;
    CARD16  sequenceNumber;
    CARD32  length;
    CARD32  context_len;
    CARD32  pad2;
    CARD32  pad3;
    CARD32  pad4;
    CARD32  pad5;
    CARD32  pad6;
} SELinuxGetContextReply;

typedef struct {
    CARD8   type;
    CARD8   pad1;
    CARD16  sequenceNumber;
    CARD32  length;
    CARD32  count;
    CARD32  pad2;
    CARD32  pad3;
    CARD32  pad4;
    CARD32  pad5;
    CARD32  pad6;
} SELinuxListItemsReply;


#ifdef _XSELINUX_NEED_FLASK
/* Private Flask definitions */
#define SECCLASS_X_DRAWABLE		1
#define SECCLASS_X_SCREEN		2
#define SECCLASS_X_GC			3
#define SECCLASS_X_FONT			4
#define SECCLASS_X_COLORMAP		5
#define SECCLASS_X_PROPERTY		6
#define SECCLASS_X_SELECTION		7
#define SECCLASS_X_CURSOR		8
#define SECCLASS_X_CLIENT		9
#define SECCLASS_X_POINTER		10
#define SECCLASS_X_KEYBOARD		11
#define SECCLASS_X_SERVER		12
#define SECCLASS_X_EXTENSION		13
#define SECCLASS_X_EVENT		14
#define SECCLASS_X_FAKEEVENT		15
#define SECCLASS_X_RESOURCE		16

/* Mapping from DixAccess bits to Flask permissions */
static struct security_class_mapping map[] = {
    { "x_drawable",
        { "read",		/* DixReadAccess */
          "write",		/* DixWriteAccess */
          "destroy",		/* DixDestroyAccess */
          "create",		/* DixCreateAccess */
          "getattr",		/* DixGetAttrAccess */
          "setattr",		/* DixSetAttrAccess */
          "list_property",	/* DixListPropAccess */
          "get_property",	/* DixGetPropAccess */
          "set_property",	/* DixSetPropAccess */
          "",			/* DixGetFocusAccess */
          "",			/* DixSetFocusAccess */
          "list_child",		/* DixListAccess */
          "add_child",		/* DixAddAccess */
          "remove_child",	/* DixRemoveAccess */
          "hide",		/* DixHideAccess */
          "show",		/* DixShowAccess */
          "blend",		/* DixBlendAccess */
          "override",		/* DixGrabAccess */
          "",			/* DixFreezeAccess */
          "",			/* DixForceAccess */
          "",			/* DixInstallAccess */
          "",			/* DixUninstallAccess */
          "send",		/* DixSendAccess */
          "receive",		/* DixReceiveAccess */
          "",			/* DixUseAccess */
          "manage",		/* DixManageAccess */
          NULL }},
    { "x_screen",
        { "",			/* DixReadAccess */
          "",			/* DixWriteAccess */
          "",			/* DixDestroyAccess */
          "",			/* DixCreateAccess */
          "getattr",		/* DixGetAttrAccess */
          "setattr",		/* DixSetAttrAccess */
          "saver_getattr",	/* DixListPropAccess */
          "saver_setattr",	/* DixGetPropAccess */
          "",			/* DixSetPropAccess */
          "",			/* DixGetFocusAccess */
          "",			/* DixSetFocusAccess */
          "",			/* DixListAccess */
          "",			/* DixAddAccess */
          "",			/* DixRemoveAccess */
          "hide_cursor",	/* DixHideAccess */
          "show_cursor",	/* DixShowAccess */
          "saver_hide",		/* DixBlendAccess */
          "saver_show",		/* DixGrabAccess */
          NULL }},
    { "x_gc",
        { "",			/* DixReadAccess */
          "",			/* DixWriteAccess */
          "destroy",		/* DixDestroyAccess */
          "create",		/* DixCreateAccess */
          "getattr",		/* DixGetAttrAccess */
          "setattr",		/* DixSetAttrAccess */
          "",			/* DixListPropAccess */
          "",			/* DixGetPropAccess */
          "",			/* DixSetPropAccess */
          "",			/* DixGetFocusAccess */
          "",			/* DixSetFocusAccess */
          "",			/* DixListAccess */
          "",			/* DixAddAccess */
          "",			/* DixRemoveAccess */
          "",			/* DixHideAccess */
          "",			/* DixShowAccess */
          "",			/* DixBlendAccess */
          "",			/* DixGrabAccess */
          "",			/* DixFreezeAccess */
          "",			/* DixForceAccess */
          "",			/* DixInstallAccess */
          "",			/* DixUninstallAccess */
          "",			/* DixSendAccess */
          "",			/* DixReceiveAccess */
          "use",		/* DixUseAccess */
          NULL }},
    { "x_font",
        { "",			/* DixReadAccess */
          "",			/* DixWriteAccess */
          "destroy",		/* DixDestroyAccess */
          "create",		/* DixCreateAccess */
          "getattr",		/* DixGetAttrAccess */
          "",			/* DixSetAttrAccess */
          "",			/* DixListPropAccess */
          "",			/* DixGetPropAccess */
          "",			/* DixSetPropAccess */
          "",			/* DixGetFocusAccess */
          "",			/* DixSetFocusAccess */
          "",			/* DixListAccess */
          "add_glyph",		/* DixAddAccess */
          "remove_glyph",	/* DixRemoveAccess */
          "",			/* DixHideAccess */
          "",			/* DixShowAccess */
          "",			/* DixBlendAccess */
          "",			/* DixGrabAccess */
          "",			/* DixFreezeAccess */
          "",			/* DixForceAccess */
          "",			/* DixInstallAccess */
          "",			/* DixUninstallAccess */
          "",			/* DixSendAccess */
          "",			/* DixReceiveAccess */
          "use",		/* DixUseAccess */
          NULL }},
    { "x_colormap",
        { "read",		/* DixReadAccess */
          "write",		/* DixWriteAccess */
          "destroy",		/* DixDestroyAccess */
          "create",		/* DixCreateAccess */
          "getattr",		/* DixGetAttrAccess */
          "",			/* DixSetAttrAccess */
          "",			/* DixListPropAccess */
          "",			/* DixGetPropAccess */
          "",			/* DixSetPropAccess */
          "",			/* DixGetFocusAccess */
          "",			/* DixSetFocusAccess */
          "",			/* DixListAccess */
          "add_color",		/* DixAddAccess */
          "remove_color",	/* DixRemoveAccess */
          "",			/* DixHideAccess */
          "",			/* DixShowAccess */
          "",			/* DixBlendAccess */
          "",			/* DixGrabAccess */
          "",			/* DixFreezeAccess */
          "",			/* DixForceAccess */
          "install",		/* DixInstallAccess */
          "uninstall",		/* DixUninstallAccess */
          "",			/* DixSendAccess */
          "",			/* DixReceiveAccess */
          "use",		/* DixUseAccess */
          NULL }},
    { "x_property",
        { "read",		/* DixReadAccess */
          "write",		/* DixWriteAccess */
          "destroy",		/* DixDestroyAccess */
          "create",		/* DixCreateAccess */
          "getattr",		/* DixGetAttrAccess */
          "setattr",		/* DixSetAttrAccess */
          "",			/* DixListPropAccess */
          "",			/* DixGetPropAccess */
          "",			/* DixSetPropAccess */
          "",			/* DixGetFocusAccess */
          "",			/* DixSetFocusAccess */
          "",			/* DixListAccess */
          "",			/* DixAddAccess */
          "",			/* DixRemoveAccess */
          "",			/* DixHideAccess */
          "",			/* DixShowAccess */
          "write",		/* DixBlendAccess */
          NULL }},
    { "x_selection",
        { "read",		/* DixReadAccess */
          "",			/* DixWriteAccess */
          "",			/* DixDestroyAccess */
          "setattr",		/* DixCreateAccess */
          "getattr",		/* DixGetAttrAccess */
          "setattr",		/* DixSetAttrAccess */
          NULL }},
    { "x_cursor",
        { "read",		/* DixReadAccess */
          "write",		/* DixWriteAccess */
          "destroy",		/* DixDestroyAccess */
          "create",		/* DixCreateAccess */
          "getattr",		/* DixGetAttrAccess */
          "setattr",		/* DixSetAttrAccess */
          "",			/* DixListPropAccess */
          "",			/* DixGetPropAccess */
          "",			/* DixSetPropAccess */
          "",			/* DixGetFocusAccess */
          "",			/* DixSetFocusAccess */
          "",			/* DixListAccess */
          "",			/* DixAddAccess */
          "",			/* DixRemoveAccess */
          "",			/* DixHideAccess */
          "",			/* DixShowAccess */
          "",			/* DixBlendAccess */
          "",			/* DixGrabAccess */
          "",			/* DixFreezeAccess */
          "",			/* DixForceAccess */
          "",			/* DixInstallAccess */
          "",			/* DixUninstallAccess */
          "",			/* DixSendAccess */
          "",			/* DixReceiveAccess */
          "use",		/* DixUseAccess */
          NULL }},
    { "x_client",
        { "",			/* DixReadAccess */
          "",			/* DixWriteAccess */
          "destroy",		/* DixDestroyAccess */
          "",			/* DixCreateAccess */
          "getattr",		/* DixGetAttrAccess */
          "setattr",		/* DixSetAttrAccess */
          "",			/* DixListPropAccess */
          "",			/* DixGetPropAccess */
          "",			/* DixSetPropAccess */
          "",			/* DixGetFocusAccess */
          "",			/* DixSetFocusAccess */
          "",			/* DixListAccess */
          "",			/* DixAddAccess */
          "",			/* DixRemoveAccess */
          "",			/* DixHideAccess */
          "",			/* DixShowAccess */
          "",			/* DixBlendAccess */
          "",			/* DixGrabAccess */
          "",			/* DixFreezeAccess */
          "",			/* DixForceAccess */
          "",			/* DixInstallAccess */
          "",			/* DixUninstallAccess */
          "",			/* DixSendAccess */
          "",			/* DixReceiveAccess */
          "",			/* DixUseAccess */
          "manage",		/* DixManageAccess */
          NULL }},
    { "x_pointer",
        { "read",		/* DixReadAccess */
          "write",		/* DixWriteAccess */
          "destroy",		/* DixDestroyAccess */
          "create",		/* DixCreateAccess */
          "getattr",		/* DixGetAttrAccess */
          "setattr",		/* DixSetAttrAccess */
          "list_property",	/* DixListPropAccess */
          "get_property",	/* DixGetPropAccess */
          "set_property",	/* DixSetPropAccess */
          "getfocus",		/* DixGetFocusAccess */
          "setfocus",		/* DixSetFocusAccess */
          "",			/* DixListAccess */
          "add",		/* DixAddAccess */
          "remove",		/* DixRemoveAccess */
          "",			/* DixHideAccess */
          "",			/* DixShowAccess */
          "",			/* DixBlendAccess */
          "grab",		/* DixGrabAccess */
          "freeze",		/* DixFreezeAccess */
          "force_cursor",	/* DixForceAccess */
          "",			/* DixInstallAccess */
          "",			/* DixUninstallAccess */
          "",			/* DixSendAccess */
          "",			/* DixReceiveAccess */
          "use",		/* DixUseAccess */
          "manage",		/* DixManageAccess */
          "",			/* DixDebugAccess */
          "bell",		/* DixBellAccess */
          NULL }},
    { "x_keyboard",
        { "read",		/* DixReadAccess */
          "write",		/* DixWriteAccess */
          "destroy",		/* DixDestroyAccess */
          "create",		/* DixCreateAccess */
          "getattr",		/* DixGetAttrAccess */
          "setattr",		/* DixSetAttrAccess */
          "list_property",	/* DixListPropAccess */
          "get_property",	/* DixGetPropAccess */
          "set_property",	/* DixSetPropAccess */
          "getfocus",		/* DixGetFocusAccess */
          "setfocus",		/* DixSetFocusAccess */
          "",			/* DixListAccess */
          "add",		/* DixAddAccess */
          "remove",		/* DixRemoveAccess */
          "",			/* DixHideAccess */
          "",			/* DixShowAccess */
          "",			/* DixBlendAccess */
          "grab",		/* DixGrabAccess */
          "freeze",		/* DixFreezeAccess */
          "force_cursor",	/* DixForceAccess */
          "",			/* DixInstallAccess */
          "",			/* DixUninstallAccess */
          "",			/* DixSendAccess */
          "",			/* DixReceiveAccess */
          "use",		/* DixUseAccess */
          "manage",		/* DixManageAccess */
          "",			/* DixDebugAccess */
          "bell",		/* DixBellAccess */
          NULL }},
    { "x_server",
        { "record",		/* DixReadAccess */
          "",			/* DixWriteAccess */
          "",			/* DixDestroyAccess */
          "",			/* DixCreateAccess */
          "getattr",		/* DixGetAttrAccess */
          "setattr",		/* DixSetAttrAccess */
          "",			/* DixListPropAccess */
          "",			/* DixGetPropAccess */
          "",			/* DixSetPropAccess */
          "",			/* DixGetFocusAccess */
          "",			/* DixSetFocusAccess */
          "",			/* DixListAccess */
          "",			/* DixAddAccess */
          "",			/* DixRemoveAccess */
          "",			/* DixHideAccess */
          "",			/* DixShowAccess */
          "",			/* DixBlendAccess */
          "grab",		/* DixGrabAccess */
          "",			/* DixFreezeAccess */
          "",			/* DixForceAccess */
          "",			/* DixInstallAccess */
          "",			/* DixUninstallAccess */
          "",			/* DixSendAccess */
          "",			/* DixReceiveAccess */
          "",			/* DixUseAccess */
          "manage",		/* DixManageAccess */
          "debug",		/* DixDebugAccess */
          NULL }},
    { "x_extension",
        { "",			/* DixReadAccess */
          "",			/* DixWriteAccess */
          "",			/* DixDestroyAccess */
          "",			/* DixCreateAccess */
          "query",		/* DixGetAttrAccess */
          "",			/* DixSetAttrAccess */
          "",			/* DixListPropAccess */
          "",			/* DixGetPropAccess */
          "",			/* DixSetPropAccess */
          "",			/* DixGetFocusAccess */
          "",			/* DixSetFocusAccess */
          "",			/* DixListAccess */
          "",			/* DixAddAccess */
          "",			/* DixRemoveAccess */
          "",			/* DixHideAccess */
          "",			/* DixShowAccess */
          "",			/* DixBlendAccess */
          "",			/* DixGrabAccess */
          "",			/* DixFreezeAccess */
          "",			/* DixForceAccess */
          "",			/* DixInstallAccess */
          "",			/* DixUninstallAccess */
          "",			/* DixSendAccess */
          "",			/* DixReceiveAccess */
          "use",		/* DixUseAccess */
          NULL }},
    { "x_event",
        { "",			/* DixReadAccess */
          "",			/* DixWriteAccess */
          "",			/* DixDestroyAccess */
          "",			/* DixCreateAccess */
          "",			/* DixGetAttrAccess */
          "",			/* DixSetAttrAccess */
          "",			/* DixListPropAccess */
          "",			/* DixGetPropAccess */
          "",			/* DixSetPropAccess */
          "",			/* DixGetFocusAccess */
          "",			/* DixSetFocusAccess */
          "",			/* DixListAccess */
          "",			/* DixAddAccess */
          "",			/* DixRemoveAccess */
          "",			/* DixHideAccess */
          "",			/* DixShowAccess */
          "",			/* DixBlendAccess */
          "",			/* DixGrabAccess */
          "",			/* DixFreezeAccess */
          "",			/* DixForceAccess */
          "",			/* DixInstallAccess */
          "",			/* DixUninstallAccess */
          "send",		/* DixSendAccess */
          "receive",		/* DixReceiveAccess */
          NULL }},
    { "x_synthetic_event",
        { "",			/* DixReadAccess */
          "",			/* DixWriteAccess */
          "",			/* DixDestroyAccess */
          "",			/* DixCreateAccess */
          "",			/* DixGetAttrAccess */
          "",			/* DixSetAttrAccess */
          "",			/* DixListPropAccess */
          "",			/* DixGetPropAccess */
          "",			/* DixSetPropAccess */
          "",			/* DixGetFocusAccess */
          "",			/* DixSetFocusAccess */
          "",			/* DixListAccess */
          "",			/* DixAddAccess */
          "",			/* DixRemoveAccess */
          "",			/* DixHideAccess */
          "",			/* DixShowAccess */
          "",			/* DixBlendAccess */
          "",			/* DixGrabAccess */
          "",			/* DixFreezeAccess */
          "",			/* DixForceAccess */
          "",			/* DixInstallAccess */
          "",			/* DixUninstallAccess */
          "send",		/* DixSendAccess */
          "receive",		/* DixReceiveAccess */
          NULL }},
    { "x_resource",
        { "read",		/* DixReadAccess */
          "write",		/* DixWriteAccess */
          "write",		/* DixDestroyAccess */
          "write",		/* DixCreateAccess */
          "read",		/* DixGetAttrAccess */
          "write",		/* DixSetAttrAccess */
          "read",		/* DixListPropAccess */
          "read",		/* DixGetPropAccess */
          "write",		/* DixSetPropAccess */
          "read",		/* DixGetFocusAccess */
          "write",		/* DixSetFocusAccess */
          "read",		/* DixListAccess */
          "write",		/* DixAddAccess */
          "write",		/* DixRemoveAccess */
          "write",		/* DixHideAccess */
          "read",		/* DixShowAccess */
          "read",		/* DixBlendAccess */
          "write",		/* DixGrabAccess */
          "write",		/* DixFreezeAccess */
          "write",		/* DixForceAccess */
          "write",		/* DixInstallAccess */
          "write",		/* DixUninstallAccess */
          "write",		/* DixSendAccess */
          "read",		/* DixReceiveAccess */
          "read",		/* DixUseAccess */
          "write",		/* DixManageAccess */
          "read",		/* DixDebugAccess */
          "write",		/* DixBellAccess */
          NULL }},
    { NULL }
};

/* x_resource "read" bits from the list above */
#define SELinuxReadMask (DixReadAccess|DixGetAttrAccess|DixListPropAccess| \
			 DixGetPropAccess|DixGetFocusAccess|DixListAccess| \
			 DixShowAccess|DixBlendAccess|DixReceiveAccess| \
			 DixUseAccess|DixDebugAccess)

#endif /* _XSELINUX_NEED_FLASK */
#endif /* _XSELINUX_H */

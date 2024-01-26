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

#ifndef _XACE_H
#define _XACE_H

#define XACE_EXTENSION_NAME		"XAccessControlExtension"
#define XACE_MAJOR_VERSION		1
#define XACE_MINOR_VERSION		0

#include "pixmap.h"     /* for DrawablePtr */
#include "regionstr.h"  /* for RegionPtr */

#define XaceNumberEvents		0
#define XaceNumberErrors		0

/* security hooks */
/* Constants used to identify the available security hooks
 */
#define XACE_CORE_DISPATCH		0
#define XACE_EXT_DISPATCH		1
#define XACE_RESOURCE_ACCESS		2
#define XACE_DEVICE_ACCESS		3
#define XACE_PROPERTY_ACCESS		4
#define XACE_DRAWABLE_ACCESS		5
#define XACE_MAP_ACCESS			6
#define XACE_BACKGRND_ACCESS		7
#define XACE_EXT_ACCESS			8
#define XACE_HOSTLIST_ACCESS		9
#define XACE_SITE_POLICY		10
#define XACE_DECLARE_EXT_SECURE		11
#define XACE_AUTH_AVAIL			12
#define XACE_KEY_AVAIL			13
#define XACE_WINDOW_INIT		14
#define XACE_AUDIT_BEGIN		15
#define XACE_AUDIT_END			16
#define XACE_NUM_HOOKS			17

extern CallbackListPtr XaceHooks[XACE_NUM_HOOKS];

/* Entry point for hook functions.  Called by Xserver.
 */
extern int XaceHook(
    int /*hook*/,
    ... /*appropriate args for hook*/
    ); 

/* Register a callback for a given hook.
 */
#define XaceRegisterCallback(hook,callback,data) \
    AddCallback(XaceHooks+(hook), callback, data)

/* Unregister an existing callback for a given hook.
 */
#define XaceDeleteCallback(hook,callback,data) \
    DeleteCallback(XaceHooks+(hook), callback, data)


/* From the original Security extension...
 */

/* Hook return codes */
#define SecurityAllowOperation  0
#define SecurityIgnoreOperation 1
#define SecurityErrorOperation  2

/* Proc vectors for untrusted clients, swapped and unswapped versions.
 * These are the same as the normal proc vectors except that extensions
 * that haven't declared themselves secure will have ProcBadRequest plugged
 * in for their major opcode dispatcher.  This prevents untrusted clients
 * from guessing extension major opcodes and using the extension even though
 * the extension can't be listed or queried.
 */
extern int (*UntrustedProcVector[256])(ClientPtr client);
extern int (*SwappedUntrustedProcVector[256])(ClientPtr client);

extern void XaceCensorImage(
    ClientPtr client,
    RegionPtr pVisibleRegion,
    long widthBytesLine,
    DrawablePtr pDraw,
    int x, int y, int w, int h,
    unsigned int format,
    char * pBuf
    );

#endif /* _XACE_H */

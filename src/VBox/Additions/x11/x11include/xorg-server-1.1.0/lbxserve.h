/* $Xorg: lbxserve.h,v 1.4 2001/02/09 02:05:17 xorgcvs Exp $ */
/*

Copyright 1996, 1998  The Open Group

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from The Open Group.

*/
/*
 * Copyright 1992 Network Computing Devices
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of NCD. not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  NCD. makes no representations about the
 * suitability of this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 *
 * NCD. DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL NCD.
 * BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */
/* $XFree86: xc/programs/Xserver/lbx/lbxserve.h,v 1.4 2001/08/01 00:44:58 tsi Exp $ */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#ifndef _LBXSERVE_H_

#include "colormap.h"
#include "property.h"

#define _LBXSERVE_H_
#define _XLBX_SERVER_
#include <X11/extensions/lbxstr.h>
#include <X11/extensions/lbxdeltastr.h>
#include <X11/extensions/lbxopts.h>

#define MAX_LBX_CLIENTS	MAXCLIENTS
#define	MAX_NUM_PROXIES	(MAXCLIENTS >> 1)

typedef struct _LbxClient *LbxClientPtr;
typedef struct _LbxProxy *LbxProxyPtr;

typedef struct _LbxClient {
    CARD32	id;
    ClientPtr   client;
    LbxProxyPtr proxy;
    Bool	ignored;
    Bool        input_blocked;
    int         reqs_pending;
    long	bytes_in_reply;
    long	bytes_remaining;
    Drawable	drawableCache[GFX_CACHE_SIZE];
    GContext	gcontextCache[GFX_CACHE_SIZE];
    pointer	gfx_buffer;	/* tmp buffer for unpacking gfx requests */
    unsigned long	gb_size;
}           LbxClientRec;

typedef struct _connectionOutput *OSBufPtr;

typedef struct _LbxProxy {
    LbxProxyPtr next;
    /* this array is indexed by lbx proxy index */
    LbxClientPtr lbxClients[MAX_LBX_CLIENTS];
    LbxClientPtr curRecv,
                curDix;
    int         fd;
    int         pid;		/* proxy ID */
    int		uid;
    int         numClients;
    int		maxIndex;
    Bool        aborted;
    int		grabClient;
    pointer	compHandle;
    Bool        dosquishing;
    Bool        useTags;
    LBXDeltasRec indeltas;
    LBXDeltasRec outdeltas;
    char	*iDeltaBuf;
    char	*replyBuf;
    char	*oDeltaBuf;
    OSBufPtr    ofirst;
    OSBufPtr    olast;
    CARD32      cur_send_id;

    LbxStreamOpts streamOpts;

    int		numBitmapCompMethods;
    unsigned char	*bitmapCompMethods;   /* array of indices */
    int		numPixmapCompMethods;
    unsigned char	*pixmapCompMethods;   /* array of indices */
    int		**pixmapCompDepths;   /* depths supported from each method */

    struct _ColormapRec *grabbedCmaps; /* chained via lbx private */
    int		motion_allowed_events;
    lbxMotionCache motionCache;
}           LbxProxyRec;

/* This array is indexed by server client index, not lbx proxy index */

extern LbxClientPtr lbxClients[MAXCLIENTS];

#define LbxClient(client)   (lbxClients[(client)->index])
#define LbxProxy(client)    (LbxClient(client)->proxy)
#define LbxMaybeProxy(client)	(LbxClient(client) ? LbxProxy(client) : 0)
#define	LbxProxyID(client)  (LbxProxy(client)->pid)
#define LbxProxyClient(proxy) ((proxy)->lbxClients[0]->client)

extern int LbxEventCode;


/* os/connection.c */
extern ClientPtr AllocLbxClientConnection ( ClientPtr client, 
					    LbxProxyPtr proxy );
extern void LbxProxyConnection ( ClientPtr client, LbxProxyPtr proxy );

/* os/libxio.c */
extern int UncompressedWriteToClient ( ClientPtr who, int count, char *buf );
extern void LbxForceOutput ( LbxProxyPtr proxy );
extern void SwitchClientInput ( ClientPtr client, Bool pending );
extern int PrepareLargeReqBuffer ( ClientPtr client );
extern Bool AppendFakeRequest ( ClientPtr client, char *data, int count );
extern void LbxFreeOsBuffers ( LbxProxyPtr proxy );
extern Bool AllocateLargeReqBuffer ( ClientPtr client, int size );
extern Bool AddToLargeReqBuffer ( ClientPtr client, char *data, int size );
extern void LbxPrimeInput ( ClientPtr client, LbxProxyPtr proxy );

/* lbxcmap.c */
extern int LbxCmapInit ( void );
extern Bool LbxCheckColorRequest ( ClientPtr client, ColormapPtr pmap, 
				   xReq *req );
extern int LbxCheckCmapGrabbed ( ColormapPtr pmap );
extern void LbxDisableSmartGrab ( ColormapPtr pmap );
extern void LbxBeginFreeCellsEvent ( ColormapPtr pmap );
extern void LbxAddFreeCellToEvent ( ColormapPtr pmap, Pixel pixel );
extern void LbxEndFreeCellsEvent ( ColormapPtr pmap );
extern void LbxSortPixelList ( Pixel *pixels, int count );
extern int ProcLbxGrabCmap ( ClientPtr client );
extern void LbxReleaseCmap ( ColormapPtr pmap, Bool smart );
extern int ProcLbxReleaseCmap ( ClientPtr client );
extern int ProcLbxAllocColor ( ClientPtr client );
extern int ProcLbxIncrementPixel ( ClientPtr client );

/* lbxdix.h */
extern void LbxDixInit ( void );
extern void LbxResetTags ( void );
extern int LbxSendConnSetup ( ClientPtr client, char *reason );
extern int LbxGetModifierMapping ( ClientPtr client );
extern int LbxGetKeyboardMapping ( ClientPtr client );
extern int LbxQueryFont ( ClientPtr client );
extern int LbxTagData ( ClientPtr client, XID tag, unsigned long len, 
			pointer data );
extern int LbxInvalidateTag ( ClientPtr client, XID tag );
extern void LbxAllowMotion ( ClientPtr client, int num );
extern void LbxFlushModifierMapTag ( void );
extern void LbxFlushKeyboardMapTag ( void );
extern void LbxFreeFontTag ( FontPtr pfont );
extern void LbxSendInvalidateTag ( ClientPtr client, XID tag, int tagtype );
extern Bool LbxFlushQTag ( XID tag );
extern void ProcessQTagZombies ( void );
extern void LbxQueryTagData ( ClientPtr client, int owner_pid, XID tag, 
			      int tagtype );

/* lbxexts.c */
extern Bool LbxAddExtension ( char *name, int opcode, int ev_base, 
			      int err_base );
extern Bool LbxAddExtensionAlias ( int idx, char *alias );
extern void LbxDeclareExtensionSecurity ( char *extname, Bool secure );
extern Bool LbxRegisterExtensionGenerationMasks ( int idx, int num_reqs, 
						  char *rep_mask, 
						  char *ev_mask );
extern int LbxQueryExtension ( ClientPtr client, char *ename, int nlen );
extern void LbxCloseDownExtensions ( void );
extern void LbxSetReqMask ( CARD8 *mask, int req, Bool on );

/* lbxgfx.c */
extern int LbxDecodePoly( ClientPtr client, CARD8 xreqtype,
			  int (*decode_rtn)(char *, char *, short *) );
extern int LbxDecodeFillPoly ( ClientPtr client );
extern int LbxDecodeCopyArea ( ClientPtr client );
extern int LbxDecodeCopyPlane ( ClientPtr client );
extern int LbxDecodePolyText ( ClientPtr client );
extern int LbxDecodeImageText ( ClientPtr client );
extern int LbxDecodePutImage ( ClientPtr client );
extern int LbxDecodeGetImage ( ClientPtr client );
extern int LbxDecodePoints ( char *in, char *inend, short *out );
extern int LbxDecodeSegment ( char *in, char *inend, short *out );
extern int LbxDecodeRectangle ( char *in, char *inend, short *out );
extern int LbxDecodeArc ( char *in, char *inend, short *out );

/* lbxmain.c */
extern LbxProxyPtr LbxPidToProxy ( int pid );
extern void LbxReencodeOutput ( ClientPtr client, char *pbuf, int *pcount,
				char *cbuf, int *ccount );
extern void LbxExtensionInit ( void );
extern void LbxCloseClient ( ClientPtr client );
extern void LbxSetForBlock ( LbxClientPtr lbxClient );
extern int ProcLbxDispatch ( ClientPtr client );
extern int ProcLbxSwitch ( ClientPtr client );
extern int ProcLbxQueryVersion ( ClientPtr client );
extern int ProcLbxStartProxy ( ClientPtr client );
extern int ProcLbxStopProxy ( ClientPtr client );
extern int ProcLbxBeginLargeRequest ( ClientPtr client );
extern int ProcLbxLargeRequestData ( ClientPtr client );
extern int ProcLbxEndLargeRequest ( ClientPtr client );
extern int ProcLbxInternAtoms ( ClientPtr client );
extern int ProcLbxGetWinAttrAndGeom ( ClientPtr client );
extern int ProcLbxNewClient ( ClientPtr client );
extern int ProcLbxEstablishConnection ( ClientPtr client );
extern int ProcLbxCloseClient ( ClientPtr client );
extern int ProcLbxModifySequence ( ClientPtr client );
extern int ProcLbxAllowMotion ( ClientPtr client );
extern int ProcLbxGetModifierMapping ( ClientPtr client );
extern int ProcLbxGetKeyboardMapping ( ClientPtr client );
extern int ProcLbxQueryFont ( ClientPtr client );
extern int ProcLbxChangeProperty ( ClientPtr client );
extern int ProcLbxGetProperty ( ClientPtr client );
extern int ProcLbxTagData ( ClientPtr client );
extern int ProcLbxInvalidateTag ( ClientPtr client );
extern int ProcLbxPolyPoint ( ClientPtr client );
extern int ProcLbxPolyLine ( ClientPtr client );
extern int ProcLbxPolySegment ( ClientPtr client );
extern int ProcLbxPolyRectangle ( ClientPtr client );
extern int ProcLbxPolyArc ( ClientPtr client );
extern int ProcLbxFillPoly ( ClientPtr client );
extern int ProcLbxPolyFillRectangle ( ClientPtr client );
extern int ProcLbxPolyFillArc ( ClientPtr client );
extern int ProcLbxCopyArea ( ClientPtr client );
extern int ProcLbxCopyPlane ( ClientPtr client );
extern int ProcLbxPolyText ( ClientPtr client );
extern int ProcLbxImageText ( ClientPtr client );
extern int ProcLbxQueryExtension ( ClientPtr client );
extern int ProcLbxPutImage ( ClientPtr client );
extern int ProcLbxGetImage ( ClientPtr client );
extern int ProcLbxSync ( ClientPtr client );

/* lbxprop.c */
extern int LbxChangeProperty ( ClientPtr client );
extern int LbxGetProperty ( ClientPtr client );
extern void LbxStallPropRequest ( ClientPtr client, PropertyPtr pProp );
extern int LbxChangeWindowProperty ( ClientPtr client, WindowPtr pWin, 
				     Atom property, Atom type, int format, 
				     int mode, unsigned long len, 
				     Bool have_data, pointer value, 
				     Bool sendevent, XID *tag );
/* lbxsquish.c */
extern int LbxSquishEvent ( char *buf );

/* lbwswap.c */
extern int SProcLbxDispatch( ClientPtr client );
extern int SProcLbxSwitch ( ClientPtr client );
extern int SProcLbxBeginLargeRequest ( ClientPtr client );
extern int SProcLbxLargeRequestData ( ClientPtr client );
extern int SProcLbxEndLargeRequest ( ClientPtr client );
extern void LbxWriteSConnSetupPrefix ( ClientPtr pClient, 
				       xLbxConnSetupPrefix *pcsp );
extern void LbxSwapFontInfo ( xLbxFontInfo *pr, Bool compressed );

/* lbxzerorep.c */
extern void ZeroReplyPadBytes ( char *buf, int reqType );

#endif				/* _LBXSERVE_H_ */

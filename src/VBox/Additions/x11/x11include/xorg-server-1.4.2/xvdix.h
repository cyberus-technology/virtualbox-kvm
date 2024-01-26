/***********************************************************
Copyright 1991 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its 
documentation for any purpose and without fee is hereby granted, 
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in 
supporting documentation, and that the names of Digital or MIT not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.  

DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

******************************************************************/

#ifndef XVDIX_H
#define XVDIX_H
/*
** File: 
**
**   xvdix.h --- Xv device independent header file
**
** Author: 
**
**   David Carver (Digital Workstation Engineering/Project Athena)
**
** Revisions:
**
**   29.08.91 Carver
**     - removed UnrealizeWindow wrapper unrealizing windows no longer 
**       preempts video
**
**   11.06.91 Carver
**     - changed SetPortControl to SetPortAttribute
**     - changed GetPortControl to GetPortAttribute
**     - changed QueryBestSize
**
**   15.05.91 Carver
**     - version 2.0 upgrade
**
**   24.01.91 Carver
**     - version 1.4 upgrade
**
*/

#include "scrnintstr.h"
#include <X11/extensions/Xvproto.h>

extern int  XvScreenIndex;
extern unsigned long XvExtensionGeneration;
extern unsigned long XvScreenGeneration;
extern unsigned long XvResourceGeneration;

extern int XvReqCode;
extern int XvEventBase;
extern int XvErrorBase;

extern unsigned long XvRTPort;
extern unsigned long XvRTEncoding;
extern unsigned long XvRTGrab;
extern unsigned long XvRTVideoNotify;
extern unsigned long XvRTVideoNotifyList;
extern unsigned long XvRTPortNotify;

typedef struct {
  int numerator;
  int denominator;
} XvRationalRec, *XvRationalPtr;

typedef struct {
  char depth;
  unsigned long visual;
} XvFormatRec, *XvFormatPtr;

typedef struct {
  unsigned long id;
  ClientPtr client;
} XvGrabRec, *XvGrabPtr;

typedef struct _XvVideoNotifyRec {
  struct _XvVideoNotifyRec *next;
  ClientPtr client;
  unsigned long id;
  unsigned long mask;
} XvVideoNotifyRec, *XvVideoNotifyPtr;

typedef struct _XvPortNotifyRec {
  struct _XvPortNotifyRec *next;
  ClientPtr client;
  unsigned long id;
} XvPortNotifyRec, *XvPortNotifyPtr;

typedef struct {
  int id;
  ScreenPtr pScreen;
  char *name;
  unsigned short width, height;
  XvRationalRec rate;
} XvEncodingRec, *XvEncodingPtr;

typedef struct _XvAttributeRec {
  int flags;
  int min_value;
  int max_value;
  char *name;
} XvAttributeRec, *XvAttributePtr;

typedef struct {
  int id;
  int type;
  int byte_order;
  char guid[16];
  int bits_per_pixel;
  int format;
  int num_planes;

  /* for RGB formats only */
  int depth;
  unsigned int red_mask;       
  unsigned int green_mask;   
  unsigned int blue_mask;   

  /* for YUV formats only */
  unsigned int y_sample_bits;
  unsigned int u_sample_bits;
  unsigned int v_sample_bits;   
  unsigned int horz_y_period;
  unsigned int horz_u_period;
  unsigned int horz_v_period;
  unsigned int vert_y_period;
  unsigned int vert_u_period;
  unsigned int vert_v_period;
  char component_order[32];
  int scanline_order;
} XvImageRec, *XvImagePtr; 

typedef struct {
  unsigned long base_id;
  unsigned char type; 
  char *name;
  int nEncodings;
  XvEncodingPtr pEncodings;  
  int nFormats;
  XvFormatPtr pFormats; 
  int nAttributes;
  XvAttributePtr pAttributes;
  int nImages;
  XvImagePtr pImages;
  int nPorts;
  struct _XvPortRec *pPorts;
  ScreenPtr pScreen; 
  int (* ddAllocatePort)(unsigned long, struct _XvPortRec*, 
				struct _XvPortRec**);
  int (* ddFreePort)(struct _XvPortRec*);
  int (* ddPutVideo)(ClientPtr, DrawablePtr,struct _XvPortRec*, GCPtr,
   				INT16, INT16, CARD16, CARD16, 
				INT16, INT16, CARD16, CARD16); 
  int (* ddPutStill)(ClientPtr, DrawablePtr,struct _XvPortRec*, GCPtr,
   				INT16, INT16, CARD16, CARD16, 
				INT16, INT16, CARD16, CARD16);
  int (* ddGetVideo)(ClientPtr, DrawablePtr,struct _XvPortRec*, GCPtr,
   				INT16, INT16, CARD16, CARD16, 
				INT16, INT16, CARD16, CARD16);
  int (* ddGetStill)(ClientPtr, DrawablePtr,struct _XvPortRec*, GCPtr,
   				INT16, INT16, CARD16, CARD16, 
				INT16, INT16, CARD16, CARD16);
  int (* ddStopVideo)(ClientPtr, struct _XvPortRec*, DrawablePtr);
  int (* ddSetPortAttribute)(ClientPtr, struct _XvPortRec*, Atom, INT32);
  int (* ddGetPortAttribute)(ClientPtr, struct _XvPortRec*, Atom, INT32*);
  int (* ddQueryBestSize)(ClientPtr, struct _XvPortRec*, CARD8,
   				CARD16, CARD16,CARD16, CARD16, 
				unsigned int*, unsigned int*);
  int (* ddPutImage)(ClientPtr, DrawablePtr, struct _XvPortRec*, GCPtr,
   				INT16, INT16, CARD16, CARD16, 
				INT16, INT16, CARD16, CARD16,
				XvImagePtr, unsigned char*, Bool,
				CARD16, CARD16);
  int (* ddQueryImageAttributes)(ClientPtr, struct _XvPortRec*, XvImagePtr, 
				CARD16*, CARD16*, int*, int*);
  DevUnion devPriv;
} XvAdaptorRec, *XvAdaptorPtr;

typedef struct _XvPortRec {
  unsigned long id;
  XvAdaptorPtr pAdaptor;
  XvPortNotifyPtr pNotify;
  DrawablePtr pDraw;
  ClientPtr client;
  XvGrabRec grab;
  TimeStamp time;
  DevUnion devPriv;
} XvPortRec, *XvPortPtr;

#define LOOKUP_PORT(_id, client)\
     ((XvPortPtr)LookupIDByType(_id, XvRTPort))

#define LOOKUP_ENCODING(_id, client)\
     ((XvEncodingPtr)LookupIDByType(_id, XvRTEncoding))

#define LOOKUP_VIDEONOTIFY_LIST(_id, client)\
     ((XvVideoNotifyPtr)LookupIDByType(_id, XvRTVideoNotifyList))

#define LOOKUP_PORTNOTIFY_LIST(_id, client)\
     ((XvPortNotifyPtr)LookupIDByType(_id, XvRTPortNotifyList))

typedef struct {
  int version, revision;
  int nAdaptors;
  XvAdaptorPtr pAdaptors;
  DestroyWindowProcPtr DestroyWindow;
  DestroyPixmapProcPtr DestroyPixmap;
  CloseScreenProcPtr CloseScreen;
  Bool (* ddCloseScreen)(int, ScreenPtr);
  int (* ddQueryAdaptors)(ScreenPtr, XvAdaptorPtr*, int*);
  DevUnion devPriv;
} XvScreenRec, *XvScreenPtr;

#define SCREEN_PROLOGUE(pScreen, field)\
  ((pScreen)->field = \
   ((XvScreenPtr) \
    (pScreen)->devPrivates[XvScreenIndex].ptr)->field)

#define SCREEN_EPILOGUE(pScreen, field, wrapper)\
    ((pScreen)->field = wrapper)

/* Errors */

#define _XvBadPort (XvBadPort+XvErrorBase)
#define _XvBadEncoding (XvBadEncoding+XvErrorBase)

extern int ProcXvDispatch(ClientPtr);
extern int SProcXvDispatch(ClientPtr);

extern void XvExtensionInit(void);
extern int XvScreenInit(ScreenPtr);
extern int XvGetScreenIndex(void);
extern unsigned long XvGetRTPort(void);
extern int XvdiSendPortNotify(XvPortPtr, Atom, INT32);
extern int XvdiVideoStopped(XvPortPtr, int);

extern int XvdiPutVideo(ClientPtr, DrawablePtr, XvPortPtr, GCPtr,
   				INT16, INT16, CARD16, CARD16, 
				INT16, INT16, CARD16, CARD16);
extern int XvdiPutStill(ClientPtr, DrawablePtr, XvPortPtr, GCPtr,
   				INT16, INT16, CARD16, CARD16, 
				INT16, INT16, CARD16, CARD16);
extern int XvdiGetVideo(ClientPtr, DrawablePtr, XvPortPtr, GCPtr,
   				INT16, INT16, CARD16, CARD16, 
				INT16, INT16, CARD16, CARD16);
extern int XvdiGetStill(ClientPtr, DrawablePtr, XvPortPtr, GCPtr,
   				INT16, INT16, CARD16, CARD16, 
				INT16, INT16, CARD16, CARD16);
extern int XvdiPutImage(ClientPtr, DrawablePtr, XvPortPtr, GCPtr,
   				INT16, INT16, CARD16, CARD16, 
				INT16, INT16, CARD16, CARD16,
				XvImagePtr, unsigned char*, Bool,
				CARD16, CARD16);
extern int XvdiSelectVideoNotify(ClientPtr, DrawablePtr, BOOL);
extern int XvdiSelectPortNotify(ClientPtr, XvPortPtr, BOOL);
extern int XvdiSetPortAttribute(ClientPtr, XvPortPtr, Atom, INT32);
extern int XvdiGetPortAttribute(ClientPtr, XvPortPtr, Atom, INT32*);
extern int XvdiStopVideo(ClientPtr, XvPortPtr, DrawablePtr);
extern int XvdiPreemptVideo(ClientPtr, XvPortPtr, DrawablePtr);
extern int XvdiMatchPort(XvPortPtr, DrawablePtr);
extern int XvdiGrabPort(ClientPtr, XvPortPtr, Time, int *);
extern int XvdiUngrabPort( ClientPtr, XvPortPtr, Time);


#if !defined(UNIXCPP)

#define XVCALL(name) Xv##name

#else

#define XVCALL(name) Xv/**/name

#endif


#endif /* XVDIX_H */


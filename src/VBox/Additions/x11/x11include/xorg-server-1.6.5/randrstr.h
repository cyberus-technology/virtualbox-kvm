/*
 * Copyright © 2000 Compaq Computer Corporation
 * Copyright © 2002 Hewlett-Packard Company
 * Copyright © 2006 Intel Corporation
 * Copyright © 2008 Red Hat, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that copyright
 * notice and this permission notice appear in supporting documentation, and
 * that the name of the copyright holders not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  The copyright holders make no representations
 * about the suitability of this software for any purpose.  It is provided "as
 * is" without express or implied warranty.
 *
 * THE COPYRIGHT HOLDERS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
 * OF THIS SOFTWARE.
 *
 * Author:  Jim Gettys, Hewlett-Packard Company, Inc.
 *	    Keith Packard, Intel Corporation
 */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#ifndef _RANDRSTR_H_
#define _RANDRSTR_H_

#include <X11/X.h>
#include <X11/Xproto.h>
#include "misc.h"
#include "os.h"
#include "dixstruct.h"
#include "resource.h"
#include "scrnintstr.h"
#include "windowstr.h"
#include "pixmapstr.h"
#include "extnsionst.h"
#include "servermd.h"
#include "rrtransform.h"
#include <X11/extensions/randr.h>
#include <X11/extensions/randrproto.h>
#ifdef RENDER
#include <X11/extensions/render.h> 	/* we share subpixel order information */
#include "picturestr.h"
#endif
#include <X11/Xfuncproto.h>

/* required for ABI compatibility for now */
#define RANDR_10_INTERFACE 1
#define RANDR_12_INTERFACE 1
#define RANDR_13_INTERFACE 1 /* requires RANDR_12_INTERFACE */
#define RANDR_GET_CRTC_INTERFACE 1

#define RANDR_INTERFACE_VERSION 0x0103

typedef XID	RRMode;
typedef XID	RROutput;
typedef XID	RRCrtc;

extern int	RREventBase, RRErrorBase;

extern int (*ProcRandrVector[RRNumberRequests])(ClientPtr);
extern int (*SProcRandrVector[RRNumberRequests])(ClientPtr);
    
/*
 * Modeline for a monitor. Name follows directly after this struct
 */

#define RRModeName(pMode) ((char *) (pMode + 1))
typedef struct _rrMode		RRModeRec, *RRModePtr;
typedef struct _rrPropertyValue	RRPropertyValueRec, *RRPropertyValuePtr;
typedef struct _rrProperty	RRPropertyRec, *RRPropertyPtr;
typedef struct _rrCrtc		RRCrtcRec, *RRCrtcPtr;
typedef struct _rrOutput	RROutputRec, *RROutputPtr;

struct _rrMode {
    int		    refcnt;
    xRRModeInfo	    mode;
    char	    *name;
    ScreenPtr	    userScreen;
};

struct _rrPropertyValue {
    Atom	    type;       /* ignored by server */
    short	    format;     /* format of data for swapping - 8,16,32 */
    long	    size;	/* size of data in (format/8) bytes */
    pointer         data;	/* private to client */
};

struct _rrProperty {
    RRPropertyPtr   next;
    ATOM 	    propertyName;
    Bool	    is_pending;
    Bool	    range;
    Bool	    immutable;
    int		    num_valid;
    INT32	    *valid_values;
    RRPropertyValueRec	current, pending;
};

struct _rrCrtc {
    RRCrtc	    id;
    ScreenPtr	    pScreen;
    RRModePtr	    mode;
    int		    x, y;
    Rotation	    rotation;
    Rotation	    rotations;
    Bool	    changed;
    int		    numOutputs;
    RROutputPtr	    *outputs;
    int		    gammaSize;
    CARD16	    *gammaRed;
    CARD16	    *gammaBlue;
    CARD16	    *gammaGreen;
    void	    *devPrivate;
    Bool	    transforms;
    RRTransformRec  client_pending_transform;
    RRTransformRec  client_current_transform;
    PictTransform   transform;
    struct pict_f_transform f_transform;
    struct pict_f_transform f_inverse;
};

struct _rrOutput {
    RROutput	    id;
    ScreenPtr	    pScreen;
    char	    *name;
    int		    nameLength;
    CARD8	    connection;
    CARD8	    subpixelOrder;
    int		    mmWidth;
    int		    mmHeight;
    RRCrtcPtr	    crtc;
    int		    numCrtcs;
    RRCrtcPtr	    *crtcs;
    int		    numClones;
    RROutputPtr	    *clones;
    int		    numModes;
    int		    numPreferred;
    RRModePtr	    *modes;
    int		    numUserModes;
    RRModePtr	    *userModes;
    Bool	    changed;
    RRPropertyPtr   properties;
    Bool	    pendingProperties;
    void	    *devPrivate;
};

#if RANDR_12_INTERFACE
typedef Bool (*RRScreenSetSizeProcPtr) (ScreenPtr	pScreen,
					CARD16		width,
					CARD16		height,
					CARD32		mmWidth,
					CARD32		mmHeight);
					
typedef Bool (*RRCrtcSetProcPtr) (ScreenPtr		pScreen,
				  RRCrtcPtr		crtc,
				  RRModePtr		mode,
				  int			x,
				  int			y,
				  Rotation		rotation,
				  int			numOutputs,
				  RROutputPtr		*outputs);

typedef Bool (*RRCrtcSetGammaProcPtr) (ScreenPtr	pScreen,
				       RRCrtcPtr	crtc);

typedef Bool (*RROutputSetPropertyProcPtr) (ScreenPtr		pScreen,
					    RROutputPtr		output,
					    Atom		property,
					    RRPropertyValuePtr	value);

typedef Bool (*RROutputValidateModeProcPtr) (ScreenPtr		pScreen,
					     RROutputPtr	output,
					     RRModePtr		mode);

typedef void (*RRModeDestroyProcPtr) (ScreenPtr	    pScreen,
				      RRModePtr	    mode);

#endif

#if RANDR_13_INTERFACE
typedef Bool (*RROutputGetPropertyProcPtr) (ScreenPtr		pScreen,
					    RROutputPtr		output,
					    Atom		property);
typedef Bool (*RRGetPanningProcPtr)    (ScreenPtr		pScrn,
					RRCrtcPtr		crtc,
					BoxPtr		totalArea,
					BoxPtr		trackingArea,
					INT16		*border);
typedef Bool (*RRSetPanningProcPtr)    (ScreenPtr		pScrn,
					RRCrtcPtr		crtc,
					BoxPtr		totalArea,
					BoxPtr		trackingArea,
					INT16		*border);

#endif /* RANDR_13_INTERFACE */

typedef Bool (*RRGetInfoProcPtr) (ScreenPtr pScreen, Rotation *rotations);
typedef Bool (*RRCloseScreenProcPtr) ( int i, ScreenPtr pscreen);

/* These are for 1.0 compatibility */
 
typedef struct _rrRefresh {
    CARD16	    rate;
    RRModePtr	    mode;
} RRScreenRate, *RRScreenRatePtr;

typedef struct _rrScreenSize {
    int		    id;
    short	    width, height;
    short	    mmWidth, mmHeight;
    int		    nRates;
    RRScreenRatePtr pRates;
} RRScreenSize, *RRScreenSizePtr;

#ifdef RANDR_10_INTERFACE

typedef Bool (*RRSetConfigProcPtr) (ScreenPtr		pScreen,
				    Rotation		rotation,
				    int			rate,
				    RRScreenSizePtr	pSize);

#endif
	

typedef struct _rrScrPriv {
    /*
     * 'public' part of the structure; DDXen fill this in
     * as they initialize
     */
#if RANDR_10_INTERFACE
    RRSetConfigProcPtr	    rrSetConfig;
#endif
    RRGetInfoProcPtr	    rrGetInfo;
#if RANDR_12_INTERFACE
    RRScreenSetSizeProcPtr  rrScreenSetSize;
    RRCrtcSetProcPtr	    rrCrtcSet;
    RRCrtcSetGammaProcPtr   rrCrtcSetGamma;
    RROutputSetPropertyProcPtr	rrOutputSetProperty;
    RROutputValidateModeProcPtr	rrOutputValidateMode;
    RRModeDestroyProcPtr	rrModeDestroy;
#endif
#if RANDR_13_INTERFACE
    RROutputGetPropertyProcPtr	rrOutputGetProperty;
    RRGetPanningProcPtr	rrGetPanning;
    RRSetPanningProcPtr	rrSetPanning;
#endif
    
    /*
     * Private part of the structure; not considered part of the ABI
     */
    TimeStamp		    lastSetTime;	/* last changed by client */
    TimeStamp		    lastConfigTime;	/* possible configs changed */
    RRCloseScreenProcPtr    CloseScreen;

    Bool		    changed;		/* some config changed */
    Bool		    configChanged;	/* configuration changed */
    Bool		    layoutChanged;	/* screen layout changed */

    CARD16		    minWidth, minHeight;
    CARD16		    maxWidth, maxHeight;
    CARD16		    width, height;	/* last known screen size */
    CARD16		    mmWidth, mmHeight;	/* last known screen size */

    int			    numOutputs;
    RROutputPtr		    *outputs;
    RROutputPtr		    primaryOutput;

    int			    numCrtcs;
    RRCrtcPtr		    *crtcs;

    /* Last known pointer position */
    RRCrtcPtr		    pointerCrtc;

#ifdef RANDR_10_INTERFACE
    /*
     * Configuration information
     */
    Rotation		    rotations;
    CARD16		    reqWidth, reqHeight;
    
    int			    nSizes;
    RRScreenSizePtr	    pSizes;
    
    Rotation		    rotation;
    int			    rate;
    int			    size;
#endif
} rrScrPrivRec, *rrScrPrivPtr;

extern DevPrivateKey rrPrivKey;

#define rrGetScrPriv(pScr)  ((rrScrPrivPtr)dixLookupPrivate(&(pScr)->devPrivates, rrPrivKey))
#define rrScrPriv(pScr)	rrScrPrivPtr    pScrPriv = rrGetScrPriv(pScr)
#define SetRRScreen(s,p) dixSetPrivate(&(s)->devPrivates, rrPrivKey, p)

/*
 * each window has a list of clients requesting
 * RRNotify events.  Each client has a resource
 * for each window it selects RRNotify input for,
 * this resource is used to delete the RRNotifyRec
 * entry from the per-window queue.
 */

typedef struct _RREvent *RREventPtr;

typedef struct _RREvent {
    RREventPtr  next;
    ClientPtr	client;
    WindowPtr	window;
    XID		clientResource;
    int		mask;
} RREventRec;

typedef struct _RRTimes {
    TimeStamp	setTime;
    TimeStamp	configTime;
} RRTimesRec, *RRTimesPtr;

typedef struct _RRClient {
    int		major_version;
    int		minor_version;
/*  RRTimesRec	times[0]; */
} RRClientRec, *RRClientPtr;

extern RESTYPE	RRClientType, RREventType; /* resource types for event masks */
extern DevPrivateKey RRClientPrivateKey;
extern RESTYPE	RRCrtcType, RRModeType, RROutputType;

#define LookupOutput(client,id,a) ((RROutputPtr) \
				   (SecurityLookupIDByType (client, id, \
							    RROutputType, a)))
#define LookupCrtc(client,id,a) ((RRCrtcPtr) \
				 (SecurityLookupIDByType (client, id, \
							  RRCrtcType, a)))
#define LookupMode(client,id,a) ((RRModePtr) \
				 (SecurityLookupIDByType (client, id, \
							  RRModeType, a)))

#define GetRRClient(pClient)    ((RRClientPtr)dixLookupPrivate(&(pClient)->devPrivates, RRClientPrivateKey))
#define rrClientPriv(pClient)	RRClientPtr pRRClient = GetRRClient(pClient)

/* Initialize the extension */
void
RRExtensionInit (void);

#ifdef RANDR_12_INTERFACE
/*
 * Set the range of sizes for the screen
 */
void
RRScreenSetSizeRange (ScreenPtr	pScreen,
		      CARD16	minWidth,
		      CARD16	minHeight,
		      CARD16	maxWidth,
		      CARD16	maxHeight);
#endif

/* rrscreen.c */
/*
 * Notify the extension that the screen size has been changed.
 * The driver is responsible for calling this whenever it has changed
 * the size of the screen
 */
void
RRScreenSizeNotify (ScreenPtr	pScreen);

/*
 * Request that the screen be resized
 */
Bool
RRScreenSizeSet (ScreenPtr  pScreen,
		 CARD16	    width,
		 CARD16	    height,
		 CARD32	    mmWidth,
		 CARD32	    mmHeight);

/*
 * Send ConfigureNotify event to root window when 'something' happens
 */
void
RRSendConfigNotify (ScreenPtr pScreen);
    
/*
 * screen dispatch
 */
int 
ProcRRGetScreenSizeRange (ClientPtr client);

int
ProcRRSetScreenSize (ClientPtr client);

int
ProcRRGetScreenResources (ClientPtr client);

int
ProcRRGetScreenResourcesCurrent (ClientPtr client);

int
ProcRRSetScreenConfig (ClientPtr client);

int
ProcRRGetScreenInfo (ClientPtr client);

/*
 * Deliver a ScreenNotify event
 */
void
RRDeliverScreenEvent (ClientPtr client, WindowPtr pWin, ScreenPtr pScreen);
    
/* mirandr.c */
Bool
miRandRInit (ScreenPtr pScreen);

Bool
miRRGetInfo (ScreenPtr pScreen, Rotation *rotations);

Bool
miRRGetScreenInfo (ScreenPtr pScreen);

Bool
miRRCrtcSet (ScreenPtr	pScreen,
	     RRCrtcPtr	crtc,
	     RRModePtr	mode,
	     int	x,
	     int	y,
	     Rotation	rotation,
	     int	numOutput,
	     RROutputPtr *outputs);

Bool
miRROutputSetProperty (ScreenPtr	    pScreen,
		       RROutputPtr	    output,
		       Atom		    property,
		       RRPropertyValuePtr   value);

Bool
miRROutputGetProperty (ScreenPtr	    pScreen,
		       RROutputPtr	    output,
		       Atom		    property);

Bool
miRROutputValidateMode (ScreenPtr	    pScreen,
			RROutputPtr	    output,
			RRModePtr	    mode);

void
miRRModeDestroy (ScreenPtr  pScreen,
		 RRModePtr  mode);

/* randr.c */
/*
 * Send all pending events
 */
void
RRTellChanged (ScreenPtr pScreen);

/*
 * Poll the driver for changed information
 */
Bool
RRGetInfo (ScreenPtr pScreen, Bool force_query);

Bool RRInit (void);

Bool RRScreenInit(ScreenPtr pScreen);

RROutputPtr
RRFirstOutput (ScreenPtr pScreen);

Rotation
RRGetRotation (ScreenPtr pScreen);

CARD16
RRVerticalRefresh (xRRModeInfo *mode);

#ifdef RANDR_10_INTERFACE					
/*
 * This is the old interface, deprecated but left
 * around for compatibility
 */

/*
 * Then, register the specific size with the screen
 */

RRScreenSizePtr
RRRegisterSize (ScreenPtr		pScreen,
		short			width, 
		short			height,
		short			mmWidth,
		short			mmHeight);

Bool RRRegisterRate (ScreenPtr		pScreen,
		     RRScreenSizePtr	pSize,
		     int		rate);

/*
 * Finally, set the current configuration of the screen
 */

void
RRSetCurrentConfig (ScreenPtr		pScreen,
		    Rotation		rotation,
		    int			rate,
		    RRScreenSizePtr	pSize);

Bool RRScreenInit (ScreenPtr pScreen);

Rotation
RRGetRotation (ScreenPtr pScreen);

int
RRSetScreenConfig (ScreenPtr		pScreen,
		   Rotation		rotation,
		   int			rate,
		   RRScreenSizePtr	pSize);

#endif					

/* rrcrtc.c */

/*
 * Notify the CRTC of some change; layoutChanged indicates that
 * some position or size element changed
 */
void
RRCrtcChanged (RRCrtcPtr crtc, Bool layoutChanged);

/*
 * Create a CRTC
 */
RRCrtcPtr
RRCrtcCreate (ScreenPtr pScreen, void	*devPrivate);

/*
 * Set the allowed rotations on a CRTC
 */
void
RRCrtcSetRotations (RRCrtcPtr crtc, Rotation rotations);

/*
 * Set whether transforms are allowed on a CRTC
 */
void
RRCrtcSetTransformSupport (RRCrtcPtr crtc, Bool transforms);

/*
 * Notify the extension that the Crtc has been reconfigured,
 * the driver calls this whenever it has updated the mode
 */
Bool
RRCrtcNotify (RRCrtcPtr	    crtc,
	      RRModePtr	    mode,
	      int	    x,
	      int	    y,
	      Rotation	    rotation,
	      RRTransformPtr transform,
	      int	    numOutputs,
	      RROutputPtr   *outputs);

void
RRDeliverCrtcEvent (ClientPtr client, WindowPtr pWin, RRCrtcPtr crtc);
    
/*
 * Request that the Crtc be reconfigured
 */
Bool
RRCrtcSet (RRCrtcPtr    crtc,
	   RRModePtr	mode,
	   int		x,
	   int		y,
	   Rotation	rotation,
	   int		numOutput,
	   RROutputPtr  *outputs);

/*
 * Request that the Crtc gamma be changed
 */

Bool
RRCrtcGammaSet (RRCrtcPtr   crtc,
		CARD16	    *red,
		CARD16	    *green,
		CARD16	    *blue);

/*
 * Notify the extension that the Crtc gamma has been changed
 * The driver calls this whenever it has changed the gamma values
 * in the RRCrtcRec
 */

Bool
RRCrtcGammaNotify (RRCrtcPtr	crtc);

/*
 * Set the size of the gamma table at server startup time
 */

Bool
RRCrtcGammaSetSize (RRCrtcPtr	crtc,
		    int		size);

/*
 * Return the area of the frame buffer scanned out by the crtc,
 * taking into account the current mode and rotation
 */

void
RRCrtcGetScanoutSize(RRCrtcPtr crtc, int *width, int *height);

/*
 * Compute the complete transformation matrix including
 * client-specified transform, rotation/reflection values and the crtc 
 * offset.
 *
 * Return TRUE if the resulting transform is not a simple translation.
 */
Bool
RRTransformCompute (int			    x,
		    int			    y,
		    int			    width,
		    int			    height,
		    Rotation		    rotation,
		    RRTransformPtr	    rr_transform,

		    PictTransformPtr	    transform,
		    struct pict_f_transform *f_transform,
		    struct pict_f_transform *f_inverse);

/*
 * Return crtc transform
 */
RRTransformPtr
RRCrtcGetTransform (RRCrtcPtr crtc);

/*
 * Check whether the pending and current transforms are the same
 */
Bool
RRCrtcPendingTransform (RRCrtcPtr crtc);

/*
 * Destroy a Crtc at shutdown
 */
void
RRCrtcDestroy (RRCrtcPtr crtc);


/*
 * Set the pending CRTC transformation
 */

int
RRCrtcTransformSet (RRCrtcPtr		crtc,
		    PictTransformPtr	transform,
		    struct pict_f_transform *f_transform,
		    struct pict_f_transform *f_inverse,
		    char		*filter,
		    int			filter_len,
		    xFixed		*params,
		    int			nparams);

/*
 * Initialize crtc type
 */
Bool
RRCrtcInit (void);

/*
 * Crtc dispatch
 */

int
ProcRRGetCrtcInfo (ClientPtr client);

int
ProcRRSetCrtcConfig (ClientPtr client);

int
ProcRRGetCrtcGammaSize (ClientPtr client);

int
ProcRRGetCrtcGamma (ClientPtr client);

int
ProcRRSetCrtcGamma (ClientPtr client);

int
ProcRRSetCrtcTransform (ClientPtr client);

int
ProcRRGetCrtcTransform (ClientPtr client);

int
ProcRRGetPanning (ClientPtr client);

int
ProcRRSetPanning (ClientPtr client);

/* rrdispatch.c */
Bool
RRClientKnowsRates (ClientPtr	pClient);

/* rrmode.c */
/*
 * Find, and if necessary, create a mode
 */

RRModePtr
RRModeGet (xRRModeInfo	*modeInfo,
	   const char	*name);

void
RRModePruneUnused (ScreenPtr pScreen);

/*
 * Destroy a mode.
 */

void
RRModeDestroy (RRModePtr mode);

/*
 * Return a list of modes that are valid for some output in pScreen
 */
RRModePtr *
RRModesForScreen (ScreenPtr pScreen, int *num_ret);
    
/*
 * Initialize mode type
 */
Bool
RRModeInit (void);
    
int
ProcRRCreateMode (ClientPtr client);

int
ProcRRDestroyMode (ClientPtr client);

int
ProcRRAddOutputMode (ClientPtr client);

int
ProcRRDeleteOutputMode (ClientPtr client);

/* rroutput.c */

/*
 * Notify the output of some change. configChanged indicates whether
 * any external configuration (mode list, clones, connected status)
 * has changed, or whether the change was strictly internal
 * (which crtc is in use)
 */
void
RROutputChanged (RROutputPtr output, Bool configChanged);

/*
 * Create an output
 */

RROutputPtr
RROutputCreate (ScreenPtr   pScreen,
		const char  *name,
		int	    nameLength,
		void	    *devPrivate);

/*
 * Notify extension that output parameters have been changed
 */
Bool
RROutputSetClones (RROutputPtr  output,
		   RROutputPtr  *clones,
		   int		numClones);

Bool
RROutputSetModes (RROutputPtr	output,
		  RRModePtr	*modes,
		  int		numModes,
		  int		numPreferred);

int
RROutputAddUserMode (RROutputPtr    output,
		     RRModePtr	    mode);

int
RROutputDeleteUserMode (RROutputPtr output,
			RRModePtr   mode);

Bool
RROutputSetCrtcs (RROutputPtr	output,
		  RRCrtcPtr	*crtcs,
		  int		numCrtcs);

Bool
RROutputSetConnection (RROutputPtr  output,
		       CARD8	    connection);

Bool
RROutputSetSubpixelOrder (RROutputPtr output,
			  int	      subpixelOrder);

Bool
RROutputSetPhysicalSize (RROutputPtr	output,
			 int		mmWidth,
			 int		mmHeight);

void
RRDeliverOutputEvent(ClientPtr client, WindowPtr pWin, RROutputPtr output);

void
RROutputDestroy (RROutputPtr	output);

int
ProcRRGetOutputInfo (ClientPtr client);

extern int
ProcRRSetOutputPrimary (ClientPtr client);

extern int
ProcRRGetOutputPrimary (ClientPtr client);

/*
 * Initialize output type
 */
Bool
RROutputInit (void);
    
/* rrpointer.c */
void
RRPointerMoved (ScreenPtr pScreen, int x, int y);

void
RRPointerScreenConfigured (ScreenPtr pScreen);

/* rrproperty.c */

void
RRDeleteAllOutputProperties (RROutputPtr output);

RRPropertyValuePtr
RRGetOutputProperty (RROutputPtr output, Atom property, Bool pending);

RRPropertyPtr
RRQueryOutputProperty (RROutputPtr output, Atom property);
		       
void
RRDeleteOutputProperty (RROutputPtr output, Atom property);

Bool
RRPostPendingProperties (RROutputPtr output);
    
int
RRChangeOutputProperty (RROutputPtr output, Atom property, Atom type,
			int format, int mode, unsigned long len,
			pointer value, Bool sendevent, Bool pending);

int
RRConfigureOutputProperty (RROutputPtr output, Atom property,
			   Bool pending, Bool range, Bool immutable,
			   int num_values, INT32 *values);
int
ProcRRChangeOutputProperty (ClientPtr client);

int
ProcRRGetOutputProperty (ClientPtr client);

int
ProcRRListOutputProperties (ClientPtr client);

int
ProcRRQueryOutputProperty (ClientPtr client);

int
ProcRRConfigureOutputProperty (ClientPtr client);

int
ProcRRDeleteOutputProperty (ClientPtr client);

/* rrxinerama.c */
void
RRXineramaExtensionInit(void);

#endif /* _RANDRSTR_H_ */

/*
 
randr extension implementation structure

Query state:
    ProcRRGetScreenInfo/ProcRRGetScreenResources
	RRGetInfo
 
	    • Request configuration from driver, either 1.0 or 1.2 style
	    • These functions only record state changes, all
	      other actions are pended until RRTellChanged is called
 
	    ->rrGetInfo
	    1.0:
		RRRegisterSize
		RRRegisterRate
		RRSetCurrentConfig
	    1.2:
		RRScreenSetSizeRange
		RROutputSetCrtcs
		RRModeGet
		RROutputSetModes
		RROutputSetConnection
		RROutputSetSubpixelOrder
		RROutputSetClones
		RRCrtcNotify
 
	• Must delay scanning configuration until after ->rrGetInfo returns
	  because some drivers will call SetCurrentConfig in the middle
	  of the ->rrGetInfo operation.
 
	1.0:

	    • Scan old configuration, mirror to new structures
 
	    RRScanOldConfig
		RRCrtcCreate
		RROutputCreate
		RROutputSetCrtcs
		RROutputSetConnection
		RROutputSetSubpixelOrder
		RROldModeAdd	• This adds modes one-at-a-time
		    RRModeGet
		RRCrtcNotify
 
	• send events, reset pointer if necessary
 
	RRTellChanged
	    WalkTree (sending events)
 
	    • when layout has changed:
		RRPointerScreenConfigured
		RRSendConfigNotify
 
Asynchronous state setting (1.2 only)
    When setting state asynchronously, the driver invokes the
    ->rrGetInfo function and then calls RRTellChanged to flush
    the changes to the clients and reset pointer if necessary

Set state

    ProcRRSetScreenConfig
	RRCrtcSet
	    1.2:
		->rrCrtcSet
		    RRCrtcNotify
	    1.0:
		->rrSetConfig
		RRCrtcNotify
	    RRTellChanged
 */

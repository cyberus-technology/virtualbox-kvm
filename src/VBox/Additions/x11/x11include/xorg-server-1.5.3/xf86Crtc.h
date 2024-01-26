/*
 * Copyright © 2006 Keith Packard
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
 */
#ifndef _XF86CRTC_H_
#define _XF86CRTC_H_

#include <edid.h>
#include "randrstr.h"
#if XF86_MODES_RENAME
#include "xf86Rename.h"
#endif
#include "xf86Modes.h"
#include "xf86Cursor.h"
#include "xf86i2c.h"
#include "damage.h"
#include "picturestr.h"

/* Compat definitions for older X Servers. */
#ifndef M_T_PREFERRED
#define M_T_PREFERRED	0x08
#endif
#ifndef M_T_DRIVER
#define M_T_DRIVER	0x40
#endif
#ifndef M_T_USERPREF
#define M_T_USERPREF	0x80
#endif
#ifndef HARDWARE_CURSOR_ARGB
#define HARDWARE_CURSOR_ARGB				0x00004000
#endif

typedef struct _xf86Crtc xf86CrtcRec, *xf86CrtcPtr;
typedef struct _xf86Output xf86OutputRec, *xf86OutputPtr;

/* define a standard for connector types */
typedef enum _xf86ConnectorType {
   XF86ConnectorNone,
   XF86ConnectorVGA,
   XF86ConnectorDVI_I,
   XF86ConnectorDVI_D,
   XF86ConnectorDVI_A,
   XF86ConnectorComposite,
   XF86ConnectorSvideo,
   XF86ConnectorComponent,
   XF86ConnectorLFP,
   XF86ConnectorProprietary,
   XF86ConnectorHDMI,
   XF86ConnectorDisplayPort,
} xf86ConnectorType;

typedef enum _xf86OutputStatus {
   XF86OutputStatusConnected,
   XF86OutputStatusDisconnected,
   XF86OutputStatusUnknown
} xf86OutputStatus;

typedef struct _xf86CrtcFuncs {
   /**
    * Turns the crtc on/off, or sets intermediate power levels if available.
    *
    * Unsupported intermediate modes drop to the lower power setting.  If the
    * mode is DPMSModeOff, the crtc must be disabled sufficiently for it to
    * be safe to call mode_set.
    */
   void
    (*dpms)(xf86CrtcPtr		crtc,
	    int		    	mode);

   /**
    * Saves the crtc's state for restoration on VT switch.
    */
   void
    (*save)(xf86CrtcPtr		crtc);

   /**
    * Restore's the crtc's state at VT switch.
    */
   void
    (*restore)(xf86CrtcPtr	crtc);

    /**
     * Lock CRTC prior to mode setting, mostly for DRI.
     * Returns whether unlock is needed
     */
    Bool
    (*lock) (xf86CrtcPtr crtc);
    
    /**
     * Unlock CRTC after mode setting, mostly for DRI
     */
    void
    (*unlock) (xf86CrtcPtr crtc);
    
    /**
     * Callback to adjust the mode to be set in the CRTC.
     *
     * This allows a CRTC to adjust the clock or even the entire set of
     * timings, which is used for panels with fixed timings or for
     * buses with clock limitations.
     */
    Bool
    (*mode_fixup)(xf86CrtcPtr crtc,
		  DisplayModePtr mode,
		  DisplayModePtr adjusted_mode);

    /**
     * Prepare CRTC for an upcoming mode set.
     */
    void
    (*prepare)(xf86CrtcPtr crtc);

    /**
     * Callback for setting up a video mode after fixups have been made.
     */
    void
    (*mode_set)(xf86CrtcPtr crtc,
		DisplayModePtr mode,
		DisplayModePtr adjusted_mode,
		int x, int y);

    /**
     * Commit mode changes to a CRTC
     */
    void
    (*commit)(xf86CrtcPtr crtc);

    /* Set the color ramps for the CRTC to the given values. */
    void
    (*gamma_set)(xf86CrtcPtr crtc, CARD16 *red, CARD16 *green, CARD16 *blue,
		 int size);

    /**
     * Allocate the shadow area, delay the pixmap creation until needed
     */
    void *
    (*shadow_allocate) (xf86CrtcPtr crtc, int width, int height);
    
    /**
     * Create shadow pixmap for rotation support
     */
    PixmapPtr
    (*shadow_create) (xf86CrtcPtr crtc, void *data, int width, int height);
    
    /**
     * Destroy shadow pixmap
     */
    void
    (*shadow_destroy) (xf86CrtcPtr crtc, PixmapPtr pPixmap, void *data);

    /**
     * Set cursor colors
     */
    void
    (*set_cursor_colors) (xf86CrtcPtr crtc, int bg, int fg);

    /**
     * Set cursor position
     */
    void
    (*set_cursor_position) (xf86CrtcPtr crtc, int x, int y);

    /**
     * Show cursor
     */
    void
    (*show_cursor) (xf86CrtcPtr crtc);

    /**
     * Hide cursor
     */
    void
    (*hide_cursor) (xf86CrtcPtr crtc);

    /**
     * Load monochrome image
     */
    void
    (*load_cursor_image) (xf86CrtcPtr crtc, CARD8 *image);

    /**
     * Load ARGB image
     */
     void
     (*load_cursor_argb) (xf86CrtcPtr crtc, CARD32 *image);
     
    /**
     * Clean up driver-specific bits of the crtc
     */
    void
    (*destroy) (xf86CrtcPtr	crtc);

    /**
     * Less fine-grained mode setting entry point for kernel modesetting
     */
    Bool
    (*set_mode_major)(xf86CrtcPtr crtc, DisplayModePtr mode,
		      Rotation rotation, int x, int y);
} xf86CrtcFuncsRec, *xf86CrtcFuncsPtr;

struct _xf86Crtc {
    /**
     * Associated ScrnInfo
     */
    ScrnInfoPtr	    scrn;
    
    /**
     * Active state of this CRTC
     *
     * Set when this CRTC is driving one or more outputs 
     */
    Bool	    enabled;
    
    /**
     * Active mode
     *
     * This reflects the mode as set in the CRTC currently
     * It will be cleared when the VT is not active or
     * during server startup
     */
    DisplayModeRec  mode;
    Rotation	    rotation;
    PixmapPtr	    rotatedPixmap;
    void	    *rotatedData;
    
    /**
     * Position on screen
     *
     * Locates this CRTC within the frame buffer
     */
    int		    x, y;
    
    /**
     * Desired mode
     *
     * This is set to the requested mode, independent of
     * whether the VT is active. In particular, it receives
     * the startup configured mode and saves the active mode
     * on VT switch.
     */
    DisplayModeRec  desiredMode;
    Rotation	    desiredRotation;
    int		    desiredX, desiredY;
    
    /** crtc-specific functions */
    const xf86CrtcFuncsRec *funcs;

    /**
     * Driver private
     *
     * Holds driver-private information
     */
    void	    *driver_private;

#ifdef RANDR_12_INTERFACE
    /**
     * RandR crtc
     *
     * When RandR 1.2 is available, this
     * points at the associated crtc object
     */
    RRCrtcPtr	    randr_crtc;
#else
    void	    *randr_crtc;
#endif

    /**
     * Current cursor is ARGB
     */
    Bool	    cursor_argb;
    /**
     * Track whether cursor is within CRTC range 
     */
    Bool	    cursor_in_range;
    /**
     * Track state of cursor associated with this CRTC
     */
    Bool	    cursor_shown;

    /**
     * Current transformation matrix
     */
    PictTransform   crtc_to_framebuffer;
    PictTransform   framebuffer_to_crtc;
    Bool	    transform_in_use;
    /**
     * Bounding box in screen space
     */
    BoxRec	    bounds;
};

typedef struct _xf86OutputFuncs {
    /**
     * Called to allow the output a chance to create properties after the
     * RandR objects have been created.
     */
    void
    (*create_resources)(xf86OutputPtr output);

    /**
     * Turns the output on/off, or sets intermediate power levels if available.
     *
     * Unsupported intermediate modes drop to the lower power setting.  If the
     * mode is DPMSModeOff, the output must be disabled, as the DPLL may be
     * disabled afterwards.
     */
    void
    (*dpms)(xf86OutputPtr	output,
	    int			mode);

    /**
     * Saves the output's state for restoration on VT switch.
     */
    void
    (*save)(xf86OutputPtr	output);

    /**
     * Restore's the output's state at VT switch.
     */
    void
    (*restore)(xf86OutputPtr	output);

    /**
     * Callback for testing a video mode for a given output.
     *
     * This function should only check for cases where a mode can't be supported
     * on the output specifically, and not represent generic CRTC limitations.
     *
     * \return MODE_OK if the mode is valid, or another MODE_* otherwise.
     */
    int
    (*mode_valid)(xf86OutputPtr	    output,
		  DisplayModePtr    pMode);

    /**
     * Callback to adjust the mode to be set in the CRTC.
     *
     * This allows an output to adjust the clock or even the entire set of
     * timings, which is used for panels with fixed timings or for
     * buses with clock limitations.
     */
    Bool
    (*mode_fixup)(xf86OutputPtr output,
		  DisplayModePtr mode,
		  DisplayModePtr adjusted_mode);

    /**
     * Callback for preparing mode changes on an output
     */
    void
    (*prepare)(xf86OutputPtr output);

    /**
     * Callback for committing mode changes on an output
     */
    void
    (*commit)(xf86OutputPtr output);

    /**
     * Callback for setting up a video mode after fixups have been made.
     *
     * This is only called while the output is disabled.  The dpms callback
     * must be all that's necessary for the output, to turn the output on
     * after this function is called.
     */
    void
    (*mode_set)(xf86OutputPtr  output,
		DisplayModePtr mode,
		DisplayModePtr adjusted_mode);

    /**
     * Probe for a connected output, and return detect_status.
     */
    xf86OutputStatus
    (*detect)(xf86OutputPtr	    output);

    /**
     * Query the device for the modes it provides.
     *
     * This function may also update MonInfo, mm_width, and mm_height.
     *
     * \return singly-linked list of modes or NULL if no modes found.
     */
    DisplayModePtr
    (*get_modes)(xf86OutputPtr	    output);

#ifdef RANDR_12_INTERFACE
    /**
     * Callback when an output's property has changed.
     */
    Bool
    (*set_property)(xf86OutputPtr output,
		    Atom property,
		    RRPropertyValuePtr value);
#endif
    /**
     * Clean up driver-specific bits of the output
     */
    void
    (*destroy) (xf86OutputPtr	    output);
} xf86OutputFuncsRec, *xf86OutputFuncsPtr;

struct _xf86Output {
    /**
     * Associated ScrnInfo
     */
    ScrnInfoPtr		scrn;

    /**
     * Currently connected crtc (if any)
     *
     * If this output is not in use, this field will be NULL.
     */
    xf86CrtcPtr		crtc;

    /**
     * Possible CRTCs for this output as a mask of crtc indices
     */
    CARD32		possible_crtcs;

    /**
     * Possible outputs to share the same CRTC as a mask of output indices
     */
    CARD32		possible_clones;
    
    /**
     * Whether this output can support interlaced modes
     */
    Bool		interlaceAllowed;

    /**
     * Whether this output can support double scan modes
     */
    Bool		doubleScanAllowed;

    /**
     * List of available modes on this output.
     *
     * This should be the list from get_modes(), plus perhaps additional
     * compatible modes added later.
     */
    DisplayModePtr	probed_modes;

    /**
     * Options parsed from the related monitor section
     */
    OptionInfoPtr	options;
    
    /**
     * Configured monitor section
     */
    XF86ConfMonitorPtr  conf_monitor;
    
    /**
     * Desired initial position
     */
    int			initial_x, initial_y;

    /**
     * Desired initial rotation
     */
    Rotation		initial_rotation;

    /**
     * Current connection status
     *
     * This indicates whether a monitor is known to be connected
     * to this output or not, or whether there is no way to tell
     */
    xf86OutputStatus	status;

    /** EDID monitor information */
    xf86MonPtr		MonInfo;

    /** subpixel order */
    int			subpixel_order;

    /** Physical size of the currently attached output device. */
    int			mm_width, mm_height;

    /** Output name */
    char		*name;

    /** output-specific functions */
    const xf86OutputFuncsRec *funcs;

    /** driver private information */
    void		*driver_private;
    
    /** Whether to use the old per-screen Monitor config section */
    Bool		use_screen_monitor;

#ifdef RANDR_12_INTERFACE
    /**
     * RandR 1.2 output structure.
     *
     * When RandR 1.2 is available, this points at the associated
     * RandR output structure and is created when this output is created
     */
    RROutputPtr		randr_output;
#else
    void		*randr_output;
#endif
};

typedef struct _xf86CrtcConfigFuncs {
    /**
     * Requests that the driver resize the screen.
     *
     * The driver is responsible for updating scrn->virtualX and scrn->virtualY.
     * If the requested size cannot be set, the driver should leave those values
     * alone and return FALSE.
     *
     * A naive driver that cannot reallocate the screen may simply change
     * virtual[XY].  A more advanced driver will want to also change the
     * devPrivate.ptr and devKind of the screen pixmap, update any offscreen
     * pixmaps it may have moved, and change pScrn->displayWidth.
     */
    Bool
    (*resize)(ScrnInfoPtr	scrn,
	      int		width,
	      int		height);
} xf86CrtcConfigFuncsRec, *xf86CrtcConfigFuncsPtr;

typedef struct _xf86CrtcConfig {
    int			num_output;
    xf86OutputPtr	*output;
    /**
     * compat_output is used whenever we deal
     * with legacy code that only understands a single
     * output. pScrn->modes will be loaded from this output,
     * adjust frame will whack this output, etc.
     */
    int			compat_output;

    int			num_crtc;
    xf86CrtcPtr		*crtc;

    int			minWidth, minHeight;
    int			maxWidth, maxHeight;
    
    /* For crtc-based rotation */
    DamagePtr		rotation_damage;
    Bool		rotation_damage_registered;

    /* DGA */
    unsigned int	dga_flags;
    unsigned long	dga_address;
    DGAModePtr		dga_modes;
    int			dga_nmode;
    int			dga_width, dga_height, dga_stride;
    DisplayModePtr	dga_save_mode;

    const xf86CrtcConfigFuncsRec *funcs;

    CreateScreenResourcesProcPtr    CreateScreenResources;

    CloseScreenProcPtr		    CloseScreen;

    /* Cursor information */
    xf86CursorInfoPtr	cursor_info;
    CursorPtr		cursor;
    CARD8		*cursor_image;
    Bool		cursor_on;
    CARD32		cursor_fg, cursor_bg;

    /**
     * Options parsed from the related device section
     */
    OptionInfoPtr	options;

    Bool		debug_modes;

    /* wrap screen BlockHandler for rotation */
    ScreenBlockHandlerProcPtr	BlockHandler;

} xf86CrtcConfigRec, *xf86CrtcConfigPtr;

extern int xf86CrtcConfigPrivateIndex;

#define XF86_CRTC_CONFIG_PTR(p)	((xf86CrtcConfigPtr) ((p)->privates[xf86CrtcConfigPrivateIndex].ptr))

/*
 * Initialize xf86CrtcConfig structure
 */

void
xf86CrtcConfigInit (ScrnInfoPtr				scrn,
		    const xf86CrtcConfigFuncsRec	*funcs);

void
xf86CrtcSetSizeRange (ScrnInfoPtr scrn,
		      int minWidth, int minHeight,
		      int maxWidth, int maxHeight);

/*
 * Crtc functions
 */
xf86CrtcPtr
xf86CrtcCreate (ScrnInfoPtr		scrn,
		const xf86CrtcFuncsRec	*funcs);

void
xf86CrtcDestroy (xf86CrtcPtr		crtc);


/**
 * Sets the given video mode on the given crtc
 */
Bool
xf86CrtcSetMode (xf86CrtcPtr crtc, DisplayModePtr mode, Rotation rotation,
		 int x, int y);

/*
 * Assign crtc rotation during mode set
 */
Bool
xf86CrtcRotate (xf86CrtcPtr crtc, DisplayModePtr mode, Rotation rotation);

/*
 * Clean up rotation during CloseScreen
 */
void
xf86RotateCloseScreen (ScreenPtr pScreen);

/**
 * Return whether any output is assigned to the crtc
 */
Bool
xf86CrtcInUse (xf86CrtcPtr crtc);

/*
 * Output functions
 */
xf86OutputPtr
xf86OutputCreate (ScrnInfoPtr		    scrn,
		  const xf86OutputFuncsRec  *funcs,
		  const char		    *name);

void
xf86OutputUseScreenMonitor (xf86OutputPtr output, Bool use_screen_monitor);

Bool
xf86OutputRename (xf86OutputPtr output, const char *name);

void
xf86OutputDestroy (xf86OutputPtr	output);

void
xf86ProbeOutputModes (ScrnInfoPtr pScrn, int maxX, int maxY);

void
xf86SetScrnInfoModes (ScrnInfoPtr pScrn);

Bool
xf86CrtcScreenInit (ScreenPtr pScreen);

Bool
xf86InitialConfiguration (ScrnInfoPtr pScrn, Bool canGrow);

void
xf86DPMSSet(ScrnInfoPtr pScrn, int PowerManagementMode, int flags);
    
Bool
xf86SaveScreen(ScreenPtr pScreen, int mode);

void
xf86DisableUnusedFunctions(ScrnInfoPtr pScrn);

DisplayModePtr
xf86OutputFindClosestMode (xf86OutputPtr output, DisplayModePtr desired);
    
Bool
xf86SetSingleMode (ScrnInfoPtr pScrn, DisplayModePtr desired, Rotation rotation);

/**
 * Set the EDID information for the specified output
 */
void
xf86OutputSetEDID (xf86OutputPtr output, xf86MonPtr edid_mon);

/**
 * Return the list of modes supported by the EDID information
 * stored in 'output'
 */
DisplayModePtr
xf86OutputGetEDIDModes (xf86OutputPtr output);

xf86MonPtr
xf86OutputGetEDID (xf86OutputPtr output, I2CBusPtr pDDCBus);

/**
 * Initialize dga for this screen
 */

Bool
xf86DiDGAInit (ScreenPtr pScreen, unsigned long dga_address);

/**
 * Re-initialize dga for this screen (as when the set of modes changes)
 */

Bool
xf86DiDGAReInit (ScreenPtr pScreen);

/*
 * Set the subpixel order reported for the screen using
 * the information from the outputs
 */

void
xf86CrtcSetScreenSubpixelOrder (ScreenPtr pScreen);

/*
 * Get a standard string name for a connector type 
 */
char *
xf86ConnectorGetName(xf86ConnectorType connector);

/*
 * Using the desired mode information in each crtc, set
 * modes (used in EnterVT functions, or at server startup)
 */

Bool
xf86SetDesiredModes (ScrnInfoPtr pScrn);

/**
 * Initialize the CRTC-based cursor code. CRTC function vectors must
 * contain relevant cursor setting functions.
 *
 * Driver should call this from ScreenInit function
 */
Bool
xf86_cursors_init (ScreenPtr screen, int max_width, int max_height, int flags);

/**
 * Called when anything on the screen is reconfigured.
 *
 * Reloads cursor images as needed, then adjusts cursor positions.
 * 
 * Driver should call this from crtc commit function.
 */
void
xf86_reload_cursors (ScreenPtr screen);

/**
 * Called from EnterVT to turn the cursors back on
 */
void
xf86_show_cursors (ScrnInfoPtr scrn);

/**
 * Called by the driver to turn cursors off
 */
void
xf86_hide_cursors (ScrnInfoPtr scrn);

/**
 * Clean up CRTC-based cursor code. Driver must call this at CloseScreen time.
 */
void
xf86_cursors_fini (ScreenPtr screen);

/*
 * For overlay video, compute the relevant CRTC and
 * clip video to that.
 * wraps xf86XVClipVideoHelper()
 */

Bool
xf86_crtc_clip_video_helper(ScrnInfoPtr pScrn,
			    xf86CrtcPtr *crtc_ret,
			    xf86CrtcPtr desired_crtc,
			    BoxPtr      dst,
			    INT32	*xa,
			    INT32	*xb,
			    INT32	*ya,
			    INT32	*yb,
			    RegionPtr   reg,
			    INT32	width,
			    INT32	height);
    
#endif /* _XF86CRTC_H_ */

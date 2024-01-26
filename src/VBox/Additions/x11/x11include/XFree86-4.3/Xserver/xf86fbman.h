/* $XFree86: xc/programs/Xserver/hw/xfree86/common/xf86fbman.h,v 1.12 2001/08/28 16:55:09 tsi Exp $ */

#ifndef _XF86FBMAN_H
#define _XF86FBMAN_H


#include "scrnintstr.h"
#include "regionstr.h"


#define FAVOR_AREA_THEN_WIDTH		0
#define FAVOR_AREA_THEN_HEIGHT		1
#define FAVOR_WIDTH_THEN_AREA		2
#define FAVOR_HEIGHT_THEN_AREA		3

#define PRIORITY_LOW			0
#define PRIORITY_NORMAL			1
#define PRIORITY_EXTREME		2


typedef struct _FBArea {
   ScreenPtr    pScreen;
   BoxRec   	box;
   int 		granularity;
   void 	(*MoveAreaCallback)(struct _FBArea*, struct _FBArea*);
   void 	(*RemoveAreaCallback)(struct _FBArea*);
   DevUnion 	devPrivate;
} FBArea, *FBAreaPtr;

typedef struct _FBLinear {
   ScreenPtr    pScreen;
   int		size;
   int 		offset;
   int 		granularity;
   void 	(*MoveLinearCallback)(struct _FBLinear*, struct _FBLinear*);
   void 	(*RemoveLinearCallback)(struct _FBLinear*);
   DevUnion 	devPrivate;
} FBLinear, *FBLinearPtr;

typedef void (*FreeBoxCallbackProcPtr)(ScreenPtr, RegionPtr, pointer);
typedef void (*MoveAreaCallbackProcPtr)(FBAreaPtr, FBAreaPtr);
typedef void (*RemoveAreaCallbackProcPtr)(FBAreaPtr);

typedef void (*MoveLinearCallbackProcPtr)(FBLinearPtr, FBLinearPtr);
typedef void (*RemoveLinearCallbackProcPtr)(FBLinearPtr);


typedef struct {
    FBAreaPtr (*AllocateOffscreenArea)(
		ScreenPtr pScreen, 
		int w, int h,
		int granularity,
		MoveAreaCallbackProcPtr moveCB,
		RemoveAreaCallbackProcPtr removeCB,
		pointer privData);
    void      (*FreeOffscreenArea)(FBAreaPtr area);
    Bool      (*ResizeOffscreenArea)(FBAreaPtr area, int w, int h);
    Bool      (*QueryLargestOffscreenArea)(
		ScreenPtr pScreen,
		int *width, int *height,
		int granularity,
		int preferences,
		int priority);
    Bool      (*RegisterFreeBoxCallback)( 
		ScreenPtr pScreen,  
		FreeBoxCallbackProcPtr FreeBoxCallback,
		pointer devPriv);
/* linear functions */
    FBLinearPtr (*AllocateOffscreenLinear)(
		ScreenPtr pScreen, 
		int size,
		int granularity,
		MoveLinearCallbackProcPtr moveCB,
		RemoveLinearCallbackProcPtr removeCB,
		pointer privData);
    void      (*FreeOffscreenLinear)(FBLinearPtr area);
    Bool      (*ResizeOffscreenLinear)(FBLinearPtr area, int size);
    Bool      (*QueryLargestOffscreenLinear)(
		ScreenPtr pScreen,
		int *size,
		int granularity,
		int priority);
    Bool      (*PurgeOffscreenAreas) (ScreenPtr);
} FBManagerFuncs, *FBManagerFuncsPtr;


Bool xf86RegisterOffscreenManager(
    ScreenPtr pScreen, 
    FBManagerFuncsPtr funcs
);

Bool
xf86InitFBManagerRegion(
    ScreenPtr pScreen, 
    RegionPtr ScreenRegion
);

Bool
xf86InitFBManagerArea(
    ScreenPtr pScreen,
    int PixalArea,
    int Verbosity
);

Bool
xf86InitFBManager(
    ScreenPtr pScreen, 
    BoxPtr FullBox
);

Bool 
xf86FBManagerRunning(
    ScreenPtr pScreen
);

FBAreaPtr 
xf86AllocateOffscreenArea (
   ScreenPtr pScreen, 
   int w, int h,
   int granularity,
   MoveAreaCallbackProcPtr moveCB,
   RemoveAreaCallbackProcPtr removeCB,
   pointer privData
);

FBAreaPtr 
xf86AllocateLinearOffscreenArea (
   ScreenPtr pScreen, 
   int length,
   int granularity,
   MoveAreaCallbackProcPtr moveCB,
   RemoveAreaCallbackProcPtr removeCB,
   pointer privData
);

FBLinearPtr 
xf86AllocateOffscreenLinear (
   ScreenPtr pScreen, 
   int length,
   int granularity,
   MoveLinearCallbackProcPtr moveCB,
   RemoveLinearCallbackProcPtr removeCB,
   pointer privData
);

void xf86FreeOffscreenArea(FBAreaPtr area);
void xf86FreeOffscreenLinear(FBLinearPtr area);

Bool 
xf86ResizeOffscreenArea(
   FBAreaPtr resize,
   int w, int h
);

Bool 
xf86ResizeOffscreenLinear(
   FBLinearPtr resize,
   int size
);


Bool
xf86RegisterFreeBoxCallback(
    ScreenPtr pScreen,  
    FreeBoxCallbackProcPtr FreeBoxCallback,
    pointer devPriv
);

Bool
xf86PurgeUnlockedOffscreenAreas(
    ScreenPtr pScreen
);


Bool
xf86QueryLargestOffscreenArea(
    ScreenPtr pScreen,
    int *width, int *height,
    int granularity,
    int preferences,
    int priority
);

Bool
xf86QueryLargestOffscreenLinear(
    ScreenPtr pScreen,
    int *size,
    int granularity,
    int priority
);


#endif /* _XF86FBMAN_H */

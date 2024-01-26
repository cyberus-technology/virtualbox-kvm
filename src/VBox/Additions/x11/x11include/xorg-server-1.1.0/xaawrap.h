/* $XFree86: xc/programs/Xserver/hw/xfree86/xaa/xaawrap.h,v 1.3 1998/10/25 07:12:14 dawes Exp $ */

#define XAA_SCREEN_PROLOGUE(pScreen, field)\
  ((pScreen)->field = \
   ((XAAScreenPtr) (pScreen)->devPrivates[XAAGetScreenIndex()].ptr)->field)

#define XAA_SCREEN_EPILOGUE(pScreen, field, wrapper)\
    ((pScreen)->field = wrapper)


#define XAA_GC_FUNC_PROLOGUE(pGC)\
    XAAGCPtr   pGCPriv = (XAAGCPtr) (pGC)->devPrivates[XAAGetGCIndex()].ptr;\
    (pGC)->funcs = pGCPriv->wrapFuncs;\
    if(pGCPriv->flags)\
	(pGC)->ops = pGCPriv->wrapOps

#define XAA_GC_FUNC_EPILOGUE(pGC)\
    pGCPriv->wrapFuncs = (pGC)->funcs;\
    (pGC)->funcs = &XAAGCFuncs;\
    if(pGCPriv->flags) {\
	pGCPriv->wrapOps = (pGC)->ops;\
	(pGC)->ops = (pGCPriv->flags & OPS_ARE_ACCEL) ? pGCPriv->XAAOps :\
				&XAAPixmapOps;\
    }


#define XAA_GC_OP_PROLOGUE(pGC)\
    XAAGCPtr pGCPriv = (XAAGCPtr)(pGC->devPrivates[XAAGetGCIndex()].ptr);\
    GCFuncs *oldFuncs = pGC->funcs;\
    pGC->funcs = pGCPriv->wrapFuncs;\
    pGC->ops = pGCPriv->wrapOps

#define XAA_GC_OP_PROLOGUE_WITH_RETURN(pGC)\
    XAAGCPtr pGCPriv = (XAAGCPtr)(pGC->devPrivates[XAAGetGCIndex()].ptr);\
    GCFuncs *oldFuncs = pGC->funcs;\
    if(!REGION_NUM_RECTS(pGC->pCompositeClip)) return; \
    pGC->funcs = pGCPriv->wrapFuncs;\
    pGC->ops = pGCPriv->wrapOps

    
#define XAA_GC_OP_EPILOGUE(pGC)\
    pGCPriv->wrapOps = pGC->ops;\
    pGC->funcs = oldFuncs;\
    pGC->ops   = pGCPriv->XAAOps


#define XAA_PIXMAP_OP_PROLOGUE(pGC, pDraw)\
    XAAGCPtr pGCPriv = (XAAGCPtr)(pGC->devPrivates[XAAGetGCIndex()].ptr);\
    XAAPixmapPtr pixPriv = XAA_GET_PIXMAP_PRIVATE((PixmapPtr)(pDraw));\
    GCFuncs *oldFuncs = pGC->funcs;\
    pGC->funcs = pGCPriv->wrapFuncs;\
    pGC->ops = pGCPriv->wrapOps

    
#define XAA_PIXMAP_OP_EPILOGUE(pGC)\
    pGCPriv->wrapOps = pGC->ops;\
    pGC->funcs = oldFuncs;\
    pGC->ops   = &XAAPixmapOps;\
    pixPriv->flags |= DIRTY

#ifdef HAVE_XORG_CONFIG_H
#include <xorg-config.h>
#endif

#ifdef RENDER
#define XAA_RENDER_PROLOGUE(pScreen,field)\
    (GetPictureScreen(pScreen)->field = \
     ((XAAScreenPtr) (pScreen)->devPrivates[XAAGetScreenIndex()].ptr)->field)

#define XAA_RENDER_EPILOGUE(pScreen, field, wrapper)\
    (GetPictureScreen(pScreen)->field = wrapper)
#endif

/* This also works fine for drawables */

#define SYNC_CHECK(pGC) {\
     XAAInfoRecPtr infoRec =\
((XAAScreenPtr)((pGC)->pScreen->devPrivates[XAAGetScreenIndex()].ptr))->AccelInfoRec;\
    if(infoRec->NeedToSync) {\
	(*infoRec->Sync)(infoRec->pScrn);\
	infoRec->NeedToSync = FALSE;\
    }}

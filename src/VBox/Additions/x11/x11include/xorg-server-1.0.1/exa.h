/*
 *
 * Copyright (C) 2000 Keith Packard
 *               2004 Eric Anholt
 *               2005 Zack Rusin
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of copyright holders not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission. Copyright holders make no
 * representations about the suitability of this software for any purpose.  It
 * is provided "as is" without express or implied warranty.
 *
 * THE COPYRIGHT HOLDERS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS
 * SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS, IN NO EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
 * AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING
 * OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 */
#ifndef EXA_H
#define EXA_H

#include "scrnintstr.h"
#include "pixmapstr.h"
#include "windowstr.h"
#include "gcstruct.h"
#include "picturestr.h"

#define EXA_VERSION_MAJOR   0
#define EXA_VERSION_MINOR   2
#define EXA_VERSION_RELEASE 0

typedef struct _ExaOffscreenArea ExaOffscreenArea;

typedef void (*ExaOffscreenSaveProc) (ScreenPtr pScreen, ExaOffscreenArea *area);

typedef enum _ExaOffscreenState {
    ExaOffscreenAvail,
    ExaOffscreenRemovable,
    ExaOffscreenLocked
} ExaOffscreenState;

struct _ExaOffscreenArea {
    int                 base_offset;	/* allocation base */
    int                 offset;         /* aligned offset */
    int                 size;           /* total allocation size */
    int                 score;
    pointer             privData;

    ExaOffscreenSaveProc save;

    ExaOffscreenState   state;

    ExaOffscreenArea    *next;
};

typedef struct _ExaCardInfo {
    /* These are here because I don't want to be adding more to
     * ScrnInfoRec */
    CARD8         *memoryBase;
    unsigned long  offScreenBase;

    /* It's fix.smem_len.
       This one could be potentially substituted by ScrnInfoRec
       videoRam member, but I do not want to be doing the silly
       << 10, >>10 all over the place */
    unsigned long memorySize;

    int pixmapOffsetAlign;
    int pixmapPitchAlign;
    int flags;

    /* The coordinate limitations for rendering for this hardware.
     * Exa breaks larger pixmaps into smaller pieces and calls Prepare multiple times
     * to handle larger pixmaps
     */
    int maxX;
    int maxY;

    /* private */
    ExaOffscreenArea *offScreenAreas;
    Bool              needsSync;
    int               lastMarker;
} ExaCardInfoRec, *ExaCardInfoPtr;

typedef struct _ExaAccelInfo {
    /* PrepareSolid may fail if the pixmaps can't be accelerated to/from.
     * This is an important feature for handling strange corner cases
     * in hardware that are poorly expressed through flags.
     */
    Bool        (*PrepareSolid) (PixmapPtr      pPixmap,
                                 int            alu,
                                 Pixel          planemask,
                                 Pixel          fg);
    void        (*Solid) (PixmapPtr      pPixmap, int x1, int y1, int x2, int y2);
    void        (*DoneSolid) (PixmapPtr      pPixmap);

    /* PrepareSolid may fail if the pixmaps can't be accelerated to/from.
     * This is an important feature for handling strange corner cases
     * in hardware that are poorly expressed through flags.
     */
    Bool        (*PrepareCopy) (PixmapPtr       pSrcPixmap,
                                PixmapPtr       pDstPixmap,
                                int             dx,
                                int             dy,
                                int             alu,
                                Pixel           planemask);
    void        (*Copy) (PixmapPtr       pDstPixmap,
                         int    srcX,
                         int    srcY,
                         int    dstX,
                         int    dstY,
                         int    width,
                         int    height);
    void        (*DoneCopy) (PixmapPtr       pDstPixmap);

    /* The Composite hooks are a wrapper around the Composite operation.
     * The CheckComposite occurs before pixmap migration occurs,
     * and may fail for many hardware-dependent reasons.
     * PrepareComposite should not fail, and the Bool return may
     * not be necessary if we can
     * adequately represent pixmap location/pitch limitations..
     */
    Bool        (*CheckComposite) (int          op,
                                   PicturePtr   pSrcPicture,
                                   PicturePtr   pMaskPicture,
                                   PicturePtr   pDstPicture);
    Bool        (*PrepareComposite) (int                op,
                                     PicturePtr         pSrcPicture,
                                     PicturePtr         pMaskPicture,
                                     PicturePtr         pDstPicture,
                                     PixmapPtr          pSrc,
                                     PixmapPtr          pMask,
                                     PixmapPtr          pDst);
    void        (*Composite) (PixmapPtr         pDst,
                              int       srcX,
                              int        srcY,
                              int        maskX,
                              int        maskY,
                              int        dstX,
                              int        dstY,
                              int        width,
                              int        height);
    void        (*DoneComposite) (PixmapPtr         pDst);

    /* Attempt to upload the data from src into the rectangle of the
     * in-framebuffer pDst beginning at x,y and of width w,h.  May fail.
     */
    Bool        (*UploadToScreen) (PixmapPtr            pDst,
				   int                  x,
				   int                  y,
				   int                  w,
				   int                  h,
                                   char                 *src,
                                   int                  src_pitch);
    Bool        (*UploadToScratch) (PixmapPtr           pSrc,
                                    PixmapPtr           pDst);

    /* Attempt to download the rectangle from the in-framebuffer pSrc into
     * dst, given the pitch.  May fail.  Since it is likely
     * accelerated, a markSync will follow it as with other acceleration
     * hooks.
     */
    Bool (*DownloadFromScreen)(PixmapPtr pSrc,
                               int x,  int y,
                               int w,  int h,
                               char *dst,  int dst_pitch);

    /* Should return a hrdware-dependent marker number which can
     * be waited for with WaitMarker. It can be not implemented in which
     * case WaitMarker must wait for idle on any given marker
     * number.
     */
    int		(*MarkSync)   (ScreenPtr pScreen);
    void	(*WaitMarker) (ScreenPtr pScreen, int marker);

    /* These are wrapping all fb or composite operations that will cause
     * a direct access to the framebuffer. You can use them to update
     * endian swappers, force migration to RAM, or whatever else you find
     * useful at this point. EXA can stack up to 3 calls to Prepare/Finish
     * access, though they will have a different index. If your hardware
     * doesn't have enough separate configurable swapper, you can return
     * FALSE from PrepareAccess() to force EXA to migrate the pixmap to RAM.
     * Note that DownloadFromScreen and UploadToScreen can both be called
     * between PrepareAccess() and FinishAccess(). If they need to use a
     * swapper, they should save & restore its setting.
     */
    Bool	(*PrepareAccess)(PixmapPtr pPix, int index);
    void	(*FinishAccess)(PixmapPtr pPix, int index);
	#define EXA_PREPARE_DEST	0
	#define EXA_PREPARE_SRC		1
	#define EXA_PREPARE_MASK	2

} ExaAccelInfoRec, *ExaAccelInfoPtr;

typedef struct _ExaDriver {
    ExaCardInfoRec  card;
    ExaAccelInfoRec accel;
} ExaDriverRec, *ExaDriverPtr;

#define EXA_OFFSCREEN_PIXMAPS           (1 << 0)
#define EXA_OFFSCREEN_ALIGN_POT         (1 << 1)


#define EXA_MAKE_VERSION(a, b, c) (((a) << 16) | ((b) << 8) | (c))
#define EXA_VERSION \
    EXA_MAKE_VERSION(EXA_VERSION_MAJOR, EXA_VERSION_MINOR, EXA_VERSION_RELEASE)
#define EXA_IS_VERSION(a,b,c) (EXA_VERSION >= EXA_MAKE_VERSION(a,b,c))

unsigned int
exaGetVersion(void);

Bool
exaDriverInit(ScreenPtr                pScreen,
              ExaDriverPtr   pScreenInfo);

void
exaDriverFini(ScreenPtr                pScreen);

void
exaMarkSync(ScreenPtr pScreen);
void
exaWaitSync(ScreenPtr pScreen);

Bool
exaOffscreenInit(ScreenPtr pScreen);

ExaOffscreenArea *
exaOffscreenAlloc(ScreenPtr pScreen, int size, int align,
                  Bool locked,
                  ExaOffscreenSaveProc save,
                  pointer privData);

ExaOffscreenArea *
exaOffscreenFree(ScreenPtr pScreen, ExaOffscreenArea *area);

unsigned long
exaGetPixmapOffset(PixmapPtr pPix);

unsigned long
exaGetPixmapPitch(PixmapPtr pPix);

unsigned long
exaGetPixmapSize(PixmapPtr pPix);

#define exaInitCard(exa, sync, memory_base, off_screen_base, memory_size, \
                    offscreen_byte_align, offscreen_pitch, flags, \
                    max_x, max_y) \
    (exa)->card.Sync               = sync; \
    (exa)->card.memoryBase         = memory_base; \
    (exa)->card.offScreenBase      = off_screen_base; \
    (exa)->card.memorySize         = memory_size; \
    (exa)->card.offscreenByteAlign = offscreen_byte_align; \
    (exa)->card.offscreenPitch     = offscreen_pitch; \
    (exa)->card.flags              = flags; \
    (exa)->card.maxX               = max_x; \
    (exa)->card.maxY               = max_y

#endif /* EXA_H */

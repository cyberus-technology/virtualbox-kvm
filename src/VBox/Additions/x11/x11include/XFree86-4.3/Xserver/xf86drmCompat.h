/* xf86drmCompat.h -- OS-independent header for old device specific DRM user-level
 *                    library interface
 *
 * Copyright 2000 VA Linux Systems, Inc., Fremont, California.
 * Copyright 2002 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * PRECISION INSIGHT AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *   Gareth Hughes <gareth@valinux.com>
 *   Kevin E. Martin <martin@valinux.com>
 *   Keith Whitwell <keith@tungstengraphics.com>
 *
 * Backwards compatability modules broken out by:
 *   Jens Owen <jens@tungstengraphics.com>
 *
 * $XFree86: xc/programs/Xserver/hw/xfree86/os-support/xf86drmCompat.h,v 1.1 2002/10/30 12:52:23 alanh Exp $
 *
 */

#ifndef _XF86DRI_COMPAT_H_
#define _XF86DRI_COMPAT_H_

/* WARNING: Do not change, or add, anything to this file.  It is only provided
 * for binary backwards compatability with the old driver specific DRM 
 * extensions used before XFree86 4.3.
 */

/* I810 */

typedef struct {
   unsigned int start; 
   unsigned int end; 
   unsigned int size;
   unsigned int mmio_offset;
   unsigned int buffers_offset;
   int sarea_off;

   unsigned int front_offset;
   unsigned int back_offset;
   unsigned int depth_offset;
   unsigned int overlay_offset;
   unsigned int overlay_physical;
   unsigned int w;
   unsigned int h;
   unsigned int pitch;
   unsigned int pitch_bits;
} drmCompatI810Init;

extern Bool drmI810CleanupDma(int driSubFD);
extern Bool drmI810InitDma(int driSubFD, drmCompatI810Init *info );

/* Mga */

typedef struct {
   unsigned long sarea_priv_offset;
   int chipset;
   int sgram;
   unsigned int maccess;
   unsigned int fb_cpp;
   unsigned int front_offset, front_pitch;
   unsigned int back_offset, back_pitch;
   unsigned int depth_cpp;
   unsigned int depth_offset, depth_pitch;
   unsigned int texture_offset[2];
   unsigned int texture_size[2];
   unsigned long fb_offset;
   unsigned long mmio_offset;
   unsigned long status_offset;
   unsigned long warp_offset;
   unsigned long primary_offset;
   unsigned long buffers_offset;
} drmCompatMGAInit;

extern int drmMGAInitDMA( int fd, drmCompatMGAInit *info );
extern int drmMGACleanupDMA( int fd );
extern int drmMGAFlushDMA( int fd, drmLockFlags flags );
extern int drmMGAEngineReset( int fd );
extern int drmMGAFullScreen( int fd, int enable );
extern int drmMGASwapBuffers( int fd );
extern int drmMGAClear( int fd, unsigned int flags,
			unsigned int clear_color, unsigned int clear_depth,
			unsigned int color_mask, unsigned int depth_mask );
extern int drmMGAFlushVertexBuffer( int fd, int indx, int used, int discard );
extern int drmMGAFlushIndices( int fd, int indx,
			       int start, int end, int discard );
extern int drmMGATextureLoad( int fd, int indx,
			      unsigned int dstorg, unsigned int length );
extern int drmMGAAgpBlit( int fd, unsigned int planemask,
			  unsigned int src, int src_pitch,
			  unsigned int dst, int dst_pitch,
			  int delta_sx, int delta_sy,
			  int delta_dx, int delta_dy,
			  int height, int ydir );

/* R128 */

typedef struct {
   unsigned long sarea_priv_offset;
   int is_pci;
   int cce_mode;
   int cce_secure;
   int ring_size;
   int usec_timeout;
   unsigned int fb_bpp;
   unsigned int front_offset, front_pitch;
   unsigned int back_offset, back_pitch;
   unsigned int depth_bpp;
   unsigned int depth_offset, depth_pitch;
   unsigned int span_offset;
   unsigned long fb_offset;
   unsigned long mmio_offset;
   unsigned long ring_offset;
   unsigned long ring_rptr_offset;
   unsigned long buffers_offset;
   unsigned long agp_textures_offset;
} drmCompatR128Init;

extern int drmR128InitCCE( int fd, drmCompatR128Init *info );
extern int drmR128CleanupCCE( int fd );
extern int drmR128StartCCE( int fd );
extern int drmR128StopCCE( int fd );
extern int drmR128ResetCCE( int fd );
extern int drmR128WaitForIdleCCE( int fd );
extern int drmR128EngineReset( int fd );
extern int drmR128FullScreen( int fd, int enable );
extern int drmR128SwapBuffers( int fd );
extern int drmR128Clear( int fd, unsigned int flags,
			 unsigned int clear_color, unsigned int clear_depth,
			 unsigned int color_mask, unsigned int depth_mask );
extern int drmR128FlushVertexBuffer( int fd, int prim, int indx,
				     int count, int discard );
extern int drmR128FlushIndices( int fd, int prim, int indx,
				int start, int end, int discard );
extern int drmR128TextureBlit( int fd, int indx,
			       int offset, int pitch, int format,
			       int x, int y, int width, int height );
extern int drmR128WriteDepthSpan( int fd, int n, int x, int y,
				  const unsigned int depth[],
				  const unsigned char mask[] );
extern int drmR128WriteDepthPixels( int fd, int n,
				    const int x[], const int y[],
				    const unsigned int depth[],
				    const unsigned char mask[] );
extern int drmR128ReadDepthSpan( int fd, int n, int x, int y );
extern int drmR128ReadDepthPixels( int fd, int n,
				   const int x[], const int y[] );
extern int drmR128PolygonStipple( int fd, unsigned int *mask );
extern int drmR128FlushIndirectBuffer( int fd, int indx,
				       int start, int end, int discard );

/* Radeon */

typedef struct {
   unsigned long sarea_priv_offset;
   int is_pci;
   int cp_mode;
   int agp_size;
   int ring_size;
   int usec_timeout;

   unsigned int fb_bpp;
   unsigned int front_offset, front_pitch;
   unsigned int back_offset, back_pitch;
   unsigned int depth_bpp;
   unsigned int depth_offset, depth_pitch;

   unsigned long fb_offset;
   unsigned long mmio_offset;
   unsigned long ring_offset;
   unsigned long ring_rptr_offset;
   unsigned long buffers_offset;
   unsigned long agp_textures_offset;
} drmCompatRadeonInit;

typedef struct {
   unsigned int x;
   unsigned int y;
   unsigned int width;
   unsigned int height;
   void *data;
} drmCompatRadeonTexImage;

extern int drmRadeonInitCP( int fd, drmCompatRadeonInit *info );
extern int drmRadeonCleanupCP( int fd );
extern int drmRadeonStartCP( int fd );
extern int drmRadeonStopCP( int fd );
extern int drmRadeonResetCP( int fd );
extern int drmRadeonWaitForIdleCP( int fd );
extern int drmRadeonEngineReset( int fd );
extern int drmRadeonFullScreen( int fd, int enable );
extern int drmRadeonSwapBuffers( int fd );
extern int drmRadeonClear( int fd, unsigned int flags,
			   unsigned int clear_color, unsigned int clear_depth,
			   unsigned int color_mask, unsigned int stencil,
			   void *boxes, int nbox );
extern int drmRadeonFlushVertexBuffer( int fd, int prim, int indx,
				       int count, int discard );
extern int drmRadeonFlushIndices( int fd, int prim, int indx,
				  int start, int end, int discard );
extern int drmRadeonLoadTexture( int fd, int offset, int pitch, int format,
				 int width, int height,
				 drmCompatRadeonTexImage *image );
extern int drmRadeonPolygonStipple( int fd, unsigned int *mask );
extern int drmRadeonFlushIndirectBuffer( int fd, int indx,
					 int start, int end, int discard );

/* SiS */
extern Bool drmSiSAgpInit(int driSubFD, int offset, int size);

/* I830 */
typedef struct {
   unsigned int start;
   unsigned int end;
   unsigned int size;
   unsigned int mmio_offset;
   unsigned int buffers_offset;
   int sarea_off;
   unsigned int front_offset;
   unsigned int back_offset;
   unsigned int depth_offset;
   unsigned int w;
   unsigned int h;
   unsigned int pitch;
   unsigned int pitch_bits;
   unsigned int cpp;
} drmCompatI830Init;

extern Bool drmI830CleanupDma(int driSubFD);
extern Bool drmI830InitDma(int driSubFD, drmCompatI830Init *info );

#endif

/* WARNING: Do not change, or add, anything to this file.  It is only provided
 * for binary backwards compatability with the old driver specific DRM 
 * extensions used before XFree86 4.3.
 */

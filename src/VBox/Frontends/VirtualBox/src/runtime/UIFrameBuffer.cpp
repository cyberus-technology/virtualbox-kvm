/* $Id: UIFrameBuffer.cpp $ */
/** @file
 * VBox Qt GUI - UIFrameBuffer class implementation.
 */

/*
 * Copyright (C) 2010-2023 Oracle and/or its affiliates.
 *
 * This file is part of VirtualBox base platform packages, as
 * available from https://www.virtualbox.org.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, in version 3 of the
 * License.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <https://www.gnu.org/licenses>.
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */

/* Qt includes: */
#include <QImage>
#include <QRegion>
#include <QPainter>
#include <QTransform>

/* GUI includes: */
#include "UIActionPool.h"
#include "UIActionPoolRuntime.h"
#include "UIFrameBuffer.h"
#include "UISession.h"
#include "UIMachineLogic.h"
#include "UIMachineWindow.h"
#include "UIMachineView.h"
#include "UINotificationCenter.h"
#include "UIExtraDataManager.h"
#include "UICommon.h"
#ifdef VBOX_WITH_MASKED_SEAMLESS
# include "UIMachineWindow.h"
#endif /* VBOX_WITH_MASKED_SEAMLESS */
#ifdef VBOX_WS_X11
# include "VBoxUtils-x11.h"
#endif

/* COM includes: */
#include "CConsole.h"
#include "CDisplay.h"
#include "CFramebuffer.h"
#include "CDisplaySourceBitmap.h"

/* VirtualBox interface declarations: */
#include <VBox/com/VirtualBox.h>

/* Other VBox includes: */
#include <iprt/critsect.h>
#include <VBoxVideo3D.h>

/* Other includes: */
#include <math.h>

#ifdef VBOX_WS_X11
/* X11 includes: */
# include <X11/Xlib.h>
# undef Bool // Qt5 vs Xlib gift..
#endif /* VBOX_WS_X11 */


#ifdef VBOX_GUI_WITH_QTGLFRAMEBUFFER
/* Experimental code. */

/* Qt OpenGL includes: */
/* On Windows host they require the following two include files, otherwise compilation will fail with warnings.
 * The two files are already included, but they are needed if the Qt files are moved to the 'Qt includes:' section. */
// #include <iprt/stdint.h>
// #include <iprt/win/windows.h>
#include <QOffscreenSurface>
#include <QOpenGLFunctions>
#include <QOpenGLTexture>
#include <QOpenGLWidget>

# ifdef RT_OS_LINUX
/* GL/glx.h must be included after Qt GL headers (which define GL_GLEXT_LEGACY) to avoid GL_GLEXT_VERSION conflict. */
#include <GL/glx.h>
# endif

class UIFrameBufferPrivate;
class GLWidget;

/* Handles the guest screen texture for the target GLWidget. */
class GLWidgetSource
{
public:

    GLWidgetSource(GLWidget *pTarget);
    virtual ~GLWidgetSource();

    GLWidget *Target() { return m_pTarget; }

    virtual void initGuestScreenTexture(int w, int h) { RT_NOREF(w, h); };
    virtual void uninitGuestScreenTexture() {};
    virtual void updateGuestImage() {};
    virtual void cleanup() {};
    virtual bool IsHW() { return false; };

private:

    GLWidget *m_pTarget;
};

/* Update the guest texture from a QImage. */
class GLWidgetSourceImage : public GLWidgetSource
{
public:

    GLWidgetSourceImage(GLWidget *pTarget, QImage *pImage);
    virtual ~GLWidgetSourceImage();

    virtual void initGuestScreenTexture(int w, int h);
    virtual void updateGuestImage();

private:

    QImage *m_pImage;
};

# ifdef RT_OS_LINUX
/* The guest texture is a X pixmap. */
class GLWidgetSourcePixmap : public GLWidgetSource
{
public:

    GLWidgetSourcePixmap(GLWidget *pTarget, Pixmap pixmap, VisualID visualid);
    virtual ~GLWidgetSourcePixmap();

    virtual void initGuestScreenTexture(int w, int h);
    virtual void uninitGuestScreenTexture();
    virtual void cleanup();
    virtual bool IsHW() { return true; };

private:

    /* HW accelerated graphics output from a pixmap. */
    Pixmap m_Pixmap;
    VisualID m_visualid;

    GLXPixmap m_glxPixmap;

    Display *m_display;
    PFNGLXBINDTEXIMAGEEXTPROC m_pfnglXBindTexImageEXT;
    PFNGLXRELEASETEXIMAGEEXTPROC m_pfnglXReleaseTexImageEXT;
};
# endif /* RT_OS_LINUX */

/* This widget allows to use OpenGL. */
class GLWidget : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT

public:

    GLWidget(QWidget *parent, UIFrameBufferPrivate *pFramebuffer);
    virtual ~GLWidget();

    /* Whether OpenGL is supported at all. */
    static bool isSupported();

    void lock() { RTCritSectEnter(&m_critSect); }
    void unlock() { RTCritSectLeave(&m_critSect); }

    /* Notification about the guest screen size. */
    void resizeGuestScreen(int w, int h);
    /* Which guest area is visible in the VM window. */
    void setGuestVisibleRect(int x, int y, int w, int h);
    /* Update the guest texture before painting. */
    void updateGuestImage();

    /* The guest texture OpenGL target. */
    static GLenum const kTextureTarget = GL_TEXTURE_2D;

    /* The the guest screen source. */
    void setSource(GLWidgetSource *pSource, bool fForce);

public slots:

    void cleanup();

protected:

    /* QOpenGLWidget methods, which must be reimplemented. */
    void initializeGL() RT_OVERRIDE;
    void paintGL() RT_OVERRIDE;
    void resizeGL(int w, int h) RT_OVERRIDE;

private:

    /* Get and possibly initialize the guest source. */
    GLWidgetSource *getSource();

    /* Create the texture which contains the guest screen bitmap. */
    void createGuestTexture();
    /* Delete the texture which contains the guest screen bitmap. */
    void deleteGuestTexture();

    /* Backlink. */
    UIFrameBufferPrivate *m_pFramebuffer;

    /* Fallback when no guest screen is available. */
    GLWidgetSource m_nullSource;
    /* The current guest screen bitmap source. */
    GLWidgetSource *m_pSource;

    /* The guest screen resolution. */
    QSize m_guestSize;
    /* The visible area of the guest screen in guest pixels. */
    QRect m_guestVisibleRect;

    /** RTCRITSECT object to protect frame-buffer access. */
    RTCRITSECT m_critSect;

    /* A new guest screen source has been set and needs reinitialization. */
    bool m_fReinitSource;

    /* Texture which contains entire guest screen. Size is m_guestSize. */
    GLuint m_guestTexture;
};
#endif /* VBOX_GUI_WITH_QTGLFRAMEBUFFER */

/** IFramebuffer implementation used to maintain VM display video memory. */
class ATL_NO_VTABLE UIFrameBufferPrivate : public QObject,
                                           public ATL::CComObjectRootEx<ATL::CComMultiThreadModel>,
                                           VBOX_SCRIPTABLE_IMPL(IFramebuffer)
{
    Q_OBJECT;

signals:

    /** Notifies listener about guest-screen resolution changes. */
    void sigNotifyChange(int iWidth, int iHeight);
    /** Notifies listener about guest-screen updates. */
    void sigNotifyUpdate(int iX, int iY, int iWidth, int iHeight);
    /** Notifies listener about guest-screen visible-region changes. */
    void sigSetVisibleRegion(QRegion region);

public:

    /** Frame-buffer constructor. */
    UIFrameBufferPrivate();
    /** Frame-buffer destructor. */
    ~UIFrameBufferPrivate();

    /** Frame-buffer initialization.
      * @param pMachineView defines machine-view this frame-buffer is bounded to. */
    virtual HRESULT init(UIMachineView *pMachineView);

    /** Assigns machine-view frame-buffer will be bounded to.
      * @param pMachineView defines machine-view this frame-buffer is bounded to. */
    virtual void setView(UIMachineView *pMachineView);

    /** Returns the copy of the IDisplay wrapper. */
    CDisplay display() const { return m_display; }
    /** Attach frame-buffer to IDisplay. */
    void attach();
    /** Detach frame-buffer from IDisplay. */
    void detach();

    /** Returns frame-buffer data address. */
    uchar *address() { return m_image.bits(); }
    /** Returns frame-buffer width. */
    ulong width() const { return m_iWidth; }
    /** Returns frame-buffer height. */
    ulong height() const { return m_iHeight; }
    /** Returns frame-buffer bits-per-pixel value. */
    ulong bitsPerPixel() const { return m_image.depth(); }
    /** Returns frame-buffer bytes-per-line value. */
    ulong bytesPerLine() const { return m_image.bytesPerLine(); }
    /** Returns default frame-buffer pixel-format. */
    ulong pixelFormat() const { return KBitmapFormat_BGR; }
    /** Returns the visual-state this frame-buffer is used for. */
    UIVisualStateType visualState() const { return m_pMachineView ? m_pMachineView->visualStateType() : UIVisualStateType_Invalid; }

    /** Defines whether frame-buffer is <b>unused</b>.
      * @note Refer to m_fUnused for more information.
      * @note Calls to this and any other EMT callback are synchronized (from GUI side). */
    void setMarkAsUnused(bool fUnused);

    /** Returns the frame-buffer's scaled-size. */
    QSize scaledSize() const { return m_scaledSize; }
    /** Defines host-to-guest scale ratio as @a size. */
    void setScaledSize(const QSize &size = QSize()) { m_scaledSize = size; }
    /** Returns x-origin of the host (scaled) content corresponding to x-origin of guest (actual) content. */
    inline int convertGuestXTo(int x) const { return m_scaledSize.isValid() ? qRound((double)m_scaledSize.width() / m_iWidth * x) : x; }
    /** Returns y-origin of the host (scaled) content corresponding to y-origin of guest (actual) content. */
    inline int convertGuestYTo(int y) const { return m_scaledSize.isValid() ? qRound((double)m_scaledSize.height() / m_iHeight * y) : y; }
    /** Returns x-origin of the guest (actual) content corresponding to x-origin of host (scaled) content. */
    inline int convertHostXTo(int x) const  { return m_scaledSize.isValid() ? qRound((double)m_iWidth / m_scaledSize.width() * x) : x; }
    /** Returns y-origin of the guest (actual) content corresponding to y-origin of host (scaled) content. */
    inline int convertHostYTo(int y) const  { return m_scaledSize.isValid() ? qRound((double)m_iHeight / m_scaledSize.height() * y) : y; }

    /** Returns the scale-factor used by the frame-buffer. */
    double scaleFactor() const { return m_dScaleFactor; }
    /** Define the scale-factor used by the frame-buffer. */
    void setScaleFactor(double dScaleFactor) { m_dScaleFactor = dScaleFactor; }

    /** Returns device-pixel-ratio set for HiDPI frame-buffer. */
    double devicePixelRatio() const { return m_dDevicePixelRatio; }
    /** Defines device-pixel-ratio set for HiDPI frame-buffer. */
    void setDevicePixelRatio(double dDevicePixelRatio) { m_dDevicePixelRatio = dDevicePixelRatio; }
    /** Returns actual device-pixel-ratio set for HiDPI frame-buffer. */
    double devicePixelRatioActual() const { return m_dDevicePixelRatioActual; }
    /** Defines actual device-pixel-ratio set for HiDPI frame-buffer. */
    void setDevicePixelRatioActual(double dDevicePixelRatioActual) { m_dDevicePixelRatioActual = dDevicePixelRatioActual; }

    /** Returns whether frame-buffer should use unscaled HiDPI output. */
    bool useUnscaledHiDPIOutput() const { return m_fUseUnscaledHiDPIOutput; }
    /** Defines whether frame-buffer should use unscaled HiDPI output. */
    void setUseUnscaledHiDPIOutput(bool fUseUnscaledHiDPIOutput) { m_fUseUnscaledHiDPIOutput = fUseUnscaledHiDPIOutput; }

    /** Returns frame-buffer scaling optimization type. */
    ScalingOptimizationType scalingOptimizationType() const { return m_enmScalingOptimizationType; }
    /** Defines frame-buffer scaling optimization type: */
    void setScalingOptimizationType(ScalingOptimizationType type) { m_enmScalingOptimizationType = type; }

    DECLARE_NOT_AGGREGATABLE(UIFrameBufferPrivate)

    DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_COM_MAP(UIFrameBufferPrivate)
        COM_INTERFACE_ENTRY(IFramebuffer)
        COM_INTERFACE_ENTRY2(IDispatch,IFramebuffer)
        COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pUnkMarshaler.m_p)
    END_COM_MAP()

    HRESULT FinalConstruct();
    void FinalRelease();

    STDMETHOD(COMGETTER(Width))(ULONG *puWidth);
    STDMETHOD(COMGETTER(Height))(ULONG *puHeight);
    STDMETHOD(COMGETTER(BitsPerPixel))(ULONG *puBitsPerPixel);
    STDMETHOD(COMGETTER(BytesPerLine))(ULONG *puBytesPerLine);
    STDMETHOD(COMGETTER(PixelFormat))(BitmapFormat_T *puPixelFormat);
    STDMETHOD(COMGETTER(HeightReduction))(ULONG *puHeightReduction);
    STDMETHOD(COMGETTER(Overlay))(IFramebufferOverlay **ppOverlay);
    STDMETHOD(COMGETTER(WinId))(LONG64 *pWinId);
    STDMETHOD(COMGETTER(Capabilities))(ComSafeArrayOut(FramebufferCapabilities_T, aCapabilities));

    /** EMT callback: Notifies frame-buffer about guest-screen size change.
      * @param        uScreenId Guest screen number.
      * @param        uX        Horizontal origin of the update rectangle, in pixels.
      * @param        uY        Vertical origin of the update rectangle, in pixels.
      * @param        uWidth    Width of the guest display, in pixels.
      * @param        uHeight   Height of the guest display, in pixels.
      * @note         Any EMT callback is subsequent. No any other EMT callback can be called until this one processed.
      * @note         Calls to this and #setMarkAsUnused method are synchronized (from GUI side). */
    STDMETHOD(NotifyChange)(ULONG uScreenId, ULONG uX, ULONG uY, ULONG uWidth, ULONG uHeight);

    /** EMT callback: Notifies frame-buffer about guest-screen update.
      * @param        uX      Horizontal origin of the update rectangle, in pixels.
      * @param        uY      Vertical origin of the update rectangle, in pixels.
      * @param        uWidth  Width of the update rectangle, in pixels.
      * @param        uHeight Height of the update rectangle, in pixels.
      * @note         Any EMT callback is subsequent. No any other EMT callback can be called until this one processed.
      * @note         Calls to this and #setMarkAsUnused method are synchronized (from GUI side). */
    STDMETHOD(NotifyUpdate)(ULONG uX, ULONG uY, ULONG uWidth, ULONG uHeight);

    /** EMT callback: Notifies frame-buffer about guest-screen update.
      * @param        uX      Horizontal origin of the update rectangle, in pixels.
      * @param        uY      Vertical origin of the update rectangle, in pixels.
      * @param        uWidth  Width of the update rectangle, in pixels.
      * @param        uHeight Height of the update rectangle, in pixels.
      * @param        image   Brings image container which can be used to copy data from.
      * @note         Any EMT callback is subsequent. No any other EMT callback can be called until this one processed.
      * @note         Calls to this and #setMarkAsUnused method are synchronized (from GUI side). */
    STDMETHOD(NotifyUpdateImage)(ULONG uX, ULONG uY, ULONG uWidth, ULONG uHeight, ComSafeArrayIn(BYTE, image));

    /** EMT callback: Returns whether the frame-buffer implementation is willing to support a given video-mode.
      * @param        uWidth      Width of the guest display, in pixels.
      * @param        uHeight     Height of the guest display, in pixels.
      * @param        uBPP        Color depth, bits per pixel.
      * @param        pfSupported Is frame-buffer able/willing to render the video mode or not.
      * @note         Any EMT callback is subsequent. No any other EMT callback can be called until this one processed.
      * @note         Calls to this and #setMarkAsUnused method are synchronized (from GUI side). */
    STDMETHOD(VideoModeSupported)(ULONG uWidth, ULONG uHeight, ULONG uBPP, BOOL *pbSupported);

    /** EMT callback which is not used in current implementation. */
    STDMETHOD(GetVisibleRegion)(BYTE *pRectangles, ULONG uCount, ULONG *puCountCopied);
    /** EMT callback: Suggests new visible-region to this frame-buffer.
      * @param        pRectangles Pointer to the RTRECT array.
      * @param        uCount      Number of RTRECT elements in the rectangles array.
      * @note         Any EMT callback is subsequent. No any other EMT callback can be called until this one processed.
      * @note         Calls to this and #setMarkAsUnused method are synchronized (from GUI side). */
    STDMETHOD(SetVisibleRegion)(BYTE *pRectangles, ULONG uCount);

    /** EMT callback which is not used in current implementation. */
    STDMETHOD(ProcessVHWACommand)(BYTE *pCommand, LONG enmCmd, BOOL fGuestCmd);

    /** EMT callback: Notifies frame-buffer about 3D backend event.
      * @param        uType Event type. VBOX3D_NOTIFY_TYPE_*.
      * @param        aData Event-specific data, depends on the supplied event type.
      * @note         Any EMT callback is subsequent. No any other EMT callback can be called until this one processed.
      * @note         Calls to this and #setMarkAsUnused method are synchronized (from GUI side). */
    STDMETHOD(Notify3DEvent)(ULONG uType, ComSafeArrayIn(BYTE, data));

    /** Locks frame-buffer access. */
    void lock() const { RTCritSectEnter(&m_critSect); }
    /** Unlocks frame-buffer access. */
    void unlock() const { RTCritSectLeave(&m_critSect); }

    /** Handles frame-buffer notify-change-event. */
    virtual void handleNotifyChange(int iWidth, int iHeight);
    /** Handles frame-buffer paint-event. */
    virtual void handlePaintEvent(QPaintEvent *pEvent);
    /** Handles frame-buffer set-visible-region-event. */
    virtual void handleSetVisibleRegion(const QRegion &region);

    /** Performs frame-buffer resizing. */
    virtual void performResize(int iWidth, int iHeight);
    /** Performs frame-buffer rescaling. */
    virtual void performRescale();

    /** Handles viewport resize-event. */
    virtual void viewportResized(QResizeEvent*)
    {
#ifdef VBOX_GUI_WITH_QTGLFRAMEBUFFER
        /* Sync GL widget size with the MachineView widget: */
        /** @todo This can be probably done in a more automated way. */
        if (m_pGLWidget && m_pMachineView)
            m_pGLWidget->resize(m_pMachineView->viewport()->size());
#endif
    }

#ifdef VBOX_GUI_WITH_QTGLFRAMEBUFFER
    bool isGLWidgetSupported()
    {
        QString s = uiCommon().virtualBox().GetExtraData("GUI/GLWidget");
        return s == "1" && GLWidget::isSupported();
    }
#endif

protected slots:

    /** Handles guest requests to change mouse pointer shape or position. */
    void sltMousePointerShapeOrPositionChange();

protected:

    /** Prepare connections routine. */
    void prepareConnections();
    /** Cleanup connections routine. */
    void cleanupConnections();

    /** Updates coordinate-system: */
    void updateCoordinateSystem();

    /** Default paint routine. */
    void paintDefault(QPaintEvent *pEvent);
    /** Paint routine for seamless mode. */
    void paintSeamless(QPaintEvent *pEvent);

    /** Returns the transformation mode corresponding to the passed @a dScaleFactor and ScalingOptimizationType. */
    static Qt::TransformationMode transformationMode(ScalingOptimizationType type, double dScaleFactor = 0);

    /** Erases corresponding @a rect with @a painter. */
    static void eraseImageRect(QPainter &painter, const QRect &rect,
                               double dDevicePixelRatio);
    /** Draws corresponding @a rect of passed @a image with @a painter. */
    static void drawImageRect(QPainter &painter, const QImage &image, const QRect &rect,
                              int iContentsShiftX, int iContentsShiftY,
                              double dDevicePixelRatio);

    /** Holds the screen-id. */
    ulong m_uScreenId;

    /** Holds the QImage buffer. */
    QImage m_image;
    /** Holds frame-buffer width. */
    int m_iWidth;
    /** Holds frame-buffer height. */
    int m_iHeight;

    /** Holds the copy of the IDisplay wrapper. */
    CDisplay m_display;
    /** Source bitmap from IDisplay. */
    CDisplaySourceBitmap m_sourceBitmap;
    /** Source bitmap from IDisplay (acquired but not yet applied). */
    CDisplaySourceBitmap m_pendingSourceBitmap;
    /** Holds whether there is a pending source bitmap which must be applied. */
    bool m_fPendingSourceBitmap;

    /** Holds machine-view this frame-buffer is bounded to. */
    UIMachineView *m_pMachineView;
    /** Holds window ID this frame-buffer referring to. */
    int64_t m_iWinId;

    /** Reflects whether screen-updates are allowed. */
    bool m_fUpdatesAllowed;

    /** Defines whether frame-buffer is <b>unused</b>.
      * <b>Unused</b> status determines whether frame-buffer should ignore EMT events or not. */
    bool m_fUnused;

    /** RTCRITSECT object to protect frame-buffer access. */
    mutable RTCRITSECT m_critSect;

    /** @name Scale-factor related variables.
     * @{ */
    /** Holds the scale-factor used by the scaled-size. */
    double m_dScaleFactor;
    /** Holds the scaling optimization type used by the scaling mechanism. */
    ScalingOptimizationType m_enmScalingOptimizationType;
    /** Holds the coordinate-system for the scale-factor above. */
    QTransform m_transform;
    /** Holds the frame-buffer's scaled-size. */
    QSize m_scaledSize;
    /** @} */

    /** @name Seamless mode related variables.
     * @{ */
    /* To avoid a seamless flicker which caused by the latency between
     * the initial visible-region arriving from EMT thread
     * and actual visible-region appliance on GUI thread
     * it was decided to use two visible-region instances: */
    /** Sync visible-region which being updated synchronously by locking EMT thread.
      * Used for immediate manual clipping of the painting operations. */
    QRegion m_syncVisibleRegion;
    /** Async visible-region which being updated asynchronously by posting async-event from EMT to GUI thread,
      * Used to update viewport parts for visible-region changes,
      * because NotifyUpdate doesn't take into account these changes. */
    QRegion m_asyncVisibleRegion;
    /** When the frame-buffer is being resized, visible region is saved here.
      * The saved region is applied when updates are enabled again. */
    QRegion m_pendingSyncVisibleRegion;
    /** @} */

    /** @name HiDPI screens related variables.
     * @{ */
    /** Holds device-pixel-ratio set for HiDPI frame-buffer. */
    double m_dDevicePixelRatio;
    /** Holds actual device-pixel-ratio set for HiDPI frame-buffer. */
    double m_dDevicePixelRatioActual;
    /** Holds whether frame-buffer should use unscaled HiDPI output. */
    bool m_fUseUnscaledHiDPIOutput;
    /** @} */

#ifdef VBOX_GUI_WITH_QTGLFRAMEBUFFER
    GLWidget *m_pGLWidget;
#endif

private:

#ifdef Q_OS_WIN
     ComPtr<IUnknown> m_pUnkMarshaler;
#endif /* Q_OS_WIN */
     /** Identifier returned by AttachFramebuffer. Used in DetachFramebuffer. */
     QUuid m_uFramebufferId;

     /** Holds the last cursor rectangle. */
     QRect  m_cursorRectangle;
};


#ifdef VBOX_GUI_WITH_QTGLFRAMEBUFFER
#define GLCHECK() \
do { \
    int glErr = glGetError(); \
    if (glErr != GL_NO_ERROR) LogRel4(("GUI GL 0x%x @%d\n", glErr, __LINE__)); \
} while(0)

GLWidgetSource::GLWidgetSource(GLWidget *pTarget)
    : m_pTarget(pTarget)
{
}

GLWidgetSource::~GLWidgetSource()
{
    cleanup();
}

GLWidgetSourceImage::GLWidgetSourceImage(GLWidget *pTarget, QImage *pImage)
    : GLWidgetSource(pTarget)
    , m_pImage(pImage)
{
}

GLWidgetSourceImage::~GLWidgetSourceImage()
{
}

void GLWidgetSourceImage::initGuestScreenTexture(int w, int h)
{
    glTexImage2D(GLWidget::kTextureTarget, 0, GL_RGBA, w, h, 0, GL_BGRA, GL_UNSIGNED_BYTE, NULL);
    GLCHECK();
}

void GLWidgetSourceImage::updateGuestImage()
{
    /* Copy the image content to the texture. */
    glTexSubImage2D(GLWidget::kTextureTarget, 0, 0, 0, m_pImage->width(), m_pImage->height(),
                    GL_BGRA, GL_UNSIGNED_BYTE, m_pImage->bits());
    GLCHECK();
}

# ifdef RT_OS_LINUX
GLWidgetSourcePixmap::GLWidgetSourcePixmap(GLWidget *pTarget, Pixmap pixmap, VisualID visualid)
    : GLWidgetSource(pTarget)
    , m_Pixmap(pixmap)
    , m_visualid(visualid)
    , m_glxPixmap(0)
    , m_display(0)
    , m_pfnglXBindTexImageEXT(0)
    , m_pfnglXReleaseTexImageEXT(0)
{
}

GLWidgetSourcePixmap::~GLWidgetSourcePixmap()
{
}

void GLWidgetSourcePixmap::cleanup()
{
    m_pfnglXBindTexImageEXT = 0;
    m_pfnglXReleaseTexImageEXT = 0;
    m_Pixmap = 0;
    m_visualid = 0;

    if (m_glxPixmap)
    {
        glXDestroyPixmap(m_display, m_glxPixmap);
        m_glxPixmap = 0;
    }

    if (m_display)
    {
        XCloseDisplay(m_display);
        m_display = 0;
    }
}

void GLWidgetSourcePixmap::initGuestScreenTexture(int w, int h)
{
    RT_NOREF(w, h);

    LogRel4(("GUI: GLWidgetSourcePixmap::initGuestScreenTexture: Search for vid = %lu\n", m_visualid));

    if (m_display)
        return; /* Already initialized. */

    m_display = XOpenDisplay(0);
    if (m_display)
    {
        const char *glXExt = glXQueryExtensionsString(m_display, 0);
        if (glXExt && RTStrStr(glXExt, "GLX_EXT_texture_from_pixmap"))
        {
            m_pfnglXBindTexImageEXT = (PFNGLXBINDTEXIMAGEEXTPROC)glXGetProcAddress((const GLubyte *)"glXBindTexImageEXT");
            m_pfnglXReleaseTexImageEXT = (PFNGLXRELEASETEXIMAGEEXTPROC)glXGetProcAddress((const GLubyte *)"glXReleaseTexImageEXT");
            if (m_pfnglXBindTexImageEXT && m_pfnglXReleaseTexImageEXT)
            {
                LogRelMax(1, ("GUI: GLX_EXT_texture_from_pixmap supported\n"));

                /* FBConfig attributes. */
                static int const aConfigAttribList[] =
                {
                    // GLX_RENDER_TYPE,                 GLX_RGBA_BIT,
                    // GLX_X_VISUAL_TYPE,               GLX_TRUE_COLOR,
                    // GLX_X_RENDERABLE,                True,                   // Render to GLX pixmaps
                    GLX_DRAWABLE_TYPE,               GLX_PIXMAP_BIT,         // Must support GLX pixmaps
                    GLX_BIND_TO_TEXTURE_RGBA_EXT,    True,                   // Must support GLX_EXT_texture_from_pixmap
                    GLX_BIND_TO_TEXTURE_TARGETS_EXT, GLX_TEXTURE_2D_BIT_EXT, // Must support GL_TEXTURE_2D because the device creates the pixmap as TEXTURE_2D
                    GLX_DOUBLEBUFFER,                False,                  // No need for double buffering for a pixmap.
                    GLX_RED_SIZE,                    8,                      // True color RGB with 8 bits per channel.
                    GLX_GREEN_SIZE,                  8,
                    GLX_BLUE_SIZE,                   8,
                    GLX_ALPHA_SIZE,                  8,
                    GLX_STENCIL_SIZE,                0,                      // No stencil buffer
                    GLX_DEPTH_SIZE,                  0,                      // No depth buffer
                    None
                };

                /* Find a suitable FB config. */
                int cConfigs = 0;
                GLXFBConfig *paConfigs = glXChooseFBConfig(m_display, 0, aConfigAttribList, &cConfigs);
                LogRel4(("GUI: GLWidgetSourcePixmap::initGuestScreenTexture: paConfigs %p cConfigs %d\n", (void *)paConfigs, cConfigs));
                if (paConfigs)
                {
                    XVisualInfo *vi = NULL;
                    int i = 0;
                    for (; i < cConfigs; ++i)
                    {
                        /* Use XFree to free the data returned in the previous iteration of this loop. */
                        if (vi)
                            XFree(vi);

                        vi = glXGetVisualFromFBConfig(m_display, paConfigs[i]);
                        if (!vi)
                            continue;

                        LogRel4(("GUI: GLWidgetSourcePixmap::initGuestScreenTexture: %p vid %lu screen %d depth %d r %lu g %lu b %lu clrmap %d bitsperrgb %d\n",
                                 (void *)vi->visual, vi->visualid, vi->screen, vi->depth,
                                 vi->red_mask, vi->green_mask, vi->blue_mask, vi->colormap_size, vi->bits_per_rgb));

                        if (vi->visualid != m_visualid)
                            continue;

                        /* This FB config can be used. */
                        break;
                    }

                    if (vi)
                    {
                        XFree(vi);
                        vi = 0;
                    }

                    if (i < cConfigs)
                    {
                        /* Found the requested config. */
                        static int const aPixmapAttribList[] =
                        {
                            GLX_TEXTURE_TARGET_EXT, GLX_TEXTURE_2D_EXT,
                            GLX_TEXTURE_FORMAT_EXT, GLX_TEXTURE_FORMAT_RGBA_EXT,
                            None
                        };
                        m_glxPixmap = glXCreatePixmap(m_display, paConfigs[i], m_Pixmap, aPixmapAttribList);
                        LogRel4(("GUI: GLWidgetSourcePixmap::initGuestScreenTexture: m_glxPixmap %ld\n", m_glxPixmap));

                        m_pfnglXBindTexImageEXT(m_display, m_glxPixmap, GLX_FRONT_LEFT_EXT, NULL);

                        /* "Use XFree to free the memory returned by glXChooseFBConfig." */
                        XFree(paConfigs);

                        /* Success. */
                        return;
                    }

                    LogRel4(("GUI: GLWidgetSourcePixmap::initGuestScreenTexture: fbconfig not found\n"));
                    /* "Use XFree to free the memory returned by glXChooseFBConfig." */
                    XFree(paConfigs);
                }
            }

            m_pfnglXBindTexImageEXT = 0;
            m_pfnglXReleaseTexImageEXT = 0;
        }

        XCloseDisplay(m_display);
        m_display = 0;
    }
    else
    {
        LogRel4(("GUI: GLWidgetSourcePixmap::initGuestScreenTexture: failed to open Display\n"));
    }
}

void GLWidgetSourcePixmap::uninitGuestScreenTexture()
{
    if (!m_glxPixmap)
        return;

    AssertReturnVoid(m_display && m_pfnglXReleaseTexImageEXT);
    m_pfnglXReleaseTexImageEXT(m_display, m_glxPixmap, GLX_FRONT_LEFT_EXT);
}
# endif /* RT_OS_LINUX */

GLWidget::GLWidget(QWidget *parent, UIFrameBufferPrivate *pFramebuffer)
    : QOpenGLWidget(parent)
    , m_pFramebuffer(pFramebuffer)
    , m_nullSource(this)
    , m_pSource(0)
    , m_fReinitSource(false)
    , m_guestTexture(0)
{
    int rc = RTCritSectInit(&m_critSect);
    AssertRC(rc);

    setMouseTracking(true);

#if 0
    QSurfaceFormat format;
    format.setVersion(3, 3);
    //format.setProfile(QSurfaceFormat::CoreProfile);
    format.setRenderableType(QSurfaceFormat::OpenGL);
    format.setRedBufferSize(8);
    format.setGreenBufferSize(8);
    format.setBlueBufferSize(8);
    format.setAlphaBufferSize(8);
    format.setDepthBufferSize(0);
    format.setStencilBufferSize(0);
    format.setSwapInterval(0);
    format.setSwapBehavior(QSurfaceFormat::DoubleBuffer);
    setFormat(format);
#endif
}

GLWidget::~GLWidget()
{
    cleanup();

    RTCritSectDelete(&m_critSect);
    RT_ZERO(m_critSect);
}

/* Whether OpenGL is usable.
 * OpenGL 2.0 required.
 */
/* static */ bool GLWidget::isSupported()
{
    /* Create an OpenGL conntext: */
    QOpenGLContext contextGL;
    contextGL.create();
    if (!contextGL.isValid())
        return false;

    /* Create an offscreen surface: */
    QOffscreenSurface surface;
    surface.create();
    if (!surface.isValid())
        return false;

    /* Make the OpenGL context current: */
    contextGL.makeCurrent(&surface);

    /* Get the OpenGL version: */
    char const *pszVersion = (char const *)contextGL.functions()->glGetString(GL_VERSION);
    size_t cchVersion = pszVersion ? strlen(pszVersion) : 0;

    int const verMajor = cchVersion >= 1 && '0' <= pszVersion[0] && pszVersion[0] <= '9'? pszVersion[0] - '0' : 0;
    int const verMinor = cchVersion >= 3 && '0' <= pszVersion[2] && pszVersion[2] <= '9'? pszVersion[2] - '0' : 0;
    int const ver = verMajor * 10 + verMinor;

    /* Check if GL_TEXTURE_RECTANGLE is supported: */
    //bool const fTextureRectangle = contextGL.hasExtension("GL_ARB_texture_rectangle")
    //                            || contextGL.hasExtension("GL_NV_texture_rectangle")
    //                            || ver >= 31;

    /* Reset the current OpenGL context: */
    contextGL.doneCurrent();

    /* Decide if OpenGL support is good enough: */
    return ver >= 20 /* && fTextureRectangle */;
}

/** @todo fForce is a bit of a hack. It does not allow to change the HW source to the QImage source,
 * when QImage source is automatically set during the guest screen resize. Think again!
 */
void GLWidget::setSource(GLWidgetSource *pSource, bool fForce)
{
    lock();
    if (   !fForce
        && m_pSource
        && m_pSource->IsHW())
    {
        LogRel4(("GUI: GLWidgetSourcePixmap::setSource: keeping HW source\n"));
        unlock();
        return;
    }

    if (m_pSource)
        delete m_pSource;

    m_pSource = pSource;
    m_fReinitSource = true;
    unlock();
}

GLWidgetSource *GLWidget::getSource()
{
    Assert(RTCritSectIsOwner(&m_critSect));
    if (m_pSource)
    {
        if (m_fReinitSource)
        {
            m_fReinitSource = false;
            LogRel4(("GUI: GLWidgetSourcePixmap::getSource: recreate guest texture\n"));

            /* If OpenGL context has been created: */
            if (context())
            {
                 /* Delete the current guest texture: */
                 deleteGuestTexture();

                 /* Create and bind the new guest texture: */
                 createGuestTexture();

                 glBindTexture(kTextureTarget, m_guestTexture); GLCHECK();
            }
        }
        return m_pSource;
    }
    return &m_nullSource;
}

void GLWidget::resizeGuestScreen(int w, int h)
{
    /* The guest screen has been resized. Remember the size: */
    m_guestSize = QSize(w, h);
}

void GLWidget::setGuestVisibleRect(int x, int y, int w, int h)
{
    /* Remember the area of the guest screen which must be displayed: */
    m_guestVisibleRect.setRect(x, y, w, h);
}

void GLWidget::updateGuestImage()
{
    /* If OpenGL context has been created: */
    if (!context())
        return;

    makeCurrent();

    lock();
    GLWidgetSource *pSource = getSource();
    if (m_guestTexture)
    {
        /* Copy the image content to the texture. */
        glBindTexture(kTextureTarget, m_guestTexture);
        GLCHECK();

        pSource->updateGuestImage();
    }
    unlock();

    doneCurrent();
}

void GLWidget::cleanup()
{
    if (!RTCritSectIsInitialized(&m_critSect))
        return;

    /* If OpenGL context has been created: */
    if (!context())
        return;

    makeCurrent();

    lock();
    getSource()->cleanup();
    setSource(0, true);
    unlock();

    /* Delete all OpenGL resources which are used by this widget: */
    deleteGuestTexture();

    doneCurrent();
}

void GLWidget::initializeGL()
{
    /* QOpenGLWidget documentation recommends to connect to the context's aboutToBeDestroyed() signal.
     * See https://doc.qt.io/qt-5/qopenglwidget.html#details
     * Connect the signal: */
    connect(context(), &QOpenGLContext::aboutToBeDestroyed, this, &GLWidget::cleanup);

    /* Required initialization for QOpenGLFunctions: */
    initializeOpenGLFunctions();

    /* Create OpenGL resources: */
    createGuestTexture();

    /* Setup the OpenGL context state: */
    glClearColor(0, 0, 0, 1); GLCHECK();
    glDisable(GL_DEPTH_TEST); GLCHECK();
    glDisable(GL_CULL_FACE);  GLCHECK();
}

void GLWidget::paintGL()
{
    lock();
    if (m_guestTexture)
    {
        /* Dimensions of the target window, i.e. the widget's dimensions. */
        GLint const w = width();
        GLint const h = height();

        /* The guest coordinates of the visible guest screen area: */
        float x1 = m_guestVisibleRect.x();
        float y1 = m_guestVisibleRect.y();
        float x2 = x1 + m_guestVisibleRect.width();
        float y2 = y1 + m_guestVisibleRect.height();

        x1 /= (float)m_guestSize.width();
        y1 /= (float)m_guestSize.height();
        x2 /= (float)m_guestSize.width();
        y2 /= (float)m_guestSize.height();

        glDisable(GL_DEPTH_TEST); GLCHECK();
        glDisable(GL_CULL_FACE); GLCHECK();

        glEnable(kTextureTarget); GLCHECK();

        /* Bind the guest texture: */
        glBindTexture(kTextureTarget, m_guestTexture); GLCHECK();

        /* This will reinitialize the source if necessary. */
        getSource();

        /* Draw the texture (upside down, because QImage and OpenGL store the bitmap differently): */
        glBegin(GL_QUADS);
        glTexCoord2f(x1, y1); glVertex2i(0, h);
        glTexCoord2f(x1, y2); glVertex2i(0, 0);
        glTexCoord2f(x2, y2); glVertex2i(w, 0);
        glTexCoord2f(x2, y1); glVertex2i(w, h);
        glEnd(); GLCHECK();

        glBindTexture(kTextureTarget, 0); GLCHECK();

        glDisable(kTextureTarget); GLCHECK();

        glFlush(); GLCHECK();
    }
    unlock();
}

void GLWidget::resizeGL(int w, int h)
{
    /* Setup ModelViewProjection to work in the window cordinates: */
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glOrtho(0, w, 0, h, -1, 1);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    GLCHECK();
}

void GLWidget::createGuestTexture()
{
    if (m_guestSize.isEmpty())
        return;

    /* Choose GL_NEAREST if no scaling or the scaling factor is an integer: */
    double const scaleFactor = m_pFramebuffer->scaleFactor();
    GLenum const filter = floor(scaleFactor) == scaleFactor ? GL_NEAREST : GL_LINEAR;

    /* Create a new guest texture, which must be the same size as the guest screen: */
    glGenTextures(1, &m_guestTexture);
    glEnable(kTextureTarget); GLCHECK();
    glBindTexture(kTextureTarget, m_guestTexture);
    glTexParameteri(kTextureTarget, GL_TEXTURE_MAG_FILTER, filter);
    glTexParameteri(kTextureTarget, GL_TEXTURE_MIN_FILTER, filter);

    lock();
    getSource()->initGuestScreenTexture(m_guestSize.width(), m_guestSize.height());
    unlock();

    glBindTexture(kTextureTarget, 0);
    GLCHECK();
    glDisable(kTextureTarget); GLCHECK();
}

void GLWidget::deleteGuestTexture()
{
    if (m_guestTexture)
    {
        glBindTexture(kTextureTarget, m_guestTexture);

        lock();
        getSource()->uninitGuestScreenTexture();
        unlock();

        glBindTexture(kTextureTarget, 0); GLCHECK();
        glDeleteTextures(1, &m_guestTexture); GLCHECK();
        m_guestTexture = 0;
    }
}
#endif /* VBOX_GUI_WITH_QTGLFRAMEBUFFER */


#ifdef VBOX_WITH_XPCOM
NS_DECL_CLASSINFO(UIFrameBufferPrivate)
NS_IMPL_THREADSAFE_ISUPPORTS1_CI(UIFrameBufferPrivate, IFramebuffer)
#endif /* VBOX_WITH_XPCOM */


UIFrameBufferPrivate::UIFrameBufferPrivate()
    : m_uScreenId(0)
    , m_iWidth(0), m_iHeight(0)
    , m_fPendingSourceBitmap(false)
    , m_pMachineView(NULL)
    , m_iWinId(0)
    , m_fUpdatesAllowed(false)
    , m_fUnused(false)
    , m_dScaleFactor(1.0)
    , m_enmScalingOptimizationType(ScalingOptimizationType_None)
    , m_dDevicePixelRatio(1.0)
    , m_dDevicePixelRatioActual(1.0)
    , m_fUseUnscaledHiDPIOutput(false)
#ifdef VBOX_GUI_WITH_QTGLFRAMEBUFFER
    , m_pGLWidget(0)
#endif
{
    /* Update coordinate-system: */
    updateCoordinateSystem();
}

HRESULT UIFrameBufferPrivate::init(UIMachineView *pMachineView)
{
    LogRel2(("GUI: UIFrameBufferPrivate::init %p\n", this));

    /* Assign mahine-view: */
    m_pMachineView = pMachineView;

    /* Assign index: */
    m_uScreenId = m_pMachineView->screenId();

    /* Cache window ID: */
    m_iWinId = (m_pMachineView && m_pMachineView->viewport()) ? (LONG64)m_pMachineView->viewport()->winId() : 0;

#ifdef VBOX_WS_X11
    /* Sync Qt and X11 Server (see xTracker #7547). */
    XSync(NativeWindowSubsystem::X11GetDisplay(), false);
#endif

    /* Assign display: */
    m_display = m_pMachineView->uisession()->display();

    /* Initialize critical-section: */
    int rc = RTCritSectInit(&m_critSect);
    AssertRC(rc);

    /* Connect handlers: */
    if (m_pMachineView)
        prepareConnections();

#ifdef VBOX_GUI_WITH_QTGLFRAMEBUFFER
    /* Decide if we are going to use GL to draw the guest screen: */
    if (isGLWidgetSupported())
        m_pGLWidget = new GLWidget(m_pMachineView->viewport(), this);
#endif

    /* Resize/rescale frame-buffer to the default size: */
    performResize(640, 480);
    performRescale();

#ifdef Q_OS_WIN
    CoCreateFreeThreadedMarshaler(this, m_pUnkMarshaler.asOutParam());
#endif /* Q_OS_WIN */
    return S_OK;
}

UIFrameBufferPrivate::~UIFrameBufferPrivate()
{
    LogRel2(("GUI: UIFrameBufferPrivate::~UIFrameBufferPrivate %p\n", this));

    /* Disconnect handlers: */
    if (m_pMachineView)
        cleanupConnections();

    /* Deinitialize critical-section: */
    RTCritSectDelete(&m_critSect);
}

void UIFrameBufferPrivate::setView(UIMachineView *pMachineView)
{
    /* Disconnect old handlers: */
    if (m_pMachineView)
        cleanupConnections();

    /* Reassign machine-view: */
    m_pMachineView = pMachineView;
    /* Recache window ID: */
    m_iWinId = (m_pMachineView && m_pMachineView->viewport()) ? (LONG64)m_pMachineView->viewport()->winId() : 0;

#ifdef VBOX_WS_X11
    /* Sync Qt and X11 Server (see xTracker #7547). */
    XSync(NativeWindowSubsystem::X11GetDisplay(), false);
#endif

    /* Connect new handlers: */
    if (m_pMachineView)
        prepareConnections();

#ifdef VBOX_GUI_WITH_QTGLFRAMEBUFFER
    /* Decide if we are going to use GL to draw the guest screen: */
    m_pGLWidget = 0;
    if (m_pMachineView && isGLWidgetSupported())
        m_pGLWidget = new GLWidget(m_pMachineView->viewport(), this);
#endif
}

void UIFrameBufferPrivate::attach()
{
    m_uFramebufferId = display().AttachFramebuffer(m_uScreenId, CFramebuffer(this));
}

void UIFrameBufferPrivate::detach()
{
    CFramebuffer frameBuffer = display().QueryFramebuffer(m_uScreenId);
    if (!frameBuffer.isNull())
    {
        display().DetachFramebuffer(m_uScreenId, m_uFramebufferId);
        m_uFramebufferId = QUuid();
    }
}

void UIFrameBufferPrivate::setMarkAsUnused(bool fUnused)
{
    lock();
    m_fUnused = fUnused;
    unlock();
}

HRESULT UIFrameBufferPrivate::FinalConstruct()
{
    return 0;
}

void UIFrameBufferPrivate::FinalRelease()
{
    return;
}

STDMETHODIMP UIFrameBufferPrivate::COMGETTER(Width)(ULONG *puWidth)
{
    if (!puWidth)
        return E_POINTER;
    *puWidth = (ULONG)width();
    return S_OK;
}

STDMETHODIMP UIFrameBufferPrivate::COMGETTER(Height)(ULONG *puHeight)
{
    if (!puHeight)
        return E_POINTER;
    *puHeight = (ULONG)height();
    return S_OK;
}

STDMETHODIMP UIFrameBufferPrivate::COMGETTER(BitsPerPixel)(ULONG *puBitsPerPixel)
{
    if (!puBitsPerPixel)
        return E_POINTER;
    *puBitsPerPixel = bitsPerPixel();
    return S_OK;
}

STDMETHODIMP UIFrameBufferPrivate::COMGETTER(BytesPerLine)(ULONG *puBytesPerLine)
{
    if (!puBytesPerLine)
        return E_POINTER;
    *puBytesPerLine = bytesPerLine();
    return S_OK;
}

STDMETHODIMP UIFrameBufferPrivate::COMGETTER(PixelFormat)(BitmapFormat_T *puPixelFormat)
{
    if (!puPixelFormat)
        return E_POINTER;
    *puPixelFormat = (BitmapFormat_T)pixelFormat();
    return S_OK;
}

STDMETHODIMP UIFrameBufferPrivate::COMGETTER(HeightReduction)(ULONG *puHeightReduction)
{
    if (!puHeightReduction)
        return E_POINTER;
    *puHeightReduction = 0;
    return S_OK;
}

STDMETHODIMP UIFrameBufferPrivate::COMGETTER(Overlay)(IFramebufferOverlay **ppOverlay)
{
    if (!ppOverlay)
        return E_POINTER;
    *ppOverlay = 0;
    return S_OK;
}

STDMETHODIMP UIFrameBufferPrivate::COMGETTER(WinId)(LONG64 *pWinId)
{
    if (!pWinId)
        return E_POINTER;
    *pWinId = m_iWinId;
    return S_OK;
}

STDMETHODIMP UIFrameBufferPrivate::COMGETTER(Capabilities)(ComSafeArrayOut(FramebufferCapabilities_T, enmCapabilities))
{
    if (ComSafeArrayOutIsNull(enmCapabilities))
        return E_POINTER;

    com::SafeArray<FramebufferCapabilities_T> caps;
    if (uiCommon().isSeparateProcess())
    {
       caps.resize(2);
       caps[0] = FramebufferCapabilities_UpdateImage;
       caps[1] = FramebufferCapabilities_RenderCursor;
    }
    else
    {
       caps.resize(3);
       caps[0] = FramebufferCapabilities_VHWA;
       caps[1] = FramebufferCapabilities_VisibleRegion;
       caps[2] = FramebufferCapabilities_RenderCursor;
    }

    caps.detachTo(ComSafeArrayOutArg(enmCapabilities));
    return S_OK;
}

STDMETHODIMP UIFrameBufferPrivate::NotifyChange(ULONG uScreenId, ULONG uX, ULONG uY, ULONG uWidth, ULONG uHeight)
{
    CDisplaySourceBitmap sourceBitmap;
    if (!uiCommon().isSeparateProcess())
        display().QuerySourceBitmap(uScreenId, sourceBitmap);

    /* Lock access to frame-buffer: */
    lock();

    /* Make sure frame-buffer is used: */
    if (m_fUnused)
    {
        LogRel(("GUI: UIFrameBufferPrivate::NotifyChange: Screen=%lu, Origin=%lux%lu, Size=%lux%lu, Ignored!\n",
                (unsigned long)uScreenId,
                (unsigned long)uX, (unsigned long)uY,
                (unsigned long)uWidth, (unsigned long)uHeight));

        /* Unlock access to frame-buffer: */
        unlock();

        /* Ignore NotifyChange: */
        return E_FAIL;
    }

    /* Disable screen updates: */
    m_fUpdatesAllowed = false;

    /* While updates are disabled, visible region will be saved:  */
    m_pendingSyncVisibleRegion = QRegion();

    if (!uiCommon().isSeparateProcess())
    {
       /* Acquire new pending bitmap: */
       m_pendingSourceBitmap = sourceBitmap;
       m_fPendingSourceBitmap = true;
    }

    /* Widget resize is NOT thread-safe and *probably* never will be,
     * We have to notify machine-view with the async-signal to perform resize operation. */
    LogRel2(("GUI: UIFrameBufferPrivate::NotifyChange: Screen=%lu, Origin=%lux%lu, Size=%lux%lu, Sending to async-handler\n",
             (unsigned long)uScreenId,
             (unsigned long)uX, (unsigned long)uY,
             (unsigned long)uWidth, (unsigned long)uHeight));
    emit sigNotifyChange(uWidth, uHeight);

    /* Unlock access to frame-buffer: */
    unlock();

    /* Give up control token to other thread: */
    RTThreadYield();

    /* Confirm NotifyChange: */
    return S_OK;
}

STDMETHODIMP UIFrameBufferPrivate::NotifyUpdate(ULONG uX, ULONG uY, ULONG uWidth, ULONG uHeight)
{
    /* Lock access to frame-buffer: */
    lock();

    /* Make sure frame-buffer is used: */
    if (m_fUnused)
    {
        LogRel3(("GUI: UIFrameBufferPrivate::NotifyUpdate: Origin=%lux%lu, Size=%lux%lu, Ignored!\n",
                 (unsigned long)uX, (unsigned long)uY,
                 (unsigned long)uWidth, (unsigned long)uHeight));
        /* Unlock access to frame-buffer: */
        unlock();

        /* Ignore NotifyUpdate: */
        return E_FAIL;
    }

    /* Widget update is NOT thread-safe and *seems* never will be,
     * We have to notify machine-view with the async-signal to perform update operation. */
    LogRel3(("GUI: UIFrameBufferPrivate::NotifyUpdate: Origin=%lux%lu, Size=%lux%lu, Sending to async-handler\n",
             (unsigned long)uX, (unsigned long)uY,
             (unsigned long)uWidth, (unsigned long)uHeight));
    emit sigNotifyUpdate(uX, uY, uWidth, uHeight);

    /* Unlock access to frame-buffer: */
    unlock();

    /* Confirm NotifyUpdate: */
    return S_OK;
}

STDMETHODIMP UIFrameBufferPrivate::NotifyUpdateImage(ULONG uX, ULONG uY,
                                                     ULONG uWidth, ULONG uHeight,
                                                     ComSafeArrayIn(BYTE, image))
{
    /* Wrapping received data: */
    com::SafeArray<BYTE> imageData(ComSafeArrayInArg(image));

    /* Lock access to frame-buffer: */
    lock();

    /* Make sure frame-buffer is used: */
    if (m_fUnused)
    {
        LogRel3(("GUI: UIFrameBufferPrivate::NotifyUpdateImage: Origin=%lux%lu, Size=%lux%lu, Ignored!\n",
                 (unsigned long)uX, (unsigned long)uY,
                 (unsigned long)uWidth, (unsigned long)uHeight));

        /* Unlock access to frame-buffer: */
        unlock();

        /* Ignore NotifyUpdate: */
        return E_FAIL;
    }
    /* Directly update m_image if update fits: */
    if (   m_fUpdatesAllowed
        && uX + uWidth <= (ULONG)m_image.width()
        && uY + uHeight <= (ULONG)m_image.height())
    {
        /* Copy to m_image: */
        uchar *pu8Dst = m_image.bits() + uY * m_image.bytesPerLine() + uX * 4;
        uchar *pu8Src = imageData.raw();
        ULONG h;
        for (h = 0; h < uHeight; ++h)
        {
            memcpy(pu8Dst, pu8Src, uWidth * 4);
            pu8Dst += m_image.bytesPerLine();
            pu8Src += uWidth * 4;
        }

        /* Widget update is NOT thread-safe and *seems* never will be,
         * We have to notify machine-view with the async-signal to perform update operation. */
        LogRel3(("GUI: UIFrameBufferPrivate::NotifyUpdateImage: Origin=%lux%lu, Size=%lux%lu, Sending to async-handler\n",
                 (unsigned long)uX, (unsigned long)uY,
                 (unsigned long)uWidth, (unsigned long)uHeight));
        emit sigNotifyUpdate(uX, uY, uWidth, uHeight);
    }

    /* Unlock access to frame-buffer: */
    unlock();

    /* Confirm NotifyUpdateImage: */
    return S_OK;
}

STDMETHODIMP UIFrameBufferPrivate::VideoModeSupported(ULONG uWidth, ULONG uHeight, ULONG uBPP, BOOL *pfSupported)
{
    /* Make sure result pointer is valid: */
    if (!pfSupported)
    {
        LogRel2(("GUI: UIFrameBufferPrivate::IsVideoModeSupported: Mode: BPP=%lu, Size=%lux%lu, Invalid pfSupported pointer!\n",
                 (unsigned long)uBPP, (unsigned long)uWidth, (unsigned long)uHeight));

        return E_POINTER;
    }

    /* Lock access to frame-buffer: */
    lock();

    /* Make sure frame-buffer is used: */
    if (m_fUnused)
    {
        LogRel2(("GUI: UIFrameBufferPrivate::IsVideoModeSupported: Mode: BPP=%lu, Size=%lux%lu, Ignored!\n",
                 (unsigned long)uBPP, (unsigned long)uWidth, (unsigned long)uHeight));

        /* Unlock access to frame-buffer: */
        unlock();

        /* Ignore VideoModeSupported: */
        return E_FAIL;
    }

    /* Determine if supported: */
    *pfSupported = TRUE;
    const QSize screenSize = m_pMachineView->maximumGuestSize();
    if (   (screenSize.width() != 0)
        && (uWidth > (ULONG)screenSize.width())
        && (uWidth > (ULONG)width()))
        *pfSupported = FALSE;
    if (   (screenSize.height() != 0)
        && (uHeight > (ULONG)screenSize.height())
        && (uHeight > (ULONG)height()))
        *pfSupported = FALSE;
    if (*pfSupported)
       LogRel2(("GUI: UIFrameBufferPrivate::IsVideoModeSupported: Mode: BPP=%lu, Size=%lux%lu is supported\n",
                (unsigned long)uBPP, (unsigned long)uWidth, (unsigned long)uHeight));
    else
       LogRel(("GUI: UIFrameBufferPrivate::IsVideoModeSupported: Mode: BPP=%lu, Size=%lux%lu is NOT supported\n",
               (unsigned long)uBPP, (unsigned long)uWidth, (unsigned long)uHeight));

    /* Unlock access to frame-buffer: */
    unlock();

    /* Confirm VideoModeSupported: */
    return S_OK;
}

STDMETHODIMP UIFrameBufferPrivate::GetVisibleRegion(BYTE *pRectangles, ULONG uCount, ULONG *puCountCopied)
{
    PRTRECT rects = (PRTRECT)pRectangles;

    if (!rects)
        return E_POINTER;

    Q_UNUSED(uCount);
    Q_UNUSED(puCountCopied);

    return S_OK;
}

STDMETHODIMP UIFrameBufferPrivate::SetVisibleRegion(BYTE *pRectangles, ULONG uCount)
{
    /* Make sure rectangles were passed: */
    if (!pRectangles)
    {
        LogRel2(("GUI: UIFrameBufferPrivate::SetVisibleRegion: Rectangle count=%lu, Invalid pRectangles pointer!\n",
                 (unsigned long)uCount));

        return E_POINTER;
    }

    /* Lock access to frame-buffer: */
    lock();

    /* Make sure frame-buffer is used: */
    if (m_fUnused)
    {
        LogRel2(("GUI: UIFrameBufferPrivate::SetVisibleRegion: Rectangle count=%lu, Ignored!\n",
                 (unsigned long)uCount));

        /* Unlock access to frame-buffer: */
        unlock();

        /* Ignore SetVisibleRegion: */
        return E_FAIL;
    }

    /* Compose region: */
    QRegion region;
    PRTRECT rects = (PRTRECT)pRectangles;
    for (ULONG uIndex = 0; uIndex < uCount; ++uIndex)
    {
        /* Get current rectangle: */
        QRect rect;
        rect.setLeft(rects->xLeft);
        rect.setTop(rects->yTop);
        /* Which is inclusive: */
        rect.setRight(rects->xRight - 1);
        rect.setBottom(rects->yBottom - 1);
        /* Append region: */
        region += rect;
        ++rects;
    }
    /* Tune according scale-factor: */
    if (scaleFactor() != 1.0 || devicePixelRatio() > 1.0)
        region = m_transform.map(region);

    if (m_fUpdatesAllowed)
    {
        /* We are directly updating synchronous visible-region: */
        m_syncVisibleRegion = region;
        /* And send async-signal to update asynchronous one: */
        LogRel2(("GUI: UIFrameBufferPrivate::SetVisibleRegion: Rectangle count=%lu, Sending to async-handler\n",
                 (unsigned long)uCount));
        emit sigSetVisibleRegion(region);
    }
    else
    {
        /* Save the region. */
        m_pendingSyncVisibleRegion = region;
        LogRel2(("GUI: UIFrameBufferPrivate::SetVisibleRegion: Rectangle count=%lu, Saved\n",
                 (unsigned long)uCount));
    }

    /* Unlock access to frame-buffer: */
    unlock();

    /* Confirm SetVisibleRegion: */
    return S_OK;
}

STDMETHODIMP UIFrameBufferPrivate::ProcessVHWACommand(BYTE *pCommand, LONG enmCmd, BOOL fGuestCmd)
{
    RT_NOREF(pCommand, enmCmd, fGuestCmd);
    return E_NOTIMPL;
}

STDMETHODIMP UIFrameBufferPrivate::Notify3DEvent(ULONG uType, ComSafeArrayIn(BYTE, data))
{
    /* Lock access to frame-buffer: */
    lock();

    /* Make sure frame-buffer is used: */
    if (m_fUnused)
    {
        LogRel2(("GUI: UIFrameBufferPrivate::Notify3DEvent: Ignored!\n"));

        /* Unlock access to frame-buffer: */
        unlock();

        /* Ignore Notify3DEvent: */
        return E_FAIL;
    }

    Q_UNUSED(data);
#ifdef VBOX_WITH_XPCOM
    Q_UNUSED(dataSize);
#endif /* VBOX_WITH_XPCOM */
    // com::SafeArray<BYTE> eventData(ComSafeArrayInArg(data));
    switch (uType)
    {
        case VBOX3D_NOTIFY_TYPE_3DDATA_VISIBLE:
        case VBOX3D_NOTIFY_TYPE_3DDATA_HIDDEN:
        {
            /// @todo wipe out whole case when confirmed
            // We are no more supporting this, am I right?
            AssertFailed();
            /* Confirm Notify3DEvent: */
            return S_OK;
        }

        case VBOX3D_NOTIFY_TYPE_TEST_FUNCTIONAL:
        {
            HRESULT hr = m_fUnused ? E_FAIL : S_OK;
            unlock();
            return hr;
        }

#if defined(VBOX_GUI_WITH_QTGLFRAMEBUFFER) && defined(RT_OS_LINUX)
        case VBOX3D_NOTIFY_TYPE_HW_SCREEN_CREATED:
        case VBOX3D_NOTIFY_TYPE_HW_SCREEN_DESTROYED:
        case VBOX3D_NOTIFY_TYPE_HW_SCREEN_UPDATE_BEGIN:
        case VBOX3D_NOTIFY_TYPE_HW_SCREEN_UPDATE_END:
        {
            HRESULT hr = S_OK;
            com::SafeArray<BYTE> notifyData(ComSafeArrayInArg(data));
            if (m_pGLWidget)
            {
                if (uType == VBOX3D_NOTIFY_TYPE_HW_SCREEN_CREATED)
                {
                    LogRel4(("GUI: Notify3DEvent VBOX3D_NOTIFY_TYPE_3D_SCREEN_CREATED\n"));

                    struct NotifyData
                    {
                        uint64_t u64NativeHandle;
                        VisualID visualid;
                    };
                    struct NotifyData *pData = (struct NotifyData *)notifyData.raw();

                    GLWidgetSource *p = new GLWidgetSourcePixmap(m_pGLWidget, (Pixmap)pData->u64NativeHandle, pData->visualid);
                    m_pGLWidget->setSource(p, true);

                    LogRelMax(1, ("GUI: Created a HW accelerated screen\n"));
                }
                else if (uType == VBOX3D_NOTIFY_TYPE_HW_SCREEN_DESTROYED)
                {
                    LogRel4(("GUI: Notify3DEvent VBOX3D_NOTIFY_TYPE_3D_SCREEN_DESTROYED\n"));

                    GLWidgetSource *p = new GLWidgetSourceImage(m_pGLWidget, &m_image);
                    m_pGLWidget->setSource(p, true);
                }
                else if (uType == VBOX3D_NOTIFY_TYPE_HW_SCREEN_UPDATE_BEGIN)
                {
                    /* Do nothing. */
                }
                else if (uType == VBOX3D_NOTIFY_TYPE_HW_SCREEN_UPDATE_END)
                {
                    struct NotifyData
                    {
                        uint64_t u64NativeHandle;
                        int32_t left;
                        int32_t top;
                        int32_t right;
                        int32_t bottom;
                    };
                    struct NotifyData *pData = (struct NotifyData *)notifyData.raw();

                    /* Send the screen update message. */
                    int iX = pData->left;
                    int iY = pData->top;
                    int iWidth = pData->right - pData->left;
                    int iHeight = pData->bottom - pData->top;
                    emit sigNotifyUpdate(iX, iY, iWidth, iHeight);
                }
            }
            else
            {
                hr = E_FAIL; // Not supported
            }
            unlock();
            return hr;
        }
#endif /* defined(VBOX_GUI_WITH_QTGLFRAMEBUFFER) && defined(RT_OS_LINUX) */

        default:
            break;
    }

    /* Unlock access to frame-buffer: */
    unlock();

    /* Ignore Notify3DEvent: */
    return E_INVALIDARG;
}

void UIFrameBufferPrivate::handleNotifyChange(int iWidth, int iHeight)
{
    LogRel2(("GUI: UIFrameBufferPrivate::handleNotifyChange: Size=%dx%d\n", iWidth, iHeight));

    /* Make sure machine-view is assigned: */
    AssertPtrReturnVoid(m_pMachineView);

    /* Lock access to frame-buffer: */
    lock();

    /* If there is NO pending source-bitmap: */
    if (!uiCommon().isSeparateProcess() && !m_fPendingSourceBitmap)
    {
        /* Do nothing, change-event already processed: */
        LogRel2(("GUI: UIFrameBufferPrivate::handleNotifyChange: Already processed.\n"));
        /* Unlock access to frame-buffer: */
        unlock();
        /* Return immediately: */
        return;
    }

    /* Release the current bitmap and keep the pending one: */
    m_sourceBitmap = m_pendingSourceBitmap;
    m_pendingSourceBitmap = 0;
    m_fPendingSourceBitmap = false;

    /* Unlock access to frame-buffer: */
    unlock();

    /* Perform frame-buffer resize: */
    performResize(iWidth, iHeight);
}

void UIFrameBufferPrivate::handlePaintEvent(QPaintEvent *pEvent)
{
    LogRel3(("GUI: UIFrameBufferPrivate::handlePaintEvent: Origin=%lux%lu, Size=%dx%d\n",
             pEvent->rect().x(), pEvent->rect().y(),
             pEvent->rect().width(), pEvent->rect().height()));

    /* On mode switch the enqueued paint-event may still come
     * while the machine-view is already null (before the new machine-view set),
     * ignore paint-event in that case. */
    if (!m_pMachineView)
        return;

    /* Lock access to frame-buffer: */
    lock();

    /* But if updates disabled: */
    if (!m_fUpdatesAllowed)
    {
        /* Unlock access to frame-buffer: */
        unlock();
        /* And return immediately: */
        return;
    }

    /* Depending on visual-state type: */
    switch (m_pMachineView->machineLogic()->visualStateType())
    {
        case UIVisualStateType_Seamless:
            paintSeamless(pEvent);
            break;
        default:
            paintDefault(pEvent);
            break;
    }

    /* Unlock access to frame-buffer: */
    unlock();
}

void UIFrameBufferPrivate::handleSetVisibleRegion(const QRegion &region)
{
    /* Make sure async visible-region has changed or wasn't yet applied: */
    if (   m_asyncVisibleRegion == region
#ifdef VBOX_WITH_MASKED_SEAMLESS
        && m_asyncVisibleRegion == m_pMachineView->machineWindow()->mask()
#endif /* VBOX_WITH_MASKED_SEAMLESS */
           )
        return;

    /* We are accounting async visible-regions one-by-one
     * to keep corresponding viewport area always updated! */
    if (!m_asyncVisibleRegion.isEmpty())
        m_pMachineView->viewport()->update(m_asyncVisibleRegion - region);

    /* Remember last visible region: */
    m_asyncVisibleRegion = region;

#ifdef VBOX_WITH_MASKED_SEAMLESS
    /* We have to use async visible-region to apply to [Q]Widget [set]Mask: */
    m_pMachineView->machineWindow()->setMask(m_asyncVisibleRegion);
#endif /* VBOX_WITH_MASKED_SEAMLESS */
}

void UIFrameBufferPrivate::performResize(int iWidth, int iHeight)
{
    /* Make sure machine-view is assigned: */
    AssertReturnVoidStmt(m_pMachineView, LogRel(("GUI: UIFrameBufferPrivate::performResize: Size=%dx%d\n", iWidth, iHeight)));

    /* Invalidate visible-region (if necessary): */
    if (m_pMachineView->machineLogic()->visualStateType() == UIVisualStateType_Seamless &&
        (m_iWidth != iWidth || m_iHeight != iHeight))
    {
        lock();
        m_syncVisibleRegion = QRegion();
        m_asyncVisibleRegion = QRegion();
        unlock();
    }

    /* If source-bitmap invalid: */
    if (m_sourceBitmap.isNull())
    {
        /* Remember new size came from hint: */
        m_iWidth = iWidth;
        m_iHeight = iHeight;
        LogRel(("GUI: UIFrameBufferPrivate::performResize: Size=%dx%d, Using fallback buffer since no source bitmap is provided\n",
                m_iWidth, m_iHeight));

        /* And recreate fallback buffer: */
        m_image = QImage(m_iWidth, m_iHeight, QImage::Format_RGB32);
        m_image.fill(0);
    }
    /* If source-bitmap valid: */
    else
    {
        /* Acquire source-bitmap attributes: */
        BYTE *pAddress = NULL;
        ULONG ulWidth = 0;
        ULONG ulHeight = 0;
        ULONG ulBitsPerPixel = 0;
        ULONG ulBytesPerLine = 0;
        KBitmapFormat bitmapFormat = KBitmapFormat_Opaque;
        m_sourceBitmap.QueryBitmapInfo(pAddress,
                                       ulWidth,
                                       ulHeight,
                                       ulBitsPerPixel,
                                       ulBytesPerLine,
                                       bitmapFormat);
        Assert(ulBitsPerPixel == 32);

        /* Remember new actual size: */
        m_iWidth = (int)ulWidth;
        m_iHeight = (int)ulHeight;
        LogRel2(("GUI: UIFrameBufferPrivate::performResize: Size=%dx%d, Directly using source bitmap content\n",
                 m_iWidth, m_iHeight));

        /* Recreate QImage on the basis of source-bitmap content: */
        m_image = QImage(pAddress, m_iWidth, m_iHeight, ulBytesPerLine, QImage::Format_RGB32);

        /* Check whether guest color depth differs from the bitmap color depth: */
        ULONG ulGuestBitsPerPixel = 0;
        LONG xOrigin = 0;
        LONG yOrigin = 0;
        KGuestMonitorStatus monitorStatus = KGuestMonitorStatus_Enabled;
        display().GetScreenResolution(m_uScreenId, ulWidth, ulHeight, ulGuestBitsPerPixel, xOrigin, yOrigin, monitorStatus);

        /* Remind user if necessary, ignore text and VGA modes: */
        /* This check (supports graphics) is not quite right due to past mistakes
         * in the Guest Additions protocol, but in practice it should be fine. */
        if (   ulGuestBitsPerPixel != ulBitsPerPixel
            && ulGuestBitsPerPixel != 0
            && m_pMachineView->uisession()->isGuestSupportsGraphics())
            UINotificationMessage::remindAboutWrongColorDepth(ulGuestBitsPerPixel, ulBitsPerPixel);
        else
            UINotificationMessage::forgetAboutWrongColorDepth();
    }

#ifdef VBOX_GUI_WITH_QTGLFRAMEBUFFER
   if (m_pGLWidget)
   {
       m_pGLWidget->resizeGuestScreen(m_iWidth, m_iHeight);

       GLWidgetSource *p = new GLWidgetSourceImage(m_pGLWidget, &m_image);
       m_pGLWidget->setSource(p, false);
   }
#endif

    lock();

    /* Enable screen updates: */
    m_fUpdatesAllowed = true;

    if (!m_pendingSyncVisibleRegion.isEmpty())
    {
        /* Directly update synchronous visible-region: */
        m_syncVisibleRegion = m_pendingSyncVisibleRegion;
        m_pendingSyncVisibleRegion = QRegion();

        /* And send async-signal to update asynchronous one: */
        LogRel2(("GUI: UIFrameBufferPrivate::performResize: Rectangle count=%lu, Sending to async-handler\n",
                 (unsigned long)m_syncVisibleRegion.rectCount()));
        emit sigSetVisibleRegion(m_syncVisibleRegion);
    }

    /* Make sure that the current screen image is immediately displayed: */
    m_pMachineView->viewport()->update();

    unlock();

    /* Make sure action-pool knows frame-buffer size: */
    m_pMachineView->uisession()->actionPool()->toRuntime()->setGuestScreenSize(m_pMachineView->screenId(),
                                                                               QSize(m_iWidth, m_iHeight));
}

void UIFrameBufferPrivate::performRescale()
{
//    printf("UIFrameBufferPrivate::performRescale\n");

    /* Make sure machine-view is assigned: */
    AssertPtrReturnVoid(m_pMachineView);

    /* Depending on current visual state: */
    switch (m_pMachineView->machineLogic()->visualStateType())
    {
        case UIVisualStateType_Scale:
            m_scaledSize = scaledSize().width() == m_iWidth && scaledSize().height() == m_iHeight ? QSize() : scaledSize();
            break;
        default:
            m_scaledSize = scaleFactor() == 1.0 ? QSize() : QSize((int)(m_iWidth * scaleFactor()), (int)(m_iHeight * scaleFactor()));
            break;
    }

    /* Update coordinate-system: */
    updateCoordinateSystem();

//    printf("UIFrameBufferPrivate::performRescale: Complete: Scale-factor=%f, Scaled-size=%dx%d\n",
//           scaleFactor(), scaledSize().width(), scaledSize().height());
}

void UIFrameBufferPrivate::sltMousePointerShapeOrPositionChange()
{
    /* Do we have view and valid cursor position?
     * Also, please take into account, we are not currently painting
     * framebuffer cursor if mouse integration is supported and enabled. */
    if (   m_pMachineView
        && !m_pMachineView->uisession()->isHidingHostPointer()
        && m_pMachineView->uisession()->isValidPointerShapePresent()
        && m_pMachineView->uisession()->isValidCursorPositionPresent()
        && (   !m_pMachineView->uisession()->isMouseIntegrated()
            || !m_pMachineView->uisession()->isMouseSupportsAbsolute()))
    {
        /* Acquire cursor hotspot: */
        QPoint cursorHotspot = m_pMachineView->uisession()->cursorHotspot();
        /* Apply the scale-factor if necessary: */
        cursorHotspot /= scaleFactor();
        /* Take the device-pixel-ratio into account: */
        if (!useUnscaledHiDPIOutput())
            cursorHotspot /= devicePixelRatioActual();

        /* Acquire cursor position and size: */
        QPoint cursorPosition = m_pMachineView->uisession()->cursorPosition() - cursorHotspot;
        QSize cursorSize = m_pMachineView->uisession()->cursorSize();
        /* Apply the scale-factor if necessary: */
        cursorPosition *= scaleFactor();
        cursorSize *= scaleFactor();
        /* Take the device-pixel-ratio into account: */
        if (!useUnscaledHiDPIOutput())
        {
            cursorPosition *= devicePixelRatioActual();
            cursorSize *= devicePixelRatioActual();
        }
        cursorPosition /= devicePixelRatio();
        cursorSize /= devicePixelRatio();

        /* Call for a viewport update, we need to update cumulative
         * region containing previous and current cursor rectagles. */
        const QRect cursorRectangle = QRect(cursorPosition, cursorSize);
        m_pMachineView->viewport()->update(QRegion(m_cursorRectangle) + cursorRectangle);

        /* Remember current cursor rectangle: */
        m_cursorRectangle = cursorRectangle;
    }
    /* Don't forget to clear the rectangle in opposite case: */
    else if (   m_pMachineView
             && m_cursorRectangle.isValid())
    {
        /* Call for a cursor area update: */
        m_pMachineView->viewport()->update(m_cursorRectangle);
    }
}

void UIFrameBufferPrivate::prepareConnections()
{
    /* Attach EMT connections: */
    connect(this, &UIFrameBufferPrivate::sigNotifyChange,
            m_pMachineView, &UIMachineView::sltHandleNotifyChange,
            Qt::QueuedConnection);
    connect(this, &UIFrameBufferPrivate::sigNotifyUpdate,
            m_pMachineView, &UIMachineView::sltHandleNotifyUpdate,
            Qt::QueuedConnection);
    connect(this, &UIFrameBufferPrivate::sigSetVisibleRegion,
            m_pMachineView, &UIMachineView::sltHandleSetVisibleRegion,
            Qt::QueuedConnection);

    /* Attach GUI connections: */
    connect(m_pMachineView->uisession(), &UISession::sigMousePointerShapeChange,
            this, &UIFrameBufferPrivate::sltMousePointerShapeOrPositionChange);
    connect(m_pMachineView->uisession(), &UISession::sigCursorPositionChange,
            this, &UIFrameBufferPrivate::sltMousePointerShapeOrPositionChange);
}

void UIFrameBufferPrivate::cleanupConnections()
{
    /* Detach EMT connections: */
    disconnect(this, &UIFrameBufferPrivate::sigNotifyChange,
               m_pMachineView, &UIMachineView::sltHandleNotifyChange);
    disconnect(this, &UIFrameBufferPrivate::sigNotifyUpdate,
               m_pMachineView, &UIMachineView::sltHandleNotifyUpdate);
    disconnect(this, &UIFrameBufferPrivate::sigSetVisibleRegion,
               m_pMachineView, &UIMachineView::sltHandleSetVisibleRegion);

    /* Detach GUI connections: */
    disconnect(m_pMachineView->uisession(), &UISession::sigMousePointerShapeChange,
               this, &UIFrameBufferPrivate::sltMousePointerShapeOrPositionChange);
    disconnect(m_pMachineView->uisession(), &UISession::sigCursorPositionChange,
               this, &UIFrameBufferPrivate::sltMousePointerShapeOrPositionChange);
}

void UIFrameBufferPrivate::updateCoordinateSystem()
{
    /* Reset to default: */
    m_transform = QTransform();

    /* Apply the scale-factor if necessary: */
    if (scaleFactor() != 1.0)
        m_transform = m_transform.scale(scaleFactor(), scaleFactor());

    /* Take the device-pixel-ratio into account: */
    if (!useUnscaledHiDPIOutput())
        m_transform = m_transform.scale(devicePixelRatioActual(), devicePixelRatioActual());
    m_transform = m_transform.scale(1.0 / devicePixelRatio(), 1.0 / devicePixelRatio());
}

void UIFrameBufferPrivate::paintDefault(QPaintEvent *pEvent)
{
    /* Make sure cached image is valid: */
    if (m_image.isNull())
        return;

#ifdef VBOX_GUI_WITH_QTGLFRAMEBUFFER
    if (m_pGLWidget)
    {
        /* Draw the actually visible guest rectangle on the entire GLWidget.
         * This code covers non-HiDPI normal and scaled modes. Scrollbars work too. */

        /** @todo HiDPI support. Possibly need to split the geometry calculations from the QImage handling below
         *        and share the geometry code with the OpenGL code path. */

        /* Set the visible guest rectangle: */
        m_pGLWidget->setGuestVisibleRect(m_pMachineView->contentsX(), m_pMachineView->contentsY(),
                                         convertHostXTo(m_pGLWidget->width()), convertHostYTo(m_pGLWidget->height()));

        /* Tell the GL Widget to update the guest screen content from the source: */
        m_pGLWidget->updateGuestImage();

        /* Redraw: */
        m_pGLWidget->update();

        /* Done: */
        return;
    }
#endif /* VBOX_GUI_WITH_QTGLFRAMEBUFFER */

    /* First we take the cached image as the source: */
    QImage *pSourceImage = &m_image;

    /* But if we should scale image by some reason: */
    if (   scaledSize().isValid()
        || (!useUnscaledHiDPIOutput() && devicePixelRatioActual() != 1.0))
    {
        /* Calculate final scaled size: */
        QSize effectiveSize = !scaledSize().isValid() ? pSourceImage->size() : scaledSize();
        /* Take the device-pixel-ratio into account: */
        if (!useUnscaledHiDPIOutput() && devicePixelRatioActual() != 1.0)
            effectiveSize *= devicePixelRatioActual();
        /* We scale the image to requested size and retain it
         * by making heap shallow copy of that temporary object: */
        switch (m_pMachineView->visualStateType())
        {
            case UIVisualStateType_Scale:
                pSourceImage = new QImage(pSourceImage->scaled(effectiveSize, Qt::IgnoreAspectRatio,
                                                               transformationMode(scalingOptimizationType())));
                break;
            default:
                pSourceImage = new QImage(pSourceImage->scaled(effectiveSize, Qt::IgnoreAspectRatio,
                                                               transformationMode(scalingOptimizationType(), m_dScaleFactor)));
                break;
        }
    }

    /* Take the device-pixel-ratio into account: */
    pSourceImage->setDevicePixelRatio(devicePixelRatio());

    /* Prepare the base and hidpi paint rectangles: */
    const QRect paintRect = pEvent->rect();
    QRect paintRectHiDPI = paintRect;

    /* Take the device-pixel-ratio into account: */
    paintRectHiDPI.moveTo(paintRectHiDPI.topLeft() * devicePixelRatio());
    paintRectHiDPI.setSize(paintRectHiDPI.size() * devicePixelRatio());

    /* Make sure hidpi paint rectangle is within the image boundary: */
    paintRectHiDPI = paintRectHiDPI.intersected(pSourceImage->rect());
    if (paintRectHiDPI.isEmpty())
        return;

    /* Create painter: */
    QPainter painter(m_pMachineView->viewport());

#ifdef VBOX_WS_MAC
    /* On OSX for Qt5 we need to fill the backing store first: */
    painter.setCompositionMode(QPainter::CompositionMode_Source);
    painter.fillRect(paintRect, QColor(Qt::black));
    painter.setCompositionMode(QPainter::CompositionMode_SourceAtop);
#endif /* VBOX_WS_MAC */

    /* Draw hidpi image rectangle: */
    drawImageRect(painter, *pSourceImage, paintRectHiDPI,
                  m_pMachineView->contentsX(), m_pMachineView->contentsY(),
                  devicePixelRatio());

    /* If we had to scale image for some reason: */
    if (   scaledSize().isValid()
        || (!useUnscaledHiDPIOutput() && devicePixelRatioActual() != 1.0))
    {
        /* Wipe out copied image: */
        delete pSourceImage;
        pSourceImage = 0;
    }

    /* Paint cursor if it has valid shape and position.
     * Also, please take into account, we are not currently painting
     * framebuffer cursor if mouse integration is supported and enabled. */
    if (   !m_cursorRectangle.isNull()
        && !m_pMachineView->uisession()->isHidingHostPointer()
        && m_pMachineView->uisession()->isValidPointerShapePresent()
        && m_pMachineView->uisession()->isValidCursorPositionPresent()
        && (   !m_pMachineView->uisession()->isMouseIntegrated()
            || !m_pMachineView->uisession()->isMouseSupportsAbsolute()))
    {
        /* Acquire session cursor shape pixmap: */
        QPixmap cursorPixmap = m_pMachineView->uisession()->cursorShapePixmap();

        /* Take the device-pixel-ratio into account: */
        cursorPixmap.setDevicePixelRatio(devicePixelRatio());

        /* Draw sub-pixmap: */
        painter.drawPixmap(m_cursorRectangle.topLeft(), cursorPixmap);
    }
}

void UIFrameBufferPrivate::paintSeamless(QPaintEvent *pEvent)
{
    /* Make sure cached image is valid: */
    if (m_image.isNull())
        return;

    /* First we take the cached image as the source: */
    QImage *pSourceImage = &m_image;

    /* But if we should scale image by some reason: */
    if (   scaledSize().isValid()
        || (!useUnscaledHiDPIOutput() && devicePixelRatioActual() != 1.0))
    {
        /* Calculate final scaled size: */
        QSize effectiveSize = !scaledSize().isValid() ? pSourceImage->size() : scaledSize();
        /* Take the device-pixel-ratio into account: */
        if (!useUnscaledHiDPIOutput() && devicePixelRatioActual() != 1.0)
            effectiveSize *= devicePixelRatioActual();
        /* We scale the image to requested size and retain it
         * by making heap shallow copy of that temporary object: */
        switch (m_pMachineView->visualStateType())
        {
            case UIVisualStateType_Scale:
                pSourceImage = new QImage(pSourceImage->scaled(effectiveSize, Qt::IgnoreAspectRatio,
                                                               transformationMode(scalingOptimizationType())));
                break;
            default:
                pSourceImage = new QImage(pSourceImage->scaled(effectiveSize, Qt::IgnoreAspectRatio,
                                                               transformationMode(scalingOptimizationType(), m_dScaleFactor)));
                break;
        }
    }

    /* Take the device-pixel-ratio into account: */
    pSourceImage->setDevicePixelRatio(devicePixelRatio());

    /* Prepare the base and hidpi paint rectangles: */
    const QRect paintRect = pEvent->rect();
    QRect paintRectHiDPI = paintRect;

    /* Take the device-pixel-ratio into account: */
    paintRectHiDPI.moveTo(paintRectHiDPI.topLeft() * devicePixelRatio());
    paintRectHiDPI.setSize(paintRectHiDPI.size() * devicePixelRatio());

    /* Make sure hidpi paint rectangle is within the image boundary: */
    paintRectHiDPI = paintRectHiDPI.intersected(pSourceImage->rect());
    if (paintRectHiDPI.isEmpty())
        return;

    /* Create painter: */
    QPainter painter(m_pMachineView->viewport());

    /* Adjust painter for erasing: */
    lock();
    painter.setClipRegion(QRegion(paintRect) - m_syncVisibleRegion);
    painter.setCompositionMode(QPainter::CompositionMode_Clear);
    unlock();

    /* Erase hidpi rectangle: */
    eraseImageRect(painter, paintRectHiDPI,
                   devicePixelRatio());

    /* Adjust painter for painting: */
    lock();
    painter.setClipRegion(QRegion(paintRect) & m_syncVisibleRegion);
    painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
    unlock();

#ifdef VBOX_WITH_TRANSLUCENT_SEAMLESS
    /* In case of translucent seamless for Qt5 we need to fill the backing store first: */
    painter.setCompositionMode(QPainter::CompositionMode_Source);
    painter.fillRect(paintRect, QColor(Qt::black));
    painter.setCompositionMode(QPainter::CompositionMode_SourceAtop);
#endif /* VBOX_WITH_TRANSLUCENT_SEAMLESS */

    /* Draw hidpi image rectangle: */
    drawImageRect(painter, *pSourceImage, paintRectHiDPI,
                  m_pMachineView->contentsX(), m_pMachineView->contentsY(),
                  devicePixelRatio());

    /* If we had to scale image for some reason: */
    if (   scaledSize().isValid()
        || (!useUnscaledHiDPIOutput() && devicePixelRatioActual() != 1.0))
    {
        /* Wipe out copied image: */
        delete pSourceImage;
        pSourceImage = 0;
    }

    /* Paint cursor if it has valid shape and position.
     * Also, please take into account, we are not currently painting
     * framebuffer cursor if mouse integration is supported and enabled. */
    if (   !m_cursorRectangle.isNull()
        && !m_pMachineView->uisession()->isHidingHostPointer()
        && m_pMachineView->uisession()->isValidPointerShapePresent()
        && m_pMachineView->uisession()->isValidCursorPositionPresent()
        && (   !m_pMachineView->uisession()->isMouseIntegrated()
            || !m_pMachineView->uisession()->isMouseSupportsAbsolute()))
    {
        /* Acquire session cursor shape pixmap: */
        QPixmap cursorPixmap = m_pMachineView->uisession()->cursorShapePixmap();

        /* Take the device-pixel-ratio into account: */
        cursorPixmap.setDevicePixelRatio(devicePixelRatio());

        /* Draw sub-pixmap: */
        painter.drawPixmap(m_cursorRectangle.topLeft(), cursorPixmap);
    }
}

/* static */
Qt::TransformationMode UIFrameBufferPrivate::transformationMode(ScalingOptimizationType type, double dScaleFactor /* = 0 */)
{
    switch (type)
    {
        /* Check if optimization type is forced to be 'Performance': */
        case ScalingOptimizationType_Performance: return Qt::FastTransformation;
        default: break;
    }
    /* For integer-scaling we are choosing the 'Performance' optimization type ourselves: */
    return dScaleFactor && floor(dScaleFactor) == dScaleFactor ? Qt::FastTransformation : Qt::SmoothTransformation;;
}

/* static */
void UIFrameBufferPrivate::eraseImageRect(QPainter &painter, const QRect &rect,
                                          double dDevicePixelRatio)
{
    /* Prepare sub-pixmap: */
    QPixmap subPixmap = QPixmap(rect.width(), rect.height());
    /* Take the device-pixel-ratio into account: */
    subPixmap.setDevicePixelRatio(dDevicePixelRatio);

    /* Which point we should draw corresponding sub-pixmap? */
    QPoint paintPoint = rect.topLeft();
    /* Take the device-pixel-ratio into account: */
    paintPoint /= dDevicePixelRatio;

    /* Draw sub-pixmap: */
    painter.drawPixmap(paintPoint, subPixmap);
}

/* static */
void UIFrameBufferPrivate::drawImageRect(QPainter &painter, const QImage &image, const QRect &rect,
                                         int iContentsShiftX, int iContentsShiftY,
                                         double dDevicePixelRatio)
{
    /* Calculate offset: */
    const size_t offset = (rect.x() + iContentsShiftX) * image.depth() / 8 +
                          (rect.y() + iContentsShiftY) * image.bytesPerLine();

    /* Restrain boundaries: */
    const int iSubImageWidth = qMin(rect.width(), image.width() - rect.x() - iContentsShiftX);
    const int iSubImageHeight = qMin(rect.height(), image.height() - rect.y() - iContentsShiftY);

    /* Create sub-image (no copy involved): */
    QImage subImage = QImage(image.bits() + offset,
                             iSubImageWidth, iSubImageHeight,
                             image.bytesPerLine(), image.format());

    /* Create sub-pixmap on the basis of sub-image above (1st copy involved): */
    QPixmap subPixmap = QPixmap::fromImage(subImage);
    /* Take the device-pixel-ratio into account: */
    subPixmap.setDevicePixelRatio(dDevicePixelRatio);

    /* Which point we should draw corresponding sub-pixmap? */
    QPoint paintPoint = rect.topLeft();
    /* Take the device-pixel-ratio into account: */
    paintPoint /= dDevicePixelRatio;

    /* Draw sub-pixmap: */
    painter.drawPixmap(paintPoint, subPixmap);
}


UIFrameBuffer::UIFrameBuffer()
{
    m_pFrameBuffer.createObject();
}

UIFrameBuffer::~UIFrameBuffer()
{
    m_pFrameBuffer.setNull();
}

HRESULT UIFrameBuffer::init(UIMachineView *pMachineView)
{
    return m_pFrameBuffer->init(pMachineView);
}

void UIFrameBuffer::attach()
{
    m_pFrameBuffer->attach();
}

void UIFrameBuffer::detach()
{
    m_pFrameBuffer->detach();
}

uchar* UIFrameBuffer::address()
{
    return m_pFrameBuffer->address();
}

ulong UIFrameBuffer::width() const
{
    return m_pFrameBuffer->width();
}

ulong UIFrameBuffer::height() const
{
    return m_pFrameBuffer->height();
}

ulong UIFrameBuffer::bitsPerPixel() const
{
    return m_pFrameBuffer->bitsPerPixel();
}

ulong UIFrameBuffer::bytesPerLine() const
{
    return m_pFrameBuffer->bytesPerLine();
}

UIVisualStateType UIFrameBuffer::visualState() const
{
    return m_pFrameBuffer->visualState();
}

void UIFrameBuffer::setView(UIMachineView *pMachineView)
{
    m_pFrameBuffer->setView(pMachineView);
}

void UIFrameBuffer::setMarkAsUnused(bool fUnused)
{
    m_pFrameBuffer->setMarkAsUnused(fUnused);
}

QSize UIFrameBuffer::scaledSize() const
{
    return m_pFrameBuffer->scaledSize();
}

void UIFrameBuffer::setScaledSize(const QSize &size /* = QSize() */)
{
    m_pFrameBuffer->setScaledSize(size);
}

int UIFrameBuffer::convertHostXTo(int iX) const
{
    return m_pFrameBuffer->convertHostXTo(iX);
}

int UIFrameBuffer::convertHostYTo(int iY) const
{
    return m_pFrameBuffer->convertHostXTo(iY);
}

double UIFrameBuffer::scaleFactor() const
{
    return m_pFrameBuffer->scaleFactor();
}

void UIFrameBuffer::setScaleFactor(double dScaleFactor)
{
    m_pFrameBuffer->setScaleFactor(dScaleFactor);
}

double UIFrameBuffer::devicePixelRatio() const
{
    return m_pFrameBuffer->devicePixelRatio();
}

void UIFrameBuffer::setDevicePixelRatio(double dDevicePixelRatio)
{
    m_pFrameBuffer->setDevicePixelRatio(dDevicePixelRatio);
}

double UIFrameBuffer::devicePixelRatioActual() const
{
    return m_pFrameBuffer->devicePixelRatioActual();
}

void UIFrameBuffer::setDevicePixelRatioActual(double dDevicePixelRatioActual)
{
    m_pFrameBuffer->setDevicePixelRatioActual(dDevicePixelRatioActual);
}

bool UIFrameBuffer::useUnscaledHiDPIOutput() const
{
    return m_pFrameBuffer->useUnscaledHiDPIOutput();
}

void UIFrameBuffer::setUseUnscaledHiDPIOutput(bool fUseUnscaledHiDPIOutput)
{
    m_pFrameBuffer->setUseUnscaledHiDPIOutput(fUseUnscaledHiDPIOutput);
}

ScalingOptimizationType UIFrameBuffer::scalingOptimizationType() const
{
    return m_pFrameBuffer->scalingOptimizationType();
}

void UIFrameBuffer::setScalingOptimizationType(ScalingOptimizationType type)
{
    m_pFrameBuffer->setScalingOptimizationType(type);
}

void UIFrameBuffer::handleNotifyChange(int iWidth, int iHeight)
{
    m_pFrameBuffer->handleNotifyChange(iWidth, iHeight);
}

void UIFrameBuffer::handlePaintEvent(QPaintEvent *pEvent)
{
    m_pFrameBuffer->handlePaintEvent(pEvent);
}

void UIFrameBuffer::handleSetVisibleRegion(const QRegion &region)
{
    m_pFrameBuffer->handleSetVisibleRegion(region);
}

void UIFrameBuffer::performResize(int iWidth, int iHeight)
{
    m_pFrameBuffer->performResize(iWidth, iHeight);
}

void UIFrameBuffer::performRescale()
{
    m_pFrameBuffer->performRescale();
}

void UIFrameBuffer::viewportResized(QResizeEvent *pEvent)
{
    m_pFrameBuffer->viewportResized(pEvent);
}

#include "UIFrameBuffer.moc"

/* $Id: QISplitter.cpp $ */
/** @file
 * VBox Qt GUI - Qt extensions: QISplitter class implementation.
 */

/*
 * Copyright (C) 2009-2023 Oracle and/or its affiliates.
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
#include <QApplication>
#include <QEvent>
#include <QPainter>
#include <QPaintEvent>

/* GUI includes: */
#include "QISplitter.h"
#ifdef VBOX_WS_MAC
# include "UICursor.h"
#endif


/** QSplitterHandle subclass representing flat line. */
class QIFlatSplitterHandle : public QSplitterHandle
{
    Q_OBJECT;

public:

    /** Constructs flat splitter handle passing @a enmOrientation and @a pParent to the base-class. */
    QIFlatSplitterHandle(Qt::Orientation enmOrientation, QISplitter *pParent);

    /** Defines @a color. */
    void configureColor(const QColor &color);

protected:

    /** Handles paint @a pEvent. */
    virtual void paintEvent(QPaintEvent *pEvent) RT_OVERRIDE;

private:

    /** Holds the main color. */
    QColor m_color;
};


/** QSplitterHandle subclass representing shaded line. */
class QIShadeSplitterHandle : public QSplitterHandle
{
    Q_OBJECT;

public:

    /** Constructs shaded splitter handle passing @a enmOrientation and @a pParent to the base-class. */
    QIShadeSplitterHandle(Qt::Orientation enmOrientation, QISplitter *pParent);

    /** Defines colors to passed @a color1 and @a color2. */
    void configureColors(const QColor &color1, const QColor &color2);

protected:

    /** Handles paint @a pEvent. */
    virtual void paintEvent(QPaintEvent *pEvent) RT_OVERRIDE;

private:

    /** Holds the main color. */
    QColor m_color;
    /** Holds the color1. */
    QColor m_color1;
    /** Holds the color2. */
    QColor m_color2;
};


#ifdef VBOX_WS_MAC
/** QSplitterHandle subclass representing shaded line for macOS. */
class QIDarwinSplitterHandle : public QSplitterHandle
{
    Q_OBJECT;

public:

    /** Constructs shaded splitter handle passing @a enmOrientation and @a pParent to the base-class. */
    QIDarwinSplitterHandle(Qt::Orientation enmOrientation, QISplitter *pParent);

    /** Returns size-hint. */
    QSize sizeHint() const;

protected:

    /** Handles paint @a pEvent. */
    virtual void paintEvent(QPaintEvent *pEvent) RT_OVERRIDE;
};
#endif /* VBOX_WS_MAC */


/*********************************************************************************************************************************
*   Class QIFlatSplitterHandle implementation.                                                                                   *
*********************************************************************************************************************************/

QIFlatSplitterHandle::QIFlatSplitterHandle(Qt::Orientation enmOrientation, QISplitter *pParent)
    : QSplitterHandle(enmOrientation, pParent)
{
}

void QIFlatSplitterHandle::configureColor(const QColor &color)
{
    m_color = color;
    update();
}

void QIFlatSplitterHandle::paintEvent(QPaintEvent *pEvent)
{
    QPainter painter(this);
    painter.fillRect(pEvent->rect(), m_color);
}


/*********************************************************************************************************************************
*   Class QIShadeSplitterHandle implementation.                                                                                  *
*********************************************************************************************************************************/

QIShadeSplitterHandle::QIShadeSplitterHandle(Qt::Orientation enmOrientation, QISplitter *pParent)
    : QSplitterHandle(enmOrientation, pParent)
{
    QColor windowColor = QApplication::palette().color(QPalette::Active, QPalette::Window);
    QColor frameColor = QApplication::palette().color(QPalette::Active, QPalette::Text);
    frameColor.setAlpha(100);
    m_color1 = windowColor;
    m_color2 = windowColor;
    m_color = frameColor;
}

void QIShadeSplitterHandle::configureColors(const QColor &color1, const QColor &color2)
{
    m_color1 = color1;
    m_color2 = color2;
    update();
}

void QIShadeSplitterHandle::paintEvent(QPaintEvent *pEvent)
{
    QPainter painter(this);
    QLinearGradient gradient;
    QGradientStop point1(0, m_color1);
    QGradientStop point2(0.5, m_color);
    QGradientStop point3(1, m_color2);
    QGradientStops stops;
    stops << point1 << point2 << point3;
    gradient.setStops(stops);
    if (orientation() == Qt::Horizontal)
    {
        gradient.setStart(rect().left() + 1, 0);
        gradient.setFinalStop(rect().right(), 0);
    }
    else
    {
        gradient.setStart(0, rect().top() + 1);
        gradient.setFinalStop(0, rect().bottom());
    }
    painter.fillRect(pEvent->rect(), gradient);
}


/*********************************************************************************************************************************
*   Class QIDarwinSplitterHandle implementation.                                                                                 *
*********************************************************************************************************************************/

#ifdef VBOX_WS_MAC

QIDarwinSplitterHandle::QIDarwinSplitterHandle(Qt::Orientation enmOrientation, QISplitter *pParent)
    : QSplitterHandle(enmOrientation, pParent)
{
}

QSize QIDarwinSplitterHandle::sizeHint() const
{
    QSize parent = QSplitterHandle::sizeHint();
    if (orientation() == Qt::Vertical)
        return parent + QSize(0, 3);
    else
        return QSize(1, parent.height());
}

void QIDarwinSplitterHandle::paintEvent(QPaintEvent *)
{
    QPainter painter(this);

    QColor topColor(145, 145, 145);
    QColor bottomColor(142, 142, 142);
    QColor gradientStart(252, 252, 252);
    QColor gradientStop(223, 223, 223);

    if (orientation() == Qt::Vertical)
    {
        painter.setPen(topColor);
        painter.drawLine(0, 0, width(), 0);
        painter.setPen(bottomColor);
        painter.drawLine(0, height() - 1, width(), height() - 1);

        QLinearGradient linearGrad(QPointF(0, 0), QPointF(0, height() -3));
        linearGrad.setColorAt(0, gradientStart);
        linearGrad.setColorAt(1, gradientStop);
        painter.fillRect(QRect(QPoint(0,1), size() - QSize(0, 2)), QBrush(linearGrad));
    }
    else
    {
        painter.setPen(topColor);
        painter.drawLine(0, 0, 0, height());
    }
}

#endif /* VBOX_WS_MAC */


/*********************************************************************************************************************************
*   Class QISplitter  implementation.                                                                                            *
*********************************************************************************************************************************/

QISplitter::QISplitter(QWidget *pParent /* = 0 */)
    : QSplitter(pParent)
    , m_enmType(Shade)
    , m_fPolished(false)
#ifdef VBOX_WS_MAC
    , m_fHandleGrabbed(false)
#endif
{
    qApp->installEventFilter(this);
}

QISplitter::QISplitter(Qt::Orientation enmOrientation, Type enmType, QWidget *pParent /* = 0 */)
    : QSplitter(enmOrientation, pParent)
    , m_enmType(enmType)
    , m_fPolished(false)
#ifdef VBOX_WS_MAC
    , m_fHandleGrabbed(false)
#endif
{
    qApp->installEventFilter(this);
}

void QISplitter::configureColor(const QColor &color)
{
    m_color = color;
    for (int i = 1; i < count(); ++i)
    {
        QIFlatSplitterHandle *pHandle = qobject_cast<QIFlatSplitterHandle*>(handle(i));
        if (pHandle && m_color.isValid())
            pHandle->configureColor(m_color);
    }
}

void QISplitter::configureColors(const QColor &color1, const QColor &color2)
{
    m_color1 = color1; m_color2 = color2;
    for (int i = 1; i < count(); ++i)
    {
        QIShadeSplitterHandle *pHandle = qobject_cast<QIShadeSplitterHandle*>(handle(i));
        if (pHandle && m_color1.isValid() && m_color2.isValid())
            pHandle->configureColors(m_color1, m_color2);
    }
}

bool QISplitter::eventFilter(QObject *pWatched, QEvent *pEvent)
{
    /* Handles events for handle: */
    if (pWatched == handle(1))
    {
        switch (pEvent->type())
        {
            /* Restore default position on double-click: */
            case QEvent::MouseButtonDblClick:
                restoreState(m_baseState);
                break;
            default:
                break;
        }
    }

#ifdef VBOX_WS_MAC
    // WORKAROUND:
    // Special handling on the Mac. Cause there the horizontal handle is only 1
    // pixel wide, its hard to catch. Therefor we make some invisible area
    // around the handle and forwarding the mouse events to the handle, if the
    // user presses the left mouse button.
    else if (   m_enmType == Native
             && orientation() == Qt::Horizontal
             && count() > 1
             && qApp->activeWindow() == window())
    {
        switch (pEvent->type())
        {
            case QEvent::MouseButtonPress:
            case QEvent::MouseMove:
            {
                const int margin = 3;
                QMouseEvent *pMouseEvent = static_cast<QMouseEvent*>(pEvent);
                for (int i=1; i < count(); ++i)
                {
                    QWidget *pHandle = handle(i);
                    if (   pHandle
                        && pHandle != pWatched)
                    {
                        /* Check if we hit the handle */
                        bool fMarginHit = QRect(pHandle->mapToGlobal(QPoint(0, 0)), pHandle->size()).adjusted(-margin, 0, margin, 0).contains(pMouseEvent->globalPos());
                        if (pEvent->type() == QEvent::MouseButtonPress)
                        {
                            /* If we have a handle position hit and the left button is pressed, start the grabbing. */
                            if (   fMarginHit
                                && pMouseEvent->buttons().testFlag(Qt::LeftButton))
                            {
                                m_fHandleGrabbed = true;
                                UICursor::setCursor(this, Qt::SplitHCursor);
                                qApp->postEvent(pHandle, new QMouseEvent(pMouseEvent->type(),
                                                                         pHandle->mapFromGlobal(pMouseEvent->globalPos()),
                                                                         pMouseEvent->button(),
                                                                         pMouseEvent->buttons(),
                                                                         pMouseEvent->modifiers()));
                                return true;
                            }
                        }
                        else if (pEvent->type() == QEvent::MouseMove)
                        {
                            /* If we are in the near of the handle or currently dragging, forward the mouse event. */
                            if (   fMarginHit
                                || (   m_fHandleGrabbed
                                    && pMouseEvent->buttons().testFlag(Qt::LeftButton)))
                            {
                                UICursor::setCursor(this, Qt::SplitHCursor);
                                qApp->postEvent(pHandle, new QMouseEvent(pMouseEvent->type(),
                                                                         pHandle->mapFromGlobal(pMouseEvent->globalPos()),
                                                                         pMouseEvent->button(),
                                                                         pMouseEvent->buttons(),
                                                                         pMouseEvent->modifiers()));
                                return true;
                            }

                            /* If not, reset the state. */
                            m_fHandleGrabbed = false;
                            UICursor::setCursor(this, Qt::ArrowCursor);
                        }
                    }
                }
                break;
            }
            case QEvent::WindowDeactivate:
            case QEvent::MouseButtonRelease:
            {
                m_fHandleGrabbed = false;
                UICursor::setCursor(this, Qt::ArrowCursor);
                break;
            }
            default:
                break;
        }
    }
#endif /* VBOX_WS_MAC */

    /* Call to base-class: */
    return QSplitter::eventFilter(pWatched, pEvent);
}

void QISplitter::showEvent(QShowEvent *pEvent)
{
    /* Remember default position: */
    if (!m_fPolished)
    {
        m_fPolished = true;
        m_baseState = saveState();
    }

    /* Call to base-class: */
    return QSplitter::showEvent(pEvent);
}

QSplitterHandle *QISplitter::createHandle()
{
    /* Create native handle: */
    switch (m_enmType)
    {
        case Flat:
        {
            QIFlatSplitterHandle *pHandle = new QIFlatSplitterHandle(orientation(), this);
            if (m_color.isValid())
                pHandle->configureColor(m_color);
            return pHandle;
        }
        case Shade:
        {
            QIShadeSplitterHandle *pHandle = new QIShadeSplitterHandle(orientation(), this);
            if (m_color1.isValid() && m_color2.isValid())
                pHandle->configureColors(m_color1, m_color2);
            return pHandle;
        }
        case Native:
        {
#ifdef VBOX_WS_MAC
            return new QIDarwinSplitterHandle(orientation(), this);
#else
            return new QSplitterHandle(orientation(), this);
#endif
        }
    }
    return 0;
}


#include "QISplitter.moc"

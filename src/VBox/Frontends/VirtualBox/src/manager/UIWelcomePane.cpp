/* $Id: UIWelcomePane.cpp $ */
/** @file
 * VBox Qt GUI - UIWelcomePane class implementation.
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
#include <QHBoxLayout>
#include <QLabel>
#include <QStyle>
#include <QVBoxLayout>

/* GUI includes */
#include "QIWithRetranslateUI.h"
#include "UICommon.h"
#include "UIIconPool.h"
#include "UIWelcomePane.h"

/* Forward declarations: */
class QEvent;
class QHBoxLayout;
class QString;
class QResizeEvent;
class QVBoxLayout;


/** Wrappable QLabel extension for tools pane of the desktop widget.
  * The main idea behind this stuff is to allow dynamically calculate
  * [minimum] size hint for changeable one-the-fly widget width.
  * That's a "white unicorn" task for QLabel which never worked since
  * the beginning, because out-of-the-box version just uses static
  * hints calculation which is very stupid taking into account
  * QLayout "eats it raw" and tries to be dynamical on it's basis. */
class UIWrappableLabel : public QLabel
{
    Q_OBJECT;

public:

    /** Constructs wrappable label passing @a pParent to the base-class. */
    UIWrappableLabel(QWidget *pParent = 0);

protected:

    /** Handles resize @a pEvent. */
    virtual void resizeEvent(QResizeEvent *pEvent) RT_OVERRIDE;

    /** Returns whether the widget's preferred height depends on its width. */
    virtual bool hasHeightForWidth() const RT_OVERRIDE;

    /** Holds the minimum widget size. */
    virtual QSize minimumSizeHint() const RT_OVERRIDE;

    /** Holds the preferred widget size. */
    virtual QSize sizeHint() const RT_OVERRIDE;
};


/*********************************************************************************************************************************
*   Class UIWrappableLabel implementation.                                                                                       *
*********************************************************************************************************************************/

UIWrappableLabel::UIWrappableLabel(QWidget *pParent /* = 0 */)
    : QLabel(pParent)
{
}

void UIWrappableLabel::resizeEvent(QResizeEvent *pEvent)
{
    /* Call to base-class: */
    QLabel::resizeEvent(pEvent);

    // WORKAROUND:
    // That's not cheap procedure but we need it to
    // make sure geometry is updated after width changed.
    if (minimumWidth() > 0)
        updateGeometry();
}

bool UIWrappableLabel::hasHeightForWidth() const
{
    // WORKAROUND:
    // No need to panic, we do it ourselves in resizeEvent() and
    // this 'false' here to prevent automatic layout fighting for it.
    return   minimumWidth() > 0
           ? false
           : QLabel::hasHeightForWidth();
}

QSize UIWrappableLabel::minimumSizeHint() const
{
    // WORKAROUND:
    // We should calculate hint height on the basis of width,
    // keeping the hint width equal to minimum we have set.
    return   minimumWidth() > 0
           ? QSize(minimumWidth(), heightForWidth(width()))
           : QLabel::minimumSizeHint();
}

QSize UIWrappableLabel::sizeHint() const
{
    // WORKAROUND:
    // Keep widget always minimal.
    return minimumSizeHint();
}


/*********************************************************************************************************************************
*   Class UIWelcomePane implementation.                                                                                          *
*********************************************************************************************************************************/

UIWelcomePane::UIWelcomePane(QWidget *pParent /* = 0 */)
    : QIWithRetranslateUI<QWidget>(pParent)
    , m_pLabelText(0)
    , m_pLabelIcon(0)
{
    /* Prepare: */
    prepare();
}

bool UIWelcomePane::event(QEvent *pEvent)
{
    /* Handle known event types: */
    switch (pEvent->type())
    {
        case QEvent::Show:
        case QEvent::ScreenChangeInternal:
        {
            /* Update pixmap: */
            updatePixmap();
            break;
        }
        default:
            break;
    }

    /* Call to base-class: */
    return QIWithRetranslateUI<QWidget>::event(pEvent);
}

void UIWelcomePane::retranslateUi()
{
    /* Translate welcome text: */
    m_pLabelText->setText(tr("<h3>Welcome to VirtualBox!</h3>"
                             "<p>The left part of application window contains global tools and "
                             "lists all virtual machines and virtual machine groups on your computer. "
                             "You can import, add and create new VMs using corresponding toolbar buttons. "
                             "You can popup a tools of currently selected element using corresponding element button.</p>"
                             "<p>You can press the <b>%1</b> key to get instant help, or visit "
                             "<a href=https://www.virtualbox.org>www.virtualbox.org</a> "
                             "for more information and latest news.</p>")
                             .arg(QKeySequence(QKeySequence::HelpContents).toString(QKeySequence::NativeText)));
}

void UIWelcomePane::sltHandleLinkActivated(const QString &strLink)
{
    uiCommon().openURL(strLink);
}

void UIWelcomePane::prepare()
{
    /* Prepare default welcome icon: */
    m_icon = UIIconPool::iconSet(":/tools_banner_global_200px.png");

    /* Create main layout: */
    QVBoxLayout *pMainLayout = new QVBoxLayout(this);
    if (pMainLayout)
    {
        /* Create welcome layout: */
        QHBoxLayout *pLayoutWelcome = new QHBoxLayout;
        if (pLayoutWelcome)
        {
            /* Configure layout: */
            const int iL = qApp->style()->pixelMetric(QStyle::PM_LayoutLeftMargin) / 2;
            pLayoutWelcome->setContentsMargins(iL, 0, 0, 0);

            /* Create welcome text label: */
            m_pLabelText = new UIWrappableLabel;
            if (m_pLabelText)
            {
                /* Configure label: */
                m_pLabelText->setWordWrap(true);
                m_pLabelText->setMinimumWidth(160); /// @todo make dynamic
                m_pLabelText->setAlignment(Qt::AlignLeading | Qt::AlignTop);
                m_pLabelText->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Minimum);
                connect(m_pLabelText, &QLabel::linkActivated, this, &UIWelcomePane::sltHandleLinkActivated);

                /* Add into layout: */
                pLayoutWelcome->addWidget(m_pLabelText);
            }

            /* Create welcome picture label: */
            m_pLabelIcon = new QLabel;
            if (m_pLabelIcon)
            {
                /* Configure label: */
                m_pLabelIcon->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

                /* Add into layout: */
                pLayoutWelcome->addWidget(m_pLabelIcon);
                pLayoutWelcome->setAlignment(m_pLabelIcon, Qt::AlignHCenter | Qt::AlignTop);
            }

            /* Add into layout: */
            pMainLayout->addLayout(pLayoutWelcome);
        }

        /* Add stretch: */
        pMainLayout->addStretch();
    }

    uiCommon().setHelpKeyword(this, "intro-starting");

    /* Translate finally: */
    retranslateUi();
    /* Update pixmap: */
    updatePixmap();
}

void UIWelcomePane::updatePixmap()
{
    /* Assign corresponding icon: */
    if (!m_icon.isNull())
    {
        const QList<QSize> sizes = m_icon.availableSizes();
        const QSize firstOne = sizes.isEmpty() ? QSize(200, 200) : sizes.first();
        m_pLabelIcon->setPixmap(m_icon.pixmap(window()->windowHandle(),
                                                       QSize(firstOne.width(),
                                                             firstOne.height())));
    }
}


#include "UIWelcomePane.moc"

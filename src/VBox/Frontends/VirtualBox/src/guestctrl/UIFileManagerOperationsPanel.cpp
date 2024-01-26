/* $Id: UIFileManagerOperationsPanel.cpp $ */
/** @file
 * VBox Qt GUI - UIVMLogViewer class implementation.
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
#include <QGridLayout>
#include <QLabel>
#include <QMenu>
#include <QProgressBar>
#include <QScrollArea>
#include <QScrollBar>
#include <QStyle>


/* GUI includes: */
#include "QIToolButton.h"
#include "QILabel.h"
#include "UIErrorString.h"
#include "UIIconPool.h"
#include "UIFileManager.h"
#include "UIFileManagerOperationsPanel.h"
#include "UIProgressEventHandler.h"

/* COM includes: */
#include "CProgress.h"


/*********************************************************************************************************************************
*   UIFileOperationProgressWidget definition.                                                                                    *
*********************************************************************************************************************************/

class UIFileOperationProgressWidget : public QIWithRetranslateUI<QFrame>
{

    Q_OBJECT;

signals:

    void sigProgressComplete(QUuid progressId);
    void sigProgressFail(QString strErrorString, QString strSourceTableName, FileManagerLogType eLogType);
    void sigFocusIn(QWidget *pWidget);
    void sigFocusOut(QWidget *pWidget);

public:

    UIFileOperationProgressWidget(const CProgress &comProgress, const QString &strSourceTableName, QWidget *pParent = 0);
    ~UIFileOperationProgressWidget();
    bool isCompleted() const;
    bool isCanceled() const;

protected:

    virtual void retranslateUi() RT_OVERRIDE;
    virtual void focusInEvent(QFocusEvent *pEvent) RT_OVERRIDE;
    virtual void focusOutEvent(QFocusEvent *pEvent) RT_OVERRIDE;

private slots:

    void sltHandleProgressPercentageChange(const QUuid &uProgressId, const int iPercent);
    void sltHandleProgressComplete(const QUuid &uProgressId);
    void sltCancelProgress();

private:
    enum OperationStatus
    {
        OperationStatus_NotStarted,
        OperationStatus_Working,
        OperationStatus_Paused,
        OperationStatus_Canceled,
        OperationStatus_Succeded,
        OperationStatus_Failed,
        OperationStatus_Invalid,
        OperationStatus_Max
    };

    void prepare();
    void prepareWidgets();
    void prepareEventHandler();
    void cleanupEventHandler();

    OperationStatus         m_eStatus;
    CProgress               m_comProgress;
    UIProgressEventHandler *m_pEventHandler;
    QGridLayout            *m_pMainLayout;
    QProgressBar           *m_pProgressBar;
    QIToolButton           *m_pCancelButton;
    QILabel                *m_pStatusLabel;
    QILabel                *m_pOperationDescriptionLabel;
    /** Name of the table from which this operation has originated. */
    QString                 m_strSourceTableName;
};


/*********************************************************************************************************************************
*   UIFileOperationProgressWidget implementation.                                                                                *
*********************************************************************************************************************************/

UIFileOperationProgressWidget::UIFileOperationProgressWidget(const CProgress &comProgress, const QString &strSourceTableName, QWidget *pParent /* = 0 */)
    : QIWithRetranslateUI<QFrame>(pParent)
    , m_eStatus(OperationStatus_NotStarted)
    , m_comProgress(comProgress)
    , m_pEventHandler(0)
    , m_pMainLayout(0)
    , m_pProgressBar(0)
    , m_pCancelButton(0)
    , m_pStatusLabel(0)
    , m_pOperationDescriptionLabel(0)
    , m_strSourceTableName(strSourceTableName)
{
    prepare();
    setFocusPolicy(Qt::ClickFocus);
    setStyleSheet("QFrame:focus {  border-width: 1px; border-style: dashed; border-color: black; }");
}

UIFileOperationProgressWidget::~UIFileOperationProgressWidget()
{
    cleanupEventHandler();
}

bool UIFileOperationProgressWidget::isCompleted() const
{
    if (m_comProgress.isNull())
        return true;
    return m_comProgress.GetCompleted();
}

bool UIFileOperationProgressWidget::isCanceled() const
{
    if (m_comProgress.isNull())
        return true;
    return m_comProgress.GetCanceled();
}

void UIFileOperationProgressWidget::retranslateUi()
{
    if (m_pCancelButton)
        m_pCancelButton->setToolTip(UIFileManager::tr("Cancel"));

    switch (m_eStatus)
    {
        case OperationStatus_NotStarted:
            m_pStatusLabel->setText(UIFileManager::tr("Not yet started"));
            break;
        case OperationStatus_Working:
            m_pStatusLabel->setText(UIFileManager::tr("Working"));
            break;
        case OperationStatus_Paused:
            m_pStatusLabel->setText(UIFileManager::tr("Paused"));
            break;
        case OperationStatus_Canceled:
            m_pStatusLabel->setText(UIFileManager::tr("Canceled"));
            break;
        case OperationStatus_Succeded:
            m_pStatusLabel->setText(UIFileManager::tr("Succeded"));
            break;
        case OperationStatus_Failed:
            m_pStatusLabel->setText(UIFileManager::tr("Failed"));
            break;
        case OperationStatus_Invalid:
        case OperationStatus_Max:
        default:
            m_pStatusLabel->setText(UIFileManager::tr("Invalid"));
            break;
    }
}

void UIFileOperationProgressWidget::focusInEvent(QFocusEvent *pEvent)
{
    QFrame::focusInEvent(pEvent);
    emit sigFocusIn(this);
}

void UIFileOperationProgressWidget::focusOutEvent(QFocusEvent *pEvent)
{
    QFrame::focusOutEvent(pEvent);
    emit sigFocusOut(this);
}

void UIFileOperationProgressWidget::prepare()
{
    prepareWidgets();
    prepareEventHandler();
    retranslateUi();
}

void UIFileOperationProgressWidget::prepareWidgets()
{
    m_pMainLayout = new QGridLayout;
    if (!m_pMainLayout)
        return;
    //m_pMainLayout->setSpacing(0);

    m_pOperationDescriptionLabel = new QILabel;
    if (m_pOperationDescriptionLabel)
    {
        m_pOperationDescriptionLabel->setContextMenuPolicy(Qt::NoContextMenu);
        m_pMainLayout->addWidget(m_pOperationDescriptionLabel, 0, 0, 1, 3);
        if (!m_comProgress.isNull())
            m_pOperationDescriptionLabel->setText(m_comProgress.GetDescription());
    }

    m_pProgressBar = new QProgressBar;
    if (m_pProgressBar)
    {
        m_pProgressBar->setMinimum(0);
        m_pProgressBar->setMaximum(100);
        m_pProgressBar->setTextVisible(true);
        m_pMainLayout->addWidget(m_pProgressBar, 1, 0, 1, 2);
    }

    m_pCancelButton = new QIToolButton;
    if (m_pCancelButton)
    {
        m_pCancelButton->setIcon(QApplication::style()->standardIcon(QStyle::SP_DockWidgetCloseButton));
        connect(m_pCancelButton, &QIToolButton::clicked, this, &UIFileOperationProgressWidget::sltCancelProgress);
        if (!m_comProgress.isNull() && !m_comProgress.GetCancelable())
            m_pCancelButton->setEnabled(false);
        m_pMainLayout->addWidget(m_pCancelButton, 1, 2, 1, 1);
    }

    m_pStatusLabel = new QILabel;
    if (m_pStatusLabel)
    {
        m_pStatusLabel->setContextMenuPolicy(Qt::NoContextMenu);
        m_pMainLayout->addWidget(m_pStatusLabel, 1, 3, 1, 1);
    }

    setLayout(m_pMainLayout);
    retranslateUi();
}

void UIFileOperationProgressWidget::prepareEventHandler()
{
    if (m_comProgress.isNull())
        return;
    m_pEventHandler = new UIProgressEventHandler(this, m_comProgress);
    connect(m_pEventHandler, &UIProgressEventHandler::sigProgressPercentageChange,
            this, &UIFileOperationProgressWidget::sltHandleProgressPercentageChange);
    connect(m_pEventHandler, &UIProgressEventHandler::sigProgressTaskComplete,
            this, &UIFileOperationProgressWidget::sltHandleProgressComplete);
    m_eStatus = OperationStatus_Working;
    retranslateUi();
}

void UIFileOperationProgressWidget::cleanupEventHandler()
{
    delete m_pEventHandler;
    m_pEventHandler = 0;
}

void UIFileOperationProgressWidget::sltHandleProgressPercentageChange(const QUuid &uProgressId, const int iPercent)
{
    Q_UNUSED(uProgressId);
    m_pProgressBar->setValue(iPercent);
}

void UIFileOperationProgressWidget::sltHandleProgressComplete(const QUuid &uProgressId)
{
    Q_UNUSED(uProgressId);
    if (m_pCancelButton)
        m_pCancelButton->setEnabled(false);

    if (!m_comProgress.isOk() || m_comProgress.GetResultCode() != 0)
    {
        emit sigProgressFail(UIErrorString::formatErrorInfo(m_comProgress), m_strSourceTableName, FileManagerLogType_Error);
        m_eStatus = OperationStatus_Failed;
    }
    else
    {
        emit sigProgressComplete(m_comProgress.GetId());
        m_eStatus = OperationStatus_Succeded;
    }
    if (m_pProgressBar)
        m_pProgressBar->setValue(100);

    cleanupEventHandler();
    retranslateUi();
}

void UIFileOperationProgressWidget::sltCancelProgress()
{
    m_comProgress.Cancel();
    /* Since we dont have a "progress canceled" event we have to do this here: */
    if (m_pCancelButton)
        m_pCancelButton->setEnabled(false);
    if (m_pProgressBar)
        m_pProgressBar->setEnabled(false);
    m_eStatus = OperationStatus_Canceled;
    cleanupEventHandler();
    retranslateUi();
}


/*********************************************************************************************************************************
*   UIFileManagerOperationsPanel implementation.                                                                     *
*********************************************************************************************************************************/

UIFileManagerOperationsPanel::UIFileManagerOperationsPanel(QWidget *pParent /* = 0 */)
    : UIDialogPanel(pParent)
    , m_pScrollArea(0)
    , m_pContainerWidget(0)
    , m_pContainerLayout(0)
    , m_pContainerSpaceItem(0)
    , m_pWidgetInFocus(0)
{
    prepare();
}

void UIFileManagerOperationsPanel::addNewProgress(const CProgress &comProgress, const QString &strSourceTableName)
{
    if (!m_pContainerLayout)
        return;

    UIFileOperationProgressWidget *pOperationsWidget = new UIFileOperationProgressWidget(comProgress, strSourceTableName);
    if (!pOperationsWidget)
        return;
    m_widgetSet.insert(pOperationsWidget);
    m_pContainerLayout->insertWidget(m_pContainerLayout->count() - 1, pOperationsWidget);

    connect(pOperationsWidget, &UIFileOperationProgressWidget::sigProgressComplete,
            this, &UIFileManagerOperationsPanel::sigFileOperationComplete);
    connect(pOperationsWidget, &UIFileOperationProgressWidget::sigProgressFail,
            this, &UIFileManagerOperationsPanel::sigFileOperationFail);

    connect(pOperationsWidget, &UIFileOperationProgressWidget::sigFocusIn,
            this, &UIFileManagerOperationsPanel::sltHandleWidgetFocusIn);
    connect(pOperationsWidget, &UIFileOperationProgressWidget::sigFocusOut,
            this, &UIFileManagerOperationsPanel::sltHandleWidgetFocusOut);
    sigShowPanel(this);
}

QString UIFileManagerOperationsPanel::panelName() const
{
    return "OperationsPanel";
}

void UIFileManagerOperationsPanel::prepareWidgets()
{
    if (!mainLayout())
        return;

    QPalette pal = QApplication::palette();
    pal.setColor(QPalette::Active, QPalette::Window, pal.color(QPalette::Active, QPalette::Base));
    setPalette(pal);

    m_pScrollArea = new QScrollArea;
    m_pContainerWidget = new QWidget;
    m_pContainerLayout = new QVBoxLayout;
    if (!m_pScrollArea || !m_pContainerWidget || !m_pContainerLayout)
        return;

    QScrollBar *pVerticalScrollBar = m_pScrollArea->verticalScrollBar();
    if (pVerticalScrollBar)
        QObject::connect(pVerticalScrollBar, &QScrollBar::rangeChanged, this, &UIFileManagerOperationsPanel::sltScrollToBottom);

    m_pScrollArea->setBackgroundRole(QPalette::Window);
    m_pScrollArea->setWidgetResizable(true);

    mainLayout()->addWidget(m_pScrollArea);

    m_pScrollArea->setWidget(m_pContainerWidget);
    m_pContainerWidget->setLayout(m_pContainerLayout);
    m_pContainerLayout->addStretch(4);
}

void UIFileManagerOperationsPanel::prepareConnections()
{

}

void UIFileManagerOperationsPanel::retranslateUi()
{
    UIDialogPanel::retranslateUi();
}

void UIFileManagerOperationsPanel::contextMenuEvent(QContextMenuEvent *pEvent)
{
    QMenu *menu = new QMenu(this);

    if (m_pWidgetInFocus)
    {
        QAction *pRemoveSelected = menu->addAction(UIFileManager::tr("Remove Selected"));
        connect(pRemoveSelected, &QAction::triggered,
                this, &UIFileManagerOperationsPanel::sltRemoveSelected);
    }

    QAction *pRemoveFinished = menu->addAction(UIFileManager::tr("Remove Finished"));
    QAction *pRemoveAll = menu->addAction(UIFileManager::tr("Remove All"));

    connect(pRemoveFinished, &QAction::triggered,
            this, &UIFileManagerOperationsPanel::sltRemoveFinished);
    connect(pRemoveAll, &QAction::triggered,
            this, &UIFileManagerOperationsPanel::sltRemoveAll);

    menu->exec(pEvent->globalPos());
    delete menu;
}

void UIFileManagerOperationsPanel::sltRemoveFinished()
{
    QList<UIFileOperationProgressWidget*> widgetsToRemove;
    foreach (QWidget *pWidget, m_widgetSet)
    {
        UIFileOperationProgressWidget *pProgressWidget = qobject_cast<UIFileOperationProgressWidget*>(pWidget);
        if (pProgressWidget && pProgressWidget->isCompleted())
        {
            delete pProgressWidget;
            widgetsToRemove << pProgressWidget;
        }
    }
    foreach (UIFileOperationProgressWidget *pWidget, widgetsToRemove)
        m_widgetSet.remove(pWidget);
}

void UIFileManagerOperationsPanel::sltRemoveAll()
{
    foreach (QWidget *pWidget, m_widgetSet)
    {
        if (pWidget)
        {
            delete pWidget;
        }
    }
    m_widgetSet.clear();
}

void UIFileManagerOperationsPanel::sltRemoveSelected()
{
    if (!m_pWidgetInFocus)
        return;
    delete m_pWidgetInFocus;
    m_widgetSet.remove(m_pWidgetInFocus);
}

void UIFileManagerOperationsPanel::sltHandleWidgetFocusIn(QWidget *pWidget)
{
    if (!pWidget)
        return;
    m_pWidgetInFocus = pWidget;
}

void UIFileManagerOperationsPanel::sltHandleWidgetFocusOut(QWidget *pWidget)
{
    if (!pWidget)
        return;
    m_pWidgetInFocus = 0;
}

void UIFileManagerOperationsPanel::sltScrollToBottom(int iMin, int iMax)
{
    Q_UNUSED(iMin);
    if (m_pScrollArea)
    m_pScrollArea->verticalScrollBar()->setValue(iMax);
}

#include "UIFileManagerOperationsPanel.moc"

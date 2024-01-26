/* $Id: UIBootOrderEditor.cpp $ */
/** @file
 * VBox Qt GUI - UIBootListWidget class implementation.
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
#include <QGridLayout>
#include <QHeaderView>
#include <QLabel>
#include <QRegularExpression>
#include <QScrollBar>

/* GUI includes: */
#include "UIBootOrderEditor.h"
#include "UICommon.h"
#include "UIConverter.h"
#include "UIIconPool.h"
#include "QIToolBar.h"
#include "QITreeWidget.h"

/* COM includes: */
#include "COMEnums.h"
#include "CMachine.h"
#include "CSystemProperties.h"


/** QITreeWidgetItem extension for our UIBootListWidget. */
class UIBootListWidgetItem : public QITreeWidgetItem
{
    Q_OBJECT;

public:

    /** Constructs boot-table item of passed @a enmType. */
    UIBootListWidgetItem(KDeviceType enmType);

    /** Returns the item type. */
    KDeviceType deviceType() const;

    /** Performs item translation. */
    void retranslateUi();

private:

    /** Holds the item type. */
    KDeviceType m_enmType;
};


/** QITreeWidget subclass used as system settings boot-table. */
class UIBootListWidget : public QIWithRetranslateUI<QITreeWidget>
{
    Q_OBJECT;

signals:

    /** Notifies listeners about current table row changed. */
    void sigRowChanged();

public:

    /** Constructs boot-table passing @a pParent to the base-class. */
    UIBootListWidget(QWidget *pParent = 0);

    /** Defines @a bootItems list. */
    void setBootItems(const UIBootItemDataList &bootItems);
    /** Returns boot item list. */
    UIBootItemDataList bootItems() const;

public slots:

    /** Moves current item up. */
    void sltMoveItemUp();
    /** Moves current item down. */
    void sltMoveItemDown();

protected:

    /** Return size hint. */
    virtual QSize sizeHint() const;
    /** Return minimum size hint. */
    virtual QSize minimumSizeHint() const;

    /** Handles translation event. */
    virtual void retranslateUi() RT_OVERRIDE;

    /** Handles drop @a pEvent. */
    virtual void dropEvent(QDropEvent *pEvent) RT_OVERRIDE;

    /** Returns a QModelIndex object pointing to the next object in the view,
      * based on the given @a cursorAction and keyboard @a fModifiers. */
    virtual QModelIndex moveCursor(QAbstractItemView::CursorAction cursorAction,
                                   Qt::KeyboardModifiers fModifiers) RT_OVERRIDE;

private:

    /** Prepares all. */
    void prepare();

    /** Moves item with passed @a index to specified @a iRow. */
    QModelIndex moveItemTo(const QModelIndex &index, int iRow);
};


/*********************************************************************************************************************************
*   Class UIBootListWidgetItem implementation.                                                                                   *
*********************************************************************************************************************************/

UIBootListWidgetItem::UIBootListWidgetItem(KDeviceType enmType)
    : m_enmType(enmType)
{
    setCheckState(0, Qt::Unchecked);
    switch(enmType)
    {
        case KDeviceType_Floppy:   setIcon(0, UIIconPool::iconSet(":/fd_16px.png")); break;
        case KDeviceType_DVD:      setIcon(0, UIIconPool::iconSet(":/cd_16px.png")); break;
        case KDeviceType_HardDisk: setIcon(0, UIIconPool::iconSet(":/hd_16px.png")); break;
        case KDeviceType_Network:  setIcon(0, UIIconPool::iconSet(":/nw_16px.png")); break;
        default: break; /* Shut up, MSC! */
    }
    retranslateUi();
}

KDeviceType UIBootListWidgetItem::deviceType() const
{
    return m_enmType;
}

void UIBootListWidgetItem::retranslateUi()
{
    setText(0, gpConverter->toString(m_enmType));
}


/*********************************************************************************************************************************
*   Class UIBootListWidget implementation.                                                                                       *
*********************************************************************************************************************************/

UIBootListWidget::UIBootListWidget(QWidget *pParent /* = 0 */)
    : QIWithRetranslateUI<QITreeWidget>(pParent)
{
    prepare();
}

void UIBootListWidget::setBootItems(const UIBootItemDataList &bootItems)
{
    /* Clear initially: */
    clear();

    /* Apply internal variables data to QWidget(s): */
    foreach (const UIBootItemData &data, bootItems)
    {
        UIBootListWidgetItem *pItem = new UIBootListWidgetItem(data.m_enmType);
        pItem->setCheckState(0, data.m_fEnabled ? Qt::Checked : Qt::Unchecked);
        addTopLevelItem(pItem);
    }

    /* Make sure at least one is chosen: */
    if (topLevelItemCount())
        setCurrentItem(topLevelItem(0));

    /* That changes the size: */
    updateGeometry();
}

UIBootItemDataList UIBootListWidget::bootItems() const
{
    /* Prepare boot items: */
    UIBootItemDataList bootItems;

    /* Enumerate all the items we have: */
    for (int i = 0; i < topLevelItemCount(); ++i)
    {
        QTreeWidgetItem *pItem = topLevelItem(i);
        UIBootItemData bootData;
        bootData.m_enmType = static_cast<UIBootListWidgetItem*>(pItem)->deviceType();
        bootData.m_fEnabled = pItem->checkState(0) == Qt::Checked;
        bootItems << bootData;
    }

    /* Return boot items: */
    return bootItems;
}

void UIBootListWidget::sltMoveItemUp()
{
    QModelIndex index = currentIndex();
    moveItemTo(index, index.row() - 1);
}

void UIBootListWidget::sltMoveItemDown()
{
    QModelIndex index = currentIndex();
    moveItemTo(index, index.row() + 2);
}

QSize UIBootListWidget::sizeHint() const
{
    return minimumSizeHint();
}

QSize UIBootListWidget::minimumSizeHint() const
{
    const int iH = 2 * frameWidth();
    const int iW = iH;
    return QSize(sizeHintForColumn(0) + iW,
                 sizeHintForRow(0) * topLevelItemCount() + iH);

}

void UIBootListWidget::retranslateUi()
{
    for (int i = 0; i < topLevelItemCount(); ++i)
        static_cast<UIBootListWidgetItem*>(topLevelItem(i))->retranslateUi();
}

void UIBootListWidget::dropEvent(QDropEvent *pEvent)
{
    /* Call to base-class: */
    QITreeWidget::dropEvent(pEvent);
    /* Separately notify listeners: */
    emit sigRowChanged();
}

QModelIndex UIBootListWidget::moveCursor(QAbstractItemView::CursorAction cursorAction, Qt::KeyboardModifiers fModifiers)
{
    if (fModifiers.testFlag(Qt::ControlModifier))
    {
        switch (cursorAction)
        {
            case QAbstractItemView::MoveUp:
            {
                QModelIndex index = currentIndex();
                return moveItemTo(index, index.row() - 1);
            }
            case QAbstractItemView::MoveDown:
            {
                QModelIndex index = currentIndex();
                return moveItemTo(index, index.row() + 2);
            }
            case QAbstractItemView::MovePageUp:
            {
                QModelIndex index = currentIndex();
                return moveItemTo(index, qMax(0, index.row() - verticalScrollBar()->pageStep()));
            }
            case QAbstractItemView::MovePageDown:
            {
                QModelIndex index = currentIndex();
                return moveItemTo(index, qMin(model()->rowCount(), index.row() + verticalScrollBar()->pageStep() + 1));
            }
            case QAbstractItemView::MoveHome:
                return moveItemTo(currentIndex(), 0);
            case QAbstractItemView::MoveEnd:
                return moveItemTo(currentIndex(), model()->rowCount());
            default:
                break;
        }
    }
    return QITreeWidget::moveCursor(cursorAction, fModifiers);
}

void UIBootListWidget::prepare()
{
    header()->hide();
    setRootIsDecorated(false);
    setDragDropMode(QAbstractItemView::InternalMove);
    setSelectionMode(QAbstractItemView::SingleSelection);
    setDropIndicatorShown(true);
    connect(this, &UIBootListWidget::currentItemChanged,
            this, &UIBootListWidget::sigRowChanged);
}

QModelIndex UIBootListWidget::moveItemTo(const QModelIndex &index, int iRow)
{
    /* Check validity: */
    if (!index.isValid())
        return QModelIndex();

    /* Check sanity: */
    if (iRow < 0 || iRow > model()->rowCount())
        return QModelIndex();

    QPersistentModelIndex oldIndex(index);
    UIBootListWidgetItem *pItem = static_cast<UIBootListWidgetItem*>(itemFromIndex(oldIndex));
    insertTopLevelItem(iRow, new UIBootListWidgetItem(pItem->deviceType()));
    topLevelItem(iRow)->setCheckState(0, pItem->checkState(0));
    QPersistentModelIndex newIndex = model()->index(iRow, 0);
    delete takeTopLevelItem(oldIndex.row());
    setCurrentItem(topLevelItem(newIndex.row()));
    return QModelIndex(newIndex);
}


/*********************************************************************************************************************************
*   Class UIBootDataTools implementation.                                                                                        *
*********************************************************************************************************************************/

UIBootItemDataList UIBootDataTools::loadBootItems(const CMachine &comMachine)
{
    /* Gather a list of all possible boot items.
     * Currently, it seems, we are supporting only 4 possible boot device types:
     * 1. Floppy, 2. DVD-ROM, 3. Hard Disk, 4. Network.
     * But maximum boot devices count supported by machine should be retrieved
     * through the ISystemProperties getter.  Moreover, possible boot device
     * types are not listed in some separate Main vector, so we should get them
     * (randomely?) from the list of all device types.  Until there will be a
     * separate Main getter for list of supported boot device types, this list
     * will be hard-coded here... */
    QVector<KDeviceType> possibleBootItems = QVector<KDeviceType>() << KDeviceType_Floppy
                                                                    << KDeviceType_DVD
                                                                    << KDeviceType_HardDisk
                                                                    << KDeviceType_Network;
    const CSystemProperties comProperties = uiCommon().virtualBox().GetSystemProperties();
    const int iPossibleBootListSize = qMin((ULONG)4, comProperties.GetMaxBootPosition());
    possibleBootItems.resize(iPossibleBootListSize);

    /* Prepare boot items: */
    UIBootItemDataList bootItems;

    /* Gather boot-items of current VM: */
    QList<KDeviceType> usedBootItems;
    for (int i = 1; i <= possibleBootItems.size(); ++i)
    {
        const KDeviceType enmType = comMachine.GetBootOrder(i);
        if (enmType != KDeviceType_Null)
        {
            usedBootItems << enmType;
            UIBootItemData data;
            data.m_enmType = enmType;
            data.m_fEnabled = true;
            bootItems << data;
        }
    }
    /* Gather other unique boot-items: */
    for (int i = 0; i < possibleBootItems.size(); ++i)
    {
        const KDeviceType enmType = possibleBootItems.at(i);
        if (!usedBootItems.contains(enmType))
        {
            UIBootItemData data;
            data.m_enmType = enmType;
            data.m_fEnabled = false;
            bootItems << data;
        }
    }

    /* Return boot items: */
    return bootItems;
}

void UIBootDataTools::saveBootItems(const UIBootItemDataList &bootItems, CMachine &comMachine)
{
    bool fSuccess = true;
    int iBootIndex = 0;
    for (int i = 0; fSuccess && i < bootItems.size(); ++i)
    {
        if (bootItems.at(i).m_fEnabled)
        {
            comMachine.SetBootOrder(++iBootIndex, bootItems.at(i).m_enmType);
            fSuccess = comMachine.isOk();
        }
    }
    for (int i = 0; fSuccess && i < bootItems.size(); ++i)
    {
        if (!bootItems.at(i).m_fEnabled)
        {
            comMachine.SetBootOrder(++iBootIndex, KDeviceType_Null);
            fSuccess = comMachine.isOk();
        }
    }
}

QString UIBootDataTools::bootItemsToReadableString(const UIBootItemDataList &bootItems)
{
    /* Prepare list: */
    QStringList list;
    /* We are reflecting only enabled items: */
    foreach (const UIBootItemData &bootItem, bootItems)
        if (bootItem.m_fEnabled)
            list << gpConverter->toString(bootItem.m_enmType);
    /* But if list is empty we are adding Null item at least: */
    if (list.isEmpty())
        list << gpConverter->toString(KDeviceType_Null);
    /* Join list to string: */
    return list.join(", ");
}

QString UIBootDataTools::bootItemsToSerializedString(const UIBootItemDataList &bootItems)
{
    /* Prepare list: */
    QStringList list;
    /* This is simple, we are adding '+' before enabled types and '-' before disabled: */
    foreach (const UIBootItemData &bootItem, bootItems)
        list << (bootItem.m_fEnabled ? QString("+%1").arg(bootItem.m_enmType) : QString("-%1").arg(bootItem.m_enmType));
    /* Join list to string: */
    return list.join(';');
}

UIBootItemDataList UIBootDataTools::bootItemsFromSerializedString(const QString &strBootItems)
{
    /* Prepare list: */
    UIBootItemDataList list;
    /* First of all, split passed string to arguments: */
    const QStringList arguments = strBootItems.split(';');
    /* Now parse in backward direction, we have added '+' before enabled types and '-' before disabled: */
    foreach (QString strArgument, arguments)
    {
        UIBootItemData data;
        data.m_fEnabled = strArgument.startsWith('+');
        strArgument.remove(QRegularExpression("[+-]"));
        data.m_enmType = static_cast<KDeviceType>(strArgument.toInt());
        list << data;
    }
    /* Return list: */
    return list;
}


/*********************************************************************************************************************************
*   Class UIBootOrderEditor implementation.                                                                                      *
*********************************************************************************************************************************/

UIBootOrderEditor::UIBootOrderEditor(QWidget *pParent /* = 0 */)
    : QIWithRetranslateUI<QWidget>(pParent)
    , m_pLayout(0)
    , m_pLabel(0)
    , m_pTable(0)
    , m_pToolbar(0)
    , m_pMoveUp(0)
    , m_pMoveDown(0)
{
    prepare();
}

void UIBootOrderEditor::setValue(const UIBootItemDataList &guiValue)
{
    if (m_pTable)
        m_pTable->setBootItems(guiValue);
}

UIBootItemDataList UIBootOrderEditor::value() const
{
    return m_pTable ? m_pTable->bootItems() : UIBootItemDataList();
}

int UIBootOrderEditor::minimumLabelHorizontalHint() const
{
    return m_pLabel ? m_pLabel->minimumSizeHint().width() : 0;
}

void UIBootOrderEditor::setMinimumLayoutIndent(int iIndent)
{
    if (m_pLayout)
        m_pLayout->setColumnMinimumWidth(0, iIndent);
}

bool UIBootOrderEditor::eventFilter(QObject *pObject, QEvent *pEvent)
{
    /* Skip events sent to unrelated objects: */
    if (m_pTable && pObject != m_pTable)
        return QIWithRetranslateUI<QWidget>::eventFilter(pObject, pEvent);

    /* Handle only required event types: */
    switch (pEvent->type())
    {
        case QEvent::FocusIn:
        case QEvent::FocusOut:
        {
            /* On focus in/out events we'd like
             * to update actions availability: */
            updateActionAvailability();
            break;
        }
        default:
            break;
    }

    /* Call to base-class: */
    return QIWithRetranslateUI<QWidget>::eventFilter(pObject, pEvent);
}

void UIBootOrderEditor::retranslateUi()
{
    if (m_pLabel)
        m_pLabel->setText(tr("&Boot Order:"));
    if (m_pTable)
        m_pTable->setWhatsThis(tr("Defines the boot device order. Use the "
                                  "checkboxes on the left to enable or disable individual boot devices."
                                  "Move items up and down to change the device order."));
    if (m_pMoveUp)
        m_pMoveUp->setToolTip(tr("Moves selected boot item up."));
    if (m_pMoveDown)
        m_pMoveDown->setToolTip(tr("Moves selected boot item down."));
}

void UIBootOrderEditor::sltHandleCurrentBootItemChange()
{
    /* On current item change signals we'd like
     * to update actions availability: */
    updateActionAvailability();
}

void UIBootOrderEditor::prepare()
{
    /* Configure self: */
    setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));

    /* Create main layout: */
    m_pLayout = new QGridLayout(this);
    if (m_pLayout)
    {
        m_pLayout->setContentsMargins(0, 0, 0, 0);

        /* Create label: */
        m_pLabel = new QLabel(this);
        if (m_pLabel)
        {
            m_pLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
            m_pLayout->addWidget(m_pLabel, 0, 0);
        }

        /* Create table layout: */
        QHBoxLayout *pTableLayout = new QHBoxLayout;
        if (pTableLayout)
        {
            pTableLayout->setContentsMargins(0, 0, 0, 0);
            pTableLayout->setSpacing(1);

            /* Create table: */
            m_pTable = new UIBootListWidget(this);
            if (m_pTable)
            {
                setFocusProxy(m_pTable);
                if (m_pLabel)
                    m_pLabel->setBuddy(m_pTable);
                m_pTable->setAlternatingRowColors(true);
                m_pTable->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
                m_pTable->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
                connect(m_pTable, &UIBootListWidget::sigRowChanged,
                        this, &UIBootOrderEditor::sltHandleCurrentBootItemChange);
                pTableLayout->addWidget(m_pTable);
            }

            /* Create tool-bar: */
            m_pToolbar = new QIToolBar(this);
            if (m_pToolbar)
            {
                m_pToolbar->setIconSize(QSize(16, 16));
                m_pToolbar->setOrientation(Qt::Vertical);

                /* Create Up action: */
                m_pMoveUp = m_pToolbar->addAction(UIIconPool::iconSet(":/list_moveup_16px.png",
                                                                      ":/list_moveup_disabled_16px.png"),
                                                  QString(), m_pTable, &UIBootListWidget::sltMoveItemUp);
                /* Create Down action: */
                m_pMoveDown = m_pToolbar->addAction(UIIconPool::iconSet(":/list_movedown_16px.png",
                                                                        ":/list_movedown_disabled_16px.png"),
                                                    QString(), m_pTable, &UIBootListWidget::sltMoveItemDown);

                /* Add tool-bar into table layout: */
                pTableLayout->addWidget(m_pToolbar);
            }

            /* Add table layout to main layout: */
            m_pLayout->addLayout(pTableLayout, 0, 1, 4, 1);
        }
    }

    /* Update initial action availability: */
    updateActionAvailability();
    /* Apply language settings: */
    retranslateUi();
}

void UIBootOrderEditor::updateActionAvailability()
{
    /* Update move up/down actions: */
    QTreeWidgetItem *pCurrentTopLevelItem = m_pTable->currentItem();
    const int iCurrentTopLevelItem = m_pTable->indexOfTopLevelItem(pCurrentTopLevelItem);
    if (m_pTable && m_pMoveUp && iCurrentTopLevelItem != -1)
        m_pMoveUp->setEnabled(m_pTable->hasFocus() && iCurrentTopLevelItem > 0);
    if (m_pTable && m_pMoveDown && iCurrentTopLevelItem != -1)
        m_pMoveDown->setEnabled(m_pTable->hasFocus() && iCurrentTopLevelItem < m_pTable->topLevelItemCount() - 1);
}

#include "UIBootOrderEditor.moc"

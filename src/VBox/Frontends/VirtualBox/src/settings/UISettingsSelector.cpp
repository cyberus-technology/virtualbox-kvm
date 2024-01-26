/* $Id: UISettingsSelector.cpp $ */
/** @file
 * VBox Qt GUI - UISettingsSelector class implementation.
 */

/*
 * Copyright (C) 2008-2023 Oracle and/or its affiliates.
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
#include <QAccessibleWidget>
#include <QAction>
#include <QActionGroup>
#include <QHeaderView>
#include <QLayout>
#include <QTabWidget>
#include <QToolButton>

/* GUI includes: */
#include "QITabWidget.h"
#include "QITreeWidget.h"
#include "UISettingsSelector.h"
#include "UIIconPool.h"
#include "UISettingsPage.h"
#include "QIToolBar.h"


/** QAccessibleWidget extension used as an accessibility interface for UISettingsSelectorToolBar buttons. */
class UIAccessibilityInterfaceForUISettingsSelectorToolBarButton : public QAccessibleWidget
{
public:

    /** Returns an accessibility interface for passed @a strClassname and @a pObject. */
    static QAccessibleInterface *pFactory(const QString &strClassname, QObject *pObject)
    {
        /* Creating toolbar button accessibility interface: */
        if (   pObject
            && strClassname == QLatin1String("QToolButton")
            && pObject->property("Belongs to") == "UISettingsSelectorToolBar")
            return new UIAccessibilityInterfaceForUISettingsSelectorToolBarButton(qobject_cast<QWidget*>(pObject));

        /* Null by default: */
        return 0;
    }

    /** Constructs an accessibility interface passing @a pWidget to the base-class. */
    UIAccessibilityInterfaceForUISettingsSelectorToolBarButton(QWidget *pWidget)
        : QAccessibleWidget(pWidget, QAccessible::Button)
    {}

    /** Returns the role. */
    virtual QAccessible::Role role() const RT_OVERRIDE
    {
        /* Make sure button still alive: */
        AssertPtrReturn(button(), QAccessible::NoRole);

        /* Return role for checkable button: */
        if (button()->isCheckable())
            return QAccessible::RadioButton;

        /* Return default role: */
        return QAccessible::Button;
    }

    /** Returns the state. */
    virtual QAccessible::State state() const RT_OVERRIDE
    {
        /* Prepare the button state: */
        QAccessible::State state;

        /* Make sure button still alive: */
        AssertPtrReturn(button(), state);

        /* Compose the button state: */
        state.checkable = button()->isCheckable();
        state.checked = button()->isChecked();

        /* Return the button state: */
        return state;
    }

private:

    /** Returns corresponding toolbar button. */
    QToolButton *button() const { return qobject_cast<QToolButton*>(widget()); }
};


/** Tree-widget column sections. */
enum TreeWidgetSection
{
    TreeWidgetSection_Category = 0,
    TreeWidgetSection_Id,
    TreeWidgetSection_Link
};


/** Simple container of all the selector item data. */
class UISelectorItem
{
public:

    /** Constructs selector item.
      * @param  icon       Brings the item icon.
      * @param  strText    Brings the item text.
      * @param  iID        Brings the item ID.
      * @param  strLink    Brings the item link.
      * @param  pPage      Brings the item page reference.
      * @param  iParentID  Brings the item parent ID. */
    UISelectorItem(const QIcon &icon, const QString &strText, int iID, const QString &strLink, UISettingsPage *pPage, int iParentID)
        : m_icon(icon)
        , m_strText(strText)
        , m_iID(iID)
        , m_strLink(strLink)
        , m_pPage(pPage)
        , m_iParentID(iParentID)
    {}

    /** Destructs selector item. */
    virtual ~UISelectorItem() {}

    /** Returns the item icon. */
    QIcon icon() const { return m_icon; }
    /** Returns the item text. */
    QString text() const { return m_strText; }
    /** Defines the item @a strText. */
    void setText(const QString &strText) { m_strText = strText; }
    /** Returns the item ID. */
    int id() const { return m_iID; }
    /** Returns the item link. */
    QString link() const { return m_strLink; }
    /** Returns the item page reference. */
    UISettingsPage *page() const { return m_pPage; }
    /** Returns the item parent ID. */
    int parentID() const { return m_iParentID; }

protected:

    /** Holds the item icon. */
    QIcon m_icon;
    /** Holds the item text. */
    QString m_strText;
    /** Holds the item ID. */
    int m_iID;
    /** Holds the item link. */
    QString m_strLink;
    /** Holds the item page reference. */
    UISettingsPage *m_pPage;
    /** Holds the item parent ID. */
    int m_iParentID;
};


/** UISelectorItem subclass used as tab-widget selector item. */
class UISelectorActionItem : public UISelectorItem
{
public:

    /** Constructs selector item.
      * @param  icon       Brings the item icon.
      * @param  strText    Brings the item text.
      * @param  iID        Brings the item ID.
      * @param  strLink    Brings the item link.
      * @param  pPage      Brings the item page reference.
      * @param  iParentID  Brings the item parent ID.
      * @param  pParent    Brings the item parent. */
    UISelectorActionItem(const QIcon &icon, const QString &strText, int iID, const QString &strLink, UISettingsPage *pPage, int iParentID, QObject *pParent)
        : UISelectorItem(icon, strText, iID, strLink, pPage, iParentID)
        , m_pAction(new QAction(icon, strText, pParent))
        , m_pTabWidget(0)
    {
        m_pAction->setCheckable(true);
    }

    /** Returns the action instance. */
    QAction *action() const { return m_pAction; }

    /** Defines the @a pTabWidget instance. */
    void setTabWidget(QTabWidget *pTabWidget) { m_pTabWidget = pTabWidget; }
    /** Returns the tab-widget instance. */
    QTabWidget *tabWidget() const { return m_pTabWidget; }

protected:

    /** Holds the action instance. */
    QAction *m_pAction;
    /** Holds the tab-widget instance. */
    QTabWidget *m_pTabWidget;
};


/*********************************************************************************************************************************
*   Class UISettingsSelector implementation.                                                                                     *
*********************************************************************************************************************************/

UISettingsSelector::UISettingsSelector(QWidget *pParent /* = 0 */)
    : QObject(pParent)
{
}

UISettingsSelector::~UISettingsSelector()
{
    qDeleteAll(m_list);
    m_list.clear();
}

void UISettingsSelector::setItemText(int iID, const QString &strText)
{
    if (UISelectorItem *pTtem = findItem(iID))
        pTtem->setText(strText);
}

QString UISettingsSelector::itemTextByPage(UISettingsPage *pPage) const
{
    QString strText;
    if (UISelectorItem *pItem = findItemByPage(pPage))
        strText = pItem->text();
    return strText;
}

QWidget *UISettingsSelector::idToPage(int iID) const
{
    UISettingsPage *pPage = 0;
    if (UISelectorItem *pItem = findItem(iID))
        pPage = pItem->page();
    return pPage;
}

QList<UISettingsPage*> UISettingsSelector::settingPages() const
{
    QList<UISettingsPage*> list;
    foreach (UISelectorItem *pItem, m_list)
        if (pItem->page())
            list << pItem->page();
    return list;
}

QList<QWidget*> UISettingsSelector::rootPages() const
{
    QList<QWidget*> list;
    foreach (UISelectorItem *pItem, m_list)
        if (pItem->page())
            list << pItem->page();
    return list;
}

UISelectorItem *UISettingsSelector::findItem(int iID) const
{
    UISelectorItem *pResult = 0;
    foreach (UISelectorItem *pItem, m_list)
        if (pItem->id() == iID)
        {
            pResult = pItem;
            break;
        }
    return pResult;
}

UISelectorItem *UISettingsSelector::findItemByLink(const QString &strLink) const
{
    UISelectorItem *pResult = 0;
    foreach (UISelectorItem *pItem, m_list)
        if (pItem->link() == strLink)
        {
            pResult = pItem;
            break;
        }
    return pResult;
}

UISelectorItem *UISettingsSelector::findItemByPage(UISettingsPage *pPage) const
{
    UISelectorItem *pResult = 0;
    foreach (UISelectorItem *pItem, m_list)
        if (pItem->page() == pPage)
        {
            pResult = pItem;
            break;
        }
    return pResult;
}


/*********************************************************************************************************************************
*   Class UISettingsSelectorTreeView implementation.                                                                             *
*********************************************************************************************************************************/

UISettingsSelectorTreeView::UISettingsSelectorTreeView(QWidget *pParent /* = 0 */)
    : UISettingsSelector(pParent)
    , m_pTreeWidget(0)
{
    /* Prepare the tree-widget: */
    m_pTreeWidget = new QITreeWidget(pParent);
    QSizePolicy sizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);
    sizePolicy.setHorizontalStretch(0);
    sizePolicy.setVerticalStretch(0);
    sizePolicy.setHeightForWidth(m_pTreeWidget->sizePolicy().hasHeightForWidth());
    const QStyle *pStyle = QApplication::style();
    const int iIconMetric = pStyle->pixelMetric(QStyle::PM_SmallIconSize);
    m_pTreeWidget->setSizePolicy(sizePolicy);
    m_pTreeWidget->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_pTreeWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_pTreeWidget->setRootIsDecorated(false);
    m_pTreeWidget->setUniformRowHeights(true);
    m_pTreeWidget->setIconSize(QSize((int)(1.5 * iIconMetric), (int)(1.5 * iIconMetric)));
    /* Add the columns: */
    m_pTreeWidget->headerItem()->setText(TreeWidgetSection_Category, "Category");
    m_pTreeWidget->headerItem()->setText(TreeWidgetSection_Id, "[id]");
    m_pTreeWidget->headerItem()->setText(TreeWidgetSection_Link, "[link]");
    /* Hide unnecessary columns and header: */
    m_pTreeWidget->header()->hide();
    m_pTreeWidget->hideColumn(TreeWidgetSection_Id);
    m_pTreeWidget->hideColumn(TreeWidgetSection_Link);
    /* Setup connections: */
    connect(m_pTreeWidget, &QITreeWidget::currentItemChanged,
            this, &UISettingsSelectorTreeView::sltSettingsGroupChanged);
}

UISettingsSelectorTreeView::~UISettingsSelectorTreeView()
{
    /* Cleanup the tree-widget: */
    delete m_pTreeWidget;
    m_pTreeWidget = 0;
}

QWidget *UISettingsSelectorTreeView::widget() const
{
    return m_pTreeWidget;
}

QWidget *UISettingsSelectorTreeView::addItem(const QString & /* strBigIcon */,
                                             const QString &strMediumIcon ,
                                             const QString & /* strSmallIcon */,
                                             int iID,
                                             const QString &strLink,
                                             UISettingsPage *pPage /* = 0 */,
                                             int iParentID /* = -1 */)
{
    QWidget *pResult = 0;
    if (pPage != 0)
    {
        const QIcon icon = UIIconPool::iconSet(strMediumIcon);

        UISelectorItem *pItem = new UISelectorItem(icon, "", iID, strLink, pPage, iParentID);
        m_list.append(pItem);

        QTreeWidgetItem *pTwItem = new QITreeWidgetItem(m_pTreeWidget, QStringList() << QString("")
                                                                                     << idToString(iID)
                                                                                     << strLink);
        pTwItem->setIcon(TreeWidgetSection_Category, pItem->icon());
        pPage->setContentsMargins(0, 0, 0, 0);
        pPage->layout()->setContentsMargins(0, 0, 0, 0);
        pResult = pPage;
    }
    return pResult;
}

void UISettingsSelectorTreeView::setItemText(int iID, const QString &strText)
{
    UISettingsSelector::setItemText(iID, strText);
    QTreeWidgetItem *pItem = findItem(m_pTreeWidget, idToString(iID), TreeWidgetSection_Id);
    if (pItem)
        pItem->setText(TreeWidgetSection_Category, QString(" %1 ").arg(strText));
}

QString UISettingsSelectorTreeView::itemText(int iID) const
{
    return pagePath(idToString(iID));
}

int UISettingsSelectorTreeView::currentId() const
{
    int iID = -1;
    const QTreeWidgetItem *pItem = m_pTreeWidget->currentItem();
    if (pItem)
        iID = pItem->text(TreeWidgetSection_Id).toInt();
    return iID;
}

int UISettingsSelectorTreeView::linkToId(const QString &strLink) const
{
    int iID = -1;
    const QTreeWidgetItem *pItem = findItem(m_pTreeWidget, strLink, TreeWidgetSection_Link);
    if (pItem)
        iID = pItem->text(TreeWidgetSection_Id).toInt();
    return iID;
}

void UISettingsSelectorTreeView::selectById(int iID)
{
    QTreeWidgetItem *pItem = findItem(m_pTreeWidget, idToString(iID), TreeWidgetSection_Id);
    if (pItem)
        m_pTreeWidget->setCurrentItem(pItem);
}

void UISettingsSelectorTreeView::setVisibleById(int iID, bool fVisible)
{
    QTreeWidgetItem *pItem = findItem(m_pTreeWidget, idToString(iID), TreeWidgetSection_Id);
    if (pItem)
        pItem->setHidden(!fVisible);
}

void UISettingsSelectorTreeView::polish()
{
    /* Get recommended size hint: */
    const QStyle *pStyle = QApplication::style();
    const int iIconMetric = pStyle->pixelMetric(QStyle::PM_SmallIconSize);
    int iItemWidth = static_cast<QAbstractItemView*>(m_pTreeWidget)->sizeHintForColumn(TreeWidgetSection_Category);
    int iItemHeight = qMax((int)(iIconMetric * 1.5) /* icon height */,
                           m_pTreeWidget->fontMetrics().height() /* text height */);
    /* Add some margin to every item in the tree: */
    iItemHeight += 4 /* margin itself */ * 2 /* margin count */;
    /* Set final size hint for items: */
    m_pTreeWidget->setSizeHintForItems(QSize(iItemWidth , iItemHeight));

    /* Adjust selector width/height: */
    m_pTreeWidget->setFixedWidth(iItemWidth + 2 * m_pTreeWidget->frameWidth());
    m_pTreeWidget->setMinimumHeight(m_pTreeWidget->topLevelItemCount() * iItemHeight +
                                    1 /* margin itself */ * 2 /* margin count */);

    /* Sort selector by the id column: */
    m_pTreeWidget->sortItems(TreeWidgetSection_Id, Qt::AscendingOrder);

    /* Resize column(s) to content: */
    m_pTreeWidget->resizeColumnToContents(TreeWidgetSection_Category);
}

void UISettingsSelectorTreeView::sltSettingsGroupChanged(QTreeWidgetItem *pItem,
                                                         QTreeWidgetItem * /* pPrevItem */)
{
    if (pItem)
    {
        const int iID = pItem->text(TreeWidgetSection_Id).toInt();
        Assert(iID >= 0);
        emit sigCategoryChanged(iID);
    }
}

void UISettingsSelectorTreeView::clear()
{
    m_pTreeWidget->clear();
}

QString UISettingsSelectorTreeView::pagePath(const QString &strMatch) const
{
    const QTreeWidgetItem *pTreeItem =
        findItem(m_pTreeWidget,
                 strMatch,
                 TreeWidgetSection_Id);
    return path(pTreeItem);
}

QTreeWidgetItem *UISettingsSelectorTreeView::findItem(QTreeWidget *pView,
                                                      const QString &strMatch,
                                                      int iColumn) const
{
    QList<QTreeWidgetItem*> list =
        pView->findItems(strMatch, Qt::MatchExactly, iColumn);

    return list.count() ? list[0] : 0;
}

QString UISettingsSelectorTreeView::idToString(int iID) const
{
    return QString("%1").arg(iID, 2, 10, QLatin1Char('0'));
}

/* static */
QString UISettingsSelectorTreeView::path(const QTreeWidgetItem *pItem)
{
    static QString strSep = ": ";
    QString strPath;
    const QTreeWidgetItem *pCurrentItem = pItem;
    while (pCurrentItem)
    {
        if (!strPath.isNull())
            strPath = strSep + strPath;
        strPath = pCurrentItem->text(TreeWidgetSection_Category).simplified() + strPath;
        pCurrentItem = pCurrentItem->parent();
    }
    return strPath;
}


/*********************************************************************************************************************************
*   Class UISettingsSelectorToolBar implementation.                                                                              *
*********************************************************************************************************************************/

UISettingsSelectorToolBar::UISettingsSelectorToolBar(QWidget *pParent /* = 0 */)
    : UISettingsSelector(pParent)
    , m_pToolBar(0)
    , m_pActionGroup(0)
{
    /* Install tool-bar button accessibility interface factory: */
    QAccessible::installFactory(UIAccessibilityInterfaceForUISettingsSelectorToolBarButton::pFactory);

    /* Prepare the toolbar: */
    m_pToolBar = new QIToolBar(pParent);
    m_pToolBar->setUseTextLabels(true);
    m_pToolBar->setIconSize(QSize(32, 32));
#ifdef VBOX_WS_MAC
    m_pToolBar->setShowToolBarButton(false);
#endif /* VBOX_WS_MAC */

    /* Prepare the action group: */
    m_pActionGroup = new QActionGroup(this);
    m_pActionGroup->setExclusive(true);
    connect(m_pActionGroup, &QActionGroup::triggered,
            this, static_cast<void(UISettingsSelectorToolBar::*)(QAction*)>(&UISettingsSelectorToolBar::sltSettingsGroupChanged));
}

UISettingsSelectorToolBar::~UISettingsSelectorToolBar()
{
    /* Cleanup the action group: */
    delete m_pActionGroup;
    m_pActionGroup = 0;

    /* Cleanup the toolbar: */
    delete m_pToolBar;
    m_pToolBar = 0;
}

QWidget *UISettingsSelectorToolBar::widget() const
{
    return m_pToolBar;
}

QWidget *UISettingsSelectorToolBar::addItem(const QString &strBigIcon,
                                            const QString & /* strMediumIcon */,
                                            const QString &strSmallIcon,
                                            int iID,
                                            const QString &strLink,
                                            UISettingsPage *pPage /* = 0 */,
                                            int iParentID /* = -1 */)
{
    const QIcon icon = UIIconPool::iconSet(strBigIcon);

    QWidget *pResult = 0;
    UISelectorActionItem *pItem = new UISelectorActionItem(icon, "", iID, strLink, pPage, iParentID, this);
    m_list.append(pItem);

    if (iParentID == -1 &&
        pPage != 0)
    {
        m_pActionGroup->addAction(pItem->action());
        m_pToolBar->addAction(pItem->action());
        m_pToolBar->widgetForAction(pItem->action())
            ->setProperty("Belongs to", "UISettingsSelectorToolBar");
        pPage->setContentsMargins(0, 0, 0, 0);
        pPage->layout()->setContentsMargins(0, 0, 0, 0);
        pResult = pPage;
    }
    else if (iParentID == -1 &&
             pPage == 0)
    {
        m_pActionGroup->addAction(pItem->action());
        m_pToolBar->addAction(pItem->action());
        m_pToolBar->widgetForAction(pItem->action())
            ->setProperty("Belongs to", "UISettingsSelectorToolBar");
        QITabWidget *pTabWidget = new QITabWidget();
        pTabWidget->setIconSize(QSize(16, 16));
        pTabWidget->setContentsMargins(0, 0, 0, 0);
//        connect(pTabWidget, SIGNAL(currentChanged(int)),
//                 this, SLOT(sltSettingsGroupChanged(int)));
        pItem->setTabWidget(pTabWidget);
        pResult = pTabWidget;
    }
    else
    {
        UISelectorActionItem *pParent = findActionItem(iParentID);
        if (pParent)
        {
            QTabWidget *pTabWidget = pParent->tabWidget();
            pPage->setContentsMargins(9, 5, 9, 9);
            pPage->layout()->setContentsMargins(0, 0, 0, 0);
            const QIcon icon1 = UIIconPool::iconSet(strSmallIcon);
            if (pTabWidget)
                pTabWidget->addTab(pPage, icon1, "");
        }
    }
    return pResult;
}

void UISettingsSelectorToolBar::setItemText(int iID, const QString &strText)
{
    if (UISelectorActionItem *pItem = findActionItem(iID))
    {
        pItem->setText(strText);
        if (pItem->action())
            pItem->action()->setText(strText);
        if (pItem->parentID() &&
            pItem->page())
        {
            const UISelectorActionItem *pParent = findActionItem(pItem->parentID());
            if (pParent &&
                pParent->tabWidget())
                pParent->tabWidget()->setTabText(
                    pParent->tabWidget()->indexOf(pItem->page()), strText);
        }
    }
}

QString UISettingsSelectorToolBar::itemText(int iID) const
{
    QString strResult;
    if (UISelectorItem *pItem = findItem(iID))
        strResult = pItem->text();
    return strResult;
}

int UISettingsSelectorToolBar::currentId() const
{
    const UISelectorActionItem *pAction = findActionItemByAction(m_pActionGroup->checkedAction());
    int iID = -1;
    if (pAction)
        iID = pAction->id();
    return iID;
}

int UISettingsSelectorToolBar::linkToId(const QString &strLink) const
{
    int iID = -1;
    const UISelectorItem *pItem = UISettingsSelector::findItemByLink(strLink);
    if (pItem)
        iID = pItem->id();
    return iID;
}

QWidget *UISettingsSelectorToolBar::idToPage(int iID) const
{
    QWidget *pPage = 0;
    if (const UISelectorActionItem *pItem = findActionItem(iID))
    {
        pPage = pItem->page();
        if (!pPage)
            pPage = pItem->tabWidget();
    }
    return pPage;
}

QWidget *UISettingsSelectorToolBar::rootPage(int iID) const
{
    QWidget *pPage = 0;
    if (const UISelectorActionItem *pItem = findActionItem(iID))
    {
        if (pItem->parentID() > -1)
            pPage = rootPage(pItem->parentID());
        else if (pItem->page())
            pPage = pItem->page();
        else
            pPage = pItem->tabWidget();
    }
    return pPage;
}

void UISettingsSelectorToolBar::selectById(int iID)
{
    if (const UISelectorActionItem *pItem = findActionItem(iID))
    {
        if (pItem->parentID() != -1)
        {
            const UISelectorActionItem *pParent = findActionItem(pItem->parentID());
            if (pParent &&
                pParent->tabWidget())
            {
                pParent->action()->trigger();
                pParent->tabWidget()->setCurrentIndex(
                    pParent->tabWidget()->indexOf(pItem->page()));
            }
        }
        else
            pItem->action()->trigger();
    }
}

void UISettingsSelectorToolBar::setVisibleById(int iID, bool fVisible)
{
    const UISelectorActionItem *pItem = findActionItem(iID);

    if (pItem)
    {
        pItem->action()->setVisible(fVisible);
        if (pItem->parentID() > -1 &&
            pItem->page())
        {
            const UISelectorActionItem *pParent = findActionItem(pItem->parentID());
            if (pParent &&
                pParent->tabWidget())
            {
                if (fVisible &&
                    pParent->tabWidget()->indexOf(pItem->page()) == -1)
                    pParent->tabWidget()->addTab(pItem->page(), pItem->text());
                else if (!fVisible &&
                         pParent->tabWidget()->indexOf(pItem->page()) > -1)
                    pParent->tabWidget()->removeTab(
                        pParent->tabWidget()->indexOf(pItem->page()));
            }
        }
    }

}

QList<QWidget*> UISettingsSelectorToolBar::rootPages() const
{
    QList<QWidget*> list;
    foreach (UISelectorItem *pItem, m_list)
    {
        const UISelectorActionItem *pActionItem = static_cast<UISelectorActionItem*>(pItem);
        if (pActionItem->parentID() == -1 &&
            pActionItem->page())
            list << pActionItem->page();
        else if (pActionItem->tabWidget())
            list << pActionItem->tabWidget();
    }
    return list;
}

int UISettingsSelectorToolBar::minWidth() const
{
    return m_pToolBar->sizeHint().width() + 2 * 10;
}

void UISettingsSelectorToolBar::sltSettingsGroupChanged(QAction *pAction)
{
    const UISelectorActionItem *pItem = findActionItemByAction(pAction);
    if (pItem)
    {
        emit sigCategoryChanged(pItem->id());
//        if (pItem->page() &&
//            !pItem->tabWidget())
//            emit sigCategoryChanged(pItem->id());
//        else
//        {
//
//            pItem->tabWidget()->blockSignals(true);
//            pItem->tabWidget()->setCurrentIndex(0);
//            pItem->tabWidget()->blockSignals(false);
//            printf("%s\n", qPrintable(pItem->text()));
//            UISelectorActionItem *child = static_cast<UISelectorActionItem*>(
//                findItemByPage(static_cast<UISettingsPage*>(pItem->tabWidget()->currentWidget())));
//            if (child)
//                emit sigCategoryChanged(child->id());
//        }
    }
}

void UISettingsSelectorToolBar::sltSettingsGroupChanged(int iIndex)
{
    const UISelectorActionItem *pItem = findActionItemByTabWidget(qobject_cast<QTabWidget*>(sender()), iIndex);
    if (pItem)
    {
        if (pItem->page() &&
            !pItem->tabWidget())
            emit sigCategoryChanged(pItem->id());
        else
        {
            const UISelectorActionItem *pChild = static_cast<UISelectorActionItem*>(
                findItemByPage(static_cast<UISettingsPage*>(pItem->tabWidget()->currentWidget())));
            if (pChild)
                emit sigCategoryChanged(pChild->id());
        }
    }
}

void UISettingsSelectorToolBar::clear()
{
    QList<QAction*> list = m_pActionGroup->actions();
    foreach (QAction *pAction, list)
       delete pAction;
}

UISelectorActionItem *UISettingsSelectorToolBar::findActionItem(int iID) const
{
    return static_cast<UISelectorActionItem*>(UISettingsSelector::findItem(iID));
}

UISelectorActionItem *UISettingsSelectorToolBar::findActionItemByAction(QAction *pAction) const
{
    UISelectorActionItem *pResult = 0;
    foreach (UISelectorItem *pItem, m_list)
        if (static_cast<UISelectorActionItem*>(pItem)->action() == pAction)
        {
            pResult = static_cast<UISelectorActionItem*>(pItem);
            break;
        }

    return pResult;
}

UISelectorActionItem *UISettingsSelectorToolBar::findActionItemByTabWidget(QTabWidget *pTabWidget, int iIndex) const
{
    UISelectorActionItem *pResult = 0;
    foreach (UISelectorItem *pItem, m_list)
        if (static_cast<UISelectorActionItem*>(pItem)->tabWidget() == pTabWidget)
        {
            QTabWidget *pTabWidget2 = static_cast<UISelectorActionItem*>(pItem)->tabWidget(); /// @todo r=bird: same as pTabWidget?
            pResult = static_cast<UISelectorActionItem*>(
                findItemByPage(static_cast<UISettingsPage*>(pTabWidget2->widget(iIndex))));
            break;
        }

    return pResult;

}

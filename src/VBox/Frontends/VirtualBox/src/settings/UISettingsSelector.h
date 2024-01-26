/* $Id: UISettingsSelector.h $ */
/** @file
 * VBox Qt GUI - UISettingsSelector class declaration.
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

#ifndef FEQT_INCLUDED_SRC_settings_UISettingsSelector_h
#define FEQT_INCLUDED_SRC_settings_UISettingsSelector_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* Qt includes: */
#include <QObject>

/* GUI includes: */
#include "UILibraryDefs.h"

/* Forward declarations: */
class QAction;
class QActionGroup;
class QTabWidget;
class QTreeWidget;
class QTreeWidgetItem;
class QITreeWidget;
class UISelectorActionItem;
class UISelectorItem;
class UISettingsPage;
class QIToolBar;


/** QObject subclass providing settings dialog
  * with the means to switch between settings pages. */
class SHARED_LIBRARY_STUFF UISettingsSelector : public QObject
{
    Q_OBJECT;

signals:

    /** Notifies listeners about selector @a iCategory changed. */
    void sigCategoryChanged(int iCategory);

public:

    /** Constructs settings selector passing @a pParent to the base-class. */
    UISettingsSelector(QWidget *pParent = 0);
    /** Destructs settings selector. */
    virtual ~UISettingsSelector() RT_OVERRIDE;

    /** Returns the widget selector operates on. */
    virtual QWidget *widget() const = 0;

    /** Adds a new selector item.
      * @param  strBigIcon     Brings the big icon reference.
      * @param  strMediumIcon  Brings the medium icon reference.
      * @param  strSmallIcon   Brings the small icon reference.
      * @param  iID            Brings the selector section ID.
      * @param  strLink        Brings the selector section link.
      * @param  pPage          Brings the selector section page reference.
      * @param  iParentID      Brings the parent section ID or -1 if there is no parent. */
    virtual QWidget *addItem(const QString &strBigIcon, const QString &strMediumIcon, const QString &strSmallIcon,
                             int iID, const QString &strLink, UISettingsPage *pPage = 0, int iParentID = -1) = 0;

    /** Defines the @a strText for section with @a iID. */
    virtual void setItemText(int iID, const QString &strText);
    /** Returns the text for section with @a iID. */
    virtual QString itemText(int iID) const = 0;
    /** Returns the text for section with @a pPage. */
    virtual QString itemTextByPage(UISettingsPage *pPage) const;

    /** Returns the current selector ID. */
    virtual int currentId() const = 0;

    /** Returns the section ID for passed @a strLink. */
    virtual int linkToId(const QString &strLink) const = 0;

    /** Returns the section page for passed @a iID. */
    virtual QWidget *idToPage(int iID) const;
    /** Returns the section root-page for passed @a iID. */
    virtual QWidget *rootPage(int iID) const { return idToPage(iID); }

    /** Make the section with @a iID current. */
    virtual void selectById(int iID) = 0;
    /** Make the section with @a strLink current. */
    virtual void selectByLink(const QString &strLink) { selectById(linkToId(strLink)); }

    /** Make the section with @a iID @a fVisible. */
    virtual void setVisibleById(int iID, bool fVisible) = 0;

    /** Returns the list of all selector pages. */
    virtual QList<UISettingsPage*> settingPages() const;
    /** Returns the list of all root pages. */
    virtual QList<QWidget*> rootPages() const;

    /** Performs selector polishing. */
    virtual void polish() {}

    /** Returns minimum selector width. */
    virtual int minWidth() const { return 0; }

protected:

    /** Clears selector of all the items. */
    virtual void clear() = 0;

    /** Searches for item with passed @a iID. */
    UISelectorItem *findItem(int iID) const;
    /** Searches for item with passed @a strLink. */
    UISelectorItem *findItemByLink(const QString &strLink) const;
    /** Searches for item with passed @a pPage. */
    UISelectorItem *findItemByPage(UISettingsPage *pPage) const;

    /** Holds the selector item instances. */
    QList<UISelectorItem*> m_list;
};


/** UISettingsSelector subclass providing settings dialog
  * with the means to switch between settings pages.
  * This one represented as tree-widget. */
class SHARED_LIBRARY_STUFF UISettingsSelectorTreeView : public UISettingsSelector
{
    Q_OBJECT;

public:

    /** Constructs settings selector passing @a pParent to the base-class. */
    UISettingsSelectorTreeView(QWidget *pParent = 0);
    /** Destructs settings selector. */
    virtual ~UISettingsSelectorTreeView() RT_OVERRIDE;

    /** Returns the widget selector operates on. */
    virtual QWidget *widget() const RT_OVERRIDE;

    /** Adds a new selector item.
      * @param  strBigIcon     Brings the big icon reference.
      * @param  strMediumIcon  Brings the medium icon reference.
      * @param  strSmallIcon   Brings the small icon reference.
      * @param  iID            Brings the selector section ID.
      * @param  strLink        Brings the selector section link.
      * @param  pPage          Brings the selector section page reference.
      * @param  iParentID      Brings the parent section ID or -1 if there is no parent. */
    virtual QWidget *addItem(const QString &strBigIcon, const QString &strMediumIcon, const QString &strSmallIcon,
                             int iID, const QString &strLink, UISettingsPage *pPage = 0, int iParentID = -1) RT_OVERRIDE;

    /** Defines the @a strText for section with @a iID. */
    virtual void setItemText(int iID, const QString &strText) RT_OVERRIDE;
    /** Returns the text for section with @a iID. */
    virtual QString itemText(int iID) const RT_OVERRIDE;

    /** Returns the current selector ID. */
    virtual int currentId() const RT_OVERRIDE;

    /** Returns the section ID for passed @a strLink. */
    virtual int linkToId(const QString &strLink) const RT_OVERRIDE;

    /** Make the section with @a iID current. */
    virtual void selectById(int iID) RT_OVERRIDE;

    /** Make the section with @a iID @a fVisible. */
    virtual void setVisibleById(int iID, bool fVisible) RT_OVERRIDE;

    /** Performs selector polishing. */
    virtual void polish() RT_OVERRIDE;

private slots:

    /** Handles selector section change from @a pPrevItem to @a pItem. */
    void sltSettingsGroupChanged(QTreeWidgetItem *pItem, QTreeWidgetItem *pPrevItem);

private:

    /** Clears selector of all the items. */
    virtual void clear() RT_OVERRIDE;

    /** Returns page path for passed @a strMatch. */
    QString pagePath(const QString &strMatch) const;
    /** Find item within the passed @a pView and @a iColumn matching @a strMatch. */
    QTreeWidgetItem *findItem(QTreeWidget *pView, const QString &strMatch, int iColumn) const;
    /** Performs @a iID to QString serialization. */
    QString idToString(int iID) const;

    /** Forges the full path for passed @a pItem. */
    static QString path(const QTreeWidgetItem *pItem);

    /** Holds the tree-widget instance. */
    QITreeWidget *m_pTreeWidget;
};


/** UISettingsSelector subclass providing settings dialog
  * with the means to switch between settings pages.
  * This one represented as tab-widget. */
class SHARED_LIBRARY_STUFF UISettingsSelectorToolBar : public UISettingsSelector
{
    Q_OBJECT;

public:

    /** Constructs settings selector passing @a pParent to the base-class. */
    UISettingsSelectorToolBar(QWidget *pParent = 0);
    /** Destructs settings selector. */
    virtual ~UISettingsSelectorToolBar() RT_OVERRIDE;

    /** Returns the widget selector operates on. */
    virtual QWidget *widget() const RT_OVERRIDE;

    /** Adds a new selector item.
      * @param  strBigIcon     Brings the big icon reference.
      * @param  strMediumIcon  Brings the medium icon reference.
      * @param  strSmallIcon   Brings the small icon reference.
      * @param  iID            Brings the selector section ID.
      * @param  strLink        Brings the selector section link.
      * @param  pPage          Brings the selector section page reference.
      * @param  iParentID      Brings the parent section ID or -1 if there is no parent. */
    virtual QWidget *addItem(const QString &strBigIcon, const QString &strMediumIcon, const QString &strSmallIcon,
                             int iID, const QString &strLink, UISettingsPage *pPage = 0, int iParentID = -1) RT_OVERRIDE;

    /** Defines the @a strText for section with @a iID. */
    virtual void setItemText(int iID, const QString &strText) RT_OVERRIDE;
    /** Returns the text for section with @a iID. */
    virtual QString itemText(int iID) const RT_OVERRIDE;

    /** Returns the current selector ID. */
    virtual int currentId() const RT_OVERRIDE;

    /** Returns the section ID for passed @a strLink. */
    virtual int linkToId(const QString &strLink) const RT_OVERRIDE;

    /** Returns the section page for passed @a iID. */
    virtual QWidget *idToPage(int iID) const RT_OVERRIDE;
    /** Returns the section root-page for passed @a iID. */
    virtual QWidget *rootPage(int iID) const RT_OVERRIDE;

    /** Make the section with @a iID current. */
    virtual void selectById(int iID) RT_OVERRIDE;

    /** Make the section with @a iID @a fVisible. */
    virtual void setVisibleById(int iID, bool fVisible) RT_OVERRIDE;

    /** Returns the list of all root pages. */
    virtual QList<QWidget*> rootPages() const RT_OVERRIDE;

    /** Returns minimum selector width. */
    virtual int minWidth() const RT_OVERRIDE;

private slots:

    /** Handles selector section change to @a pAction. */
    void sltSettingsGroupChanged(QAction *pAction);
    /** Handles selector section change to @a iIndex. */
    void sltSettingsGroupChanged(int iIndex);

private:

    /** Clears selector of all the items. */
    virtual void clear() RT_OVERRIDE;

    /** Searches for action item with passed @a iID. */
    UISelectorActionItem *findActionItem(int iID) const;
    /** Searches for action item with passed @a pAction. */
    UISelectorActionItem *findActionItemByAction(QAction *pAction) const;
    /** Searches for action item with passed @a pTabWidget and @a iIndex. */
    UISelectorActionItem *findActionItemByTabWidget(QTabWidget *pTabWidget, int iIndex) const;

    /** Holds the toolbar instance. */
    QIToolBar *m_pToolBar;
    /** Holds the action group instance. */
    QActionGroup *m_pActionGroup;
};


#endif /* !FEQT_INCLUDED_SRC_settings_UISettingsSelector_h */

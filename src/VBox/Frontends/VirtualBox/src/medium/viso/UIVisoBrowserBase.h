/* $Id: UIVisoBrowserBase.h $ */
/** @file
 * VBox Qt GUI - UIVisoBrowserBase class declaration.
 */

/*
 * Copyright (C) 2006-2023 Oracle and/or its affiliates.
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

#ifndef FEQT_INCLUDED_SRC_medium_viso_UIVisoBrowserBase_h
#define FEQT_INCLUDED_SRC_medium_viso_UIVisoBrowserBase_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* Qt includes: */
#include <QModelIndex>
#include <QGroupBox>

/* GUI includes: */
#include "QIWithRetranslateUI.h"

/* Forward declarations: */
class QItemSelection;
class QGridLayout;
class QTreeView;
class UILocationSelector;

/** An abstract QWidget extension hosting a tree and table view. */
class UIVisoBrowserBase : public QIWithRetranslateUI<QGroupBox>
{
    Q_OBJECT;

signals:

    void sigTreeViewVisibilityChanged(bool fVisible);
    void sigCreateFileTableViewContextMenu(QWidget *pMenuRequester, const QPoint &point);

public:

    UIVisoBrowserBase(QWidget *pParent = 0);
    ~UIVisoBrowserBase();
    virtual void showHideHiddenObjects(bool bShow) = 0;
    /* Returns true if tree view is currently visible: */
    bool isTreeViewVisible() const;
    void hideTreeView();
    virtual bool tableViewHasSelection() const = 0;

public slots:

    void sltHandleTableViewItemDoubleClick(const QModelIndex &index);

protected:

    void prepareObjects();
    void prepareConnections();
    void updateLocationSelectorText(const QString &strText);

    virtual void tableViewItemDoubleClick(const QModelIndex &index) = 0;
    virtual void treeSelectionChanged(const QModelIndex &selectedTreeIndex) = 0;
    virtual void setTableRootIndex(QModelIndex index = QModelIndex()) = 0;
    virtual void setTreeCurrentIndex(QModelIndex index = QModelIndex()) = 0;

    virtual void resizeEvent(QResizeEvent *pEvent) RT_OVERRIDE;
    virtual bool eventFilter(QObject *pObj, QEvent *pEvent) RT_OVERRIDE;
    virtual void keyPressEvent(QKeyEvent *pEvent) RT_OVERRIDE;

    QTreeView          *m_pTreeView;
    QGridLayout        *m_pMainLayout;

protected slots:

    void sltFileTableViewContextMenu(const QPoint &point);

private slots:

    void sltHandleTreeSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected);
    void sltHandleTreeItemClicked(const QModelIndex &modelIndex);
    void sltExpandCollapseTreeView();

private:

    void updateTreeViewGeometry(bool fShow);
    UILocationSelector    *m_pLocationSelector;
};

#endif /* !FEQT_INCLUDED_SRC_medium_viso_UIVisoBrowserBase_h */

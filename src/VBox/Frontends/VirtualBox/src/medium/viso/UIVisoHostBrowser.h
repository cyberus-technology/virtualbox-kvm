/* $Id: UIVisoHostBrowser.h $ */
/** @file
 * VBox Qt GUI - UIVisoHostBrowser class declaration.
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

#ifndef FEQT_INCLUDED_SRC_medium_viso_UIVisoHostBrowser_h
#define FEQT_INCLUDED_SRC_medium_viso_UIVisoHostBrowser_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* GUI includes: */
#include "UIVisoBrowserBase.h"

/* Forward declarations: */
class QItemSelection;
class QTableView;
class UIVisoHostBrowserModel;

/** A UIVisoBrowserBase extension to view host file system. Uses QFileSystemModel. */
class UIVisoHostBrowser : public UIVisoBrowserBase
{
    Q_OBJECT;

signals:

    void sigAddObjectsToViso(QStringList pathList);
    void sigTableSelectionChanged(bool fIsSelectionEmpty);

public:

    UIVisoHostBrowser(QWidget *pParent = 0);
    ~UIVisoHostBrowser();
    virtual void showHideHiddenObjects(bool bShow) final override;
    QString      currentPath() const;
    void         setCurrentPath(const QString &strPath);
    virtual bool tableViewHasSelection() const final override;

public slots:

    void sltHandleAddAction();

protected:

    virtual void retranslateUi() final override;
    virtual void tableViewItemDoubleClick(const QModelIndex &index) final override;
    virtual void setTableRootIndex(QModelIndex index = QModelIndex()) final override;
    virtual void setTreeCurrentIndex(QModelIndex index = QModelIndex()) final override;
    virtual void treeSelectionChanged(const QModelIndex &selectedTreeIndex) final override;

private slots:

    void sltHandleTableSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected);

private:

    void prepareObjects();
    void prepareConnections();

    /** We have two file system models (one for each item view) since we set different filters on each of these models. */
    UIVisoHostBrowserModel *m_pTreeModel;
    UIVisoHostBrowserModel *m_pTableModel;
    QTableView             *m_pTableView;
};


#endif /* !FEQT_INCLUDED_SRC_medium_viso_UIVisoHostBrowser_h */

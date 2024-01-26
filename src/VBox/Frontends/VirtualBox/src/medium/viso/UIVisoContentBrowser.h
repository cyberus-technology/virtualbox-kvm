/* $Id: UIVisoContentBrowser.h $ */
/** @file
 * VBox Qt GUI - UIVisoContentBrowser class declaration.
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

#ifndef FEQT_INCLUDED_SRC_medium_viso_UIVisoContentBrowser_h
#define FEQT_INCLUDED_SRC_medium_viso_UIVisoContentBrowser_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* Qt includes: */
#include <QWidget>

/* GUI includes: */
#include "UIVisoBrowserBase.h"

/* COM includes: */
#include "COMEnums.h"

/* Forward declarations: */
class QFileInfo;
class UICustomFileSystemItem;
class UICustomFileSystemModel;
class UICustomFileSystemProxyModel;
class UIVisoContentTreeProxyModel;
class UIVisoContentTableView;

/** A UIVisoBrowserBase extension to view content of a VISO as a file tree. */
class UIVisoContentBrowser : public UIVisoBrowserBase
{
    Q_OBJECT;

signals:

    void sigTableSelectionChanged(bool fIsSelectionEmpty);

public:

    UIVisoContentBrowser(QWidget *pParent = 0);
    ~UIVisoContentBrowser();
    /** Adds file objests from the host file system. @p pathList consists of list of paths to there objects. */
    void addObjectsToViso(QStringList pathList);
    /** Returns the content of the VISO as a string list. Each element of the list becomes a line in the
      * .viso file. */
    QStringList entryList();
    virtual void showHideHiddenObjects(bool bShow)  override final;
    void setVisoName(const QString &strName);
    virtual bool tableViewHasSelection() const final override;

public slots:

    void sltHandleCreateNewDirectory();
    /** Handles the signal we get from the model during setData call. Restores the old name of the file object
     *  to @p strOldName if need be (if rename fails for some reason). */
    void sltHandleItemRenameAttempt(UICustomFileSystemItem *pItem, QString strOldName, QString strNewName);
    void sltHandleRemoveItems();
    void sltHandleResetAction();
    void sltHandleItemRenameAction();

protected:

    void retranslateUi() final override;
    virtual void tableViewItemDoubleClick(const QModelIndex &index)  final override;
    /** @name Functions to set view root indices explicitly. They block the related signals. @p is converted
        to the correct index before setting.
      * @{ */
        virtual void setTableRootIndex(QModelIndex index = QModelIndex())  final override;
        virtual void setTreeCurrentIndex(QModelIndex index = QModelIndex()) final override;
    /** @} */

    virtual void treeSelectionChanged(const QModelIndex &selectedTreeIndex) final override;

private slots:

    void sltHandleTableSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected);
    /** Adds the dragged-dropped items to VISO. */
    void sltHandleDroppedItems(QStringList pathList);

private:

    void                    prepareObjects();
    void                    prepareConnections();
    void                    initializeModel();
    UICustomFileSystemItem *rootItem();

    /** @name Index conversion functions. These are half-smart and tries to determine the source model before conversion.
      * @{ */
        QModelIndex         convertIndexToTableIndex(const QModelIndex &index);
        QModelIndex         convertIndexToTreeIndex(const QModelIndex &index);
    /** @} */
    /** Lists the content of the host file system directory by using Qt file system API. */
    void                    scanHostDirectory(UICustomFileSystemItem *directory);
    KFsObjType              fileType(const QFileInfo &fsInfo);
    /** Renames the starts item's name as VISO name changes. */
    void                    updateStartItemName();
    void                    renameFileObject(UICustomFileSystemItem *pItem);
    void                    removeItems(const QList<UICustomFileSystemItem*> itemList);
    /** Creates and entry for pItem consisting of a map item (key is iso path and value is host file system path)
     *  if @p bRemove is true then the value is the string ":remove:" which effectively removes the file object
     *  from the iso image. */
    void                    createAnIsoEntry(UICustomFileSystemItem *pItem, bool bRemove = false);
    void                    reset();
    /** Returns a list of items which are currecntly selected
     *  in the table view. */
    QList<UICustomFileSystemItem*> tableSelectedItems();
    UIVisoContentTableView       *m_pTableView;
    UICustomFileSystemModel      *m_pModel;
    UICustomFileSystemProxyModel *m_pTableProxyModel;
    UIVisoContentTreeProxyModel  *m_pTreeProxyModel;

    QString                       m_strVisoName;
    /** keys of m_entryMap are iso locations and values are
     *  local location of file objects. these keys and values are
     *  concatenated and passed to the client to create ad-hoc.viso entries. */
    QMap<QString, QString>        m_entryMap;
};

#endif /* !FEQT_INCLUDED_SRC_medium_viso_UIVisoContentBrowser_h */

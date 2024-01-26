/* $Id: UIVisoContentBrowser.cpp $ */
/** @file
 * VBox Qt GUI - UIVisoContentBrowser class implementation.
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


/* Qt includes: */
#include <QDir>
#include <QFileInfo>
#include <QGridLayout>
#include <QHeaderView>
#include <QMimeData>
#include <QTableView>
#include <QTreeView>

/* GUI includes: */
#include "UICustomFileSystemModel.h"
#include "UIPathOperations.h"
#include "UIVisoContentBrowser.h"

/*********************************************************************************************************************************
*   UIVisoContentTableView definition.                                                                                      *
*********************************************************************************************************************************/

/** An QTableView extension mainly used to handle dropeed file objects from the host browser. */
class UIVisoContentTableView : public QTableView
{
    Q_OBJECT;

signals:

    void sigNewItemsDropped(QStringList pathList);

public:

    UIVisoContentTableView(QWidget *pParent = 0);
    void dragEnterEvent(QDragEnterEvent *event);
    void dropEvent(QDropEvent *event);
    void dragMoveEvent(QDragMoveEvent *event);
};


/*********************************************************************************************************************************
*   UIVisoContentTreeProxyModel definition.                                                                                      *
*********************************************************************************************************************************/

class UIVisoContentTreeProxyModel : public UICustomFileSystemProxyModel
{

    Q_OBJECT;

public:

    UIVisoContentTreeProxyModel(QObject *parent = 0);

protected:

    /** Used to filter-out files and show only directories. */
    virtual bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const RT_OVERRIDE;
};


/*********************************************************************************************************************************
*   UIVisoContentTableView implementation.                                                                                       *
*********************************************************************************************************************************/
UIVisoContentTableView::UIVisoContentTableView(QWidget *pParent /* = 0 */)
    :QTableView(pParent)
{
}

void UIVisoContentTableView::dragMoveEvent(QDragMoveEvent *event)
{
    event->acceptProposedAction();

}

void UIVisoContentTableView::dragEnterEvent(QDragEnterEvent *pEvent)
{
    if (pEvent->mimeData()->hasFormat("application/vnd.text.list"))
        pEvent->accept();
    else
        pEvent->ignore();
}

void UIVisoContentTableView::dropEvent(QDropEvent *pEvent)
{
    if (pEvent->mimeData()->hasFormat("application/vnd.text.list"))
    {
        QByteArray itemData = pEvent->mimeData()->data("application/vnd.text.list");
        QDataStream stream(&itemData, QIODevice::ReadOnly);
        QStringList pathList;

        while (!stream.atEnd()) {
            QString text;
            stream >> text;
            pathList << text;
        }
        emit sigNewItemsDropped(pathList);
    }
}


/*********************************************************************************************************************************
*   UIVisoContentTreeProxyModel implementation.                                                                                  *
*********************************************************************************************************************************/

UIVisoContentTreeProxyModel::UIVisoContentTreeProxyModel(QObject *parent /* = 0 */)
    :UICustomFileSystemProxyModel(parent)
{
}

bool UIVisoContentTreeProxyModel::filterAcceptsRow(int iSourceRow, const QModelIndex &sourceParent) const /* override */
{
    QModelIndex itemIndex = sourceModel()->index(iSourceRow, 0, sourceParent);
    if (!itemIndex.isValid())
        return false;

    UICustomFileSystemItem *item = static_cast<UICustomFileSystemItem*>(itemIndex.internalPointer());
    if (!item)
        return false;

    if (item->isUpDirectory())
        return false;
    if (item->isDirectory() || item->isSymLinkToADirectory())
        return true;

    return false;
}


/*********************************************************************************************************************************
*   UIVisoContentBrowser implementation.                                                                                         *
*********************************************************************************************************************************/

UIVisoContentBrowser::UIVisoContentBrowser(QWidget *pParent)
    : UIVisoBrowserBase(pParent)
    , m_pTableView(0)
    , m_pModel(0)
    , m_pTableProxyModel(0)
    , m_pTreeProxyModel(0)
{
    prepareObjects();
    prepareConnections();

    /* Assuming the root items only child is the one with the path '/', navigate into it. */
    /* Hack alert. for some reason without invalidating proxy models mapFromSource return invalid index. */
    if (m_pTableProxyModel)
        m_pTableProxyModel->invalidate();
    if (m_pTreeProxyModel)
        m_pTreeProxyModel->setSourceModel(m_pModel);
    if (rootItem() && rootItem()->childCount() > 0)
    {
        UICustomFileSystemItem *pStartItem = static_cast<UICustomFileSystemItem*>(rootItem()->children()[0]);
        if (pStartItem)
        {
            QModelIndex iindex = m_pModel->index(pStartItem);
            if (iindex.isValid())
                tableViewItemDoubleClick(convertIndexToTableIndex(iindex));
        }
    }
}

UIVisoContentBrowser::~UIVisoContentBrowser()
{
}

void UIVisoContentBrowser::addObjectsToViso(QStringList pathList)
{
    if (!m_pTableView)
        return;

    QModelIndex parentIndex = m_pTableProxyModel->mapToSource(m_pTableView->rootIndex());
    if (!parentIndex.isValid())
         return;

    UICustomFileSystemItem *pParentItem = static_cast<UICustomFileSystemItem*>(parentIndex.internalPointer());
    if (!pParentItem)
        return;
    foreach (const QString &strPath, pathList)
    {
        QFileInfo fileInfo(strPath);
        if (!fileInfo.exists())
            continue;
        if (pParentItem->child(fileInfo.fileName()))
            continue;

        UICustomFileSystemItem* pAddedItem = new UICustomFileSystemItem(fileInfo.fileName(), pParentItem,
                                                                        fileType(fileInfo));
        pAddedItem->setData(strPath, UICustomFileSystemModelColumn_LocalPath);
        pAddedItem->setData(UIPathOperations::mergePaths(pParentItem->path(), fileInfo.fileName()),
                           UICustomFileSystemModelColumn_Path);
        pAddedItem->setIsOpened(false);
        if (fileInfo.isSymLink())
        {
            pAddedItem->setTargetPath(fileInfo.symLinkTarget());
            pAddedItem->setIsSymLinkToADirectory(QFileInfo(fileInfo.symLinkTarget()).isDir());
        }
        createAnIsoEntry(pAddedItem);
    }
    if (m_pTableProxyModel)
        m_pTableProxyModel->invalidate();
    if (m_pTreeProxyModel)
    {
        m_pTreeProxyModel->invalidate();
        m_pTreeView->setExpanded(m_pTreeView->currentIndex(), true);
    }
}

void UIVisoContentBrowser::createAnIsoEntry(UICustomFileSystemItem *pItem, bool bRemove /* = false */)
{
    if (!pItem)
        return;
    if (pItem->data(UICustomFileSystemModelColumn_Path).toString().isEmpty())
        return;

    if (!bRemove && pItem->data(UICustomFileSystemModelColumn_LocalPath).toString().isEmpty())
        return;
    if (!bRemove)
        m_entryMap.insert(pItem->data(UICustomFileSystemModelColumn_Path).toString(),
                          pItem->data(UICustomFileSystemModelColumn_LocalPath).toString());
    else
        m_entryMap.insert(pItem->data(UICustomFileSystemModelColumn_Path).toString(),
                          ":remove:");
}

QStringList UIVisoContentBrowser::entryList()
{
    QStringList entryList;
    for (QMap<QString, QString>::const_iterator iterator = m_entryMap.begin(); iterator != m_entryMap.end(); ++iterator)
    {
        QString strEntry = QString("%1=%2").arg(iterator.key()).arg(iterator.value());
        entryList << strEntry;
    }
    return entryList;
}

void UIVisoContentBrowser::retranslateUi()
{
    UICustomFileSystemItem *pRootItem = rootItem();
    if (pRootItem)
    {
        pRootItem->setData(QApplication::translate("UIVisoCreatorWidget", "Name"), UICustomFileSystemModelColumn_Name);
        pRootItem->setData(QApplication::translate("UIVisoCreatorWidget", "Size"), UICustomFileSystemModelColumn_Size);
        pRootItem->setData(QApplication::translate("UIVisoCreatorWidget", "Change Time"), UICustomFileSystemModelColumn_ChangeTime);
        pRootItem->setData(QApplication::translate("UIVisoCreatorWidget", "Owner"), UICustomFileSystemModelColumn_Owner);
        pRootItem->setData(QApplication::translate("UIVisoCreatorWidget", "Permissions"), UICustomFileSystemModelColumn_Permissions);
        pRootItem->setData(QApplication::translate("UIVisoCreatorWidget", "Local Path"), UICustomFileSystemModelColumn_LocalPath);
        pRootItem->setData(QApplication::translate("UIVisoCreatorWidget", "ISO Path"), UICustomFileSystemModelColumn_Path);
    }
}

void UIVisoContentBrowser::tableViewItemDoubleClick(const QModelIndex &index)
{
    if (!index.isValid() || !m_pTableProxyModel)
        return;
    UICustomFileSystemItem *pClickedItem =
        static_cast<UICustomFileSystemItem*>(m_pTableProxyModel->mapToSource(index).internalPointer());
    if (pClickedItem->isUpDirectory())
    {
        QModelIndex currentRoot = m_pTableProxyModel->mapToSource(m_pTableView->rootIndex());
        /* Go up if we are not already there: */
        if (currentRoot != m_pModel->rootIndex())
        {
            setTableRootIndex(currentRoot.parent());
            setTreeCurrentIndex(currentRoot.parent());
        }
    }
    else
    {
        scanHostDirectory(pClickedItem);
        setTableRootIndex(index);
        setTreeCurrentIndex(index);
    }
}

void UIVisoContentBrowser::sltHandleCreateNewDirectory()
{
    if (!m_pTableView)
        return;
    QString strNewDirectoryName("NewDirectory");

    QModelIndex parentIndex = m_pTableProxyModel->mapToSource(m_pTableView->rootIndex());
    if (!parentIndex.isValid())
         return;

    UICustomFileSystemItem *pParentItem = static_cast<UICustomFileSystemItem*>(parentIndex.internalPointer());
    if (!pParentItem)
        return;

    /*  Check to see if we already have a directory named strNewDirectoryName: */
    const QList<UICustomFileSystemItem*> children = pParentItem->children();
    foreach (const UICustomFileSystemItem *item, children)
    {
        if (item->name() == strNewDirectoryName)
            return;
    }

    UICustomFileSystemItem* pAddedItem = new UICustomFileSystemItem(strNewDirectoryName, pParentItem,
                                                                    KFsObjType_Directory);
    pAddedItem->setData(UIPathOperations::mergePaths(pParentItem->path(), strNewDirectoryName), UICustomFileSystemModelColumn_Path);

    pAddedItem->setIsOpened(false);
    if (m_pTableProxyModel)
        m_pTableProxyModel->invalidate();

    renameFileObject(pAddedItem);
}

void UIVisoContentBrowser::sltHandleRemoveItems()
{
    removeItems(tableSelectedItems());
}

void UIVisoContentBrowser::removeItems(const QList<UICustomFileSystemItem*> itemList)
{
    foreach(UICustomFileSystemItem *pItem, itemList)
    {
        if (!pItem)
            continue;
        QString strIsoPath = pItem->data(UICustomFileSystemModelColumn_Path).toString();
        if (strIsoPath.isEmpty())
            continue;

        bool bFoundInMap = false;
        for (QMap<QString, QString>::iterator iterator = m_entryMap.begin(); iterator != m_entryMap.end(); )
        {
            if (iterator.key().startsWith(strIsoPath))
            {
                iterator = m_entryMap.erase(iterator);
                bFoundInMap = true;
            }
            else
                ++iterator;
        }
        if (!bFoundInMap)
            createAnIsoEntry(pItem, true /* bool bRemove */);
    }

    foreach(UICustomFileSystemItem *pItem, itemList)
    {
        if (!pItem)
            continue;
        /* Remove the item from the m_pModel: */
        if (m_pModel)
            m_pModel->deleteItem(pItem);
    }
    if (m_pTreeProxyModel)
        m_pTreeProxyModel->invalidate();
    if (m_pTableProxyModel)
        m_pTableProxyModel->invalidate();

}

void UIVisoContentBrowser::prepareObjects()
{
    UIVisoBrowserBase::prepareObjects();

    m_pModel = new UICustomFileSystemModel(this);
    m_pTableProxyModel = new UICustomFileSystemProxyModel(this);
    if (m_pTableProxyModel)
    {
        m_pTableProxyModel->setSourceModel(m_pModel);
        m_pTableProxyModel->setListDirectoriesOnTop(true);
    }

    m_pTreeProxyModel = new UIVisoContentTreeProxyModel(this);
    if (m_pTreeProxyModel)
    {
        m_pTreeProxyModel->setSourceModel(m_pModel);
    }

    initializeModel();

    if (m_pTreeView)
    {
        m_pTreeView->setModel(m_pTreeProxyModel);
        m_pTreeView->setCurrentIndex(m_pTreeProxyModel->mapFromSource(m_pModel->rootIndex()));
        m_pTreeView->setEditTriggers(QAbstractItemView::NoEditTriggers);
        /* Show only the 0th column that is "name': */
        m_pTreeView->hideColumn(UICustomFileSystemModelColumn_Owner);
        m_pTreeView->hideColumn(UICustomFileSystemModelColumn_Permissions);
        m_pTreeView->hideColumn(UICustomFileSystemModelColumn_Size);
        m_pTreeView->hideColumn(UICustomFileSystemModelColumn_ChangeTime);
        m_pTreeView->hideColumn(UICustomFileSystemModelColumn_Path);
        m_pTreeView->hideColumn(UICustomFileSystemModelColumn_LocalPath);
    }

    m_pTableView = new UIVisoContentTableView;
    if (m_pTableView)
    {
        m_pMainLayout->addWidget(m_pTableView, 1, 0, 6, 4);
        m_pTableView->setContextMenuPolicy(Qt::CustomContextMenu);
        m_pTableView->setSelectionMode(QAbstractItemView::ExtendedSelection);
        m_pTableView->setShowGrid(false);
        m_pTableView->setSelectionBehavior(QAbstractItemView::SelectRows);
        m_pTableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
        m_pTableView->setAlternatingRowColors(true);
        m_pTableView->setTabKeyNavigation(false);
        QHeaderView *pVerticalHeader = m_pTableView->verticalHeader();
        if (pVerticalHeader)
        {
            m_pTableView->verticalHeader()->setVisible(false);
            /* Minimize the row height: */
            m_pTableView->verticalHeader()->setDefaultSectionSize(m_pTableView->verticalHeader()->minimumSectionSize());
        }
        QHeaderView *pHorizontalHeader = m_pTableView->horizontalHeader();
        if (pHorizontalHeader)
        {
            pHorizontalHeader->setHighlightSections(false);
            pHorizontalHeader->setSectionResizeMode(QHeaderView::Stretch);
        }

        m_pTableView->setModel(m_pTableProxyModel);
        setTableRootIndex();
        m_pTableView->hideColumn(UICustomFileSystemModelColumn_Owner);
        m_pTableView->hideColumn(UICustomFileSystemModelColumn_Permissions);
        m_pTableView->hideColumn(UICustomFileSystemModelColumn_Size);
        m_pTableView->hideColumn(UICustomFileSystemModelColumn_ChangeTime);

        m_pTableView->setSortingEnabled(true);
        m_pTableView->sortByColumn(0, Qt::AscendingOrder);

        m_pTableView->setDragEnabled(false);
        m_pTableView->setAcceptDrops(true);
        m_pTableView->setDropIndicatorShown(true);
        m_pTableView->setDragDropMode(QAbstractItemView::DropOnly);
    }
    retranslateUi();
}

void UIVisoContentBrowser::prepareConnections()
{
    UIVisoBrowserBase::prepareConnections();

    if (m_pTableView)
    {
        connect(m_pTableView, &UIVisoContentTableView::doubleClicked,
                this, &UIVisoBrowserBase::sltHandleTableViewItemDoubleClick);
        connect(m_pTableView, &UIVisoContentTableView::sigNewItemsDropped,
                this, &UIVisoContentBrowser::sltHandleDroppedItems);
        connect(m_pTableView, &QTableView::customContextMenuRequested,
                this, &UIVisoContentBrowser::sltFileTableViewContextMenu);
    }

    if (m_pTableView->selectionModel())
        connect(m_pTableView->selectionModel(), &QItemSelectionModel::selectionChanged,
                this, &UIVisoContentBrowser::sltHandleTableSelectionChanged);
    if (m_pModel)
        connect(m_pModel, &UICustomFileSystemModel::sigItemRenamed,
                this, &UIVisoContentBrowser::sltHandleItemRenameAttempt);
}

UICustomFileSystemItem* UIVisoContentBrowser::rootItem()
{
    if (!m_pModel)
        return 0;
    return m_pModel->rootItem();
}

void UIVisoContentBrowser::initializeModel()
{
    if (m_pModel)
        m_pModel->reset();
    if (!rootItem())
        return;

    const QString startPath = QString("/%1").arg(m_strVisoName);

    UICustomFileSystemItem *pStartItem = new UICustomFileSystemItem(startPath, rootItem(), KFsObjType_Directory);
    pStartItem->setPath("/");
    pStartItem->setIsOpened(false);
}

void UIVisoContentBrowser::setTableRootIndex(QModelIndex index /* = QModelIndex */)
{
    if (!m_pTableView)
        return;

    QModelIndex tableIndex;
    if (index.isValid())
    {
        tableIndex = convertIndexToTableIndex(index);
        if (tableIndex.isValid())
            m_pTableView->setRootIndex(tableIndex);
    }
    else
    {
        if (m_pTreeView && m_pTreeView->selectionModel())
        {
            QItemSelectionModel *selectionModel = m_pTreeView->selectionModel();
            if (!selectionModel->selectedIndexes().isEmpty())
            {
                QModelIndex treeIndex = selectionModel->selectedIndexes().at(0);
                tableIndex = convertIndexToTableIndex(treeIndex);
                if (tableIndex.isValid())
                    m_pTableView->setRootIndex(tableIndex);
            }
        }
    }
    if (tableIndex.isValid())
    {
        UICustomFileSystemItem *pItem =
            static_cast<UICustomFileSystemItem*>(m_pTableProxyModel->mapToSource(tableIndex).internalPointer());
        if (pItem)
        {
            QString strPath = pItem->data(UICustomFileSystemModelColumn_Path).toString();
            updateLocationSelectorText(strPath);
        }
    }
}

void UIVisoContentBrowser::setTreeCurrentIndex(QModelIndex index /* = QModelIndex() */)
{
    if (!m_pTreeView)
        return;
    QItemSelectionModel *pSelectionModel = m_pTreeView->selectionModel();
    if (!pSelectionModel)
        return;
    m_pTreeView->blockSignals(true);
    pSelectionModel->blockSignals(true);
    QModelIndex treeIndex;
    if (index.isValid())
    {
        treeIndex = convertIndexToTreeIndex(index);
    }
    else
    {
        QItemSelectionModel *selectionModel = m_pTableView->selectionModel();
        if (selectionModel)
        {
            if (!selectionModel->selectedIndexes().isEmpty())
            {
                QModelIndex tableIndex = selectionModel->selectedIndexes().at(0);
                treeIndex = convertIndexToTreeIndex(tableIndex);
            }
        }
    }

    if (treeIndex.isValid())
    {
        m_pTreeView->setCurrentIndex(treeIndex);
        m_pTreeView->setExpanded(treeIndex, true);
        m_pTreeView->scrollTo(index, QAbstractItemView::PositionAtCenter);
        m_pTreeProxyModel->invalidate();
    }
    pSelectionModel->blockSignals(false);
    m_pTreeView->blockSignals(false);
}

void UIVisoContentBrowser::treeSelectionChanged(const QModelIndex &selectedTreeIndex)
{
    if (!m_pTableProxyModel || !m_pTreeProxyModel)
        return;

    /* Check if we need to scan the directory in the host system: */
    UICustomFileSystemItem *pClickedItem =
        static_cast<UICustomFileSystemItem*>(m_pTreeProxyModel->mapToSource(selectedTreeIndex).internalPointer());
    scanHostDirectory(pClickedItem);
    setTableRootIndex(selectedTreeIndex);
    m_pTableProxyModel->invalidate();
    m_pTreeProxyModel->invalidate();
}

void UIVisoContentBrowser::showHideHiddenObjects(bool bShow)
{
    Q_UNUSED(bShow);
}

void UIVisoContentBrowser::setVisoName(const QString &strName)
{
    if (m_strVisoName == strName)
        return;
    m_strVisoName = strName;
    updateStartItemName();
}

bool UIVisoContentBrowser::tableViewHasSelection() const
{
    if (!m_pTableView)
        return false;
    QItemSelectionModel *pSelectionModel = m_pTableView->selectionModel();
    if (!pSelectionModel)
        return false;
    return pSelectionModel->hasSelection();
}

QModelIndex UIVisoContentBrowser::convertIndexToTableIndex(const QModelIndex &index)
{
    if (!index.isValid())
        return QModelIndex();

    if (index.model() == m_pTableProxyModel)
        return index;
    else if (index.model() == m_pModel)
        return m_pTableProxyModel->mapFromSource(index);
    else if (index.model() == m_pTreeProxyModel)
        return m_pTableProxyModel->mapFromSource(m_pTreeProxyModel->mapToSource(index));
    return QModelIndex();
}

QModelIndex UIVisoContentBrowser::convertIndexToTreeIndex(const QModelIndex &index)
{
    if (!index.isValid())
        return QModelIndex();

    if (index.model() == m_pTreeProxyModel)
        return index;
    else if (index.model() == m_pModel)
        return m_pTreeProxyModel->mapFromSource(index);
    else if (index.model() == m_pTableProxyModel)
        return m_pTreeProxyModel->mapFromSource(m_pTableProxyModel->mapToSource(index));
    return QModelIndex();
}

void UIVisoContentBrowser::scanHostDirectory(UICustomFileSystemItem *directoryItem)
{
    if (!directoryItem)
        return;
    /* the clicked item can be a directory created with the VISO content. in that case local path data
       should be empty: */
    if (directoryItem->type() != KFsObjType_Directory ||
        directoryItem->data(UICustomFileSystemModelColumn_LocalPath).toString().isEmpty())
        return;
    QDir directory(directoryItem->data(UICustomFileSystemModelColumn_LocalPath).toString());
    if (directory.exists() && !directoryItem->isOpened())
    {
        QFileInfoList directoryContent = directory.entryInfoList();
        for (int i = 0; i < directoryContent.size(); ++i)
        {
            const QFileInfo &fileInfo = directoryContent[i];
            if (fileInfo.fileName() == ".")
                continue;
            UICustomFileSystemItem *newItem = new UICustomFileSystemItem(fileInfo.fileName(),
                                                                         directoryItem,
                                                                       fileType(fileInfo));
            newItem->setData(fileInfo.filePath(), UICustomFileSystemModelColumn_LocalPath);

            newItem->setData(UIPathOperations::mergePaths(directoryItem->path(), fileInfo.fileName()),
                             UICustomFileSystemModelColumn_Path);
            if (fileInfo.isSymLink())
            {
                newItem->setTargetPath(fileInfo.symLinkTarget());
                newItem->setIsSymLinkToADirectory(QFileInfo(fileInfo.symLinkTarget()).isDir());
            }
        }
        directoryItem->setIsOpened(true);
    }
}

/* static */ KFsObjType UIVisoContentBrowser::fileType(const QFileInfo &fsInfo)
{
    if (!fsInfo.exists())
        return KFsObjType_Unknown;
    /* first check if it is symlink becacuse for Qt
       being smylin and directory/file is not mutually exclusive: */
    if (fsInfo.isSymLink())
        return KFsObjType_Symlink;
    else if (fsInfo.isFile())
        return KFsObjType_File;
    else if (fsInfo.isDir())
        return KFsObjType_Directory;

    return KFsObjType_Unknown;
}

void UIVisoContentBrowser::updateStartItemName()
{
    if (!rootItem() || !rootItem()->child(0))
        return;
    const QString strName(QDir::toNativeSeparators("/"));

    rootItem()->child(0)->setData(strName, UICustomFileSystemModelColumn_Name);
    /* If the table root index is the start item then we have to update the location selector text here: */
    if (m_pTableProxyModel->mapToSource(m_pTableView->rootIndex()).internalPointer() == rootItem()->child(0))
        updateLocationSelectorText(strName);
    m_pTreeProxyModel->invalidate();
    m_pTableProxyModel->invalidate();
}

void UIVisoContentBrowser::renameFileObject(UICustomFileSystemItem *pItem)
{
    m_pTableView->edit(m_pTableProxyModel->mapFromSource(m_pModel->index(pItem)));
}

void UIVisoContentBrowser::sltHandleItemRenameAction()
{
    QList<UICustomFileSystemItem*> selectedItems = tableSelectedItems();
    if (selectedItems.empty())
        return;
    /* This is not complete. we have to modify the entries in the m_entryMap as well: */
    renameFileObject(selectedItems.at(0));
}

void UIVisoContentBrowser::sltHandleItemRenameAttempt(UICustomFileSystemItem *pItem, QString strOldName, QString strNewName)
{
    if (!pItem || !pItem->parentItem())
        return;
    QList<UICustomFileSystemItem*> children = pItem->parentItem()->children();
    bool bDuplicate = false;
    foreach (const UICustomFileSystemItem *item, children)
    {
        if (item->name() == strNewName && item != pItem)
            bDuplicate = true;
    }

    if (bDuplicate)
    {
        /* Restore the previous name in case the @strNewName is a duplicate: */
        pItem->setData(strOldName, static_cast<int>(UICustomFileSystemModelColumn_Name));
    }

    pItem->setData(UIPathOperations::mergePaths(pItem->parentItem()->path(), pItem->name()), UICustomFileSystemModelColumn_Path);
    if (m_pTableProxyModel)
        m_pTableProxyModel->invalidate();
}

void UIVisoContentBrowser::sltHandleTableSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected)
{
    Q_UNUSED(deselected);
    emit sigTableSelectionChanged(selected.isEmpty());
}

void UIVisoContentBrowser::sltHandleResetAction()
{
    if (!rootItem() || !rootItem()->child(0))
        return;
    rootItem()->child(0)->removeChildren();
    m_entryMap.clear();
    if (m_pTableProxyModel)
        m_pTableProxyModel->invalidate();
    if (m_pTreeProxyModel)
        m_pTreeProxyModel->invalidate();
}

void UIVisoContentBrowser::sltHandleDroppedItems(QStringList pathList)
{
    addObjectsToViso(pathList);
}

void UIVisoContentBrowser::reset()
{
    m_entryMap.clear();
}

QList<UICustomFileSystemItem*> UIVisoContentBrowser::tableSelectedItems()
{
    QList<UICustomFileSystemItem*> selectedItems;
    if (!m_pTableProxyModel)
        return selectedItems;
    QItemSelectionModel *selectionModel = m_pTableView->selectionModel();
    if (!selectionModel || selectionModel->selectedIndexes().isEmpty())
        return selectedItems;
    QModelIndexList list = selectionModel->selectedRows();
    foreach (QModelIndex index, list)
    {
        UICustomFileSystemItem *pItem =
            static_cast<UICustomFileSystemItem*>(m_pTableProxyModel->mapToSource(index).internalPointer());
        if (pItem)
            selectedItems << pItem;
    }
    return selectedItems;
}

#include "UIVisoContentBrowser.moc"

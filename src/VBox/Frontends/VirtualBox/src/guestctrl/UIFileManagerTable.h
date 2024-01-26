/* $Id: UIFileManagerTable.h $ */
/** @file
 * VBox Qt GUI - UIFileManagerTable class declaration.
 */

/*
 * Copyright (C) 2016-2023 Oracle and/or its affiliates.
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

#ifndef FEQT_INCLUDED_SRC_guestctrl_UIFileManagerTable_h
#define FEQT_INCLUDED_SRC_guestctrl_UIFileManagerTable_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* Qt includes: */
#include <QItemSelectionModel>
#include <QMutex>
#include <QThread>
#include <QWidget>

/* COM includes: */
#include "COMEnums.h"
#include "CGuestSession.h"

/* GUI includes: */
#include "QIDialog.h"
#include "QITableView.h"
#include "QIWithRetranslateUI.h"
#include "UIGuestControlDefs.h"

/* Forward declarations: */
class QAction;
class QFileInfo;
class QComboBox;
class QILabel;
class QILineEdit;
class QGridLayout;
class QSortFilterProxyModel;
class QStackedWidget;
class QTextEdit;
class QHBoxLayout;
class QVBoxLayout;
class UIActionPool;
class UICustomFileSystemItem;
class UICustomFileSystemModel;
class UICustomFileSystemProxyModel;
class UIFileManagerNavigationWidget;
class UIGuestControlFileView;
class QIToolBar;

/** A simple struck to store some statictics for a directory. Mainly used by  UIDirectoryDiskUsageComputer instances. */
class UIDirectoryStatistics
{
public:
    UIDirectoryStatistics();
    ULONG64    m_totalSize;
    unsigned   m_uFileCount;
    unsigned   m_uDirectoryCount;
    unsigned   m_uSymlinkCount;
};

Q_DECLARE_METATYPE(UIDirectoryStatistics);


/** Examines the paths in @p strStartPath and collects some staticstics from them recursively (in case directories)
 *  Runs on a worker thread to avoid GUI freezes. UIGuestFileTable and UIHostFileTable uses specialized children
 *  of this class since the calls made on file objects are different. */
class UIDirectoryDiskUsageComputer : public QThread
{
    Q_OBJECT;

signals:

    void sigResultUpdated(UIDirectoryStatistics);

public:

    UIDirectoryDiskUsageComputer(QObject *parent, QStringList strStartPath);
    /** Sets the m_fOkToContinue to false. This results an early termination
      * of the  directoryStatisticsRecursive member function. */
    void stopRecursion();

protected:

    /** Read the directory with the path @p path recursively and collect #of objects and  total size */
    virtual void directoryStatisticsRecursive(const QString &path, UIDirectoryStatistics &statistics) = 0;
    virtual void           run() RT_OVERRIDE;
    /** Returns the m_fOkToContinue flag */
    bool                  isOkToContinue() const;
    /** Stores a list of paths whose statistics are accumulated, can be file, directory etc: */
    QStringList           m_pathList;
    UIDirectoryStatistics m_resultStatistics;
    QMutex                m_mutex;

private:

    bool     m_fOkToContinue;
};

/** A QIDialog child to display properties of a file object */
class UIPropertiesDialog : public QIDialog
{

    Q_OBJECT;

public:

    UIPropertiesDialog(QWidget *pParent = 0, Qt::WindowFlags enmFlags = Qt::WindowFlags());
    void setPropertyText(const QString &strProperty);
    void addDirectoryStatistics(UIDirectoryStatistics statictics);

private:

    QVBoxLayout *m_pMainLayout;
    QTextEdit   *m_pInfoEdit;
    QString      m_strProperty;
};

/** This class serves a base class for file table. Currently a guest version
 *  and a host version are derived from this base. Each of these children
 *  populates the UICustomFileSystemModel by scanning the file system
 *  differently. The file structure kept in this class as a tree. */
class UIFileManagerTable : public QIWithRetranslateUI<QWidget>
{
    Q_OBJECT;

signals:

    void sigLogOutput(QString strLog, const QString &strMachineName, FileManagerLogType eLogType);
    void sigDeleteConfirmationOptionChanged();
    void sigSelectionChanged(bool fHasSelection);

public:

    UIFileManagerTable(UIActionPool *pActionPool, QWidget *pParent = 0);
    virtual ~UIFileManagerTable();
    /** Deletes all the tree nodes */
    void        reset();
    /** Returns the path of the rootIndex */
    QString     currentDirectoryPath() const;
    /** Returns the paths of the selected items (if any) as a list */
    QStringList selectedItemPathList();
    virtual void refresh();
    static const unsigned    m_iKiloByte;
    static QString humanReadableSize(ULONG64 size);
    /** Peroforms whatever is necessary after a UIFileManagerOptions change. */
    void optionsUpdated();
    bool hasSelection() const;

public slots:

    void sltReceiveDirectoryStatistics(UIDirectoryStatistics statictics);
    void sltCreateNewDirectory();
    /* index is passed by the item view and represents the double clicked object's 'proxy' model index */
    void sltItemDoubleClicked(const QModelIndex &index);
    void sltItemClicked(const QModelIndex &index);
    void sltGoUp();
    void sltGoHome();
    void sltRefresh();
    void sltDelete();
    /** Calls the edit on the data item over m_pView. This causes setData(..) call on the model. After setting
     *  user entered text as the name of the item m_pModel signals. This signal is handled by sltHandleItemRenameAttempt which
     *  tries to rename the corresponding file object by calling renameItem(...). If this rename fails the old name of the
     *  model item is restored and view is refreshed by sltHandleItemRenameAttempt. */
    void sltRename();
    void sltCopy();
    void sltCut();
    void sltPaste();
    void sltShowProperties();
    void sltSelectAll();
    void sltInvertSelection();

protected:

    /** This enum is used when performing a gueest-to-guest or host-to-host
     *  file operations. Paths of source file objects are kept in a single buffer
     *  and a flag to determine if it is a cut or copy operation is needed */
    enum FileOperationType
    {
        FileOperationType_Copy,
        FileOperationType_Cut,
        FileOperationType_None,
        FileOperationType_Max
    };

    void retranslateUi();
    void updateCurrentLocationEdit(const QString& strLocation);
    /* @p index is for model not for 'proxy' model */
    void changeLocation(const QModelIndex &index);
    void initializeFileTree();
    void checkDotDot(QMap<QString,UICustomFileSystemItem*> &map, UICustomFileSystemItem *parent, bool isStartDir);

    virtual void     readDirectory(const QString& strPath, UICustomFileSystemItem *parent, bool isStartDir = false) = 0;
    virtual void     deleteByItem(UICustomFileSystemItem *item) = 0;
    virtual void     deleteByPath(const QStringList &pathList) = 0;
    virtual void     goToHomeDirectory() = 0;
    virtual bool     renameItem(UICustomFileSystemItem *item, QString newBaseName) = 0;
    virtual bool     createDirectory(const QString &path, const QString &directoryName) = 0;
    virtual QString  fsObjectPropertyString() = 0;
    virtual void     showProperties() = 0;
    /** For non-windows system does nothing and for windows systems populates m_driveLetterList with
     *  drive letters */
    virtual void     determineDriveLetters() = 0;
    virtual void     determinePathSeparator() = 0;
    virtual void     prepareToolbar() = 0;
    virtual void     createFileViewContextMenu(const QWidget *pWidget, const QPoint &point) = 0;
    virtual bool     event(QEvent *pEvent) RT_OVERRIDE;

    /** @name Copy/Cut guest-to-guest (host-to-host) stuff.
     * @{ */
        /** Disable/enable paste action depending on the m_eFileOperationType. */
        virtual void  setPasteActionEnabled(bool fEnabled) = 0;
        virtual void  pasteCutCopiedObjects() = 0;
        /** stores the type of the pending guest-to-guest (host-to-host) file operation. */
        FileOperationType m_eFileOperationType;
    /** @} */

    QString          fileTypeString(KFsObjType type);
    /* @p item index is item location in model not in 'proxy' model */
    void             goIntoDirectory(const QModelIndex &itemIndex);
    /** Follows the path trail, opens directories as it descends */
    void             goIntoDirectory(const QStringList &pathTrail);
    /** Goes into directory pointed by the @p item */
    void             goIntoDirectory(UICustomFileSystemItem *item);
    UICustomFileSystemItem* indexData(const QModelIndex &index) const;
    bool             eventFilter(QObject *pObject, QEvent *pEvent) RT_OVERRIDE;
    CGuestFsObjInfo  guestFsObjectInfo(const QString& path, CGuestSession &comGuestSession) const;
    void             setSelectionDependentActionsEnabled(bool fIsEnabled);
    UICustomFileSystemItem*   rootItem();
    void             setPathSeparator(const QChar &separator);
    QHBoxLayout*     toolBarLayout();
    void             setSessionWidgetsEnabled(bool fEnabled);

    QILabel                 *m_pLocationLabel;
    UIPropertiesDialog      *m_pPropertiesDialog;
    UIActionPool            *m_pActionPool;
    QIToolBar               *m_pToolBar;
    QGridLayout     *m_pMainLayout;
    /** Stores the drive letters the file system has (for windows system). For non-windows
     *  systems this is empty and for windows system it should at least contain C:/ */
    QStringList              m_driveLetterList;
    /** The set of actions which need some selection to work on. Like cut, copy etc. */
    QSet<QAction*>           m_selectionDependentActions;
    /** The absolute path list of the file objects which user has chosen to cut/copy. this
     *  list will be cleaned after a paste operation or overwritten by a subsequent cut/copy.
     *  Currently only used by the guest side. */
    QStringList              m_copyCutBuffer;
    /** This name is appended to the log messages which are shown in the log panel. */
    QString          m_strTableName;

private slots:

    void sltCreateFileViewContextMenu(const QPoint &point);
    void sltSelectionChanged(const QItemSelection & selected, const QItemSelection & deselected);
    void sltSearchTextChanged(const QString &strText);
    /** m_pModel signals when an tree item is renamed. we try to apply this rename to the file system.
     *  if the file system rename fails we restore the old name of the item. See the comment of
     *  sltRename() for more details. */
    void sltHandleItemRenameAttempt(UICustomFileSystemItem *pItem, QString strOldName, QString strNewName);
    void sltHandleNavigationWidgetPathChange(const QString& strPath);

private:

    void             relist();
    void             prepareObjects();
    /** @p itemIndex is assumed to be 'model' index not 'proxy model' index */
    void             deleteByIndex(const QModelIndex &itemIndex);
    /** Returns the UICustomFileSystemItem for path / which is a direct (and single) child of m_pRootItem */
    UICustomFileSystemItem *getStartDirectoryItem();
    void            deSelectUpDirectoryItem();
    void            setSelectionForAll(QItemSelectionModel::SelectionFlags flags);
    void            setSelection(const QModelIndex &indexInProxyModel);
    /** The start directory requires a special attention since on file systems with drive letters
     *  drive letter are direct children of the start directory. On other systems start directory is '/' */
    void            populateStartDirectory(UICustomFileSystemItem *startItem);
    /** Root index of the m_pModel */
    QModelIndex     currentRootIndex() const;
    /* Searches the content of m_pSearchLineEdit within the current items' names and selects the item if found. */
    void            performSelectionSearch(const QString &strSearchText);
    /** Clears the m_pSearchLineEdit and hides it. */
    void            disableSelectionSearch();
    /** Checks if delete confirmation dialog is shown and users choice. Returns true
     *  if deletion can continue */
    bool            checkIfDeleteOK();
    /** Marks/umarks the search line edit to signal that there are no matches for the current search.
      * uses m_searchLineUnmarkColor and m_searchLineMarkColor. */
    void            markUnmarkSearchLineEdit(bool fMark);

    UICustomFileSystemModel       *m_pModel;
    UIGuestControlFileView        *m_pView;
    UICustomFileSystemProxyModel  *m_pProxyModel;
    /** Contains m_pBreadCrumbsWidget and m_pLocationComboBox. */
    UIFileManagerNavigationWidget *m_pNavigationWidget;

    QILineEdit      *m_pSearchLineEdit;
    QColor           m_searchLineUnmarkColor;
    QColor           m_searchLineMarkColor;
    QChar            m_pathSeparator;
    QHBoxLayout     *m_pToolBarLayout;
    QVector<QWidget*> m_sessionWidgets;
    friend class     UICustomFileSystemModel;
};

#endif /* !FEQT_INCLUDED_SRC_guestctrl_UIFileManagerTable_h */

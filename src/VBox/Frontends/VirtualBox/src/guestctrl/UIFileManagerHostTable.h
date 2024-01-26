/* $Id: UIFileManagerHostTable.h $ */
/** @file
 * VBox Qt GUI - UIFileManagerHostTable class declaration.
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

#ifndef FEQT_INCLUDED_SRC_guestctrl_UIFileManagerHostTable_h
#define FEQT_INCLUDED_SRC_guestctrl_UIFileManagerHostTable_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* Qt includes: */
#include <QFileDevice>

/* GUI includes: */
#include "UIFileManagerTable.h"

/* Forward declarations: */
class UIActionPool;
class UICustomFileSystemItem;

/** This class scans the host file system by using the Qt API
    and connects to the UICustomFileSystemModel*/
class UIFileManagerHostTable : public UIFileManagerTable
{
    Q_OBJECT;

public:

    UIFileManagerHostTable(UIActionPool *pActionPool, QWidget *pParent = 0);
    static KFsObjType  fileType(const QFileInfo &fsInfo);
    static KFsObjType  fileType(const QString &strPath);

protected:

    /** Scans the directory with the path @strPath and inserts items to the
     *  tree under the @p parent. */
    static void scanDirectory(const QString& strPath, UICustomFileSystemItem *parent,
                              QMap<QString, UICustomFileSystemItem*> &fileObjects);
    void            retranslateUi() override final;
    virtual void    readDirectory(const QString& strPath, UICustomFileSystemItem *parent, bool isStartDir = false) override final;
    virtual void    deleteByItem(UICustomFileSystemItem *item) override final;
    virtual void    deleteByPath(const QStringList &pathList) override final;
    virtual void    goToHomeDirectory() override final;
    virtual bool    renameItem(UICustomFileSystemItem *item, QString newBaseName) override final;
    virtual bool    createDirectory(const QString &path, const QString &directoryName) override final;
    virtual QString fsObjectPropertyString() override final;
    virtual void    showProperties() override final;
    virtual void    determineDriveLetters() override final;
    virtual void    determinePathSeparator() override final;
    virtual void    prepareToolbar() override final;
    virtual void    createFileViewContextMenu(const QWidget *pWidget, const QPoint &point) override final;
    /** @name Copy/Cut host-to-host stuff. Currently not implemented.
     * @{ */
        /** Disable/enable paste action depending on the m_eFileOperationType. */
        virtual void  setPasteActionEnabled(bool)  override final {}
        virtual void  pasteCutCopiedObjects()  override final {}
    /** @} */

private:

    static QString permissionString(QFileDevice::Permissions permissions);
    void    prepareActionConnections();
};

#endif /* !FEQT_INCLUDED_SRC_guestctrl_UIFileManagerHostTable_h */

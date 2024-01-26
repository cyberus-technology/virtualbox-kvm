/* $Id: UIInformationConfiguration.h $ */
/** @file
 * VBox Qt GUI - UIInformationConfiguration class declaration.
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

#ifndef FEQT_INCLUDED_SRC_runtime_information_UIInformationConfiguration_h
#define FEQT_INCLUDED_SRC_runtime_information_UIInformationConfiguration_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* Qt includes: */
#include <QWidget>

/* COM includes: */
#include "COMEnums.h"
#include "CGuest.h"
#include "CMachine.h"
#include "CConsole.h"

/* GUI includes: */
#include "QIWithRetranslateUI.h"
#include "UITextTable.h"


/* Forward declarations: */
class QTableWidget;
class QTableWidgetItem;
class QTextDocument;
class QVBoxLayout;

class UIInformationConfiguration : public QIWithRetranslateUI<QWidget>
{
    Q_OBJECT;

public:

    /** Constructs information-tab passing @a pParent to the QWidget base-class constructor.
      * @param machine is machine reference.
      * @param console is machine console reference. */
    UIInformationConfiguration(QWidget *pParent, const CMachine &machine, const CConsole &console);

protected:

    void retranslateUi() RT_OVERRIDE;

private slots:

    void sltMachineDataChanged();
    void sltHandleTableContextMenuRequest(const QPoint &position);
    void sltCopyTableToClipboard();

private:

    void createTableItems();
    void prepareObjects();
    void insertTitleRow(const QString &strTitle, const QIcon &icon, const QFontMetrics &fontMetrics);
    void insertInfoRows(const UITextTable &table, const QFontMetrics &fontMetrics, int &iMaxColumn1Length);
    void insertInfoRow(const QString strText1, const QString &strText2,
                       const QFontMetrics &fontMetrics, int &iMaxColumn1Length);
    void resetTable();
    QString removeHtmlFromString(const QString &strOriginal);
    QString tableData() const;

    CMachine m_machine;
    CConsole m_console;
    QVBoxLayout *m_pMainLayout;
    QTableWidget *m_pTableWidget;
    QAction *m_pCopyWholeTableAction;

    const int m_iColumCount;
    const int m_iRowLeftMargin;
    const int m_iRowTopMargin;
    const int m_iRowRightMargin;
    const int m_iRowBottomMargin;


   /** @name Cached translated string.
      * @{ */
        QString m_strGeneralTitle;
        QString m_strSystemTitle;
        QString m_strDisplayTitle;
        QString m_strStorageTitle;
        QString m_strAudioTitle;
        QString m_strNetworkTitle;
        QString m_strSerialPortsTitle;
        QString m_strUSBTitle;
        QString m_strSharedFoldersTitle;
    /** @} */
};

#endif /* !FEQT_INCLUDED_SRC_runtime_information_UIInformationConfiguration_h */

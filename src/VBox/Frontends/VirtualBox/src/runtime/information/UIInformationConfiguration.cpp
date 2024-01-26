/* $Id: UIInformationConfiguration.cpp $ */
/** @file
 * VBox Qt GUI - UIInformationConfiguration class implementation.
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

/* COM includes: */

/* Qt includes: */
#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QHeaderView>
#include <QMenu>
#include <QTableWidget>
#include <QTextDocument>
#include <QVBoxLayout>

/* GUI includes: */
#include "UIDetailsGenerator.h"
#include "UICommon.h"
#include "UIExtraDataManager.h"
#include "UIIconPool.h"
#include "UIInformationConfiguration.h"
#include "UIVirtualBoxEventHandler.h"

UIInformationConfiguration::UIInformationConfiguration(QWidget *pParent, const CMachine &machine, const CConsole &console)
    : QIWithRetranslateUI<QWidget>(pParent)
    , m_machine(machine)
    , m_console(console)
    , m_pMainLayout(0)
    , m_pTableWidget(0)
    , m_pCopyWholeTableAction(0)
    , m_iColumCount(3)
    , m_iRowLeftMargin(0.2 * qApp->style()->pixelMetric(QStyle::PM_LayoutLeftMargin))
    , m_iRowTopMargin(0.2 * qApp->style()->pixelMetric(QStyle::PM_LayoutTopMargin))
    , m_iRowRightMargin(0.2 * qApp->style()->pixelMetric(QStyle::PM_LayoutRightMargin))
    , m_iRowBottomMargin(0.2 * qApp->style()->pixelMetric(QStyle::PM_LayoutBottomMargin))
{
    prepareObjects();
    retranslateUi();
    connect(gVBoxEvents, &UIVirtualBoxEventHandler::sigMachineDataChange,
            this, &UIInformationConfiguration::sltMachineDataChanged);
    connect(&uiCommon(), &UICommon::sigMediumEnumerationFinished,
            this, &UIInformationConfiguration::sltMachineDataChanged);
}

void UIInformationConfiguration::sltMachineDataChanged()
{
    createTableItems();
}

void UIInformationConfiguration::sltHandleTableContextMenuRequest(const QPoint &position)
{
    if (!m_pCopyWholeTableAction)
        return;

    QMenu menu(this);
    menu.addAction(m_pCopyWholeTableAction);
    menu.exec(mapToGlobal(position));
}

void UIInformationConfiguration::sltCopyTableToClipboard()
{
    QClipboard *pClipboard = QApplication::clipboard();
    if (!pClipboard)
        return;
    pClipboard->setText(tableData(), QClipboard::Clipboard);
}

void UIInformationConfiguration::retranslateUi()
{
    m_strGeneralTitle = QApplication::translate("UIVMInformationDialog", "General");
    m_strSystemTitle = QApplication::translate("UIVMInformationDialog", "System");
    m_strDisplayTitle = QApplication::translate("UIVMInformationDialog", "Display");
    m_strStorageTitle = QApplication::translate("UIVMInformationDialog", "Storage");
    m_strAudioTitle = QApplication::translate("UIVMInformationDialog", "Audio");
    m_strNetworkTitle = QApplication::translate("UIVMInformationDialog", "Network");
    m_strSerialPortsTitle = QApplication::translate("UIVMInformationDialog", "Serial Ports");
    m_strUSBTitle = QApplication::translate("UIVMInformationDialog", "USB");
    m_strSharedFoldersTitle = QApplication::translate("UIVMInformationDialog", "Shared Folders");
    if (m_pCopyWholeTableAction)
        m_pCopyWholeTableAction->setText(QApplication::translate("UIVMInformationDialog", "Copy All"));
    createTableItems();
}

void UIInformationConfiguration::createTableItems()
{
    if (!m_pTableWidget)
        return;
    resetTable();
    QFontMetrics fontMetrics(m_pTableWidget->font());
    int iMaxColumn1Length = 0;
    /* General section: */
    insertTitleRow(m_strGeneralTitle, UIIconPool::iconSet(":/machine_16px.png"), fontMetrics);
    insertInfoRows(UIDetailsGenerator::generateMachineInformationGeneral(m_machine,
                                                                         UIExtraDataMetaDefs::DetailsElementOptionTypeGeneral_Default),
                   fontMetrics, iMaxColumn1Length);

    /* System section: */
    insertTitleRow(m_strSystemTitle, UIIconPool::iconSet(":/chipset_16px.png"), fontMetrics);
    insertInfoRows(UIDetailsGenerator::generateMachineInformationSystem(m_machine,
                                                                        UIExtraDataMetaDefs::DetailsElementOptionTypeSystem_Default),
                   fontMetrics, iMaxColumn1Length);

    /* Display section: */
    insertTitleRow(m_strDisplayTitle, UIIconPool::iconSet(":/vrdp_16px.png"), fontMetrics);
    insertInfoRows(UIDetailsGenerator::generateMachineInformationDisplay(m_machine,
                                                                         UIExtraDataMetaDefs::DetailsElementOptionTypeDisplay_Default),
                   fontMetrics, iMaxColumn1Length);

    /* Storage section: */
    insertTitleRow(m_strStorageTitle, UIIconPool::iconSet(":/hd_16px.png"), fontMetrics);
    insertInfoRows(UIDetailsGenerator::generateMachineInformationStorage(m_machine,
                                                                         UIExtraDataMetaDefs::DetailsElementOptionTypeStorage_Default),
                   fontMetrics, iMaxColumn1Length);

    /* Audio section: */
    insertTitleRow(m_strAudioTitle, UIIconPool::iconSet(":/sound_16px.png"), fontMetrics);
    insertInfoRows(UIDetailsGenerator::generateMachineInformationAudio(m_machine,
                                                                       UIExtraDataMetaDefs::DetailsElementOptionTypeAudio_Default),
                   fontMetrics, iMaxColumn1Length);

    /* Network section: */
    insertTitleRow(m_strNetworkTitle, UIIconPool::iconSet(":/nw_16px.png"), fontMetrics);
    insertInfoRows(UIDetailsGenerator::generateMachineInformationNetwork(m_machine,
                                                                         UIExtraDataMetaDefs::DetailsElementOptionTypeNetwork_Default),
                   fontMetrics, iMaxColumn1Length);

    /* Serial port section: */
    insertTitleRow(m_strSerialPortsTitle, UIIconPool::iconSet(":/serial_port_16px.png"), fontMetrics);
    insertInfoRows(UIDetailsGenerator::generateMachineInformationSerial(m_machine,
                                                                        UIExtraDataMetaDefs::DetailsElementOptionTypeSerial_Default),
                   fontMetrics, iMaxColumn1Length);

    /* USB section: */
    insertTitleRow(m_strUSBTitle, UIIconPool::iconSet(":/usb_16px.png"), fontMetrics);
    insertInfoRows(UIDetailsGenerator::generateMachineInformationUSB(m_machine,
                                                                     UIExtraDataMetaDefs::DetailsElementOptionTypeUsb_Default),
                   fontMetrics, iMaxColumn1Length);

    /* Share folders section: */
    insertTitleRow(m_strSharedFoldersTitle, UIIconPool::iconSet(":/sf_16px.png"), fontMetrics);
    insertInfoRows(UIDetailsGenerator::generateMachineInformationSharedFolders(m_machine,
                                                                               UIExtraDataMetaDefs::DetailsElementOptionTypeSharedFolders_Default),
                   fontMetrics, iMaxColumn1Length);

    m_pTableWidget->resizeColumnToContents(0);
    /* Resize the column 1 a bit larger than the max string if contains: */
    m_pTableWidget->setColumnWidth(1, 1.5 * iMaxColumn1Length);
    m_pTableWidget->resizeColumnToContents(2);
    m_pTableWidget->horizontalHeader()->setStretchLastSection(true);
}

void UIInformationConfiguration::prepareObjects()
{
    /* Create layout: */
    m_pMainLayout = new QVBoxLayout(this);
    if (!m_pMainLayout)
        return;
    m_pMainLayout->setSpacing(0);

    m_pTableWidget = new QTableWidget;
    if (m_pTableWidget)
    {
        /* Configure the table by hiding the headers etc.: */
        m_pTableWidget->setColumnCount(m_iColumCount);
        m_pTableWidget->setAlternatingRowColors(true);
        m_pTableWidget->verticalHeader()->hide();
        m_pTableWidget->horizontalHeader()->hide();
        m_pTableWidget->setShowGrid(false);
        m_pTableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
        m_pTableWidget->setFocusPolicy(Qt::NoFocus);
        m_pTableWidget->setSelectionMode(QAbstractItemView::NoSelection);
        m_pTableWidget->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(m_pTableWidget, &QTableWidget::customContextMenuRequested,
            this, &UIInformationConfiguration::sltHandleTableContextMenuRequest);
        m_pMainLayout->addWidget(m_pTableWidget);
    }
    m_pCopyWholeTableAction = new QAction(this);
    connect(m_pCopyWholeTableAction, &QAction::triggered,
            this, &UIInformationConfiguration::sltCopyTableToClipboard);
}

void UIInformationConfiguration::insertInfoRows(const UITextTable &table, const QFontMetrics &fontMetrics, int &iMaxColumn1Length)
{
    foreach (const UITextTableLine &line, table)
    {
        insertInfoRow(removeHtmlFromString(line.string1()),
                      removeHtmlFromString(line.string2()),
                      fontMetrics, iMaxColumn1Length);
    }
}

void UIInformationConfiguration::insertTitleRow(const QString &strTitle, const QIcon &icon, const QFontMetrics &fontMetrics)
{
    int iRow = m_pTableWidget->rowCount();
    m_pTableWidget->insertRow(iRow);
    QSize iconSize;
    icon.actualSize(iconSize);
    m_pTableWidget->setRowHeight(iRow,
                                 qMax(fontMetrics.height() + m_iRowTopMargin + m_iRowBottomMargin, iconSize.height()));
    m_pTableWidget->setItem(iRow, 0, new QTableWidgetItem(icon, ""));
    QTableWidgetItem *pTitleItem = new QTableWidgetItem(strTitle);
    QFont font = pTitleItem->font();
    font.setBold(true);
    pTitleItem->setFont(font);
    m_pTableWidget->setItem(iRow, 1, pTitleItem);
}

void UIInformationConfiguration::insertInfoRow(const QString strText1, const QString &strText2,
                                               const QFontMetrics &fontMetrics, int &iMaxColumn1Length)
{
    int iRow = m_pTableWidget->rowCount();
    m_pTableWidget->insertRow(iRow);
    m_pTableWidget->setRowHeight(iRow, fontMetrics.height() + m_iRowTopMargin + m_iRowBottomMargin);
#if QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
    iMaxColumn1Length = qMax(iMaxColumn1Length, fontMetrics.horizontalAdvance(strText1));
#else
    iMaxColumn1Length = qMax(iMaxColumn1Length, fontMetrics.width(strText1));
#endif
    m_pTableWidget->setItem(iRow, 1, new QTableWidgetItem(strText1));
    m_pTableWidget->setItem(iRow, 2, new QTableWidgetItem(strText2));
}

void UIInformationConfiguration::resetTable()
{
    if (m_pTableWidget)
    {
        m_pTableWidget->clear();
        m_pTableWidget->setRowCount(0);
        m_pTableWidget->setColumnCount(m_iColumCount);
    }
}

QString UIInformationConfiguration::removeHtmlFromString(const QString &strOriginal)
{
    QTextDocument textDocument;
    textDocument.setHtml(strOriginal);
    return textDocument.toPlainText();
}

QString UIInformationConfiguration::tableData() const
{
    AssertReturn(m_pTableWidget, QString());
    AssertReturn(m_pTableWidget->columnCount() == 3, QString());
    QStringList data;
    for (int i = 0; i < m_pTableWidget->rowCount(); ++i)
    {
        /* Skip the first column as it contains only icon and no text: */
        QTableWidgetItem *pItem = m_pTableWidget->item(i, 1);
        QString strColumn1 = pItem ? pItem->text() : QString();
        pItem = m_pTableWidget->item(i, 2);
        QString strColumn2 = pItem ? pItem->text() : QString();
        if (strColumn2.isEmpty())
            data << strColumn1;
        else
            data << strColumn1 << ": " << strColumn2;
        data << "\n";
    }
    return data.join(QString());
}

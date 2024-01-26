/* $Id: UIMachineSettingsSerial.cpp $ */
/** @file
 * VBox Qt GUI - UIMachineSettingsSerial class implementation.
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
#include <QCheckBox>
#include <QComboBox>
#include <QDir>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QRegularExpressionValidator>

/* GUI includes: */
#include "QITabWidget.h"
#include "QIWidgetValidator.h"
#include "UICommon.h"
#include "UIConverter.h"
#include "UIErrorString.h"
#include "UIMachineSettingsSerial.h"
#include "UITranslator.h"

/* COM includes: */
#include "CSerialPort.h"


/** Machine settings: Serial Port tab data structure. */
struct UIDataSettingsMachineSerialPort
{
    /** Constructs data. */
    UIDataSettingsMachineSerialPort()
        : m_iSlot(-1)
        , m_fPortEnabled(false)
        , m_uIRQ(0)
        , m_uIOBase(0)
        , m_hostMode(KPortMode_Disconnected)
        , m_fServer(false)
        , m_strPath(QString())
    {}

    /** Returns whether the @a other passed data is equal to this one. */
    bool equal(const UIDataSettingsMachineSerialPort &other) const
    {
        return true
               && (m_iSlot == other.m_iSlot)
               && (m_fPortEnabled == other.m_fPortEnabled)
               && (m_uIRQ == other.m_uIRQ)
               && (m_uIOBase == other.m_uIOBase)
               && (m_hostMode == other.m_hostMode)
               && (m_fServer == other.m_fServer)
               && (m_strPath == other.m_strPath)
               ;
    }

    /** Returns whether the @a other passed data is equal to this one. */
    bool operator==(const UIDataSettingsMachineSerialPort &other) const { return equal(other); }
    /** Returns whether the @a other passed data is different from this one. */
    bool operator!=(const UIDataSettingsMachineSerialPort &other) const { return !equal(other); }

    /** Holds the serial port slot number. */
    int        m_iSlot;
    /** Holds whether the serial port is enabled. */
    bool       m_fPortEnabled;
    /** Holds the serial port IRQ. */
    ulong      m_uIRQ;
    /** Holds the serial port IO base. */
    ulong      m_uIOBase;
    /** Holds the serial port host mode. */
    KPortMode  m_hostMode;
    /** Holds whether the serial port is server. */
    bool       m_fServer;
    /** Holds the serial port path. */
    QString    m_strPath;
};


/** Machine settings: Serial page data structure. */
struct UIDataSettingsMachineSerial
{
    /** Constructs data. */
    UIDataSettingsMachineSerial() {}

    /** Returns whether the @a other passed data is equal to this one. */
    bool operator==(const UIDataSettingsMachineSerial & /* other */) const { return true; }
    /** Returns whether the @a other passed data is different from this one. */
    bool operator!=(const UIDataSettingsMachineSerial & /* other */) const { return false; }
};


/** Machine settings: Serial Port tab. */
class UIMachineSettingsSerial : public QIWithRetranslateUI<QWidget>
{
    Q_OBJECT;

signals:

    /** Notifies about port changed. */
    void sigPortChanged();
    /** Notifies about path changed. */
    void sigPathChanged();
    /** Notifies about validity changed. */
    void sigValidityChanged();

public:

    /** Constructs tab passing @a pParent to the base-class. */
    UIMachineSettingsSerial(UIMachineSettingsSerialPage *pParent);

    /** Loads port data from @a portCache. */
    void getPortDataFromCache(const UISettingsCacheMachineSerialPort &portCache);
    /** Saves port data to @a portCache. */
    void putPortDataToCache(UISettingsCacheMachineSerialPort &portCache);

    /** Performs validation, updates @a messages list if something is wrong. */
    bool validate(QList<UIValidationMessage> &messages);

    /** Configures tab order according to passed @a pWidget. */
    QWidget *setOrderAfter(QWidget *pWidget);

    /** Returns tab title. */
    QString tabTitle() const;
    /** Returns whether port is enabled. */
    bool isPortEnabled() const;
    /** Returns IRQ. */
    QString irq() const;
    /** Returns IO port. */
    QString ioPort() const;
    /** Returns path. */
    QString path() const;

    /** Performs tab polishing. */
    void polishTab();

protected:

    /** Handles translation event. */
    void retranslateUi();

private slots:

    /** Handles port availability being toggled to @a fOn. */
    void sltHandlePortAvailabilityToggled(bool fOn);
    /** Handles port standard @a strOption being activated. */
    void sltHandlePortStandardOptionActivated(const QString &strOption);
    /** Handles port mode change to item with certain @a iIndex. */
    void sltHandlePortModeChange(int iIndex);

private:

    /** Prepares all. */
    void prepare();
    /** Prepares widgets. */
    void prepareWidgets();
    /** Prepares connections. */
    void prepareConnections();

    /** Populates combo-boxes. */
    void populateComboboxes();

    /** Holds the parent page reference. */
    UIMachineSettingsSerialPage *m_pParent;

    /** Holds the port slot number. */
    int        m_iSlot;
    /** Holds the port mode. */
    KPortMode  m_enmPortMode;

    /** @name Widgets
     * @{ */
        /** Holds the port check-box instance. */
        QCheckBox *m_pCheckBoxPort;
        /** Holds the port settings widget instance. */
        QWidget   *m_pWidgetPortSettings;
        /** Holds the number label instance. */
        QLabel    *m_pLabelNumber;
        /** Holds the number combo instance. */
        QComboBox *m_pComboNumber;
        /** Holds the IRQ label instance. */
        QLabel    *m_pLabelIRQ;
        /** Holds the IRQ editor instance. */
        QLineEdit *m_pLineEditIRQ;
        /** Holds the IO port label instance. */
        QLabel    *m_pLabelIOPort;
        /** Holds the IO port editor instance. */
        QLineEdit *m_pLineEditIOPort;
        /** Holds the mode label instance. */
        QLabel    *m_pLabelMode;
        /** Holds the mode combo instance. */
        QComboBox *m_pComboMode;
        /** Holds the pipe check-box instance. */
        QCheckBox *m_pCheckBoxPipe;
        /** Holds the path label instance. */
        QLabel    *m_pLabelPath;
        /** Holds the path editor instance. */
        QLineEdit *m_pEditorPath;
    /** @} */
};


/*********************************************************************************************************************************
*   Class UIMachineSettingsSerial implementation.                                                                                *
*********************************************************************************************************************************/

UIMachineSettingsSerial::UIMachineSettingsSerial(UIMachineSettingsSerialPage *pParent)
    : QIWithRetranslateUI<QWidget>(0)
    , m_pParent(pParent)
    , m_iSlot(-1)
    , m_enmPortMode(KPortMode_Max)
    , m_pCheckBoxPort(0)
    , m_pWidgetPortSettings(0)
    , m_pLabelNumber(0)
    , m_pComboNumber(0)
    , m_pLabelIRQ(0)
    , m_pLineEditIRQ(0)
    , m_pLabelIOPort(0)
    , m_pLineEditIOPort(0)
    , m_pLabelMode(0)
    , m_pComboMode(0)
    , m_pCheckBoxPipe(0)
    , m_pLabelPath(0)
    , m_pEditorPath(0)
{
    prepare();
}

void UIMachineSettingsSerial::getPortDataFromCache(const UISettingsCacheMachineSerialPort &portCache)
{
    /* Get old data: */
    const UIDataSettingsMachineSerialPort &oldPortData = portCache.base();

    /* Load port number: */
    m_iSlot = oldPortData.m_iSlot;

    /* Load port data: */
    if (m_pCheckBoxPort)
        m_pCheckBoxPort->setChecked(oldPortData.m_fPortEnabled);
    if (m_pComboNumber)
        m_pComboNumber->setCurrentIndex(m_pComboNumber->findText(UITranslator::toCOMPortName(oldPortData.m_uIRQ, oldPortData.m_uIOBase)));
    if (m_pLineEditIRQ)
        m_pLineEditIRQ->setText(QString::number(oldPortData.m_uIRQ));
    if (m_pLineEditIOPort)
        m_pLineEditIOPort->setText("0x" + QString::number(oldPortData.m_uIOBase, 16).toUpper());
    m_enmPortMode = oldPortData.m_hostMode;
    if (m_pCheckBoxPipe)
        m_pCheckBoxPipe->setChecked(!oldPortData.m_fServer);
    if (m_pEditorPath)
        m_pEditorPath->setText(oldPortData.m_strPath);

    /* Repopulate combo-boxes content: */
    populateComboboxes();
    /* Ensure everything is up-to-date */
    if (m_pCheckBoxPort)
        sltHandlePortAvailabilityToggled(m_pCheckBoxPort->isChecked());
}

void UIMachineSettingsSerial::putPortDataToCache(UISettingsCacheMachineSerialPort &portCache)
{
    /* Prepare new data: */
    UIDataSettingsMachineSerialPort newPortData;

    /* Save port number: */
    newPortData.m_iSlot = m_iSlot;

    /* Save port data: */
    if (m_pCheckBoxPort)
        newPortData.m_fPortEnabled = m_pCheckBoxPort->isChecked();
    if (m_pLineEditIRQ)
        newPortData.m_uIRQ = m_pLineEditIRQ->text().toULong(NULL, 0);
    if (m_pLineEditIOPort)
        newPortData.m_uIOBase = m_pLineEditIOPort->text().toULong(NULL, 0);
    if (m_pCheckBoxPipe)
        newPortData.m_fServer = !m_pCheckBoxPipe->isChecked();
    if (m_pComboMode)
        newPortData.m_hostMode = m_pComboMode->currentData().value<KPortMode>();
    if (m_pEditorPath)
        newPortData.m_strPath = QDir::toNativeSeparators(m_pEditorPath->text());

    /* Cache new data: */
    portCache.cacheCurrentData(newPortData);
}

bool UIMachineSettingsSerial::validate(QList<UIValidationMessage> &messages)
{
    /* Pass by default: */
    bool fPass = true;

    /* Prepare message: */
    UIValidationMessage message;
    message.first = UITranslator::removeAccelMark(tabTitle());

    if (   m_pCheckBoxPort
        && m_pCheckBoxPort->isChecked())
    {
        /* Check the port attribute emptiness & uniqueness: */
        const QString strIRQ = m_pLineEditIRQ ? m_pLineEditIRQ->text() : QString();
        const QString strIOPort = m_pLineEditIOPort ? m_pLineEditIOPort->text() : QString();
        const QPair<QString, QString> port = qMakePair(strIRQ, strIOPort);

        if (strIRQ.isEmpty())
        {
            message.second << UIMachineSettingsSerial::tr("No IRQ is currently specified.");
            fPass = false;
        }
        if (strIOPort.isEmpty())
        {
            message.second << UIMachineSettingsSerial::tr("No I/O port is currently specified.");
            fPass = false;
        }
        if (   !strIRQ.isEmpty()
            && !strIOPort.isEmpty())
        {
            QVector<QPair<QString, QString> > ports;
            if (m_pParent)
            {
                ports = m_pParent->ports();
                ports.removeAt(m_iSlot);
            }
            if (ports.contains(port))
            {
                message.second << UIMachineSettingsSerial::tr("Two or more ports have the same settings.");
                fPass = false;
            }
        }

        const KPortMode enmMode = m_pComboMode->currentData().value<KPortMode>();
        if (enmMode != KPortMode_Disconnected)
        {
            const QString strPath(m_pEditorPath->text());

            if (strPath.isEmpty())
            {
                message.second << UIMachineSettingsSerial::tr("No port path is currently specified.");
                fPass = false;
            }
            else
            {
                QVector<QString> paths;
                if (m_pParent)
                {
                    paths = m_pParent->paths();
                    paths.removeAt(m_iSlot);
                }
                if (paths.contains(strPath))
                {
                    message.second << UIMachineSettingsSerial::tr("There are currently duplicate port paths specified.");
                    fPass = false;
                }
            }
        }
    }

    /* Serialize message: */
    if (!message.second.isEmpty())
        messages << message;

    /* Return result: */
    return fPass;
}

QWidget *UIMachineSettingsSerial::setOrderAfter(QWidget *pWidget)
{
    setTabOrder(pWidget, m_pCheckBoxPort);
    setTabOrder(m_pCheckBoxPort, m_pComboNumber);
    setTabOrder(m_pComboNumber, m_pLineEditIRQ);
    setTabOrder(m_pLineEditIRQ, m_pLineEditIOPort);
    setTabOrder(m_pLineEditIOPort, m_pComboMode);
    setTabOrder(m_pComboMode, m_pCheckBoxPipe);
    setTabOrder(m_pCheckBoxPipe, m_pEditorPath);
    return m_pEditorPath;
}

QString UIMachineSettingsSerial::tabTitle() const
{
    return QString(tr("Port %1", "serial ports")).arg(QString("&%1").arg(m_iSlot + 1));
}

bool UIMachineSettingsSerial::isPortEnabled() const
{
    return m_pCheckBoxPort->isChecked();
}

QString UIMachineSettingsSerial::irq() const
{
    return m_pLineEditIRQ->text();
}

QString UIMachineSettingsSerial::ioPort() const
{
    return m_pLineEditIOPort->text();
}

QString UIMachineSettingsSerial::path() const
{
    return m_pEditorPath->text();
}

void UIMachineSettingsSerial::polishTab()
{
    /* Sanity check: */
    if (!m_pParent)
        return;

    /* Polish port page: */
    ulong uIRQ, uIOBase;
    const bool fStd = m_pComboNumber ? UITranslator::toCOMPortNumbers(m_pComboNumber->currentText(), uIRQ, uIOBase) : false;
    const KPortMode enmMode = m_pComboMode ? m_pComboMode->currentData().value<KPortMode>() : KPortMode_Max;
    if (m_pCheckBoxPort)
        m_pCheckBoxPort->setEnabled(m_pParent->isMachineOffline());
    if (m_pLabelNumber)
        m_pLabelNumber->setEnabled(m_pParent->isMachineOffline());
    if (m_pComboNumber)
        m_pComboNumber->setEnabled(m_pParent->isMachineOffline());
    if (m_pLabelIRQ)
        m_pLabelIRQ->setEnabled(m_pParent->isMachineOffline());
    if (m_pLineEditIRQ)
        m_pLineEditIRQ->setEnabled(!fStd && m_pParent->isMachineOffline());
    if (m_pLabelIOPort)
        m_pLabelIOPort->setEnabled(m_pParent->isMachineOffline());
    if (m_pLineEditIOPort)
        m_pLineEditIOPort->setEnabled(!fStd && m_pParent->isMachineOffline());
    if (m_pLabelMode)
        m_pLabelMode->setEnabled(m_pParent->isMachineOffline());
    if (m_pComboMode)
        m_pComboMode->setEnabled(m_pParent->isMachineOffline());
    if (m_pCheckBoxPipe)
        m_pCheckBoxPipe->setEnabled(   (enmMode == KPortMode_HostPipe || enmMode == KPortMode_TCP)
                                    && m_pParent->isMachineOffline());
    if (m_pLabelPath)
        m_pLabelPath->setEnabled(   enmMode != KPortMode_Disconnected
                                 && m_pParent->isMachineOffline());
    if (m_pEditorPath)
        m_pEditorPath->setEnabled(   enmMode != KPortMode_Disconnected
                                  && m_pParent->isMachineOffline());
}

void UIMachineSettingsSerial::retranslateUi()
{
    if (m_pCheckBoxPort)
    {
        m_pCheckBoxPort->setText(tr("&Enable Serial Port"));
        m_pCheckBoxPort->setToolTip(tr("When checked, enables the given serial port of the virtual machine."));
    }
    if (m_pLabelNumber)
        m_pLabelNumber->setText(tr("Port &Number:"));
    if (m_pComboNumber)
    {
        m_pComboNumber->setItemText(m_pComboNumber->count() - 1, UITranslator::toCOMPortName(0, 0));
        m_pComboNumber->setToolTip(tr("Selects the serial port number. You can choose one of the standard serial ports or select "
                                      "User-defined and specify port parameters manually."));
    }
    if (m_pLabelIRQ)
        m_pLabelIRQ->setText(tr("&IRQ:"));
    if (m_pLineEditIRQ)
        m_pLineEditIRQ->setToolTip(tr("Holds the IRQ number of this serial port. This should be a whole number between "
                                      "<tt>0</tt> and <tt>255</tt>. Values greater than <tt>15</tt> may only be used if the "
                                      "I/O APIC setting is enabled for this virtual machine."));
    if (m_pLabelIOPort)
        m_pLabelIOPort->setText(tr("I/O Po&rt:"));
    if (m_pLineEditIOPort)
        m_pLineEditIOPort->setToolTip(tr("Holds the base I/O port address of this serial port. Valid values are integer numbers "
                                         "in range from <tt>0</tt> to <tt>0xFFFF</tt>."));
    if (m_pLabelMode)
        m_pLabelMode->setText(tr("Port &Mode:"));
    if (m_pComboMode)
        m_pComboMode->setToolTip(tr("Selects the working mode of this serial port. If you select Disconnected, the guest "
                                    "OS will detect the serial port but will not be able to operate it."));
    if (m_pCheckBoxPipe)
    {
        m_pCheckBoxPipe->setText(tr("&Connect to existing pipe/socket"));
        m_pCheckBoxPipe->setToolTip(tr("When checked, the virtual machine will assume that the pipe or socket specified in the "
                                       "Path/Address field exists and try to use it. Otherwise, the pipe or socket will "
                                       "be created by the virtual machine when it starts."));
    }
    if (m_pLabelPath)
        m_pLabelPath->setText(tr("&Path/Address:"));
    if (m_pEditorPath)
        m_pEditorPath->setToolTip(tr("In Host Pipe mode: Holds the path to the serial port's pipe on the host. "
                                     "Examples: \"\\\\.\\pipe\\myvbox\" or \"/tmp/myvbox\", for Windows and UNIX-like systems "
                                     "respectively. In Host Device mode: Holds the host serial device name. "
                                     "Examples: \"COM1\" or \"/dev/ttyS0\". In Raw File mode: Holds the file-path "
                                     "on the host system, where the serial output will be dumped. In TCP mode: "
                                     "Holds the TCP \"port\" when in server mode, or \"hostname:port\" when in client mode."));

    /* Translate combo-boxes content: */
    populateComboboxes();
}

void UIMachineSettingsSerial::sltHandlePortAvailabilityToggled(bool fOn)
{
    /* Update availability: */
    m_pWidgetPortSettings->setEnabled(m_pCheckBoxPort->isChecked());
    if (fOn)
    {
        sltHandlePortStandardOptionActivated(m_pComboNumber->currentText());
        sltHandlePortModeChange(m_pComboMode->currentIndex());
    }

    /* Notify port/path changed: */
    emit sigPortChanged();
    emit sigPathChanged();
}

void UIMachineSettingsSerial::sltHandlePortStandardOptionActivated(const QString &strText)
{
    /* Update availability: */
    ulong uIRQ, uIOBase;
    bool fStd = UITranslator::toCOMPortNumbers(strText, uIRQ, uIOBase);
    m_pLineEditIRQ->setEnabled(!fStd);
    m_pLineEditIOPort->setEnabled(!fStd);
    if (fStd)
    {
        m_pLineEditIRQ->setText(QString::number(uIRQ));
        m_pLineEditIOPort->setText("0x" + QString::number(uIOBase, 16).toUpper());
    }

    /* Notify validity changed: */
    emit sigValidityChanged();
}

void UIMachineSettingsSerial::sltHandlePortModeChange(int iIndex)
{
    /* Update availability: */
    const KPortMode enmMode = m_pComboMode->itemData(iIndex).value<KPortMode>();
    m_pCheckBoxPipe->setEnabled(enmMode == KPortMode_HostPipe || enmMode == KPortMode_TCP);
    m_pEditorPath->setEnabled(enmMode != KPortMode_Disconnected);
    m_pLabelPath->setEnabled(enmMode != KPortMode_Disconnected);

    /* Notify validity changed: */
    emit sigValidityChanged();
}

void UIMachineSettingsSerial::prepare()
{
    /* Prepare everything: */
    prepareWidgets();
    prepareConnections();

    /* Apply language settings: */
    retranslateUi();
}

void UIMachineSettingsSerial::prepareWidgets()
{
    /* Prepare main layout: */
    QGridLayout *pLayoutMain = new QGridLayout(this);
    if (pLayoutMain)
    {
        pLayoutMain->setRowStretch(2, 1);

        /* Prepare port check-box: */
        m_pCheckBoxPort = new QCheckBox(this);
        if (m_pCheckBoxPort)
            pLayoutMain->addWidget(m_pCheckBoxPort, 0, 0, 1, 2);

        /* Prepare 20-px shifting spacer: */
        QSpacerItem *pSpacerItem = new QSpacerItem(20, 0, QSizePolicy::Fixed, QSizePolicy::Minimum);
        if (pSpacerItem)
            pLayoutMain->addItem(pSpacerItem, 1, 0);

        /* Prepare adapter settings widget: */
        m_pWidgetPortSettings = new QWidget(this);
        if (m_pWidgetPortSettings)
        {
            /* Prepare adapter settings widget layout: */
            QGridLayout *pLayoutPortSettings = new QGridLayout(m_pWidgetPortSettings);
            if (pLayoutPortSettings)
            {
                pLayoutPortSettings->setContentsMargins(0, 0, 0, 0);
                pLayoutPortSettings->setColumnStretch(6, 1);

                /* Prepare number label: */
                m_pLabelNumber = new QLabel(m_pWidgetPortSettings);
                if (m_pLabelNumber)
                {
                    m_pLabelNumber->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
                    pLayoutPortSettings->addWidget(m_pLabelNumber, 0, 0);
                }
                /* Prepare number combo: */
                m_pComboNumber = new QComboBox(m_pWidgetPortSettings);
                if (m_pComboNumber)
                {
                    if (m_pLabelNumber)
                        m_pLabelNumber->setBuddy(m_pComboNumber);
                    m_pComboNumber->insertItem(0, UITranslator::toCOMPortName(0, 0));
                    m_pComboNumber->insertItems(0, UITranslator::COMPortNames());
                    pLayoutPortSettings->addWidget(m_pComboNumber, 0, 1);
                }
                /* Prepare IRQ label: */
                m_pLabelIRQ = new QLabel(m_pWidgetPortSettings);
                if (m_pLabelIRQ)
                    pLayoutPortSettings->addWidget(m_pLabelIRQ, 0, 2);
                /* Prepare IRQ label: */
                m_pLineEditIRQ = new QLineEdit(m_pWidgetPortSettings);
                if (m_pLineEditIRQ)
                {
                    if (m_pLabelIRQ)
                        m_pLabelIRQ->setBuddy(m_pLineEditIRQ);
#if QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
                    m_pLineEditIRQ->setFixedWidth(m_pLineEditIRQ->fontMetrics().horizontalAdvance("8888"));
#else
                    m_pLineEditIRQ->setFixedWidth(m_pLineEditIRQ->fontMetrics().width("8888"));
#endif
                    m_pLineEditIRQ->setValidator(new QIULongValidator(0, 255, this));
                    pLayoutPortSettings->addWidget(m_pLineEditIRQ, 0, 3);
                }
                /* Prepare IO port label: */
                m_pLabelIOPort = new QLabel(m_pWidgetPortSettings);
                if (m_pLabelIOPort)
                    pLayoutPortSettings->addWidget(m_pLabelIOPort, 0, 4);
                /* Prepare IO port label: */
                m_pLineEditIOPort = new QLineEdit(m_pWidgetPortSettings);
                if (m_pLineEditIOPort)
                {
                    if (m_pLabelIOPort)
                        m_pLabelIOPort->setBuddy(m_pLineEditIOPort);
#if QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
                    m_pLineEditIOPort->setFixedWidth(m_pLineEditIOPort->fontMetrics().horizontalAdvance("8888888"));
#else
                    m_pLineEditIOPort->setFixedWidth(m_pLineEditIOPort->fontMetrics().width("8888888"));
#endif
                    m_pLineEditIOPort->setValidator(new QIULongValidator(0, 0xFFFF, this));
                    pLayoutPortSettings->addWidget(m_pLineEditIOPort, 0, 5);
                }

                /* Prepare mode label: */
                m_pLabelMode = new QLabel(m_pWidgetPortSettings);
                if (m_pLabelMode)
                {
                    m_pLabelMode->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
                    pLayoutPortSettings->addWidget(m_pLabelMode, 1, 0);
                }
                /* Prepare mode combo: */
                m_pComboMode = new QComboBox(m_pWidgetPortSettings);
                if (m_pComboMode)
                {
                    if (m_pLabelMode)
                        m_pLabelMode->setBuddy(m_pComboMode);
                    pLayoutPortSettings->addWidget(m_pComboMode, 1, 1);
                }

                /* Prepare pipe check-box: */
                m_pCheckBoxPipe = new QCheckBox(m_pWidgetPortSettings);
                if (m_pCheckBoxPipe)
                    pLayoutPortSettings->addWidget(m_pCheckBoxPipe, 2, 1, 1, 5);

                /* Prepare path label: */
                m_pLabelPath = new QLabel(m_pWidgetPortSettings);
                if (m_pLabelPath)
                {
                    m_pLabelPath->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
                    pLayoutPortSettings->addWidget(m_pLabelPath, 3, 0);
                }
                /* Prepare path editor: */
                m_pEditorPath = new QLineEdit(m_pWidgetPortSettings);
                if (m_pEditorPath)
                {
                    if (m_pLabelPath)
                        m_pLabelPath->setBuddy(m_pEditorPath);
                    m_pEditorPath->setValidator(new QRegularExpressionValidator(QRegularExpression(".+"), this));
                    pLayoutPortSettings->addWidget(m_pEditorPath, 3, 1, 1, 6);
                }
            }

            pLayoutMain->addWidget(m_pWidgetPortSettings, 1, 1);
        }
    }
}

void UIMachineSettingsSerial::prepareConnections()
{
    if (m_pCheckBoxPort)
        connect(m_pCheckBoxPort, &QCheckBox::toggled, this, &UIMachineSettingsSerial::sltHandlePortAvailabilityToggled);
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
    if (m_pComboNumber)
        connect(m_pComboNumber, static_cast<void(QComboBox::*)(const QString&)>(&QComboBox::textActivated),
                this, &UIMachineSettingsSerial::sltHandlePortStandardOptionActivated);
#else
    if (m_pComboNumber)
        connect(m_pComboNumber, static_cast<void(QComboBox::*)(const QString&)>(&QComboBox::activated),
                this, &UIMachineSettingsSerial::sltHandlePortStandardOptionActivated);
#endif
    if (m_pLineEditIRQ)
        connect(m_pLineEditIRQ, &QLineEdit::textChanged, this, &UIMachineSettingsSerial::sigPortChanged);
    if (m_pLineEditIOPort)
        connect(m_pLineEditIOPort, &QLineEdit::textChanged, this, &UIMachineSettingsSerial::sigPortChanged);
    if (m_pComboMode)
        connect(m_pComboMode, static_cast<void(QComboBox::*)(int)>(&QComboBox::activated),
                this, &UIMachineSettingsSerial::sltHandlePortModeChange);
    if (m_pEditorPath)
        connect(m_pEditorPath, &QLineEdit::textChanged, this, &UIMachineSettingsSerial::sigPathChanged);
}

void UIMachineSettingsSerial::populateComboboxes()
{
    /* Port mode: */
    {
        /* Clear the port mode combo-box: */
        m_pComboMode->clear();

        /* Load currently supported port moded: */
        CSystemProperties comProperties = uiCommon().virtualBox().GetSystemProperties();
        QVector<KPortMode> supportedModes = comProperties.GetSupportedPortModes();
        /* Take currently requested mode into account if it's sane: */
        if (!supportedModes.contains(m_enmPortMode) && m_enmPortMode != KPortMode_Max)
            supportedModes.prepend(m_enmPortMode);

        /* Populate port modes: */
        int iPortModeIndex = 0;
        foreach (const KPortMode &enmMode, supportedModes)
        {
            m_pComboMode->insertItem(iPortModeIndex, gpConverter->toString(enmMode));
            m_pComboMode->setItemData(iPortModeIndex, QVariant::fromValue(enmMode));
            m_pComboMode->setItemData(iPortModeIndex, m_pComboMode->itemText(iPortModeIndex), Qt::ToolTipRole);
            ++iPortModeIndex;
        }

        /* Choose requested port mode: */
        const int iIndex = m_pComboMode->findData(m_enmPortMode);
        m_pComboMode->setCurrentIndex(iIndex != -1 ? iIndex : 0);
    }
}


/*********************************************************************************************************************************
*   Class UIMachineSettingsSerialPage implementation.                                                                            *
*********************************************************************************************************************************/

UIMachineSettingsSerialPage::UIMachineSettingsSerialPage()
    : m_pCache(0)
    , m_pTabWidget(0)
{
    prepare();
}

UIMachineSettingsSerialPage::~UIMachineSettingsSerialPage()
{
    cleanup();
}

bool UIMachineSettingsSerialPage::changed() const
{
    return m_pCache ? m_pCache->wasChanged() : false;
}

void UIMachineSettingsSerialPage::loadToCacheFrom(QVariant &data)
{
    /* Sanity check: */
    if (   !m_pCache
        || !m_pTabWidget)
        return;

    /* Fetch data to machine: */
    UISettingsPageMachine::fetchData(data);

    /* Clear cache initially: */
    m_pCache->clear();

    /* Cache lists: */
    refreshPorts();
    refreshPaths();

    /* Prepare old data: */
    UIDataSettingsMachineSerial oldSerialData;

    /* For each serial port: */
    for (int iSlot = 0; iSlot < m_pTabWidget->count(); ++iSlot)
    {
        /* Prepare old data: */
        UIDataSettingsMachineSerialPort oldPortData;

        /* Check whether port is valid: */
        const CSerialPort &comPort = m_machine.GetSerialPort(iSlot);
        if (!comPort.isNull())
        {
            /* Gather old data: */
            oldPortData.m_iSlot = iSlot;
            oldPortData.m_fPortEnabled = comPort.GetEnabled();
            oldPortData.m_uIRQ = comPort.GetIRQ();
            oldPortData.m_uIOBase = comPort.GetIOBase();
            oldPortData.m_hostMode = comPort.GetHostMode();
            oldPortData.m_fServer = comPort.GetServer();
            oldPortData.m_strPath = comPort.GetPath();
        }

        /* Cache old data: */
        m_pCache->child(iSlot).cacheInitialData(oldPortData);
    }

    /* Cache old data: */
    m_pCache->cacheInitialData(oldSerialData);

    /* Upload machine to data: */
    UISettingsPageMachine::uploadData(data);
}

void UIMachineSettingsSerialPage::getFromCache()
{
    /* Sanity check: */
    if (   !m_pCache
        || !m_pTabWidget)
        return;

    /* Setup tab order: */
    AssertPtrReturnVoid(firstWidget());
    setTabOrder(firstWidget(), m_pTabWidget->focusProxy());
    QWidget *pLastFocusWidget = m_pTabWidget->focusProxy();

    /* For each port: */
    for (int iSlot = 0; iSlot < m_pTabWidget->count(); ++iSlot)
    {
        /* Get port page: */
        UIMachineSettingsSerial *pTab = qobject_cast<UIMachineSettingsSerial*>(m_pTabWidget->widget(iSlot));
        AssertPtrReturnVoid(pTab);

        /* Load old data from cache: */
        pTab->getPortDataFromCache(m_pCache->child(iSlot));

        /* Setup tab order: */
        pLastFocusWidget = pTab->setOrderAfter(pLastFocusWidget);
    }

    /* Apply language settings: */
    retranslateUi();

    /* Polish page finally: */
    polishPage();

    /* Revalidate: */
    revalidate();
}

void UIMachineSettingsSerialPage::putToCache()
{
    /* Sanity check: */
    if (   !m_pCache
        || !m_pTabWidget)
        return;

    /* Prepare new data: */
    UIDataSettingsMachineSerial newSerialData;

    /* For each port: */
    for (int iSlot = 0; iSlot < m_pTabWidget->count(); ++iSlot)
    {
        /* Getting port page: */
        UIMachineSettingsSerial *pTab = qobject_cast<UIMachineSettingsSerial*>(m_pTabWidget->widget(iSlot));
        AssertPtrReturnVoid(pTab);

        /* Gather new data: */
        pTab->putPortDataToCache(m_pCache->child(iSlot));
    }

    /* Cache new data: */
    m_pCache->cacheCurrentData(newSerialData);
}

void UIMachineSettingsSerialPage::saveFromCacheTo(QVariant &data)
{
    /* Fetch data to machine: */
    UISettingsPageMachine::fetchData(data);

    /* Update data and failing state: */
    setFailed(!saveData());

    /* Upload machine to data: */
    UISettingsPageMachine::uploadData(data);
}

bool UIMachineSettingsSerialPage::validate(QList<UIValidationMessage> &messages)
{
    /* Sanity check: */
    if (!m_pTabWidget)
        return false;

    /* Pass by default: */
    bool fValid = true;

    /* Delegate validation to adapter tabs: */
    for (int iIndex = 0; iIndex < m_pTabWidget->count(); ++iIndex)
    {
        UIMachineSettingsSerial *pTab = qobject_cast<UIMachineSettingsSerial*>(m_pTabWidget->widget(iIndex));
        AssertPtrReturn(pTab, false);
        if (!pTab->validate(messages))
            fValid = false;
    }

    /* Return result: */
    return fValid;
}

void UIMachineSettingsSerialPage::retranslateUi()
{
    /* Sanity check: */
    if (!m_pTabWidget)
        return;

    for (int iSlot = 0; iSlot < m_pTabWidget->count(); ++iSlot)
    {
        UIMachineSettingsSerial *pTab = qobject_cast<UIMachineSettingsSerial*>(m_pTabWidget->widget(iSlot));
        AssertPtrReturnVoid(pTab);
        m_pTabWidget->setTabText(iSlot, pTab->tabTitle());
    }
}

void UIMachineSettingsSerialPage::polishPage()
{
    /* Sanity check: */
    if (   !m_pCache
        || !m_pTabWidget)
        return;

    for (int iSlot = 0; iSlot < m_pTabWidget->count(); ++iSlot)
    {
        m_pTabWidget->setTabEnabled(iSlot,
                                    isMachineOffline() ||
                                    (isMachineInValidMode() &&
                                     m_pCache->childCount() > iSlot &&
                                     m_pCache->child(iSlot).base().m_fPortEnabled));
        UIMachineSettingsSerial *pTab = qobject_cast<UIMachineSettingsSerial*>(m_pTabWidget->widget(iSlot));
        AssertPtrReturnVoid(pTab);
        pTab->polishTab();
    }
}

void UIMachineSettingsSerialPage::sltHandlePortChange()
{
    refreshPorts();
    revalidate();
}

void UIMachineSettingsSerialPage::sltHandlePathChange()
{
    refreshPaths();
    revalidate();
}

void UIMachineSettingsSerialPage::prepare()
{
    /* Prepare cache: */
    m_pCache = new UISettingsCacheMachineSerial;
    AssertPtrReturnVoid(m_pCache);

    /* Create main layout: */
    QVBoxLayout *pLayoutMain = new QVBoxLayout(this);
    if (pLayoutMain)
    {
        /* Creating tab-widget: */
        m_pTabWidget = new QITabWidget;
        if (m_pTabWidget)
        {
            /* How many ports to display: */
            const ulong uCount = uiCommon().virtualBox().GetSystemProperties().GetSerialPortCount();

            /* Create corresponding port tabs: */
            for (ulong uSlot = 0; uSlot < uCount; ++uSlot)
            {
                /* Create port tab: */
                UIMachineSettingsSerial *pTab = new UIMachineSettingsSerial(this);
                if (pTab)
                {
                    /* Tab connections: */
                    connect(pTab, &UIMachineSettingsSerial::sigPortChanged,
                            this, &UIMachineSettingsSerialPage::sltHandlePortChange);
                    connect(pTab, &UIMachineSettingsSerial::sigPathChanged,
                            this, &UIMachineSettingsSerialPage::sltHandlePathChange);
                    connect(pTab, &UIMachineSettingsSerial::sigValidityChanged,
                            this, &UIMachineSettingsSerialPage::revalidate);

                    /* Add tab into tab-widget: */
                    m_pTabWidget->addTab(pTab, pTab->tabTitle());
                }
            }

            /* Add tab-widget into layout: */
            pLayoutMain->addWidget(m_pTabWidget);
        }
    }
}

void UIMachineSettingsSerialPage::cleanup()
{
    /* Cleanup cache: */
    delete m_pCache;
    m_pCache = 0;
}

void UIMachineSettingsSerialPage::refreshPorts()
{
    /* Sanity check: */
    if (!m_pTabWidget)
        return;

    /* Reload port list: */
    m_ports.clear();
    m_ports.resize(m_pTabWidget->count());
    /* Append port list with data from all the tabs: */
    for (int iSlot = 0; iSlot < m_pTabWidget->count(); ++iSlot)
    {
        UIMachineSettingsSerial *pTab = qobject_cast<UIMachineSettingsSerial*>(m_pTabWidget->widget(iSlot));
        AssertPtrReturnVoid(pTab);
        m_ports[iSlot] = pTab->isPortEnabled() ? qMakePair(pTab->irq(), pTab->ioPort()) : qMakePair(QString(), QString());
    }
}

void UIMachineSettingsSerialPage::refreshPaths()
{
    /* Sanity check: */
    if (!m_pTabWidget)
        return;

    /* Reload path list: */
    m_paths.clear();
    m_paths.resize(m_pTabWidget->count());
    /* Append path list with data from all the tabs: */
    for (int iSlot = 0; iSlot < m_pTabWidget->count(); ++iSlot)
    {
        UIMachineSettingsSerial *pTab = qobject_cast<UIMachineSettingsSerial*>(m_pTabWidget->widget(iSlot));
        AssertPtrReturnVoid(pTab);
        m_paths[iSlot] = pTab->isPortEnabled() ? pTab->path() : QString();
    }
}

bool UIMachineSettingsSerialPage::saveData()
{
    /* Sanity check: */
    if (   !m_pCache
        || !m_pTabWidget)
        return false;

    /* Prepare result: */
    bool fSuccess = true;
    /* Save serial settings from cache: */
    if (fSuccess && isMachineInValidMode() && m_pCache->wasChanged())
    {
        /* For each port: */
        for (int iSlot = 0; fSuccess && iSlot < m_pTabWidget->count(); ++iSlot)
            fSuccess = savePortData(iSlot);
    }
    /* Return result: */
    return fSuccess;
}

bool UIMachineSettingsSerialPage::savePortData(int iSlot)
{
    /* Sanity check: */
    if (!m_pCache)
        return false;

    /* Prepare result: */
    bool fSuccess = true;
    /* Save adapter settings from cache: */
    if (fSuccess && m_pCache->child(iSlot).wasChanged())
    {
        /* Get old data from cache: */
        const UIDataSettingsMachineSerialPort &oldPortData = m_pCache->child(iSlot).base();
        /* Get new data from cache: */
        const UIDataSettingsMachineSerialPort &newPortData = m_pCache->child(iSlot).data();

        /* Get serial port for further activities: */
        CSerialPort comPort = m_machine.GetSerialPort(iSlot);
        fSuccess = m_machine.isOk() && comPort.isNotNull();

        /* Show error message if necessary: */
        if (!fSuccess)
            notifyOperationProgressError(UIErrorString::formatErrorInfo(m_machine));
        else
        {
            // This *must* be first.
            // If the requested host mode is changed to disconnected we should do it first.
            // That allows to automatically fulfill the requirements for some of the settings below.
            /* Save port host mode: */
            if (   fSuccess && isMachineOffline()
                && newPortData.m_hostMode != oldPortData.m_hostMode
                && newPortData.m_hostMode == KPortMode_Disconnected)
            {
                comPort.SetHostMode(newPortData.m_hostMode);
                fSuccess = comPort.isOk();
            }
            /* Save whether the port is enabled: */
            if (fSuccess && isMachineOffline() && newPortData.m_fPortEnabled != oldPortData.m_fPortEnabled)
            {
                comPort.SetEnabled(newPortData.m_fPortEnabled);
                fSuccess = comPort.isOk();
            }
            /* Save port IRQ: */
            if (fSuccess && isMachineOffline() && newPortData.m_uIRQ != oldPortData.m_uIRQ)
            {
                comPort.SetIRQ(newPortData.m_uIRQ);
                fSuccess = comPort.isOk();
            }
            /* Save port IO base: */
            if (fSuccess && isMachineOffline() && newPortData.m_uIOBase != oldPortData.m_uIOBase)
            {
                comPort.SetIOBase(newPortData.m_uIOBase);
                fSuccess = comPort.isOk();
            }
            /* Save whether the port is server: */
            if (fSuccess && isMachineOffline() && newPortData.m_fServer != oldPortData.m_fServer)
            {
                comPort.SetServer(newPortData.m_fServer);
                fSuccess = comPort.isOk();
            }
            /* Save port path: */
            if (fSuccess && isMachineOffline() && newPortData.m_strPath != oldPortData.m_strPath)
            {
                comPort.SetPath(newPortData.m_strPath);
                fSuccess = comPort.isOk();
            }
            // This *must* be last.
            // The host mode will be changed to disconnected if some of the necessary
            // settings above will not meet the requirements for the selected mode.
            /* Save port host mode: */
            if (   fSuccess && isMachineOffline()
                && newPortData.m_hostMode != oldPortData.m_hostMode
                && newPortData.m_hostMode != KPortMode_Disconnected)
            {
                comPort.SetHostMode(newPortData.m_hostMode);
                fSuccess = comPort.isOk();
            }

            /* Show error message if necessary: */
            if (!fSuccess)
                notifyOperationProgressError(UIErrorString::formatErrorInfo(comPort));
        }
    }
    /* Return result: */
    return fSuccess;
}

# include "UIMachineSettingsSerial.moc"

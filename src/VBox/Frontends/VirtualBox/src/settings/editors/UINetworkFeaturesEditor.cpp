/* $Id: UINetworkFeaturesEditor.cpp $ */
/** @file
 * VBox Qt GUI - UINetworkFeaturesEditor class implementation.
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
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QTextEdit>
#include <QVBoxLayout>

/* GUI includes: */
#include "QIArrowButtonSwitch.h"
#include "QILineEdit.h"
#include "QIToolButton.h"
#include "UICommon.h"
#include "UIConverter.h"
#include "UIIconPool.h"
#include "UINetworkFeaturesEditor.h"

/* COM includes: */
#include "CSystemProperties.h"


UINetworkFeaturesEditor::UINetworkFeaturesEditor(QWidget *pParent /* = 0 */)
    : QIWithRetranslateUI<QWidget>(pParent)
    , m_fAdvancedButtonExpanded(false)
    , m_enmAdapterType(KNetworkAdapterType_Null)
    , m_enmPromiscuousMode(KNetworkAdapterPromiscModePolicy_Max)
    , m_fCableConnected(false)
    , m_pButtonAdvanced(0)
    , m_pWidgetSettings(0)
    , m_pLayoutSettings(0)
    , m_pLabelAdapterType(0)
    , m_pComboAdapterType(0)
    , m_pLabelPromiscuousMode(0)
    , m_pComboPromiscuousMode(0)
    , m_pLabelMAC(0)
    , m_pEditorMAC(0)
    , m_pButtonMAC(0)
    , m_pLabelGenericProperties(0)
    , m_pEditorGenericProperties(0)
    , m_pCheckBoxCableConnected(0)
    , m_pButtonPortForwarding(0)
{
    prepare();
}

void UINetworkFeaturesEditor::setAdvancedButtonExpanded(bool fExpanded)
{
    if (m_fAdvancedButtonExpanded != fExpanded)
    {
        m_fAdvancedButtonExpanded = fExpanded;
        if (m_pButtonAdvanced)
        {
            m_pButtonAdvanced->setExpanded(m_fAdvancedButtonExpanded);
            sltHandleAdvancedButtonStateChange();
        }
    }
}

bool UINetworkFeaturesEditor::advancedButtonExpanded() const
{
    return m_pButtonAdvanced ? m_pButtonAdvanced->isExpanded() : m_fAdvancedButtonExpanded;
}

void UINetworkFeaturesEditor::setAdapterType(const KNetworkAdapterType &enmType)
{
    if (m_enmAdapterType != enmType)
    {
        m_enmAdapterType = enmType;
        repopulateAdapterTypeCombo();
    }
}

KNetworkAdapterType UINetworkFeaturesEditor::adapterType() const
{
    return m_pComboAdapterType ? m_pComboAdapterType->currentData().value<KNetworkAdapterType>() : m_enmAdapterType;
}

void UINetworkFeaturesEditor::setPromiscuousMode(const KNetworkAdapterPromiscModePolicy &enmMode)
{
    if (m_enmPromiscuousMode != enmMode)
    {
        m_enmPromiscuousMode = enmMode;
        repopulatePromiscuousModeCombo();
    }
}

KNetworkAdapterPromiscModePolicy UINetworkFeaturesEditor::promiscuousMode() const
{
    return m_pComboPromiscuousMode ? m_pComboPromiscuousMode->currentData().value<KNetworkAdapterPromiscModePolicy>() : m_enmPromiscuousMode;
}

void UINetworkFeaturesEditor::setMACAddress(const QString &strAddress)
{
    if (m_strMACAddress != strAddress)
    {
        m_strMACAddress = strAddress;
        if (m_pEditorMAC)
            m_pEditorMAC->setText(m_strMACAddress);
    }
}

QString UINetworkFeaturesEditor::macAddress() const
{
    return m_pEditorMAC ? m_pEditorMAC->text() : m_strMACAddress;
}

void UINetworkFeaturesEditor::setGenericProperties(const QString &strProperties)
{
    if (m_strGenericProperties != strProperties)
    {
        m_strGenericProperties = strProperties;
        if (m_pEditorGenericProperties)
            m_pEditorGenericProperties->setPlainText(m_strGenericProperties);
    }
}

QString UINetworkFeaturesEditor::genericProperties() const
{
    return m_pEditorGenericProperties ? m_pEditorGenericProperties->toPlainText() : m_strGenericProperties;
}

void UINetworkFeaturesEditor::setCableConnected(bool fConnected)
{
    if (m_fCableConnected != fConnected)
    {
        m_fCableConnected = fConnected;
        if (m_pCheckBoxCableConnected)
            m_pCheckBoxCableConnected->setChecked(m_fCableConnected);
    }
}

bool UINetworkFeaturesEditor::cableConnected() const
{
    return m_pCheckBoxCableConnected ? m_pCheckBoxCableConnected->isChecked() : m_fCableConnected;
}

void UINetworkFeaturesEditor::setPortForwardingRules(const UIPortForwardingDataList &rules)
{
    if (m_portForwardingRules != rules)
        m_portForwardingRules = rules;
}

UIPortForwardingDataList UINetworkFeaturesEditor::portForwardingRules() const
{
    return m_portForwardingRules;
}

void UINetworkFeaturesEditor::setAdvancedOptionsAvailable(bool fAvailable)
{
    m_pButtonAdvanced->setEnabled(fAvailable);
}

void UINetworkFeaturesEditor::setAdapterOptionsAvailable(bool fAvailable)
{
    m_pLabelAdapterType->setEnabled(fAvailable);
    m_pComboAdapterType->setEnabled(fAvailable);
}

void UINetworkFeaturesEditor::setPromiscuousOptionsAvailable(bool fAvailable)
{
    m_pLabelPromiscuousMode->setEnabled(fAvailable);
    m_pComboPromiscuousMode->setEnabled(fAvailable);
}

void UINetworkFeaturesEditor::setMACOptionsAvailable(bool fAvailable)
{
    m_pLabelMAC->setEnabled(fAvailable);
    m_pEditorMAC->setEnabled(fAvailable);
    m_pButtonMAC->setEnabled(fAvailable);
}

void UINetworkFeaturesEditor::setGenericPropertiesAvailable(bool fAvailable)
{
    m_pLabelGenericProperties->setVisible(fAvailable);
    m_pEditorGenericProperties->setVisible(fAvailable);
}

void UINetworkFeaturesEditor::setCableOptionsAvailable(bool fAvailable)
{
    m_pCheckBoxCableConnected->setEnabled(fAvailable);
}

void UINetworkFeaturesEditor::setForwardingOptionsAvailable(bool fAvailable)
{
    m_pButtonPortForwarding->setVisible(fAvailable);
}

int UINetworkFeaturesEditor::minimumLabelHorizontalHint() const
{
    int iMinimumLabelHorizontalHint = 0;
    if (m_pLabelAdapterType)
        iMinimumLabelHorizontalHint = qMax(iMinimumLabelHorizontalHint, m_pLabelAdapterType->minimumSizeHint().width());
    if (m_pLabelPromiscuousMode)
        iMinimumLabelHorizontalHint = qMax(iMinimumLabelHorizontalHint, m_pLabelPromiscuousMode->minimumSizeHint().width());
    if (m_pLabelMAC)
        iMinimumLabelHorizontalHint = qMax(iMinimumLabelHorizontalHint, m_pLabelMAC->minimumSizeHint().width());
    if (m_pLabelGenericProperties)
        iMinimumLabelHorizontalHint = qMax(iMinimumLabelHorizontalHint, m_pLabelGenericProperties->minimumSizeHint().width());
    return iMinimumLabelHorizontalHint;
}

void UINetworkFeaturesEditor::setMinimumLayoutIndent(int iIndent)
{
    if (m_pLayoutSettings)
        m_pLayoutSettings->setColumnMinimumWidth(0, iIndent);
}

void UINetworkFeaturesEditor::generateMac()
{
    setMACAddress(uiCommon().host().GenerateMACAddress());
}

void UINetworkFeaturesEditor::retranslateUi()
{
    if (m_pButtonAdvanced)
    {
        m_pButtonAdvanced->setText(tr("A&dvanced"));
        m_pButtonAdvanced->setToolTip(tr("Shows additional network adapter options."));
    }

    if (m_pLabelAdapterType)
        m_pLabelAdapterType->setText(tr("Adapter &Type:"));
    if (m_pComboAdapterType)
    {
        for (int i = 0; i < m_pComboAdapterType->count(); ++i)
        {
            const KNetworkAdapterType enmType = m_pComboAdapterType->itemData(i).value<KNetworkAdapterType>();
            m_pComboAdapterType->setItemText(i, gpConverter->toString(enmType));
        }
        m_pComboAdapterType->setToolTip(tr("Holds the type of the virtual network adapter. Depending on this value, VirtualBox "
                                           "will provide different network hardware to the virtual machine."));
    }

    if (m_pLabelPromiscuousMode)
        m_pLabelPromiscuousMode->setText(tr("&Promiscuous Mode:"));
    if (m_pComboPromiscuousMode)
    {
        for (int i = 0; i < m_pComboPromiscuousMode->count(); ++i)
        {
            const KNetworkAdapterPromiscModePolicy enmType = m_pComboPromiscuousMode->itemData(i).value<KNetworkAdapterPromiscModePolicy>();
            m_pComboPromiscuousMode->setItemText(i, gpConverter->toString(enmType));
        }
        m_pComboPromiscuousMode->setToolTip(tr("Holds the promiscuous mode policy of the network adapter when attached to an "
                                               "internal network, host only network or a bridge."));
    }

    if (m_pLabelMAC)
        m_pLabelMAC->setText(tr("&MAC Address:"));
    if (m_pEditorMAC)
        m_pEditorMAC->setToolTip(tr("Holds the MAC address of this adapter. It contains exactly 12 characters chosen from "
                                    "{0-9,A-F}. Note that the second character must be an even digit."));
    if (m_pButtonMAC)
        m_pButtonMAC->setToolTip(tr("Generates a new random MAC address."));

    if (m_pLabelGenericProperties)
        m_pLabelGenericProperties->setText(tr("Generic Properties:"));
    if (m_pEditorGenericProperties)
        m_pEditorGenericProperties->setToolTip(tr("Holds the configuration settings for the network attachment driver. The "
                                                  "settings should be of the form name=value and will depend on the "
                                                  "driver. Use shift-enter to add a new entry."));

    if (m_pCheckBoxCableConnected)
    {
        m_pCheckBoxCableConnected->setText(tr("&Cable Connected"));
        m_pCheckBoxCableConnected->setToolTip(tr("When checked, the virtual network cable is plugged in."));
    }

    if (m_pButtonPortForwarding)
    {
        m_pButtonPortForwarding->setText(tr("&Port Forwarding"));
        m_pButtonPortForwarding->setToolTip(tr("Displays a window to configure port forwarding rules."));
    }
}

void UINetworkFeaturesEditor::sltHandleAdvancedButtonStateChange()
{
    /* What's the state? */
    const bool fExpanded = m_pButtonAdvanced->isExpanded();
    /* Update widget visibility: */
    m_pWidgetSettings->setVisible(fExpanded);
    /* Notify listeners about the button state change: */
    emit sigAdvancedButtonStateChange(fExpanded);
}

void UINetworkFeaturesEditor::sltOpenPortForwardingDlg()
{
    UIMachineSettingsPortForwardingDlg dlg(this, m_portForwardingRules);
    if (dlg.exec() == QDialog::Accepted)
        m_portForwardingRules = dlg.rules();
}

void UINetworkFeaturesEditor::prepare()
{
    /* Prepare main layout: */
    QVBoxLayout *pLayout = new QVBoxLayout(this);
    if (pLayout)
    {
        pLayout->setContentsMargins(0, 0, 0, 0);

        /* Prepare advanced arrow button: */
        m_pButtonAdvanced = new QIArrowButtonSwitch(this);
        if (m_pButtonAdvanced)
        {
            const QStyle *pStyle = QApplication::style();
            const int iIconMetric = (int)(pStyle->pixelMetric(QStyle::PM_SmallIconSize) * .625);
            m_pButtonAdvanced->setIconSize(QSize(iIconMetric, iIconMetric));
            m_pButtonAdvanced->setIcons(UIIconPool::iconSet(":/arrow_right_10px.png"),
                                        UIIconPool::iconSet(":/arrow_down_10px.png"));

            pLayout->addWidget(m_pButtonAdvanced);
        }

        /* Prepare advanced settings widget: */
        m_pWidgetSettings = new QWidget(this);
        if (m_pWidgetSettings)
        {
            /* Prepare advanced settings layout: */
            m_pLayoutSettings = new QGridLayout(m_pWidgetSettings);
            if (m_pLayoutSettings)
            {
                m_pLayoutSettings->setContentsMargins(0, 0, 0, 0);
                m_pLayoutSettings->setColumnStretch(2, 1);

                /* Prepare adapter type label: */
                m_pLabelAdapterType = new QLabel(m_pWidgetSettings);
                if (m_pLabelAdapterType)
                {
                    m_pLabelAdapterType->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
                    m_pLayoutSettings->addWidget(m_pLabelAdapterType, 0, 0);
                }
                /* Prepare adapter type combo: */
                m_pComboAdapterType = new QComboBox(m_pWidgetSettings);
                if (m_pComboAdapterType)
                {
                    if (m_pLabelAdapterType)
                        m_pLabelAdapterType->setBuddy(m_pComboAdapterType);
                    m_pLayoutSettings->addWidget(m_pComboAdapterType, 0, 1, 1, 3);
                }

                /* Prepare promiscuous mode label: */
                m_pLabelPromiscuousMode = new QLabel(m_pWidgetSettings);
                if (m_pLabelPromiscuousMode)
                {
                    m_pLabelPromiscuousMode->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
                    m_pLayoutSettings->addWidget(m_pLabelPromiscuousMode, 1, 0);
                }
                /* Prepare promiscuous mode combo: */
                m_pComboPromiscuousMode = new QComboBox(m_pWidgetSettings);
                if (m_pComboPromiscuousMode)
                {
                    if (m_pLabelPromiscuousMode)
                        m_pLabelPromiscuousMode->setBuddy(m_pComboPromiscuousMode);
                    m_pLayoutSettings->addWidget(m_pComboPromiscuousMode, 1, 1, 1, 3);
                }

                /* Prepare MAC label: */
                m_pLabelMAC = new QLabel(m_pWidgetSettings);
                if (m_pLabelMAC)
                {
                    m_pLabelMAC->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
                    m_pLayoutSettings->addWidget(m_pLabelMAC, 2, 0);
                }
                /* Prepare MAC editor: */
                m_pEditorMAC = new QILineEdit(m_pWidgetSettings);
                if (m_pEditorMAC)
                {
                    if (m_pLabelMAC)
                        m_pLabelMAC->setBuddy(m_pEditorMAC);
                    m_pEditorMAC->setAllowToCopyContentsWhenDisabled(true);
                    m_pEditorMAC->setValidator(new QRegularExpressionValidator(QRegularExpression("[0-9A-Fa-f]{12}"), this));
                    m_pEditorMAC->setMinimumWidthByText(QString().fill('0', 12));

                    m_pLayoutSettings->addWidget(m_pEditorMAC, 2, 1, 1, 2);
                }
                /* Prepare MAC button: */
                m_pButtonMAC = new QIToolButton(m_pWidgetSettings);
                if (m_pButtonMAC)
                {
                    m_pButtonMAC->setIcon(UIIconPool::iconSet(":/refresh_16px.png"));
                    m_pLayoutSettings->addWidget(m_pButtonMAC, 2, 3);
                }

                /* Prepare MAC label: */
                m_pLabelGenericProperties = new QLabel(m_pWidgetSettings);
                if (m_pLabelGenericProperties)
                {
                    m_pLabelGenericProperties->setAlignment(Qt::AlignRight | Qt::AlignTop);
                    m_pLayoutSettings->addWidget(m_pLabelGenericProperties, 3, 0);
                }
                /* Prepare MAC editor: */
                m_pEditorGenericProperties = new QTextEdit(m_pWidgetSettings);
                if (m_pEditorGenericProperties)
                    m_pLayoutSettings->addWidget(m_pEditorGenericProperties, 3, 1, 1, 3);

                /* Prepare cable connected check-box: */
                m_pCheckBoxCableConnected = new QCheckBox(m_pWidgetSettings);
                if (m_pCheckBoxCableConnected)
                    m_pLayoutSettings->addWidget(m_pCheckBoxCableConnected, 4, 1, 1, 2);

                /* Prepare port forwarding button: */
                m_pButtonPortForwarding = new QPushButton(m_pWidgetSettings);
                if (m_pButtonPortForwarding)
                    m_pLayoutSettings->addWidget(m_pButtonPortForwarding, 5, 1);
            }

            pLayout->addWidget(m_pWidgetSettings);
        }
    }

    /* Configure connections: */
    if (m_pButtonAdvanced)
        connect(m_pButtonAdvanced, &QIArrowButtonSwitch::sigClicked,
                this, &UINetworkFeaturesEditor::sltHandleAdvancedButtonStateChange);
    if (m_pEditorMAC)
        connect(m_pEditorMAC, &QILineEdit::textChanged,
                this, &UINetworkFeaturesEditor::sigMACAddressChanged);
    if (m_pButtonMAC)
        connect(m_pButtonMAC, &QIToolButton::clicked,
                this, &UINetworkFeaturesEditor::generateMac);
    if (m_pButtonPortForwarding)
        connect(m_pButtonPortForwarding, &QPushButton::clicked,
                this, &UINetworkFeaturesEditor::sltOpenPortForwardingDlg);

    /* Update widget availability: */
    m_pWidgetSettings->setVisible(m_pButtonAdvanced->isExpanded());

    /* Apply language settings: */
    retranslateUi();
}

void UINetworkFeaturesEditor::repopulateAdapterTypeCombo()
{
    if (m_pComboAdapterType)
    {
        /* Clear combo first of all: */
        m_pComboAdapterType->clear();

        /* Load currently supported types: */
        CSystemProperties comProperties = uiCommon().virtualBox().GetSystemProperties();
        QVector<KNetworkAdapterType> supportedTypes = comProperties.GetSupportedNetworkAdapterTypes();

        /* Make sure requested value if sane is present as well: */
        if (   m_enmAdapterType != KNetworkAdapterType_Null
            && !supportedTypes.contains(m_enmAdapterType))
            supportedTypes.prepend(m_enmAdapterType);

        /* Update combo with all the supported values: */
        foreach (const KNetworkAdapterType &enmType, supportedTypes)
            m_pComboAdapterType->addItem(QString(), QVariant::fromValue(enmType));

        /* Look for proper index to choose: */
        const int iIndex = m_pComboAdapterType->findData(QVariant::fromValue(m_enmAdapterType));
        if (iIndex != -1)
            m_pComboAdapterType->setCurrentIndex(iIndex);

        /* Retranslate finally: */
        retranslateUi();
    }
}

void UINetworkFeaturesEditor::repopulatePromiscuousModeCombo()
{
    if (m_pComboPromiscuousMode)
    {
        /* Clear combo first of all: */
        m_pComboPromiscuousMode->clear();

        /* Populate currently supported types: */
        QVector<KNetworkAdapterPromiscModePolicy> supportedTypes =
            QVector<KNetworkAdapterPromiscModePolicy>() << KNetworkAdapterPromiscModePolicy_Deny
                                                        << KNetworkAdapterPromiscModePolicy_AllowNetwork
                                                        << KNetworkAdapterPromiscModePolicy_AllowAll;

        /* Make sure requested value if sane is present as well: */
        if (   m_enmPromiscuousMode != KNetworkAdapterPromiscModePolicy_Max
            && !supportedTypes.contains(m_enmPromiscuousMode))
            supportedTypes.prepend(m_enmPromiscuousMode);

        /* Update combo with all the supported values: */
        foreach (const KNetworkAdapterPromiscModePolicy &enmType, supportedTypes)
            m_pComboPromiscuousMode->addItem(QString(), QVariant::fromValue(enmType));

        /* Look for proper index to choose: */
        const int iIndex = m_pComboPromiscuousMode->findData(QVariant::fromValue(m_enmPromiscuousMode));
        if (iIndex != -1)
            m_pComboPromiscuousMode->setCurrentIndex(iIndex);

        /* Retranslate finally: */
        retranslateUi();
    }
}

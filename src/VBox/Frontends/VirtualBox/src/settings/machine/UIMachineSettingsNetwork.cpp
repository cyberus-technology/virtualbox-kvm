/* $Id: UIMachineSettingsNetwork.cpp $ */
/** @file
 * VBox Qt GUI - UIMachineSettingsNetwork class implementation.
 */

/*
 * Copyright (C) 2008-2023 Oracle and/or its affiliates.
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
#include <QRegularExpression>
#include <QVBoxLayout>

/* GUI includes: */
#include "QITabWidget.h"
#include "UICommon.h"
#include "UIErrorString.h"
#include "UIMachineSettingsNetwork.h"
#include "UINetworkAttachmentEditor.h"
#include "UINetworkSettingsEditor.h"
#include "UITranslator.h"

/* COM includes: */
#include "CNATEngine.h"
#include "CNetworkAdapter.h"


QString wipedOutString(const QString &strInputString)
{
    return strInputString.isEmpty() ? QString() : strInputString;
}


/** Machine settings: Network Adapter data structure. */
struct UIDataSettingsMachineNetworkAdapter
{
    /** Constructs data. */
    UIDataSettingsMachineNetworkAdapter()
        : m_iSlot(0)
        , m_fAdapterEnabled(false)
        , m_adapterType(KNetworkAdapterType_Null)
        , m_attachmentType(KNetworkAttachmentType_Null)
        , m_promiscuousMode(KNetworkAdapterPromiscModePolicy_Deny)
        , m_strBridgedAdapterName(QString())
        , m_strInternalNetworkName(QString())
        , m_strHostInterfaceName(QString())
        , m_strGenericDriverName(QString())
        , m_strGenericProperties(QString())
        , m_strNATNetworkName(QString())
#ifdef VBOX_WITH_CLOUD_NET
        , m_strCloudNetworkName(QString())
#endif
#ifdef VBOX_WITH_VMNET
        , m_strHostOnlyNetworkName(QString())
#endif
        , m_strMACAddress(QString())
        , m_fCableConnected(false)
    {}

    /** Returns whether the @a other passed data is equal to this one. */
    bool equal(const UIDataSettingsMachineNetworkAdapter &other) const
    {
        return true
               && (m_iSlot == other.m_iSlot)
               && (m_fAdapterEnabled == other.m_fAdapterEnabled)
               && (m_adapterType == other.m_adapterType)
               && (m_attachmentType == other.m_attachmentType)
               && (m_promiscuousMode == other.m_promiscuousMode)
               && (m_strBridgedAdapterName == other.m_strBridgedAdapterName)
               && (m_strInternalNetworkName == other.m_strInternalNetworkName)
               && (m_strHostInterfaceName == other.m_strHostInterfaceName)
               && (m_strGenericDriverName == other.m_strGenericDriverName)
               && (m_strGenericProperties == other.m_strGenericProperties)
               && (m_strNATNetworkName == other.m_strNATNetworkName)
#ifdef VBOX_WITH_CLOUD_NET
               && (m_strCloudNetworkName == other.m_strCloudNetworkName)
#endif
#ifdef VBOX_WITH_VMNET
               && (m_strHostOnlyNetworkName == other.m_strHostOnlyNetworkName)
#endif
               && (m_strMACAddress == other.m_strMACAddress)
               && (m_fCableConnected == other.m_fCableConnected)
               ;
    }

    /** Returns whether the @a other passed data is equal to this one. */
    bool operator==(const UIDataSettingsMachineNetworkAdapter &other) const { return equal(other); }
    /** Returns whether the @a other passed data is different from this one. */
    bool operator!=(const UIDataSettingsMachineNetworkAdapter &other) const { return !equal(other); }

    /** Holds the network adapter slot number. */
    int                               m_iSlot;
    /** Holds whether the network adapter is enabled. */
    bool                              m_fAdapterEnabled;
    /** Holds the network adapter type. */
    KNetworkAdapterType               m_adapterType;
    /** Holds the network attachment type. */
    KNetworkAttachmentType            m_attachmentType;
    /** Holds the network promiscuous mode policy. */
    KNetworkAdapterPromiscModePolicy  m_promiscuousMode;
    /** Holds the bridged adapter name. */
    QString                           m_strBridgedAdapterName;
    /** Holds the internal network name. */
    QString                           m_strInternalNetworkName;
    /** Holds the host interface name. */
    QString                           m_strHostInterfaceName;
    /** Holds the generic driver name. */
    QString                           m_strGenericDriverName;
    /** Holds the generic driver properties. */
    QString                           m_strGenericProperties;
    /** Holds the NAT network name. */
    QString                           m_strNATNetworkName;
#ifdef VBOX_WITH_CLOUD_NET
    /** Holds the cloud network name. */
    QString                           m_strCloudNetworkName;
#endif
#ifdef VBOX_WITH_VMNET
    /** Holds the host-only network name. */
    QString                           m_strHostOnlyNetworkName;
#endif
    /** Holds the network adapter MAC address. */
    QString                           m_strMACAddress;
    /** Holds whether the network adapter is connected. */
    bool                              m_fCableConnected;
};


/** Machine settings: Network page data structure. */
struct UIDataSettingsMachineNetwork
{
    /** Constructs data. */
    UIDataSettingsMachineNetwork() {}

    /** Returns whether the @a other passed data is equal to this one. */
    bool operator==(const UIDataSettingsMachineNetwork & /* other */) const { return true; }
    /** Returns whether the @a other passed data is different from this one. */
    bool operator!=(const UIDataSettingsMachineNetwork & /* other */) const { return false; }
};


/** Machine settings: Network Adapter tab. */
class UIMachineSettingsNetwork : public QIWithRetranslateUI<QWidget>
{
    Q_OBJECT;

signals:

    /** Notifies about alternative name was changed. */
    void sigAlternativeNameChanged();

    /** Notifies about advanced button state change to @a fExpanded. */
    void sigAdvancedButtonStateChange(bool fExpanded);

    /** Notifies about validity changed. */
    void sigValidityChanged();

public:

    /** Constructs tab passing @a pParent to the base-class. */
    UIMachineSettingsNetwork(UIMachineSettingsNetworkPage *pParent);

    /** Loads adapter data from @a adapterCache. */
    void getAdapterDataFromCache(const UISettingsCacheMachineNetworkAdapter &adapterCache);
    /** Saves adapter data to @a adapterCache. */
    void putAdapterDataToCache(UISettingsCacheMachineNetworkAdapter &adapterCache);

    /** Performs validation, updates @a messages list if something is wrong. */
    bool validate(QList<UIValidationMessage> &messages);

    /** Configures tab order according to passed @a pWidget. */
    QWidget *setOrderAfter(QWidget *pWidget);

    /** Returns tab title. */
    QString tabTitle() const;
    /** Returns tab attachment type. */
    KNetworkAttachmentType attachmentType() const;
    /** Returne tab alternative name for @a enmType specified. */
    QString alternativeName(KNetworkAttachmentType enmType = KNetworkAttachmentType_Null) const;

    /** Performs tab polishing. */
    void polishTab();
    /** Reloads tab alternatives. */
    void reloadAlternatives();

    /** Defines whether the advanced button is @a fExpanded. */
    void setAdvancedButtonExpanded(bool fExpanded);

protected:

    /** Handles translation event. */
    void retranslateUi();

private slots:

    /** Handles adapter alternative name change. */
    void sltHandleAlternativeNameChange();

private:

    /** Prepares all. */
    void prepare();
    /** Prepares widgets. */
    void prepareWidgets();
    /** Prepares connections. */
    void prepareConnections();

    /** Holds parent page reference. */
    UIMachineSettingsNetworkPage *m_pParent;

    /** Holds tab slot number. */
    int  m_iSlot;

    /** Holds the network settings editor instance. */
    UINetworkSettingsEditor *m_pEditorNetworkSettings;
};


/*********************************************************************************************************************************
*   Class UIMachineSettingsNetwork implementation.                                                                               *
*********************************************************************************************************************************/

UIMachineSettingsNetwork::UIMachineSettingsNetwork(UIMachineSettingsNetworkPage *pParent)
    : QIWithRetranslateUI<QWidget>(0)
    , m_pParent(pParent)
    , m_iSlot(-1)
    , m_pEditorNetworkSettings(0)
{
    prepare();
}

void UIMachineSettingsNetwork::getAdapterDataFromCache(const UISettingsCacheMachineNetworkAdapter &adapterCache)
{
    /* Get old data: */
    const UIDataSettingsMachineNetworkAdapter &oldAdapterData = adapterCache.base();

    /* Load slot number: */
    m_iSlot = oldAdapterData.m_iSlot;

    if (m_pEditorNetworkSettings)
    {
        /* Load adapter activity state: */
        m_pEditorNetworkSettings->setFeatureEnabled(oldAdapterData.m_fAdapterEnabled);

        /* Load attachment type: */
        m_pEditorNetworkSettings->setValueType(oldAdapterData.m_attachmentType);
        /* Load alternative names: */
        m_pEditorNetworkSettings->setValueName(KNetworkAttachmentType_Bridged, wipedOutString(oldAdapterData.m_strBridgedAdapterName));
        m_pEditorNetworkSettings->setValueName(KNetworkAttachmentType_Internal, wipedOutString(oldAdapterData.m_strInternalNetworkName));
        m_pEditorNetworkSettings->setValueName(KNetworkAttachmentType_HostOnly, wipedOutString(oldAdapterData.m_strHostInterfaceName));
        m_pEditorNetworkSettings->setValueName(KNetworkAttachmentType_Generic, wipedOutString(oldAdapterData.m_strGenericDriverName));
        m_pEditorNetworkSettings->setValueName(KNetworkAttachmentType_NATNetwork, wipedOutString(oldAdapterData.m_strNATNetworkName));
#ifdef VBOX_WITH_CLOUD_NET
        m_pEditorNetworkSettings->setValueName(KNetworkAttachmentType_Cloud, wipedOutString(oldAdapterData.m_strCloudNetworkName));
#endif
#ifdef VBOX_WITH_VMNET
        m_pEditorNetworkSettings->setValueName(KNetworkAttachmentType_HostOnlyNetwork, wipedOutString(oldAdapterData.m_strHostOnlyNetworkName));
#endif

        /* Load settings: */
        m_pEditorNetworkSettings->setAdapterType(oldAdapterData.m_adapterType);
        m_pEditorNetworkSettings->setPromiscuousMode(oldAdapterData.m_promiscuousMode);
        m_pEditorNetworkSettings->setMACAddress(oldAdapterData.m_strMACAddress);
        m_pEditorNetworkSettings->setGenericProperties(oldAdapterData.m_strGenericProperties);
        m_pEditorNetworkSettings->setCableConnected(oldAdapterData.m_fCableConnected);

        /* Load port forwarding rules: */
        UIPortForwardingDataList portForwardingRules;
        for (int i = 0; i < adapterCache.childCount(); ++i)
            portForwardingRules << adapterCache.child(i).base();
        m_pEditorNetworkSettings->setPortForwardingRules(portForwardingRules);
    }

    /* Reload alternatives: */
    reloadAlternatives();
}

void UIMachineSettingsNetwork::putAdapterDataToCache(UISettingsCacheMachineNetworkAdapter &adapterCache)
{
    /* Prepare new data: */
    UIDataSettingsMachineNetworkAdapter newAdapterData;

    /* Save slot number: */
    newAdapterData.m_iSlot = m_iSlot;

    if (m_pEditorNetworkSettings)
    {
        /* Save adapter activity state: */
        newAdapterData.m_fAdapterEnabled = m_pEditorNetworkSettings->isFeatureEnabled();

        /* Save attachment type & alternative name: */
        newAdapterData.m_attachmentType = attachmentType();
        newAdapterData.m_strBridgedAdapterName = m_pEditorNetworkSettings->valueName(KNetworkAttachmentType_Bridged);
        newAdapterData.m_strInternalNetworkName = m_pEditorNetworkSettings->valueName(KNetworkAttachmentType_Internal);
        newAdapterData.m_strHostInterfaceName = m_pEditorNetworkSettings->valueName(KNetworkAttachmentType_HostOnly);
        newAdapterData.m_strGenericDriverName = m_pEditorNetworkSettings->valueName(KNetworkAttachmentType_Generic);
        newAdapterData.m_strNATNetworkName = m_pEditorNetworkSettings->valueName(KNetworkAttachmentType_NATNetwork);
#ifdef VBOX_WITH_CLOUD_NET
        newAdapterData.m_strCloudNetworkName = m_pEditorNetworkSettings->valueName(KNetworkAttachmentType_Cloud);
#endif
#ifdef VBOX_WITH_VMNET
        newAdapterData.m_strHostOnlyNetworkName = m_pEditorNetworkSettings->valueName(KNetworkAttachmentType_HostOnlyNetwork);
#endif

        /* Save settings: */
        newAdapterData.m_adapterType = m_pEditorNetworkSettings->adapterType();
        newAdapterData.m_promiscuousMode = m_pEditorNetworkSettings->promiscuousMode();
        newAdapterData.m_strMACAddress = m_pEditorNetworkSettings->macAddress();
        newAdapterData.m_strGenericProperties = m_pEditorNetworkSettings->genericProperties();
        newAdapterData.m_fCableConnected = m_pEditorNetworkSettings->cableConnected();

        /* Save port forwarding rules: */
        foreach (const UIDataPortForwardingRule &rule, m_pEditorNetworkSettings->portForwardingRules())
            adapterCache.child(rule.name).cacheCurrentData(rule);
    }

    /* Cache new data: */
    adapterCache.cacheCurrentData(newAdapterData);
}

bool UIMachineSettingsNetwork::validate(QList<UIValidationMessage> &messages)
{
    /* Pass by default: */
    bool fPass = true;

    /* Prepare message: */
    UIValidationMessage message;
    message.first = UITranslator::removeAccelMark(tabTitle());

    /* Validate enabled adapter only: */
    if (   m_pEditorNetworkSettings
        && m_pEditorNetworkSettings->isFeatureEnabled())
    {
        /* Validate alternatives: */
        switch (attachmentType())
        {
            case KNetworkAttachmentType_Bridged:
            {
                if (alternativeName().isNull())
                {
                    message.second << tr("No bridged network adapter is currently selected.");
                    fPass = false;
                }
                break;
            }
            case KNetworkAttachmentType_Internal:
            {
                if (alternativeName().isNull())
                {
                    message.second << tr("No internal network name is currently specified.");
                    fPass = false;
                }
                break;
            }
#ifndef VBOX_WITH_VMNET
            case KNetworkAttachmentType_HostOnly:
            {
                if (alternativeName().isNull())
                {
                    message.second << tr("No host-only network adapter is currently selected.");
                    fPass = false;
                }
                break;
            }
#else /* VBOX_WITH_VMNET */
            case KNetworkAttachmentType_HostOnly:
            {
                message.second << tr("Host-only adapters are no longer supported, use host-only networks instead.");
                fPass = false;
                break;
            }
#endif /* VBOX_WITH_VMNET */
            case KNetworkAttachmentType_Generic:
            {
                if (alternativeName().isNull())
                {
                    message.second << tr("No generic driver is currently selected.");
                    fPass = false;
                }
                break;
            }
            case KNetworkAttachmentType_NATNetwork:
            {
                if (alternativeName().isNull())
                {
                    message.second << tr("No NAT network name is currently specified.");
                    fPass = false;
                }
                break;
            }
#ifdef VBOX_WITH_CLOUD_NET
            case KNetworkAttachmentType_Cloud:
            {
                if (alternativeName().isNull())
                {
                    message.second << tr("No cloud network name is currently specified.");
                    fPass = false;
                }
                break;
            }
#endif /* VBOX_WITH_CLOUD_NET */
#ifdef VBOX_WITH_VMNET
            case KNetworkAttachmentType_HostOnlyNetwork:
            {
                if (alternativeName().isNull())
                {
                    message.second << tr("No host-only network name is currently specified.");
                    fPass = false;
                }
                break;
            }
#endif /* VBOX_WITH_VMNET */
            default:
                break;
        }

        /* Validate MAC-address length: */
        if (m_pEditorNetworkSettings->macAddress().size() < 12)
        {
            message.second << tr("The MAC address must be 12 hexadecimal digits long.");
            fPass = false;
        }

        /* Make sure MAC-address is unicast: */
        if (m_pEditorNetworkSettings->macAddress().size() >= 2)
        {
            if (m_pEditorNetworkSettings->macAddress().indexOf(QRegularExpression("^[0-9A-Fa-f][02468ACEace]")) != 0)
            {
                message.second << tr("The second digit in the MAC address may not be odd as only unicast addresses are allowed.");
                fPass = false;
            }
        }
    }

    /* Serialize message: */
    if (!message.second.isEmpty())
        messages << message;

    /* Return result: */
    return fPass;
}

QWidget *UIMachineSettingsNetwork::setOrderAfter(QWidget *pWidget)
{
    setTabOrder(pWidget, m_pEditorNetworkSettings);
    return m_pEditorNetworkSettings;
}

QString UIMachineSettingsNetwork::tabTitle() const
{
    return UICommon::tr("Adapter %1").arg(QString("&%1").arg(m_iSlot + 1));
}

KNetworkAttachmentType UIMachineSettingsNetwork::attachmentType() const
{
    return m_pEditorNetworkSettings ? m_pEditorNetworkSettings->valueType() : KNetworkAttachmentType_Null;
}

QString UIMachineSettingsNetwork::alternativeName(KNetworkAttachmentType enmType /* = KNetworkAttachmentType_Null */) const
{
    if (enmType == KNetworkAttachmentType_Null)
        enmType = attachmentType();
    return m_pEditorNetworkSettings ? m_pEditorNetworkSettings->valueName(enmType) : QString();
}

void UIMachineSettingsNetwork::polishTab()
{
    if (   m_pEditorNetworkSettings
        && m_pParent)
    {
        /* General stuff: */
        m_pEditorNetworkSettings->setFeatureAvailable(m_pParent->isMachineOffline());

        /* Attachment stuff: */
        m_pEditorNetworkSettings->setAttachmentOptionsAvailable(m_pParent->isMachineInValidMode());

        /* Advanced stuff: */
        m_pEditorNetworkSettings->setAdvancedOptionsAvailable(m_pParent->isMachineInValidMode());
        m_pEditorNetworkSettings->setAdapterOptionsAvailable(m_pParent->isMachineOffline());
        m_pEditorNetworkSettings->setPromiscuousOptionsAvailable(   attachmentType() != KNetworkAttachmentType_Null
                                                                 && attachmentType() != KNetworkAttachmentType_Generic
                                                                 && attachmentType() != KNetworkAttachmentType_NAT);
        m_pEditorNetworkSettings->setMACOptionsAvailable(m_pParent->isMachineOffline());
        m_pEditorNetworkSettings->setGenericPropertiesAvailable(attachmentType() == KNetworkAttachmentType_Generic);
        m_pEditorNetworkSettings->setCableOptionsAvailable(m_pParent->isMachineInValidMode());
        m_pEditorNetworkSettings->setForwardingOptionsAvailable(attachmentType() == KNetworkAttachmentType_NAT);
    }
}

void UIMachineSettingsNetwork::reloadAlternatives()
{
    if (   m_pEditorNetworkSettings
        && m_pParent)
    {
        m_pEditorNetworkSettings->setValueNames(KNetworkAttachmentType_Bridged, m_pParent->bridgedAdapterList());
        m_pEditorNetworkSettings->setValueNames(KNetworkAttachmentType_Internal, m_pParent->internalNetworkList());
        m_pEditorNetworkSettings->setValueNames(KNetworkAttachmentType_HostOnly, m_pParent->hostInterfaceList());
        m_pEditorNetworkSettings->setValueNames(KNetworkAttachmentType_Generic, m_pParent->genericDriverList());
        m_pEditorNetworkSettings->setValueNames(KNetworkAttachmentType_NATNetwork, m_pParent->natNetworkList());
#ifdef VBOX_WITH_CLOUD_NET
        m_pEditorNetworkSettings->setValueNames(KNetworkAttachmentType_Cloud, m_pParent->cloudNetworkList());
#endif
#ifdef VBOX_WITH_VMNET
        m_pEditorNetworkSettings->setValueNames(KNetworkAttachmentType_HostOnlyNetwork, m_pParent->hostOnlyNetworkList());
#endif
    }
}

void UIMachineSettingsNetwork::setAdvancedButtonExpanded(bool fExpanded)
{
    if (m_pEditorNetworkSettings)
        m_pEditorNetworkSettings->setAdvancedButtonExpanded(fExpanded);
}

void UIMachineSettingsNetwork::retranslateUi()
{
    /* Reload alternatives: */
    reloadAlternatives();
}

void UIMachineSettingsNetwork::sltHandleAlternativeNameChange()
{
    if (m_pEditorNetworkSettings)
    {
        /* Notify other adapter tabs if alternative name for certain type is changed: */
        switch (attachmentType())
        {
            case KNetworkAttachmentType_Internal:
            case KNetworkAttachmentType_Generic:
            {
                if (!m_pEditorNetworkSettings->valueName(attachmentType()).isNull())
                    emit sigAlternativeNameChanged();
                break;
            }
            default:
                break;
        }
    }

    /* Notify validity changed: */
    emit sigValidityChanged();
}

void UIMachineSettingsNetwork::prepare()
{
    /* Prepare everything: */
    prepareWidgets();
    prepareConnections();

    /* Apply language settings: */
    retranslateUi();
}

void UIMachineSettingsNetwork::prepareWidgets()
{
    /* Prepare main layout: */
    QVBoxLayout *pLayout = new QVBoxLayout(this);
    if (pLayout)
    {
        /* Prepare settings editor: */
        m_pEditorNetworkSettings = new UINetworkSettingsEditor(this);
        if (m_pEditorNetworkSettings)
            pLayout->addWidget(m_pEditorNetworkSettings);

        pLayout->addStretch();
    }
}

void UIMachineSettingsNetwork::prepareConnections()
{
    if (m_pEditorNetworkSettings)
    {
        /* Attachment connections: */
        connect(m_pEditorNetworkSettings, &UINetworkSettingsEditor::sigFeatureStateChanged,
                this, &UIMachineSettingsNetwork::sigValidityChanged);
        connect(m_pEditorNetworkSettings, &UINetworkSettingsEditor::sigAttachmentTypeChanged,
                this, &UIMachineSettingsNetwork::sigValidityChanged);
        connect(m_pEditorNetworkSettings, &UINetworkSettingsEditor::sigAlternativeNameChanged,
                this, &UIMachineSettingsNetwork::sltHandleAlternativeNameChange);

        /* Advanced connections: */
        connect(m_pEditorNetworkSettings, &UINetworkSettingsEditor::sigAdvancedButtonStateChange,
                this, &UIMachineSettingsNetwork::sigAdvancedButtonStateChange);
        connect(m_pEditorNetworkSettings, &UINetworkSettingsEditor::sigMACAddressChanged,
                this, &UIMachineSettingsNetwork::sigValidityChanged);
    }
}


/*********************************************************************************************************************************
*   Class UIMachineSettingsNetworkPage implementation.                                                                           *
*********************************************************************************************************************************/

UIMachineSettingsNetworkPage::UIMachineSettingsNetworkPage()
    : m_pCache(0)
    , m_pTabWidget(0)
{
    prepare();
}

UIMachineSettingsNetworkPage::~UIMachineSettingsNetworkPage()
{
    cleanup();
}

bool UIMachineSettingsNetworkPage::changed() const
{
    return m_pCache ? m_pCache->wasChanged() : false;
}

void UIMachineSettingsNetworkPage::loadToCacheFrom(QVariant &data)
{
    /* Sanity check: */
    if (   !m_pCache
        || !m_pTabWidget)
        return;

    /* Fetch data to machine: */
    UISettingsPageMachine::fetchData(data);

    /* Clear cache initially: */
    m_pCache->clear();

    /* Cache name lists: */
    refreshBridgedAdapterList();
    refreshInternalNetworkList(true);
    refreshHostInterfaceList();
    refreshGenericDriverList(true);
    refreshNATNetworkList();
#ifdef VBOX_WITH_CLOUD_NET
    refreshCloudNetworkList();
#endif
#ifdef VBOX_WITH_VMNET
    refreshHostOnlyNetworkList();
#endif

    /* Prepare old data: */
    UIDataSettingsMachineNetwork oldNetworkData;

    /* For each network adapter: */
    for (int iSlot = 0; iSlot < m_pTabWidget->count(); ++iSlot)
    {
        /* Prepare old data: */
        UIDataSettingsMachineNetworkAdapter oldAdapterData;

        /* Check whether adapter is valid: */
        const CNetworkAdapter &comAdapter = m_machine.GetNetworkAdapter(iSlot);
        if (!comAdapter.isNull())
        {
            /* Gather old data: */
            oldAdapterData.m_iSlot = iSlot;
            oldAdapterData.m_fAdapterEnabled = comAdapter.GetEnabled();
            oldAdapterData.m_attachmentType = comAdapter.GetAttachmentType();
            oldAdapterData.m_strBridgedAdapterName = wipedOutString(comAdapter.GetBridgedInterface());
            oldAdapterData.m_strInternalNetworkName = wipedOutString(comAdapter.GetInternalNetwork());
            oldAdapterData.m_strHostInterfaceName = wipedOutString(comAdapter.GetHostOnlyInterface());
            oldAdapterData.m_strGenericDriverName = wipedOutString(comAdapter.GetGenericDriver());
            oldAdapterData.m_strNATNetworkName = wipedOutString(comAdapter.GetNATNetwork());
#ifdef VBOX_WITH_CLOUD_NET
            oldAdapterData.m_strCloudNetworkName = wipedOutString(comAdapter.GetCloudNetwork());
#endif
#ifdef VBOX_WITH_VMNET
            oldAdapterData.m_strHostOnlyNetworkName = wipedOutString(comAdapter.GetHostOnlyNetwork());
#endif
            oldAdapterData.m_adapterType = comAdapter.GetAdapterType();
            oldAdapterData.m_promiscuousMode = comAdapter.GetPromiscModePolicy();
            oldAdapterData.m_strMACAddress = comAdapter.GetMACAddress();
            oldAdapterData.m_strGenericProperties = loadGenericProperties(comAdapter);
            oldAdapterData.m_fCableConnected = comAdapter.GetCableConnected();
            foreach (const QString &strRedirect, comAdapter.GetNATEngine().GetRedirects())
            {
                /* Gather old data & cache key: */
                const QStringList &forwardingData = strRedirect.split(',');
                AssertMsg(forwardingData.size() == 6, ("Redirect rule should be composed of 6 parts!\n"));
                const UIDataPortForwardingRule oldForwardingData(forwardingData.at(0),
                                                                 (KNATProtocol)forwardingData.at(1).toUInt(),
                                                                 forwardingData.at(2),
                                                                 forwardingData.at(3).toUInt(),
                                                                 forwardingData.at(4),
                                                                 forwardingData.at(5).toUInt());
                const QString &strForwardingKey = forwardingData.at(0);
                /* Cache old data: */
                m_pCache->child(iSlot).child(strForwardingKey).cacheInitialData(oldForwardingData);
            }
        }

        /* Cache old data: */
        m_pCache->child(iSlot).cacheInitialData(oldAdapterData);
    }

    /* Cache old data: */
    m_pCache->cacheInitialData(oldNetworkData);

    /* Upload machine to data: */
    UISettingsPageMachine::uploadData(data);
}

void UIMachineSettingsNetworkPage::getFromCache()
{
    /* Sanity check: */
    if (   !m_pCache
        || !m_pTabWidget)
        return;

    /* Setup tab order: */
    AssertPtrReturnVoid(firstWidget());
    setTabOrder(firstWidget(), m_pTabWidget->focusProxy());
    QWidget *pLastFocusWidget = m_pTabWidget->focusProxy();

    /* For each adapter: */
    for (int iSlot = 0; iSlot < m_pTabWidget->count(); ++iSlot)
    {
        /* Get adapter page: */
        UIMachineSettingsNetwork *pTab = qobject_cast<UIMachineSettingsNetwork*>(m_pTabWidget->widget(iSlot));
        AssertPtrReturnVoid(pTab);

        /* Load old data from cache: */
        pTab->getAdapterDataFromCache(m_pCache->child(iSlot));

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

void UIMachineSettingsNetworkPage::putToCache()
{
    /* Sanity check: */
    if (   !m_pCache
        || !m_pTabWidget)
        return;

    /* Prepare new data: */
    UIDataSettingsMachineNetwork newNetworkData;

    /* For each adapter: */
    for (int iSlot = 0; iSlot < m_pTabWidget->count(); ++iSlot)
    {
        /* Get adapter page: */
        UIMachineSettingsNetwork *pTab = qobject_cast<UIMachineSettingsNetwork*>(m_pTabWidget->widget(iSlot));
        AssertPtrReturnVoid(pTab);

        /* Gather new data: */
        pTab->putAdapterDataToCache(m_pCache->child(iSlot));
    }

    /* Cache new data: */
    m_pCache->cacheCurrentData(newNetworkData);
}

void UIMachineSettingsNetworkPage::saveFromCacheTo(QVariant &data)
{
    /* Fetch data to machine: */
    UISettingsPageMachine::fetchData(data);

    /* Update data and failing state: */
    setFailed(!saveData());

    /* Upload machine to data: */
    UISettingsPageMachine::uploadData(data);
}

bool UIMachineSettingsNetworkPage::validate(QList<UIValidationMessage> &messages)
{
    /* Sanity check: */
    if (!m_pTabWidget)
        return false;

    /* Pass by default: */
    bool fValid = true;

    /* Delegate validation to adapter tabs: */
    for (int iIndex = 0; iIndex < m_pTabWidget->count(); ++iIndex)
    {
        UIMachineSettingsNetwork *pTab = qobject_cast<UIMachineSettingsNetwork*>(m_pTabWidget->widget(iIndex));
        AssertPtrReturn(pTab, false);
        if (!pTab->validate(messages))
            fValid = false;
    }

    /* Return result: */
    return fValid;
}

void UIMachineSettingsNetworkPage::retranslateUi()
{
    /* Sanity check: */
    if (!m_pTabWidget)
        return;

    for (int iSlot = 0; iSlot < m_pTabWidget->count(); ++iSlot)
    {
        UIMachineSettingsNetwork *pTab = qobject_cast<UIMachineSettingsNetwork*>(m_pTabWidget->widget(iSlot));
        AssertPtrReturnVoid(pTab);
        m_pTabWidget->setTabText(iSlot, pTab->tabTitle());
    }
}

void UIMachineSettingsNetworkPage::polishPage()
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
                                     m_pCache->child(iSlot).base().m_fAdapterEnabled));
        UIMachineSettingsNetwork *pTab = qobject_cast<UIMachineSettingsNetwork*>(m_pTabWidget->widget(iSlot));
        AssertPtrReturnVoid(pTab);
        pTab->polishTab();
    }
}

void UIMachineSettingsNetworkPage::sltHandleAlternativeNameChange()
{
    /* Sanity check: */
    if (!m_pTabWidget)
        return;

    /* Determine the sender tab: */
    UIMachineSettingsNetwork *pSender = qobject_cast<UIMachineSettingsNetwork*>(sender());
    AssertPtrReturnVoid(pSender);

    /* Enumerate alternatives for certain types: */
    switch (pSender->attachmentType())
    {
        case KNetworkAttachmentType_Internal: refreshInternalNetworkList(); break;
        case KNetworkAttachmentType_Generic: refreshGenericDriverList(); break;
        default: break;
    }

    /* Update alternatives for all the tabs besides the sender: */
    for (int iSlot = 0; iSlot < m_pTabWidget->count(); ++iSlot)
    {
        /* Get the iterated tab: */
        UIMachineSettingsNetwork *pTab = qobject_cast<UIMachineSettingsNetwork*>(m_pTabWidget->widget(iSlot));
        AssertPtrReturnVoid(pTab);

        /* Update all the tabs (except sender): */
        if (pTab != pSender)
            pTab->reloadAlternatives();
    }
}

void UIMachineSettingsNetworkPage::sltHandleAdvancedButtonStateChange(bool fExpanded)
{
    /* Sanity check: */
    if (!m_pTabWidget)
        return;

    /* Update the advanced button states for all the pages: */
    for (int iSlot = 0; iSlot < m_pTabWidget->count(); ++iSlot)
    {
        UIMachineSettingsNetwork *pTab = qobject_cast<UIMachineSettingsNetwork*>(m_pTabWidget->widget(iSlot));
        AssertPtrReturnVoid(pTab);
        pTab->setAdvancedButtonExpanded(fExpanded);
    }
}

void UIMachineSettingsNetworkPage::prepare()
{
    /* Prepare cache: */
    m_pCache = new UISettingsCacheMachineNetwork;
    AssertPtrReturnVoid(m_pCache);

    /* Create main layout: */
    QVBoxLayout *pLayoutMain = new QVBoxLayout(this);
    if (pLayoutMain)
    {
        /* Creating tab-widget: */
        m_pTabWidget = new QITabWidget;
        if (m_pTabWidget)
        {
            /* How many adapters to display: */
            /** @todo r=klaus this needs to be done based on the actual chipset type of the VM,
              * but in this place the m_machine field isn't set yet. My observation (on Linux)
              * is that the limitation to 4 isn't necessary any more, but this needs to be checked
              * on all platforms to be certain that it's usable everywhere. */
            const ulong uCount = qMin((ULONG)4, uiCommon().virtualBox().GetSystemProperties().GetMaxNetworkAdapters(KChipsetType_PIIX3));

            /* Create corresponding adapter tabs: */
            for (ulong uSlot = 0; uSlot < uCount; ++uSlot)
            {
                /* Create adapter tab: */
                UIMachineSettingsNetwork *pTab = new UIMachineSettingsNetwork(this);
                if (pTab)
                {
                    /* Tab connections: */
                    connect(pTab, &UIMachineSettingsNetwork::sigAlternativeNameChanged,
                            this, &UIMachineSettingsNetworkPage::sltHandleAlternativeNameChange);
                    connect(pTab, &UIMachineSettingsNetwork::sigAdvancedButtonStateChange,
                            this, &UIMachineSettingsNetworkPage::sltHandleAdvancedButtonStateChange);
                    connect(pTab, &UIMachineSettingsNetwork::sigValidityChanged,
                            this, &UIMachineSettingsNetworkPage::revalidate);

                    /* Add tab into tab-widget: */
                    m_pTabWidget->addTab(pTab, pTab->tabTitle());
                }
            }

            /* Add tab-widget into layout: */
            pLayoutMain->addWidget(m_pTabWidget);
        }
    }
}

void UIMachineSettingsNetworkPage::cleanup()
{
    /* Cleanup cache: */
    delete m_pCache;
    m_pCache = 0;
}

void UIMachineSettingsNetworkPage::refreshBridgedAdapterList()
{
    /* Reload bridged adapters: */
    m_bridgedAdapterList = UINetworkAttachmentEditor::bridgedAdapters();
}

void UIMachineSettingsNetworkPage::refreshInternalNetworkList(bool fFullRefresh /* = false */)
{
    /* Sanity check: */
    if (!m_pTabWidget)
        return;

    /* Reload internal network list: */
    m_internalNetworkList.clear();
    /* Get internal network names from other VMs: */
    if (fFullRefresh)
        m_internalNetworkListSaved = UINetworkAttachmentEditor::internalNetworks();
    m_internalNetworkList << m_internalNetworkListSaved;
    /* Append internal network list with names from all the tabs: */
    for (int iSlot = 0; iSlot < m_pTabWidget->count(); ++iSlot)
    {
        UIMachineSettingsNetwork *pTab = qobject_cast<UIMachineSettingsNetwork*>(m_pTabWidget->widget(iSlot));
        AssertPtrReturnVoid(pTab);
        const QString strName = pTab->alternativeName(KNetworkAttachmentType_Internal);
        if (!strName.isEmpty() && !m_internalNetworkList.contains(strName))
            m_internalNetworkList << strName;
    }
}

#ifdef VBOX_WITH_CLOUD_NET
void UIMachineSettingsNetworkPage::refreshCloudNetworkList()
{
    /* Reload cloud network list: */
    m_cloudNetworkList = UINetworkAttachmentEditor::cloudNetworks();
}
#endif /* VBOX_WITH_CLOUD_NET */

#ifdef VBOX_WITH_VMNET
void UIMachineSettingsNetworkPage::refreshHostOnlyNetworkList()
{
    /* Reload host-only network list: */
    m_hostOnlyNetworkList = UINetworkAttachmentEditor::hostOnlyNetworks();
}
#endif /* VBOX_WITH_VMNET */

void UIMachineSettingsNetworkPage::refreshHostInterfaceList()
{
    /* Reload host interfaces: */
    m_hostInterfaceList = UINetworkAttachmentEditor::hostInterfaces();
}

void UIMachineSettingsNetworkPage::refreshGenericDriverList(bool fFullRefresh /* = false */)
{
    /* Sanity check: */
    if (!m_pTabWidget)
        return;

    /* Load generic driver list: */
    m_genericDriverList.clear();
    /* Get generic driver names from other VMs: */
    if (fFullRefresh)
        m_genericDriverListSaved = UINetworkAttachmentEditor::genericDrivers();
    m_genericDriverList << m_genericDriverListSaved;
    /* Append generic driver list with names from all the tabs: */
    for (int iSlot = 0; iSlot < m_pTabWidget->count(); ++iSlot)
    {
        UIMachineSettingsNetwork *pTab = qobject_cast<UIMachineSettingsNetwork*>(m_pTabWidget->widget(iSlot));
        AssertPtrReturnVoid(pTab);
        const QString strName = pTab->alternativeName(KNetworkAttachmentType_Generic);
        if (!strName.isEmpty() && !m_genericDriverList.contains(strName))
            m_genericDriverList << strName;
    }
}

void UIMachineSettingsNetworkPage::refreshNATNetworkList()
{
    /* Reload nat networks: */
    m_natNetworkList = UINetworkAttachmentEditor::natNetworks();
}

/* static */
QString UIMachineSettingsNetworkPage::loadGenericProperties(const CNetworkAdapter &adapter)
{
    /* Prepare formatted string: */
    QVector<QString> names;
    QVector<QString> props;
    props = adapter.GetProperties(QString(), names);
    QString strResult;
    /* Load generic properties: */
    for (int i = 0; i < names.size(); ++i)
    {
        strResult += names[i] + "=" + props[i];
        if (i < names.size() - 1)
          strResult += "\n";
    }
    /* Return formatted string: */
    return strResult;
}

/* static */
bool UIMachineSettingsNetworkPage::saveGenericProperties(CNetworkAdapter &comAdapter, const QString &strProperties)
{
    /* Prepare result: */
    bool fSuccess = true;
    /* Save generic properties: */
    if (fSuccess)
    {
        /* Acquire 'added' properties: */
        const QStringList newProps = strProperties.split("\n");

        /* Insert 'added' properties: */
        QHash<QString, QString> hash;
        for (int i = 0; fSuccess && i < newProps.size(); ++i)
        {
            /* Parse property line: */
            const QString strLine = newProps.at(i);
            const QString strKey = strLine.section('=', 0, 0);
            const QString strVal = strLine.section('=', 1, -1);
            if (strKey.isEmpty() || strVal.isEmpty())
                continue;
            /* Save property in the adapter and the hash: */
            comAdapter.SetProperty(strKey, strVal);
            fSuccess = comAdapter.isOk();
            hash[strKey] = strVal;
        }

        /* Acquire actual properties ('added' and 'removed'): */
        QVector<QString> names;
        QVector<QString> props;
        if (fSuccess)
        {
            props = comAdapter.GetProperties(QString(), names);
            fSuccess = comAdapter.isOk();
        }

        /* Exclude 'removed' properties: */
        for (int i = 0; fSuccess && i < names.size(); ++i)
        {
            /* Get property name and value: */
            const QString strKey = names.at(i);
            const QString strVal = props.at(i);
            if (strVal == hash.value(strKey))
                continue;
            /* Remove property from the adapter: */
            // Actually we are _replacing_ property value,
            // not _removing_ it at all, but we are replacing it
            // with default constructed value, which is QString().
            comAdapter.SetProperty(strKey, hash.value(strKey));
            fSuccess = comAdapter.isOk();
        }
    }
    /* Return result: */
    return fSuccess;
}

bool UIMachineSettingsNetworkPage::saveData()
{
    /* Sanity check: */
    if (   !m_pCache
        || !m_pTabWidget)
        return false;

    /* Prepare result: */
    bool fSuccess = true;
    /* Save network settings from cache: */
    if (fSuccess && isMachineInValidMode() && m_pCache->wasChanged())
    {
        /* For each adapter: */
        for (int iSlot = 0; fSuccess && iSlot < m_pTabWidget->count(); ++iSlot)
            fSuccess = saveAdapterData(iSlot);
    }
    /* Return result: */
    return fSuccess;
}

bool UIMachineSettingsNetworkPage::saveAdapterData(int iSlot)
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
        const UIDataSettingsMachineNetworkAdapter &oldAdapterData = m_pCache->child(iSlot).base();
        /* Get new data from cache: */
        const UIDataSettingsMachineNetworkAdapter &newAdapterData = m_pCache->child(iSlot).data();

        /* Get network adapter for further activities: */
        CNetworkAdapter comAdapter = m_machine.GetNetworkAdapter(iSlot);
        fSuccess = m_machine.isOk() && comAdapter.isNotNull();

        /* Show error message if necessary: */
        if (!fSuccess)
            notifyOperationProgressError(UIErrorString::formatErrorInfo(m_machine));
        else
        {
            /* Save whether the adapter is enabled: */
            if (fSuccess && isMachineOffline() && newAdapterData.m_fAdapterEnabled != oldAdapterData.m_fAdapterEnabled)
            {
                comAdapter.SetEnabled(newAdapterData.m_fAdapterEnabled);
                fSuccess = comAdapter.isOk();
            }
            /* Save adapter type: */
            if (fSuccess && isMachineOffline() && newAdapterData.m_adapterType != oldAdapterData.m_adapterType)
            {
                comAdapter.SetAdapterType(newAdapterData.m_adapterType);
                fSuccess = comAdapter.isOk();
            }
            /* Save adapter MAC address: */
            if (fSuccess && isMachineOffline() && newAdapterData.m_strMACAddress != oldAdapterData.m_strMACAddress)
            {
                comAdapter.SetMACAddress(newAdapterData.m_strMACAddress);
                fSuccess = comAdapter.isOk();
            }
            /* Save adapter attachment type: */
            switch (newAdapterData.m_attachmentType)
            {
                case KNetworkAttachmentType_Bridged:
                {
                    if (fSuccess && newAdapterData.m_strBridgedAdapterName != oldAdapterData.m_strBridgedAdapterName)
                    {
                        comAdapter.SetBridgedInterface(newAdapterData.m_strBridgedAdapterName);
                        fSuccess = comAdapter.isOk();
                    }
                    break;
                }
                case KNetworkAttachmentType_Internal:
                {
                    if (fSuccess && newAdapterData.m_strInternalNetworkName != oldAdapterData.m_strInternalNetworkName)
                    {
                        comAdapter.SetInternalNetwork(newAdapterData.m_strInternalNetworkName);
                        fSuccess = comAdapter.isOk();
                    }
                    break;
                }
                case KNetworkAttachmentType_HostOnly:
                {
                    if (fSuccess && newAdapterData.m_strHostInterfaceName != oldAdapterData.m_strHostInterfaceName)
                    {
                        comAdapter.SetHostOnlyInterface(newAdapterData.m_strHostInterfaceName);
                        fSuccess = comAdapter.isOk();
                    }
                    break;
                }
                case KNetworkAttachmentType_Generic:
                {
                    if (fSuccess && newAdapterData.m_strGenericDriverName != oldAdapterData.m_strGenericDriverName)
                    {
                        comAdapter.SetGenericDriver(newAdapterData.m_strGenericDriverName);
                        fSuccess = comAdapter.isOk();
                    }
                    if (fSuccess && newAdapterData.m_strGenericProperties != oldAdapterData.m_strGenericProperties)
                        fSuccess = saveGenericProperties(comAdapter, newAdapterData.m_strGenericProperties);
                    break;
                }
                case KNetworkAttachmentType_NATNetwork:
                {
                    if (fSuccess && newAdapterData.m_strNATNetworkName != oldAdapterData.m_strNATNetworkName)
                    {
                        comAdapter.SetNATNetwork(newAdapterData.m_strNATNetworkName);
                        fSuccess = comAdapter.isOk();
                    }
                    break;
                }
#ifdef VBOX_WITH_CLOUD_NET
                case KNetworkAttachmentType_Cloud:
                {
                    if (fSuccess && newAdapterData.m_strCloudNetworkName != oldAdapterData.m_strCloudNetworkName)
                    {
                        comAdapter.SetCloudNetwork(newAdapterData.m_strCloudNetworkName);
                        fSuccess = comAdapter.isOk();
                    }
                    break;
                }
#endif /* VBOX_WITH_CLOUD_NET */
#ifdef VBOX_WITH_VMNET
                case KNetworkAttachmentType_HostOnlyNetwork:
                {
                    if (fSuccess && newAdapterData.m_strHostOnlyNetworkName != oldAdapterData.m_strHostOnlyNetworkName)
                    {
                        comAdapter.SetHostOnlyNetwork(newAdapterData.m_strHostOnlyNetworkName);
                        fSuccess = comAdapter.isOk();
                    }
                    break;
                }
#endif /* VBOX_WITH_VMNET */
                default:
                    break;
            }
            if (fSuccess && newAdapterData.m_attachmentType != oldAdapterData.m_attachmentType)
            {
                comAdapter.SetAttachmentType(newAdapterData.m_attachmentType);
                fSuccess = comAdapter.isOk();
            }
            /* Save adapter promiscuous mode: */
            if (fSuccess && newAdapterData.m_promiscuousMode != oldAdapterData.m_promiscuousMode)
            {
                comAdapter.SetPromiscModePolicy(newAdapterData.m_promiscuousMode);
                fSuccess = comAdapter.isOk();
            }
            /* Save whether the adapter cable connected: */
            if (fSuccess && newAdapterData.m_fCableConnected != oldAdapterData.m_fCableConnected)
            {
                comAdapter.SetCableConnected(newAdapterData.m_fCableConnected);
                fSuccess = comAdapter.isOk();
            }

            /* Get NAT engine for further activities: */
            CNATEngine comEngine;
            if (fSuccess)
            {
                comEngine = comAdapter.GetNATEngine();
                fSuccess = comAdapter.isOk() && comEngine.isNotNull();
            }

            /* Show error message if necessary: */
            if (!fSuccess)
                notifyOperationProgressError(UIErrorString::formatErrorInfo(comAdapter));
            else
            {
                /* Save adapter port forwarding rules: */
                if (   oldAdapterData.m_attachmentType == KNetworkAttachmentType_NAT
                    || newAdapterData.m_attachmentType == KNetworkAttachmentType_NAT)
                {
                    /* For each rule: */
                    for (int iRule = 0; fSuccess && iRule < m_pCache->child(iSlot).childCount(); ++iRule)
                    {
                        /* Get rule cache: */
                        const UISettingsCachePortForwardingRule &ruleCache = m_pCache->child(iSlot).child(iRule);

                        /* Remove rule marked for 'remove' or 'update': */
                        if (ruleCache.wasRemoved() || ruleCache.wasUpdated())
                        {
                            comEngine.RemoveRedirect(ruleCache.base().name);
                            fSuccess = comEngine.isOk();
                        }
                    }
                    for (int iRule = 0; fSuccess && iRule < m_pCache->child(iSlot).childCount(); ++iRule)
                    {
                        /* Get rule cache: */
                        const UISettingsCachePortForwardingRule &ruleCache = m_pCache->child(iSlot).child(iRule);

                        /* Create rule marked for 'create' or 'update': */
                        if (ruleCache.wasCreated() || ruleCache.wasUpdated())
                        {
                            comEngine.AddRedirect(ruleCache.data().name, ruleCache.data().protocol,
                                                  ruleCache.data().hostIp, ruleCache.data().hostPort.value(),
                                                  ruleCache.data().guestIp, ruleCache.data().guestPort.value());
                            fSuccess = comEngine.isOk();
                        }
                    }

                    /* Show error message if necessary: */
                    if (!fSuccess)
                        notifyOperationProgressError(UIErrorString::formatErrorInfo(comEngine));
                }
            }
        }
    }
    /* Return result: */
    return fSuccess;
}

# include "UIMachineSettingsNetwork.moc"

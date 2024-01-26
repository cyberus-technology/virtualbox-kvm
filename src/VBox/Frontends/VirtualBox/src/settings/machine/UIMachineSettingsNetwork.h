/* $Id: UIMachineSettingsNetwork.h $ */
/** @file
 * VBox Qt GUI - UIMachineSettingsNetwork class declaration.
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

#ifndef FEQT_INCLUDED_SRC_settings_machine_UIMachineSettingsNetwork_h
#define FEQT_INCLUDED_SRC_settings_machine_UIMachineSettingsNetwork_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* GUI includes: */
#include "UISettingsPage.h"
#include "UIMachineSettingsPortForwardingDlg.h"

/* Forward declarations: */
class QITabWidget;
struct UIDataSettingsMachineNetwork;
struct UIDataSettingsMachineNetworkAdapter;
typedef UISettingsCache<UIDataPortForwardingRule> UISettingsCachePortForwardingRule;
typedef UISettingsCachePool<UIDataSettingsMachineNetworkAdapter, UISettingsCachePortForwardingRule> UISettingsCacheMachineNetworkAdapter;
typedef UISettingsCachePool<UIDataSettingsMachineNetwork, UISettingsCacheMachineNetworkAdapter> UISettingsCacheMachineNetwork;

/** Machine settings: Network page. */
class SHARED_LIBRARY_STUFF UIMachineSettingsNetworkPage : public UISettingsPageMachine
{
    Q_OBJECT;

public:

    /** Constructs Network settings page. */
    UIMachineSettingsNetworkPage();
    /** Destructs Network settings page. */
    virtual ~UIMachineSettingsNetworkPage() RT_OVERRIDE;

    /** Returns the bridged adapter list. */
    const QStringList &bridgedAdapterList() const { return m_bridgedAdapterList; }
    /** Returns the internal network list. */
    const QStringList &internalNetworkList() const { return m_internalNetworkList; }
    /** Returns the host-only interface list. */
    const QStringList &hostInterfaceList() const { return m_hostInterfaceList; }
    /** Returns the generic driver list. */
    const QStringList &genericDriverList() const { return m_genericDriverList; }
    /** Returns the NAT network list. */
    const QStringList &natNetworkList() const { return m_natNetworkList; }
#ifdef VBOX_WITH_CLOUD_NET
    /** Returns the cloud network list. */
    const QStringList &cloudNetworkList() const { return m_cloudNetworkList; }
#endif
#ifdef VBOX_WITH_VMNET
    /** Returns the host-only network list. */
    const QStringList &hostOnlyNetworkList() const { return m_hostOnlyNetworkList; }
#endif

protected:

    /** Returns whether the page content was changed. */
    virtual bool changed() const RT_OVERRIDE;

    /** Loads settings from external object(s) packed inside @a data to cache.
      * @note  This task WILL be performed in other than the GUI thread, no widget interactions! */
    virtual void loadToCacheFrom(QVariant &data) RT_OVERRIDE;
    /** Loads data from cache to corresponding widgets.
      * @note  This task WILL be performed in the GUI thread only, all widget interactions here! */
    virtual void getFromCache() RT_OVERRIDE;

    /** Saves data from corresponding widgets to cache.
      * @note  This task WILL be performed in the GUI thread only, all widget interactions here! */
    virtual void putToCache() RT_OVERRIDE;
    /** Saves settings from cache to external object(s) packed inside @a data.
      * @note  This task WILL be performed in other than the GUI thread, no widget interactions! */
    virtual void saveFromCacheTo(QVariant &data) RT_OVERRIDE;

    /** Performs validation, updates @a messages list if something is wrong. */
    virtual bool validate(QList<UIValidationMessage> &messages) RT_OVERRIDE;

    /** Handles translation event. */
    virtual void retranslateUi() RT_OVERRIDE;

    /** Performs final page polishing. */
    virtual void polishPage() RT_OVERRIDE;

private slots:

    /** Handles adapter alternative name change. */
    void sltHandleAlternativeNameChange();
    /** Handles whether the advanced button is @a fExpanded. */
    void sltHandleAdvancedButtonStateChange(bool fExpanded);

private:

    /** Prepares all. */
    void prepare();
    /** Cleanups all. */
    void cleanup();

    /** Repopulates bridged adapter list. */
    void refreshBridgedAdapterList();
    /** Repopulates internal network list. */
    void refreshInternalNetworkList(bool fFullRefresh = false);
    /** Repopulates host-only interface list. */
    void refreshHostInterfaceList();
    /** Repopulates generic driver list. */
    void refreshGenericDriverList(bool fFullRefresh = false);
    /** Repopulates NAT network list. */
    void refreshNATNetworkList();
#ifdef VBOX_WITH_CLOUD_NET
    /** Repopulates cloud network list. */
    void refreshCloudNetworkList();
#endif
#ifdef VBOX_WITH_VMNET
    /** Repopulates host-only network list. */
    void refreshHostOnlyNetworkList();
#endif

    /** Loads generic properties from passed @a adapter. */
    static QString loadGenericProperties(const CNetworkAdapter &adapter);
    /** Saves generic @a strProperties to passed @a adapter. */
    static bool saveGenericProperties(CNetworkAdapter &comAdapter, const QString &strProperties);

    /** Saves existing data from cache. */
    bool saveData();
    /** Saves existing adapter data from cache. */
    bool saveAdapterData(int iSlot);

    /** Holds the bridged adapter list. */
    QStringList  m_bridgedAdapterList;
    /** Holds the internal network list. */
    QStringList  m_internalNetworkList;
    /** Holds the saved internal network list. */
    QStringList  m_internalNetworkListSaved;
    /** Holds the host-only interface list. */
    QStringList  m_hostInterfaceList;
    /** Holds the generic driver list. */
    QStringList  m_genericDriverList;
    /** Holds the saved generic driver list. */
    QStringList  m_genericDriverListSaved;
    /** Holds the NAT network list. */
    QStringList  m_natNetworkList;
#ifdef VBOX_WITH_CLOUD_NET
    /** Holds the cloud network list. */
    QStringList  m_cloudNetworkList;
#endif
#ifdef VBOX_WITH_VMNET
    /** Holds the host-only network list. */
    QStringList  m_hostOnlyNetworkList;
#endif

    /** Holds the page data cache instance. */
    UISettingsCacheMachineNetwork *m_pCache;

    /** Holds the tab-widget instance. */
    QITabWidget *m_pTabWidget;
};

#endif /* !FEQT_INCLUDED_SRC_settings_machine_UIMachineSettingsNetwork_h */

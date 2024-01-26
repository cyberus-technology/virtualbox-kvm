/* $Id: UIMachineSettingsDisplay.cpp $ */
/** @file
 * VBox Qt GUI - UIMachineSettingsDisplay class implementation.
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
#include <QFileInfo>
#include <QVBoxLayout>

/* GUI includes: */
#include "QITabWidget.h"
#include "UICommon.h"
#include "UIConverter.h"
#include "UIErrorString.h"
#include "UIExtraDataManager.h"
#include "UIGraphicsControllerEditor.h"
#ifdef VBOX_WITH_3D_ACCELERATION
# include "UIDisplayScreenFeaturesEditor.h"
#endif
#include "UIMachineSettingsDisplay.h"
#include "UIMonitorCountEditor.h"
#include "UIRecordingSettingsEditor.h"
#include "UIScaleFactorEditor.h"
#include "UITranslator.h"
#include "UIVideoMemoryEditor.h"
#include "UIVRDESettingsEditor.h"

/* COM includes: */
#include "CExtPackManager.h"
#include "CGraphicsAdapter.h"
#include "CRecordingScreenSettings.h"
#include "CRecordingSettings.h"
#include "CVRDEServer.h"


/** Machine settings: Display page data structure. */
struct UIDataSettingsMachineDisplay
{
    /** Constructs data. */
    UIDataSettingsMachineDisplay()
        : m_iCurrentVRAM(0)
        , m_cGuestScreenCount(0)
        , m_graphicsControllerType(KGraphicsControllerType_Null)
#ifdef VBOX_WITH_3D_ACCELERATION
        , m_f3dAccelerationEnabled(false)
#endif
        , m_fRemoteDisplayServerSupported(false)
        , m_fRemoteDisplayServerEnabled(false)
        , m_strRemoteDisplayPort(QString())
        , m_remoteDisplayAuthType(KAuthType_Null)
        , m_uRemoteDisplayTimeout(0)
        , m_fRemoteDisplayMultiConnAllowed(false)
        , m_fRecordingEnabled(false)
        , m_strRecordingFolder(QString())
        , m_strRecordingFilePath(QString())
        , m_iRecordingVideoFrameWidth(0)
        , m_iRecordingVideoFrameHeight(0)
        , m_iRecordingVideoFrameRate(0)
        , m_iRecordingVideoBitRate(0)
        , m_strRecordingVideoOptions(QString())
    {}

    /** Returns whether the @a other passed data is equal to this one. */
    bool equal(const UIDataSettingsMachineDisplay &other) const
    {
        return true
               && (m_iCurrentVRAM == other.m_iCurrentVRAM)
               && (m_cGuestScreenCount == other.m_cGuestScreenCount)
               && (m_scaleFactors == other.m_scaleFactors)
               && (m_graphicsControllerType == other.m_graphicsControllerType)
#ifdef VBOX_WITH_3D_ACCELERATION
               && (m_f3dAccelerationEnabled == other.m_f3dAccelerationEnabled)
#endif
               && (m_fRemoteDisplayServerSupported == other.m_fRemoteDisplayServerSupported)
               && (m_fRemoteDisplayServerEnabled == other.m_fRemoteDisplayServerEnabled)
               && (m_strRemoteDisplayPort == other.m_strRemoteDisplayPort)
               && (m_remoteDisplayAuthType == other.m_remoteDisplayAuthType)
               && (m_uRemoteDisplayTimeout == other.m_uRemoteDisplayTimeout)
               && (m_fRemoteDisplayMultiConnAllowed == other.m_fRemoteDisplayMultiConnAllowed)
               && (m_fRecordingEnabled == other.m_fRecordingEnabled)
               && (m_strRecordingFilePath == other.m_strRecordingFilePath)
               && (m_iRecordingVideoFrameWidth == other.m_iRecordingVideoFrameWidth)
               && (m_iRecordingVideoFrameHeight == other.m_iRecordingVideoFrameHeight)
               && (m_iRecordingVideoFrameRate == other.m_iRecordingVideoFrameRate)
               && (m_iRecordingVideoBitRate == other.m_iRecordingVideoBitRate)
               && (m_vecRecordingScreens == other.m_vecRecordingScreens)
               && (m_strRecordingVideoOptions == other.m_strRecordingVideoOptions)
               ;
    }

    /** Returns whether the @a other passed data is equal to this one. */
    bool operator==(const UIDataSettingsMachineDisplay &other) const { return equal(other); }
    /** Returns whether the @a other passed data is different from this one. */
    bool operator!=(const UIDataSettingsMachineDisplay &other) const { return !equal(other); }

    /** Recording options. */
    enum RecordingOption
    {
        RecordingOption_Unknown,
        RecordingOption_AC,
        RecordingOption_VC,
        RecordingOption_AC_Profile
    };

    /** Returns enum value corresponding to passed @a strKey. */
    static RecordingOption toRecordingOptionKey(const QString &strKey)
    {
        /* Compare case-sensitive: */
        QMap<QString, RecordingOption> keys;
        keys["ac_enabled"] = RecordingOption_AC;
        keys["vc_enabled"] = RecordingOption_VC;
        keys["ac_profile"] = RecordingOption_AC_Profile;
        /* Return known value or RecordingOption_Unknown otherwise: */
        return keys.value(strKey, RecordingOption_Unknown);
    }

    /** Returns string representation for passed enum @a enmKey. */
    static QString fromRecordingOptionKey(RecordingOption enmKey)
    {
        /* Compare case-sensitive: */
        QMap<RecordingOption, QString> values;
        values[RecordingOption_AC] = "ac_enabled";
        values[RecordingOption_VC] = "vc_enabled";
        values[RecordingOption_AC_Profile] = "ac_profile";
        /* Return known value or QString() otherwise: */
        return values.value(enmKey);
    }

    /** Parses recording options. */
    static void parseRecordingOptions(const QString &strOptions,
                                      QList<RecordingOption> &outKeys,
                                      QStringList &outValues)
    {
        outKeys.clear();
        outValues.clear();
        const QStringList aPairs = strOptions.split(',');
        foreach (const QString &strPair, aPairs)
        {
            const QStringList aPair = strPair.split('=');
            if (aPair.size() != 2)
                continue;
            const RecordingOption enmKey = toRecordingOptionKey(aPair.value(0));
            if (enmKey == RecordingOption_Unknown)
                continue;
            outKeys << enmKey;
            outValues << aPair.value(1);
        }
    }

    /** Serializes recording options. */
    static void serializeRecordingOptions(const QList<RecordingOption> &inKeys,
                                          const QStringList &inValues,
                                          QString &strOptions)
    {
        QStringList aPairs;
        for (int i = 0; i < inKeys.size(); ++i)
        {
            QStringList aPair;
            aPair << fromRecordingOptionKey(inKeys.value(i));
            aPair << inValues.value(i);
            aPairs << aPair.join('=');
        }
        strOptions = aPairs.join(',');
    }

    /** Returns whether passed Recording @a enmOption is enabled. */
    static bool isRecordingOptionEnabled(const QString &strOptions,
                                         RecordingOption enmOption)
    {
        QList<RecordingOption> aKeys;
        QStringList aValues;
        parseRecordingOptions(strOptions, aKeys, aValues);
        int iIndex = aKeys.indexOf(enmOption);
        if (iIndex == -1)
            return false; /* If option is missing, assume disabled (false). */
        if (aValues.value(iIndex).compare("true", Qt::CaseInsensitive) == 0)
            return true;
        return false;
    }

    /** Searches for ac_profile and return 1 for "low", 2 for "med", and 3 for "high". Returns 2
        if ac_profile is missing */
    static int getAudioQualityFromOptions(const QString &strOptions)
    {
        QList<RecordingOption> aKeys;
        QStringList aValues;
        parseRecordingOptions(strOptions, aKeys, aValues);
        int iIndex = aKeys.indexOf(RecordingOption_AC_Profile);
        if (iIndex == -1)
            return 2;
        if (aValues.value(iIndex).compare("low", Qt::CaseInsensitive) == 0)
            return 1;
        if (aValues.value(iIndex).compare("high", Qt::CaseInsensitive) == 0)
            return 3;
        return 2;
    }

    /** Sets the video recording options for @a enmOptions to @a values. */
    static QString setRecordingOptions(const QString &strOptions,
                                       const QVector<RecordingOption> &enmOptions,
                                       const QStringList &values)
    {
        if (enmOptions.size() != values.size())
            return QString();
        QList<RecordingOption> aKeys;
        QStringList aValues;
        parseRecordingOptions(strOptions, aKeys, aValues);
        for(int i = 0; i < values.size(); ++i)
        {
            QString strValue = values[i];
            int iIndex = aKeys.indexOf(enmOptions[i]);
            if (iIndex == -1)
            {
                aKeys << enmOptions[i];
                aValues << strValue;
            }
            else
            {
                aValues[iIndex] = strValue;
            }
        }
        QString strResult;
        serializeRecordingOptions(aKeys, aValues, strResult);
        return strResult;
    }

    /** Holds the video RAM amount. */
    int                      m_iCurrentVRAM;
    /** Holds the guest screen count. */
    int                      m_cGuestScreenCount;
    /** Holds the guest screen scale-factor. */
    QList<double>            m_scaleFactors;
    /** Holds the graphics controller type. */
    KGraphicsControllerType  m_graphicsControllerType;
#ifdef VBOX_WITH_3D_ACCELERATION
    /** Holds whether the 3D acceleration is enabled. */
    bool                     m_f3dAccelerationEnabled;
#endif
    /** Holds whether the remote display server is supported. */
    bool                     m_fRemoteDisplayServerSupported;
    /** Holds whether the remote display server is enabled. */
    bool                     m_fRemoteDisplayServerEnabled;
    /** Holds the remote display server port. */
    QString                  m_strRemoteDisplayPort;
    /** Holds the remote display server auth type. */
    KAuthType                m_remoteDisplayAuthType;
    /** Holds the remote display server timeout. */
    ulong                    m_uRemoteDisplayTimeout;
    /** Holds whether the remote display server allows multiple connections. */
    bool                     m_fRemoteDisplayMultiConnAllowed;

    /** Holds whether recording is enabled. */
    bool m_fRecordingEnabled;
    /** Holds the recording folder. */
    QString m_strRecordingFolder;
    /** Holds the recording file path. */
    QString m_strRecordingFilePath;
    /** Holds the recording frame width. */
    int m_iRecordingVideoFrameWidth;
    /** Holds the recording frame height. */
    int m_iRecordingVideoFrameHeight;
    /** Holds the recording frame rate. */
    int m_iRecordingVideoFrameRate;
    /** Holds the recording bit rate. */
    int m_iRecordingVideoBitRate;
    /** Holds which of the guest screens should be recorded. */
    QVector<BOOL> m_vecRecordingScreens;
    /** Holds the video recording options. */
    QString m_strRecordingVideoOptions;
};


UIMachineSettingsDisplay::UIMachineSettingsDisplay()
    : m_comGuestOSType(CGuestOSType())
#ifdef VBOX_WITH_3D_ACCELERATION
    , m_fWddmModeSupported(false)
#endif
    , m_enmGraphicsControllerTypeRecommended(KGraphicsControllerType_Null)
    , m_pCache(0)
    , m_pTabWidget(0)
    , m_pTabScreen(0)
    , m_pEditorVideoMemorySize(0)
    , m_pEditorMonitorCount(0)
    , m_pEditorScaleFactor(0)
    , m_pEditorGraphicsController(0)
#ifdef VBOX_WITH_3D_ACCELERATION
    , m_pEditorDisplayScreenFeatures(0)
#endif
    , m_pTabRemoteDisplay(0)
    , m_pEditorVRDESettings(0)
    , m_pTabRecording(0)
    , m_pEditorRecordingSettings(0)
{
    prepare();
}

UIMachineSettingsDisplay::~UIMachineSettingsDisplay()
{
    cleanup();
}

void UIMachineSettingsDisplay::setGuestOSType(CGuestOSType comGuestOSType)
{
    /* Check if guest OS type changed: */
    if (m_comGuestOSType == comGuestOSType)
        return;

    /* Remember new guest OS type: */
    m_comGuestOSType = comGuestOSType;
    m_pEditorVideoMemorySize->setGuestOSType(m_comGuestOSType);

#ifdef VBOX_WITH_3D_ACCELERATION
    /* Check if WDDM mode supported by the guest OS type: */
    const QString strGuestOSTypeId = m_comGuestOSType.isNotNull() ? m_comGuestOSType.GetId() : QString();
    m_fWddmModeSupported = UICommon::isWddmCompatibleOsType(strGuestOSTypeId);
    m_pEditorVideoMemorySize->set3DAccelerationSupported(m_fWddmModeSupported);
#endif /* VBOX_WITH_3D_ACCELERATION */
    /* Acquire recommended graphics controller type: */
    m_enmGraphicsControllerTypeRecommended = m_comGuestOSType.GetRecommendedGraphicsController();

    /* Revalidate: */
    revalidate();
}

#ifdef VBOX_WITH_3D_ACCELERATION
bool UIMachineSettingsDisplay::isAcceleration3DSelected() const
{
    return m_pEditorDisplayScreenFeatures->isEnabled3DAcceleration();
}
#endif /* VBOX_WITH_3D_ACCELERATION */

KGraphicsControllerType UIMachineSettingsDisplay::graphicsControllerTypeRecommended() const
{
    return   m_pEditorGraphicsController->supportedValues().contains(m_enmGraphicsControllerTypeRecommended)
           ? m_enmGraphicsControllerTypeRecommended
           : graphicsControllerTypeCurrent();
}

KGraphicsControllerType UIMachineSettingsDisplay::graphicsControllerTypeCurrent() const
{
    return m_pEditorGraphicsController->value();
}

bool UIMachineSettingsDisplay::changed() const
{
    return m_pCache ? m_pCache->wasChanged() : false;
}

void UIMachineSettingsDisplay::loadToCacheFrom(QVariant &data)
{
    /* Sanity check: */
    if (!m_pCache)
        return;

    /* Fetch data to machine: */
    UISettingsPageMachine::fetchData(data);

    /* Clear cache initially: */
    m_pCache->clear();

    /* Prepare old data: */
    UIDataSettingsMachineDisplay oldDisplayData;

    /* Check whether graphics adapter is valid: */
    const CGraphicsAdapter &comGraphics = m_machine.GetGraphicsAdapter();
    if (!comGraphics.isNull())
    {
        /* Gather old 'Screen' data: */
        oldDisplayData.m_iCurrentVRAM = comGraphics.GetVRAMSize();
        oldDisplayData.m_cGuestScreenCount = comGraphics.GetMonitorCount();
        oldDisplayData.m_scaleFactors = gEDataManager->scaleFactors(m_machine.GetId());
        oldDisplayData.m_graphicsControllerType = comGraphics.GetGraphicsControllerType();
#ifdef VBOX_WITH_3D_ACCELERATION
        oldDisplayData.m_f3dAccelerationEnabled = comGraphics.GetAccelerate3DEnabled();
#endif
    }

    /* Check whether remote display server is valid: */
    const CVRDEServer &vrdeServer = m_machine.GetVRDEServer();
    oldDisplayData.m_fRemoteDisplayServerSupported = !vrdeServer.isNull();
    if (!vrdeServer.isNull())
    {
        /* Gather old 'Remote Display' data: */
        oldDisplayData.m_fRemoteDisplayServerEnabled = vrdeServer.GetEnabled();
        oldDisplayData.m_strRemoteDisplayPort = vrdeServer.GetVRDEProperty("TCP/Ports");
        oldDisplayData.m_remoteDisplayAuthType = vrdeServer.GetAuthType();
        oldDisplayData.m_uRemoteDisplayTimeout = vrdeServer.GetAuthTimeout();
        oldDisplayData.m_fRemoteDisplayMultiConnAllowed = vrdeServer.GetAllowMultiConnection();
    }

    /* Gather old 'Recording' data: */
    CRecordingSettings recordingSettings = m_machine.GetRecordingSettings();
    Assert(recordingSettings.isNotNull());
    oldDisplayData.m_fRecordingEnabled = recordingSettings.GetEnabled();

    /* For now we're using the same settings for all screens; so get settings from screen 0 and work with that. */
    /** @todo r=andy Since VBox 7.0 (settings 1.19) the per-screen settings can be handled. i.e. screens can have
     *               different settings.  See @bugref{10259} */
    CRecordingScreenSettings comRecordingScreen0Settings = recordingSettings.GetScreenSettings(0);
    if (!comRecordingScreen0Settings.isNull())
    {
        oldDisplayData.m_strRecordingFolder = QFileInfo(m_machine.GetSettingsFilePath()).absolutePath();
        oldDisplayData.m_strRecordingFilePath = comRecordingScreen0Settings.GetFilename();
        oldDisplayData.m_iRecordingVideoFrameWidth = comRecordingScreen0Settings.GetVideoWidth();
        oldDisplayData.m_iRecordingVideoFrameHeight = comRecordingScreen0Settings.GetVideoHeight();
        oldDisplayData.m_iRecordingVideoFrameRate = comRecordingScreen0Settings.GetVideoFPS();
        oldDisplayData.m_iRecordingVideoBitRate = comRecordingScreen0Settings.GetVideoRate();
        oldDisplayData.m_strRecordingVideoOptions = comRecordingScreen0Settings.GetOptions();
    }

    CRecordingScreenSettingsVector comRecordingScreenSettingsVector = recordingSettings.GetScreens();
    oldDisplayData.m_vecRecordingScreens.resize(comRecordingScreenSettingsVector.size());
    for (int iScreenIndex = 0; iScreenIndex < comRecordingScreenSettingsVector.size(); ++iScreenIndex)
    {
        CRecordingScreenSettings comRecordingScreenSettings = comRecordingScreenSettingsVector.at(iScreenIndex);
        if (!comRecordingScreenSettings.isNull())
            oldDisplayData.m_vecRecordingScreens[iScreenIndex] = comRecordingScreenSettings.GetEnabled();
    }

    /* Cache old data: */
    m_pCache->cacheInitialData(oldDisplayData);

    /* Upload machine to data: */
    UISettingsPageMachine::uploadData(data);
}

void UIMachineSettingsDisplay::getFromCache()
{
    /* Sanity check: */
    if (!m_pCache)
        return;

    /* Get old data from cache: */
    const UIDataSettingsMachineDisplay &oldDisplayData = m_pCache->base();

    /* Load old 'Screen' data from cache: */
    if (m_pEditorMonitorCount)
        m_pEditorMonitorCount->setValue(oldDisplayData.m_cGuestScreenCount);
    if (m_pEditorScaleFactor)
    {
        m_pEditorScaleFactor->setScaleFactors(oldDisplayData.m_scaleFactors);
        m_pEditorScaleFactor->setMonitorCount(oldDisplayData.m_cGuestScreenCount);
    }
    if (m_pEditorGraphicsController)
        m_pEditorGraphicsController->setValue(oldDisplayData.m_graphicsControllerType);
#ifdef VBOX_WITH_3D_ACCELERATION
    if (m_pEditorDisplayScreenFeatures)
        m_pEditorDisplayScreenFeatures->setEnable3DAcceleration(oldDisplayData.m_f3dAccelerationEnabled);
#endif
    /* Push required value to m_pEditorVideoMemorySize: */
    sltHandleMonitorCountChange();
    sltHandleGraphicsControllerComboChange();
#ifdef VBOX_WITH_3D_ACCELERATION
    sltHandle3DAccelerationFeatureStateChange();
#endif
    // Should be the last one for this tab, since it depends on some of others:
    if (m_pEditorVideoMemorySize)
        m_pEditorVideoMemorySize->setValue(oldDisplayData.m_iCurrentVRAM);

    /* If remote display server is supported: */
    if (oldDisplayData.m_fRemoteDisplayServerSupported)
    {
        /* Load old 'Remote Display' data from cache: */
        if (m_pEditorVRDESettings)
        {
            m_pEditorVRDESettings->setFeatureEnabled(oldDisplayData.m_fRemoteDisplayServerEnabled);
            m_pEditorVRDESettings->setPort(oldDisplayData.m_strRemoteDisplayPort);
            m_pEditorVRDESettings->setAuthType(oldDisplayData.m_remoteDisplayAuthType);
            m_pEditorVRDESettings->setTimeout(QString::number(oldDisplayData.m_uRemoteDisplayTimeout));
            m_pEditorVRDESettings->setMultipleConnectionsAllowed(oldDisplayData.m_fRemoteDisplayMultiConnAllowed);
        }
    }

    if (m_pEditorRecordingSettings)
    {
        /* Load old 'Recording' data from cache: */
        m_pEditorRecordingSettings->setFeatureEnabled(oldDisplayData.m_fRecordingEnabled);
        m_pEditorRecordingSettings->setFolder(oldDisplayData.m_strRecordingFolder);
        m_pEditorRecordingSettings->setFilePath(oldDisplayData.m_strRecordingFilePath);
        m_pEditorRecordingSettings->setFrameWidth(oldDisplayData.m_iRecordingVideoFrameWidth);
        m_pEditorRecordingSettings->setFrameHeight(oldDisplayData.m_iRecordingVideoFrameHeight);
        m_pEditorRecordingSettings->setFrameRate(oldDisplayData.m_iRecordingVideoFrameRate);
        m_pEditorRecordingSettings->setBitRate(oldDisplayData.m_iRecordingVideoBitRate);
        m_pEditorRecordingSettings->setScreens(oldDisplayData.m_vecRecordingScreens);

        /* Load old 'Recording' options: */
        const bool fRecordVideo =
            UIDataSettingsMachineDisplay::isRecordingOptionEnabled(oldDisplayData.m_strRecordingVideoOptions,
                                                                   UIDataSettingsMachineDisplay::RecordingOption_VC);
        const bool fRecordAudio =
            UIDataSettingsMachineDisplay::isRecordingOptionEnabled(oldDisplayData.m_strRecordingVideoOptions,
                                                                   UIDataSettingsMachineDisplay::RecordingOption_AC);
        UISettingsDefs::RecordingMode enmMode;
        if (fRecordAudio && fRecordVideo)
            enmMode = UISettingsDefs::RecordingMode_VideoAudio;
        else if (fRecordAudio && !fRecordVideo)
            enmMode = UISettingsDefs::RecordingMode_AudioOnly;
        else
            enmMode = UISettingsDefs::RecordingMode_VideoOnly;
        m_pEditorRecordingSettings->setMode(enmMode);
        const int iAudioQualityRate =
            UIDataSettingsMachineDisplay::getAudioQualityFromOptions(oldDisplayData.m_strRecordingVideoOptions);
        m_pEditorRecordingSettings->setAudioQualityRate(iAudioQualityRate);
    }

    /* Polish page finally: */
    polishPage();

    /* Revalidate: */
    revalidate();
}

void UIMachineSettingsDisplay::putToCache()
{
    /* Sanity check: */
    if (!m_pCache)
        return;

    /* Prepare new data: */
    UIDataSettingsMachineDisplay newDisplayData;

    /* Gather new 'Screen' data: */
    if (m_pEditorVideoMemorySize)
        newDisplayData.m_iCurrentVRAM = m_pEditorVideoMemorySize->value();
    if (m_pEditorMonitorCount)
        newDisplayData.m_cGuestScreenCount = m_pEditorMonitorCount->value();
    if (m_pEditorScaleFactor)
        newDisplayData.m_scaleFactors = m_pEditorScaleFactor->scaleFactors();
    if (m_pEditorGraphicsController)
        newDisplayData.m_graphicsControllerType = m_pEditorGraphicsController->value();
#ifdef VBOX_WITH_3D_ACCELERATION
    if (m_pEditorDisplayScreenFeatures)
        newDisplayData.m_f3dAccelerationEnabled = m_pEditorDisplayScreenFeatures->isEnabled3DAcceleration();
#endif

    /* If remote display server is supported: */
    newDisplayData.m_fRemoteDisplayServerSupported = m_pCache->base().m_fRemoteDisplayServerSupported;
    if (   newDisplayData.m_fRemoteDisplayServerSupported
        && m_pEditorVRDESettings)
    {
        /* Gather new 'Remote Display' data: */
        newDisplayData.m_fRemoteDisplayServerEnabled = m_pEditorVRDESettings->isFeatureEnabled();
        newDisplayData.m_strRemoteDisplayPort = m_pEditorVRDESettings->port();
        newDisplayData.m_remoteDisplayAuthType = m_pEditorVRDESettings->authType();
        newDisplayData.m_uRemoteDisplayTimeout = m_pEditorVRDESettings->timeout().toULong();
        newDisplayData.m_fRemoteDisplayMultiConnAllowed = m_pEditorVRDESettings->isMultipleConnectionsAllowed();
    }

    if (m_pEditorRecordingSettings)
    {
        /* Gather new 'Recording' data: */
        newDisplayData.m_fRecordingEnabled = m_pEditorRecordingSettings->isFeatureEnabled();
        newDisplayData.m_strRecordingFolder = m_pEditorRecordingSettings->folder();
        newDisplayData.m_strRecordingFilePath = m_pEditorRecordingSettings->filePath();
        newDisplayData.m_iRecordingVideoFrameWidth = m_pEditorRecordingSettings->frameWidth();
        newDisplayData.m_iRecordingVideoFrameHeight = m_pEditorRecordingSettings->frameHeight();
        newDisplayData.m_iRecordingVideoFrameRate = m_pEditorRecordingSettings->frameRate();
        newDisplayData.m_iRecordingVideoBitRate = m_pEditorRecordingSettings->bitRate();
        newDisplayData.m_vecRecordingScreens = m_pEditorRecordingSettings->screens();

        /* Gather new 'Recording' options: */
        const UISettingsDefs::RecordingMode enmRecordingMode = m_pEditorRecordingSettings->mode();
        QStringList optionValues;
        optionValues.append(     (enmRecordingMode == UISettingsDefs::RecordingMode_VideoAudio)
                              || (enmRecordingMode == UISettingsDefs::RecordingMode_VideoOnly)
                            ? "true" : "false");
        optionValues.append(     (enmRecordingMode == UISettingsDefs::RecordingMode_VideoAudio)
                              || (enmRecordingMode == UISettingsDefs::RecordingMode_AudioOnly)
                            ? "true" : "false");
        switch (m_pEditorRecordingSettings->audioQualityRate())
        {
            case 1: optionValues.append("low"); break;
            case 2: optionValues.append("med"); break;
            default: optionValues.append("high"); break;
        }
        QVector<UIDataSettingsMachineDisplay::RecordingOption> optionKeys;
        optionKeys.append(UIDataSettingsMachineDisplay::RecordingOption_VC);
        optionKeys.append(UIDataSettingsMachineDisplay::RecordingOption_AC);
        optionKeys.append(UIDataSettingsMachineDisplay::RecordingOption_AC_Profile);
        newDisplayData.m_strRecordingVideoOptions =
            UIDataSettingsMachineDisplay::setRecordingOptions(m_pCache->base().m_strRecordingVideoOptions,
                                                              optionKeys, optionValues);
    }

    /* Cache new data: */
    m_pCache->cacheCurrentData(newDisplayData);
}

void UIMachineSettingsDisplay::saveFromCacheTo(QVariant &data)
{
    /* Fetch data to machine: */
    UISettingsPageMachine::fetchData(data);

    /* Update data and failing state: */
    setFailed(!saveData());

    /* Upload machine to data: */
    UISettingsPageMachine::uploadData(data);
}

bool UIMachineSettingsDisplay::validate(QList<UIValidationMessage> &messages)
{
    /* Pass by default: */
    bool fPass = true;

    /* Screen tab: */
    {
        /* Prepare message: */
        UIValidationMessage message;
        message.first = UITranslator::removeAccelMark(m_pTabWidget->tabText(0));

        /* Video RAM amount test: */
        if (shouldWeWarnAboutLowVRAM() && !m_comGuestOSType.isNull())
        {
            quint64 uNeedBytes = UICommon::requiredVideoMemory(m_comGuestOSType.GetId(), m_pEditorMonitorCount->value());

            /* Basic video RAM amount test: */
            if ((quint64)m_pEditorVideoMemorySize->value() * _1M < uNeedBytes)
            {
                message.second << tr("The virtual machine is currently assigned less than <b>%1</b> of video memory "
                                     "which is the minimum amount required to switch to full-screen or seamless mode.")
                                     .arg(UITranslator::formatSize(uNeedBytes, 0, FormatSize_RoundUp));
            }
#ifdef VBOX_WITH_3D_ACCELERATION
            /* 3D acceleration video RAM amount test: */
            else if (m_pEditorDisplayScreenFeatures->isEnabled3DAcceleration() && m_fWddmModeSupported)
            {
                uNeedBytes = qMax(uNeedBytes, (quint64) 128 * _1M);
                if ((quint64)m_pEditorVideoMemorySize->value() * _1M < uNeedBytes)
                {
                    message.second << tr("The virtual machine is set up to use hardware graphics acceleration "
                                         "and the operating system hint is set to Windows Vista or later. "
                                         "For best performance you should set the machine's video memory to at least <b>%1</b>.")
                                         .arg(UITranslator::formatSize(uNeedBytes, 0, FormatSize_RoundUp));
                }
            }
#endif /* VBOX_WITH_3D_ACCELERATION */
        }

        /* Graphics controller type test: */
        if (!m_comGuestOSType.isNull())
        {
            if (graphicsControllerTypeCurrent() != graphicsControllerTypeRecommended())
            {
#ifdef VBOX_WITH_3D_ACCELERATION
                if (m_pEditorDisplayScreenFeatures->isEnabled3DAcceleration())
                    message.second << tr("The virtual machine is configured to use 3D acceleration. This will work only if you "
                                         "pick a different graphics controller (%1). Either disable 3D acceleration or switch "
                                         "to required graphics controller type. The latter will be done automatically if you "
                                         "confirm your changes.")
                                         .arg(gpConverter->toString(m_enmGraphicsControllerTypeRecommended));
                else
#endif /* VBOX_WITH_3D_ACCELERATION */
                    message.second << tr("The virtual machine is configured to use a graphics controller other than the "
                                         "recommended one (%1). Please consider switching unless you have a reason to keep the "
                                         "currently selected graphics controller.")
                                         .arg(gpConverter->toString(m_enmGraphicsControllerTypeRecommended));
            }
        }

        /* Serialize message: */
        if (!message.second.isEmpty())
            messages << message;
    }

    /* Remote Display tab: */
    {
        /* Prepare message: */
        UIValidationMessage message;
        message.first = UITranslator::removeAccelMark(m_pTabWidget->tabText(1));

        /* Extension Pack presence test: */
        if (m_pEditorVRDESettings->isFeatureEnabled())
        {
            CExtPackManager extPackManager = uiCommon().virtualBox().GetExtensionPackManager();
            if (!extPackManager.isNull() && !extPackManager.IsExtPackUsable(GUI_ExtPackName))
            {
                message.second << tr("Remote Display is currently enabled for this virtual machine. "
                                     "However, this requires the <i>%1</i> to be installed. "
                                     "Please install the Extension Pack from the VirtualBox download site as "
                                     "otherwise your VM will be started with Remote Display disabled.")
                                     .arg(GUI_ExtPackName);
            }
        }

        /* Check VRDE server port: */
        if (m_pEditorVRDESettings->port().trimmed().isEmpty())
        {
            message.second << tr("The VRDE server port value is not currently specified.");
            fPass = false;
        }

        /* Check VRDE server timeout: */
        if (m_pEditorVRDESettings->timeout().trimmed().isEmpty())
        {
            message.second << tr("The VRDE authentication timeout value is not currently specified.");
            fPass = false;
        }

        /* Serialize message: */
        if (!message.second.isEmpty())
            messages << message;
    }

    /* Return result: */
    return fPass;
}

void UIMachineSettingsDisplay::setOrderAfter(QWidget *pWidget)
{
    /* Screen tab-order: */
    setTabOrder(pWidget, m_pTabWidget->focusProxy());
    setTabOrder(m_pTabWidget->focusProxy(), m_pEditorVideoMemorySize);
    setTabOrder(m_pEditorVideoMemorySize, m_pEditorMonitorCount);
    setTabOrder(m_pEditorMonitorCount, m_pEditorScaleFactor);
    setTabOrder(m_pEditorScaleFactor, m_pEditorGraphicsController);
#ifdef VBOX_WITH_3D_ACCELERATION
    setTabOrder(m_pEditorGraphicsController, m_pEditorDisplayScreenFeatures);
    setTabOrder(m_pEditorDisplayScreenFeatures, m_pEditorVRDESettings);
#else
    setTabOrder(m_pEditorGraphicsController, m_pEditorVRDESettings);
#endif

    /* Remote Display tab-order: */
    setTabOrder(m_pEditorVRDESettings, m_pEditorRecordingSettings);
}

void UIMachineSettingsDisplay::retranslateUi()
{
    /* Translate tab-widget: */
    m_pTabWidget->setTabText(m_pTabWidget->indexOf(m_pTabScreen), tr("&Screen"));
    m_pTabWidget->setTabText(m_pTabWidget->indexOf(m_pTabRemoteDisplay), tr("&Remote Display"));
    m_pTabWidget->setTabText(m_pTabWidget->indexOf(m_pTabRecording), tr("Re&cording"));

    /* These editors have own labels, but we want them to be properly layouted according to each other: */
    int iMinimumLayoutHint = 0;
    iMinimumLayoutHint = qMax(iMinimumLayoutHint, m_pEditorVideoMemorySize->minimumLabelHorizontalHint());
    iMinimumLayoutHint = qMax(iMinimumLayoutHint, m_pEditorMonitorCount->minimumLabelHorizontalHint());
    iMinimumLayoutHint = qMax(iMinimumLayoutHint, m_pEditorScaleFactor->minimumLabelHorizontalHint());
    iMinimumLayoutHint = qMax(iMinimumLayoutHint, m_pEditorGraphicsController->minimumLabelHorizontalHint());
#ifdef VBOX_WITH_3D_ACCELERATION
    iMinimumLayoutHint = qMax(iMinimumLayoutHint, m_pEditorDisplayScreenFeatures->minimumLabelHorizontalHint());
#endif
    m_pEditorVideoMemorySize->setMinimumLayoutIndent(iMinimumLayoutHint);
    m_pEditorMonitorCount->setMinimumLayoutIndent(iMinimumLayoutHint);
    m_pEditorScaleFactor->setMinimumLayoutIndent(iMinimumLayoutHint);
    m_pEditorGraphicsController->setMinimumLayoutIndent(iMinimumLayoutHint);
#ifdef VBOX_WITH_3D_ACCELERATION
    m_pEditorDisplayScreenFeatures->setMinimumLayoutIndent(iMinimumLayoutHint);
#endif
}

void UIMachineSettingsDisplay::polishPage()
{
    /* Get old data from cache: */
    const UIDataSettingsMachineDisplay &oldDisplayData = m_pCache->base();

    /* Polish 'Screen' availability: */
    m_pEditorVideoMemorySize->setEnabled(isMachineOffline());
    m_pEditorMonitorCount->setEnabled(isMachineOffline());
    m_pEditorScaleFactor->setEnabled(isMachineInValidMode());
    m_pEditorGraphicsController->setEnabled(isMachineOffline());
#ifdef VBOX_WITH_3D_ACCELERATION
    m_pEditorDisplayScreenFeatures->setEnabled(isMachineOffline());
#endif

    /* Polish 'Remote Display' availability: */
    m_pTabWidget->setTabEnabled(1, oldDisplayData.m_fRemoteDisplayServerSupported);
    m_pTabRemoteDisplay->setEnabled(isMachineInValidMode());
    m_pEditorVRDESettings->setVRDEOptionsAvailable(isMachineOffline() || isMachineSaved());

#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    /* Polish 'Recording' visibility: */
    m_pTabWidget->setTabVisible(m_pTabWidget->indexOf(m_pTabRecording), uiCommon().supportedRecordingFeatures());
    /* Polish 'Recording' availability: */
    m_pTabRecording->setEnabled(isMachineInValidMode());
#else
    /* Polish 'Recording' availability: */
    m_pTabWidget->setTabEnabled(2, isMachineInValidMode() && uiCommon().supportedRecordingFeatures());
    m_pTabRecording->setEnabled(isMachineInValidMode() && uiCommon().supportedRecordingFeatures());
#endif
    // Recording options should be enabled only if:
    // 1. Machine is in 'offline' or 'saved' state,
    // 2. Machine is in 'online' state and video recording is *disabled* currently.
    const bool fIsRecordingOptionsEnabled =
           ((isMachineOffline() || isMachineSaved()))
        || (isMachineOnline() && !m_pCache->base().m_fRecordingEnabled);
    m_pEditorRecordingSettings->setOptionsAvailable(fIsRecordingOptionsEnabled);
    // Recording screens option should be enabled only if:
    // 1. Machine is in *any* valid state.
    const bool fIsRecordingScreenOptionsEnabled =
        isMachineInValidMode();
    m_pEditorRecordingSettings->setScreenOptionsAvailable(fIsRecordingScreenOptionsEnabled);
}

void UIMachineSettingsDisplay::sltHandleMonitorCountChange()
{
    /* Update recording tab screen count: */
    updateGuestScreenCount();

    /* Revalidate: */
    revalidate();
}

void UIMachineSettingsDisplay::sltHandleGraphicsControllerComboChange()
{
    /* Update Video RAM requirements: */
    m_pEditorVideoMemorySize->setGraphicsControllerType(m_pEditorGraphicsController->value());

    /* Revalidate: */
    revalidate();
}

#ifdef VBOX_WITH_3D_ACCELERATION
void UIMachineSettingsDisplay::sltHandle3DAccelerationFeatureStateChange()
{
    /* Update Video RAM requirements: */
    m_pEditorVideoMemorySize->set3DAccelerationEnabled(m_pEditorDisplayScreenFeatures->isEnabled3DAcceleration());

    /* Revalidate: */
    revalidate();
}
#endif /* VBOX_WITH_3D_ACCELERATION */

void UIMachineSettingsDisplay::prepare()
{
    /* Prepare cache: */
    m_pCache = new UISettingsCacheMachineDisplay;
    AssertPtrReturnVoid(m_pCache);

    /* Prepare everything: */
    prepareWidgets();
    prepareConnections();

    /* Apply language settings: */
    retranslateUi();
}

void UIMachineSettingsDisplay::prepareWidgets()
{
    /* Prepare main layout: */
    QVBoxLayout *pLayout = new QVBoxLayout(this);
    if (pLayout)
    {
        /* Prepare tab-widget: */
        m_pTabWidget = new QITabWidget(this);
        if (m_pTabWidget)
        {
            /* Prepare each tab separately: */
            prepareTabScreen();
            prepareTabRemoteDisplay();
            prepareTabRecording();

            pLayout->addWidget(m_pTabWidget);
        }
    }
}

void UIMachineSettingsDisplay::prepareTabScreen()
{
    /* Prepare 'Screen' tab: */
    m_pTabScreen = new QWidget;
    if (m_pTabScreen)
    {
        /* Prepare 'Screen' tab layout: */
        QVBoxLayout *pLayoutScreen = new QVBoxLayout(m_pTabScreen);
        if (pLayoutScreen)
        {
            /* Prepare video memory editor: */
            m_pEditorVideoMemorySize = new UIVideoMemoryEditor(m_pTabScreen);
            if (m_pEditorVideoMemorySize)
                pLayoutScreen->addWidget(m_pEditorVideoMemorySize);

            /* Prepare monitor count editor: */
            m_pEditorMonitorCount = new UIMonitorCountEditor(m_pTabScreen);
            if (m_pEditorMonitorCount)
                pLayoutScreen->addWidget(m_pEditorMonitorCount);

            /* Prepare scale factor editor: */
            m_pEditorScaleFactor = new UIScaleFactorEditor(m_pTabScreen);
            if (m_pEditorScaleFactor)
                pLayoutScreen->addWidget(m_pEditorScaleFactor);

            /* Prepare graphics controller editor: */
            m_pEditorGraphicsController = new UIGraphicsControllerEditor(m_pTabScreen);
            if (m_pEditorGraphicsController)
                pLayoutScreen->addWidget(m_pEditorGraphicsController);

#ifdef VBOX_WITH_3D_ACCELERATION
            /* Prepare display screen features editor: */
            m_pEditorDisplayScreenFeatures = new UIDisplayScreenFeaturesEditor(m_pTabScreen);
            if (m_pEditorDisplayScreenFeatures)
                pLayoutScreen->addWidget(m_pEditorDisplayScreenFeatures);
#endif /* VBOX_WITH_3D_ACCELERATION */

            pLayoutScreen->addStretch();
        }

        m_pTabWidget->addTab(m_pTabScreen, QString());
    }
}

void UIMachineSettingsDisplay::prepareTabRemoteDisplay()
{
    /* Prepare 'Remote Display' tab: */
    m_pTabRemoteDisplay = new QWidget;
    if (m_pTabRemoteDisplay)
    {
        /* Prepare 'Remote Display' tab layout: */
        QVBoxLayout *pLayoutRemoteDisplay = new QVBoxLayout(m_pTabRemoteDisplay);
        if (pLayoutRemoteDisplay)
        {
            /* Prepare remote display settings editor: */
            m_pEditorVRDESettings = new UIVRDESettingsEditor(m_pTabRemoteDisplay);
            if (m_pEditorVRDESettings)
                pLayoutRemoteDisplay->addWidget(m_pEditorVRDESettings);

            pLayoutRemoteDisplay->addStretch();
        }

        m_pTabWidget->addTab(m_pTabRemoteDisplay, QString());
    }
}

void UIMachineSettingsDisplay::prepareTabRecording()
{
    /* Prepare 'Recording' tab: */
    m_pTabRecording = new QWidget;
    if (m_pTabRecording)
    {
        /* Prepare 'Recording' tab layout: */
        QVBoxLayout *pLayoutRecording = new QVBoxLayout(m_pTabRecording);
        if (pLayoutRecording)
        {
            /* Prepare recording editor: */
            m_pEditorRecordingSettings = new UIRecordingSettingsEditor(m_pTabRecording);
            if (m_pEditorRecordingSettings)
                pLayoutRecording->addWidget(m_pEditorRecordingSettings);

            pLayoutRecording->addStretch();
        }

        m_pTabWidget->addTab(m_pTabRecording, QString());
    }
}

void UIMachineSettingsDisplay::prepareConnections()
{
    /* Configure 'Screen' connections: */
    connect(m_pEditorVideoMemorySize, &UIVideoMemoryEditor::sigValidChanged,
            this, &UIMachineSettingsDisplay::revalidate);
    connect(m_pEditorMonitorCount, &UIMonitorCountEditor::sigValidChanged,
            this, &UIMachineSettingsDisplay::sltHandleMonitorCountChange);
    connect(m_pEditorGraphicsController, &UIGraphicsControllerEditor::sigValueChanged,
            this, &UIMachineSettingsDisplay::sltHandleGraphicsControllerComboChange);
#ifdef VBOX_WITH_3D_ACCELERATION
    connect(m_pEditorDisplayScreenFeatures, &UIDisplayScreenFeaturesEditor::sig3DAccelerationFeatureStatusChange,
            this, &UIMachineSettingsDisplay::sltHandle3DAccelerationFeatureStateChange);
#endif

    /* Configure 'Remote Display' connections: */
    connect(m_pEditorVRDESettings, &UIVRDESettingsEditor::sigChanged,
            this, &UIMachineSettingsDisplay::revalidate);
}

void UIMachineSettingsDisplay::cleanup()
{
    /* Cleanup cache: */
    delete m_pCache;
    m_pCache = 0;
}

bool UIMachineSettingsDisplay::shouldWeWarnAboutLowVRAM()
{
    bool fResult = true;

    QStringList excludingOSList = QStringList()
        << "Other" << "DOS" << "Netware" << "L4" << "QNX" << "JRockitVE";
    if (m_comGuestOSType.isNull() || excludingOSList.contains(m_comGuestOSType.GetId()))
        fResult = false;

    return fResult;
}

void UIMachineSettingsDisplay::updateGuestScreenCount()
{
    /* Update copy of the cached item to get the desired result: */
    QVector<BOOL> screens = m_pCache->base().m_vecRecordingScreens;
    screens.resize(m_pEditorMonitorCount->value());
    m_pEditorRecordingSettings->setScreens(screens);
    m_pEditorScaleFactor->setMonitorCount(m_pEditorMonitorCount->value());
}

bool UIMachineSettingsDisplay::saveData()
{
    /* Sanity check: */
    if (!m_pCache)
        return false;

    /* Prepare result: */
    bool fSuccess = true;
    /* Save display settings from cache: */
    if (fSuccess && isMachineInValidMode() && m_pCache->wasChanged())
    {
        /* Save 'Screen' data from cache: */
        if (fSuccess)
            fSuccess = saveScreenData();
        /* Save 'Remote Display' data from cache: */
        if (fSuccess)
            fSuccess = saveRemoteDisplayData();
        /* Save 'Video Capture' data from cache: */
        if (fSuccess)
            fSuccess = saveRecordingData();
    }
    /* Return result: */
    return fSuccess;
}

bool UIMachineSettingsDisplay::saveScreenData()
{
    /* Sanity check: */
    if (!m_pCache)
        return false;

    /* Prepare result: */
    bool fSuccess = true;
    /* Save 'Screen' data from cache: */
    if (fSuccess)
    {
        /* Get old data from cache: */
        const UIDataSettingsMachineDisplay &oldDisplayData = m_pCache->base();
        /* Get new data from cache: */
        const UIDataSettingsMachineDisplay &newDisplayData = m_pCache->data();

        /* Get graphics adapter for further activities: */
        CGraphicsAdapter comGraphics = m_machine.GetGraphicsAdapter();
        fSuccess = m_machine.isOk() && comGraphics.isNotNull();

        /* Show error message if necessary: */
        if (!fSuccess)
            notifyOperationProgressError(UIErrorString::formatErrorInfo(m_machine));
        else
        {
            /* Save video RAM size: */
            if (fSuccess && isMachineOffline() && newDisplayData.m_iCurrentVRAM != oldDisplayData.m_iCurrentVRAM)
            {
                comGraphics.SetVRAMSize(newDisplayData.m_iCurrentVRAM);
                fSuccess = comGraphics.isOk();
            }
            /* Save guest screen count: */
            if (fSuccess && isMachineOffline() && newDisplayData.m_cGuestScreenCount != oldDisplayData.m_cGuestScreenCount)
            {
                comGraphics.SetMonitorCount(newDisplayData.m_cGuestScreenCount);
                fSuccess = comGraphics.isOk();
            }
            /* Save the Graphics Controller Type: */
            if (fSuccess && isMachineOffline() && newDisplayData.m_graphicsControllerType != oldDisplayData.m_graphicsControllerType)
            {
                comGraphics.SetGraphicsControllerType(newDisplayData.m_graphicsControllerType);
                fSuccess = comGraphics.isOk();
            }
#ifdef VBOX_WITH_3D_ACCELERATION
            /* Save whether 3D acceleration is enabled: */
            if (fSuccess && isMachineOffline() && newDisplayData.m_f3dAccelerationEnabled != oldDisplayData.m_f3dAccelerationEnabled)
            {
                comGraphics.SetAccelerate3DEnabled(newDisplayData.m_f3dAccelerationEnabled);
                fSuccess = comGraphics.isOk();
            }
#endif

            /* Get machine ID for further activities: */
            QUuid uMachineId;
            if (fSuccess)
            {
                uMachineId = m_machine.GetId();
                fSuccess = m_machine.isOk();
            }

            /* Show error message if necessary: */
            if (!fSuccess)
                notifyOperationProgressError(UIErrorString::formatErrorInfo(m_machine));

            /* Save guest-screen scale-factor: */
            if (fSuccess && newDisplayData.m_scaleFactors != oldDisplayData.m_scaleFactors)
                /* fSuccess = */ gEDataManager->setScaleFactors(newDisplayData.m_scaleFactors, uMachineId);
        }
    }
    /* Return result: */
    return fSuccess;
}

bool UIMachineSettingsDisplay::saveRemoteDisplayData()
{
    /* Sanity check: */
    if (!m_pCache)
        return false;

    /* Prepare result: */
    bool fSuccess = true;
    /* Save 'Remote Display' data from cache: */
    if (fSuccess)
    {
        /* Get old data from cache: */
        const UIDataSettingsMachineDisplay &oldDisplayData = m_pCache->base();
        /* Get new data from cache: */
        const UIDataSettingsMachineDisplay &newDisplayData = m_pCache->data();

        /* Get remote display server for further activities: */
        CVRDEServer comServer = m_machine.GetVRDEServer();
        fSuccess = m_machine.isOk() && comServer.isNotNull();

        /* Show error message if necessary: */
        if (!fSuccess)
            notifyOperationProgressError(UIErrorString::formatErrorInfo(m_machine));
        else
        {
            /* Save whether remote display server is enabled: */
            if (fSuccess && newDisplayData.m_fRemoteDisplayServerEnabled != oldDisplayData.m_fRemoteDisplayServerEnabled)
            {
                comServer.SetEnabled(newDisplayData.m_fRemoteDisplayServerEnabled);
                fSuccess = comServer.isOk();
            }
            /* Save remote display server port: */
            if (fSuccess && newDisplayData.m_strRemoteDisplayPort != oldDisplayData.m_strRemoteDisplayPort)
            {
                comServer.SetVRDEProperty("TCP/Ports", newDisplayData.m_strRemoteDisplayPort);
                fSuccess = comServer.isOk();
            }
            /* Save remote display server auth type: */
            if (fSuccess && newDisplayData.m_remoteDisplayAuthType != oldDisplayData.m_remoteDisplayAuthType)
            {
                comServer.SetAuthType(newDisplayData.m_remoteDisplayAuthType);
                fSuccess = comServer.isOk();
            }
            /* Save remote display server timeout: */
            if (fSuccess && newDisplayData.m_uRemoteDisplayTimeout != oldDisplayData.m_uRemoteDisplayTimeout)
            {
                comServer.SetAuthTimeout(newDisplayData.m_uRemoteDisplayTimeout);
                fSuccess = comServer.isOk();
            }
            /* Save whether remote display server allows multiple connections: */
            if (   fSuccess
                && (isMachineOffline() || isMachineSaved())
                && (newDisplayData.m_fRemoteDisplayMultiConnAllowed != oldDisplayData.m_fRemoteDisplayMultiConnAllowed))
            {
                comServer.SetAllowMultiConnection(newDisplayData.m_fRemoteDisplayMultiConnAllowed);
                fSuccess = comServer.isOk();
            }

            /* Show error message if necessary: */
            if (!fSuccess)
                notifyOperationProgressError(UIErrorString::formatErrorInfo(comServer));
        }
    }
    /* Return result: */
    return fSuccess;
}

bool UIMachineSettingsDisplay::saveRecordingData()
{
    /* Sanity check: */
    if (!m_pCache)
        return false;

    /* Prepare result: */
    bool fSuccess = true;

    /* Get old data from cache: */
    const UIDataSettingsMachineDisplay &oldDisplayData = m_pCache->base();
    /* Get new data from cache: */
    const UIDataSettingsMachineDisplay &newDisplayData = m_pCache->data();

    CRecordingSettings recordingSettings = m_machine.GetRecordingSettings();
    Assert(recordingSettings.isNotNull());

    /** @todo r=andy Make the code below more compact -- too much redundancy here. */

    /* Save new 'Recording' data for online case: */
    if (isMachineOnline())
    {
        /* If 'Recording' was *enabled*: */
        if (oldDisplayData.m_fRecordingEnabled)
        {
            /* Save whether recording is enabled: */
            if (fSuccess && newDisplayData.m_fRecordingEnabled != oldDisplayData.m_fRecordingEnabled)
            {
                recordingSettings.SetEnabled(newDisplayData.m_fRecordingEnabled);
                fSuccess = recordingSettings.isOk();
            }

            // We can still save the *screens* option.
            /* Save recording screens: */
            if (fSuccess)
            {
                CRecordingScreenSettingsVector comRecordingScreenSettingsVector = recordingSettings.GetScreens();
                for (int iScreenIndex = 0; fSuccess && iScreenIndex < comRecordingScreenSettingsVector.size(); ++iScreenIndex)
                {
                    if (newDisplayData.m_vecRecordingScreens[iScreenIndex] == oldDisplayData.m_vecRecordingScreens[iScreenIndex])
                        continue;

                    CRecordingScreenSettings comRecordingScreenSettings = comRecordingScreenSettingsVector.at(iScreenIndex);
                    comRecordingScreenSettings.SetEnabled(newDisplayData.m_vecRecordingScreens[iScreenIndex]);
                    fSuccess = comRecordingScreenSettings.isOk();
                }
            }
        }
        /* If 'Recording' was *disabled*: */
        else
        {
            CRecordingScreenSettingsVector comRecordingScreenSettingsVector = recordingSettings.GetScreens();
            for (int iScreenIndex = 0; fSuccess && iScreenIndex < comRecordingScreenSettingsVector.size(); ++iScreenIndex)
            {
                CRecordingScreenSettings comRecordingScreenSettings = comRecordingScreenSettingsVector.at(iScreenIndex);

                // We should save all the options *before* 'Recording' activation.
                // And finally we should *enable* Recording if necessary.
                /* Save recording file path: */
                if (fSuccess && newDisplayData.m_strRecordingFilePath != oldDisplayData.m_strRecordingFilePath)
                {
                    comRecordingScreenSettings.SetFilename(newDisplayData.m_strRecordingFilePath);
                    fSuccess = comRecordingScreenSettings.isOk();
                }
                /* Save recording frame width: */
                if (fSuccess && newDisplayData.m_iRecordingVideoFrameWidth != oldDisplayData.m_iRecordingVideoFrameWidth)
                {
                    comRecordingScreenSettings.SetVideoWidth(newDisplayData.m_iRecordingVideoFrameWidth);
                    fSuccess = comRecordingScreenSettings.isOk();
                }
                /* Save recording frame height: */
                if (fSuccess && newDisplayData.m_iRecordingVideoFrameHeight != oldDisplayData.m_iRecordingVideoFrameHeight)
                {
                    comRecordingScreenSettings.SetVideoHeight(newDisplayData.m_iRecordingVideoFrameHeight);
                    fSuccess = comRecordingScreenSettings.isOk();
                }
                /* Save recording frame rate: */
                if (fSuccess && newDisplayData.m_iRecordingVideoFrameRate != oldDisplayData.m_iRecordingVideoFrameRate)
                {
                    comRecordingScreenSettings.SetVideoFPS(newDisplayData.m_iRecordingVideoFrameRate);
                    fSuccess = comRecordingScreenSettings.isOk();
                }
                /* Save recording frame bit rate: */
                if (fSuccess && newDisplayData.m_iRecordingVideoBitRate != oldDisplayData.m_iRecordingVideoBitRate)
                {
                    comRecordingScreenSettings.SetVideoRate(newDisplayData.m_iRecordingVideoBitRate);
                    fSuccess = comRecordingScreenSettings.isOk();
                }
                /* Save recording options: */
                if (fSuccess && newDisplayData.m_strRecordingVideoOptions != oldDisplayData.m_strRecordingVideoOptions)
                {
                    comRecordingScreenSettings.SetOptions(newDisplayData.m_strRecordingVideoOptions);
                    fSuccess = comRecordingScreenSettings.isOk();
                }
                /* Finally, save the screen's recording state: */
                /* Note: Must come last, as modifying options with an enabled recording state is not possible. */
                if (fSuccess && newDisplayData.m_vecRecordingScreens != oldDisplayData.m_vecRecordingScreens)
                {
                    comRecordingScreenSettings.SetEnabled(newDisplayData.m_vecRecordingScreens[iScreenIndex]);
                    fSuccess = comRecordingScreenSettings.isOk();
                }

                if (!fSuccess)
                {
                    if (!comRecordingScreenSettings.isOk())
                        notifyOperationProgressError(UIErrorString::formatErrorInfo(comRecordingScreenSettings));
                    break; /* No point trying to handle the other screens (if any). */
                }
            }

            /* Save whether recording is enabled:
             * Do this last, as after enabling recording no changes via API aren't allowed anymore. */
            if (fSuccess && newDisplayData.m_fRecordingEnabled != oldDisplayData.m_fRecordingEnabled)
            {
                recordingSettings.SetEnabled(newDisplayData.m_fRecordingEnabled);
                fSuccess = recordingSettings.isOk();
            }
        }
    }
    /* Save new 'Recording' data for offline case: */
    else
    {
        CRecordingScreenSettingsVector comRecordingScreenSettingsVector = recordingSettings.GetScreens();
        for (int iScreenIndex = 0; fSuccess && iScreenIndex < comRecordingScreenSettingsVector.size(); ++iScreenIndex)
        {
            CRecordingScreenSettings comRecordingScreenSettings = comRecordingScreenSettingsVector.at(iScreenIndex);

            /* Save recording file path: */
            if (fSuccess && newDisplayData.m_strRecordingFilePath != oldDisplayData.m_strRecordingFilePath)
            {
                comRecordingScreenSettings.SetFilename(newDisplayData.m_strRecordingFilePath);
                fSuccess = comRecordingScreenSettings.isOk();
            }
            /* Save recording frame width: */
            if (fSuccess && newDisplayData.m_iRecordingVideoFrameWidth != oldDisplayData.m_iRecordingVideoFrameWidth)
            {
                comRecordingScreenSettings.SetVideoWidth(newDisplayData.m_iRecordingVideoFrameWidth);
                fSuccess = comRecordingScreenSettings.isOk();
            }
            /* Save recording frame height: */
            if (fSuccess && newDisplayData.m_iRecordingVideoFrameHeight != oldDisplayData.m_iRecordingVideoFrameHeight)
            {
                comRecordingScreenSettings.SetVideoHeight(newDisplayData.m_iRecordingVideoFrameHeight);
                fSuccess = comRecordingScreenSettings.isOk();
            }
            /* Save recording frame rate: */
            if (fSuccess && newDisplayData.m_iRecordingVideoFrameRate != oldDisplayData.m_iRecordingVideoFrameRate)
            {
                comRecordingScreenSettings.SetVideoFPS(newDisplayData.m_iRecordingVideoFrameRate);
                fSuccess = comRecordingScreenSettings.isOk();
            }
            /* Save recording frame bit rate: */
            if (fSuccess && newDisplayData.m_iRecordingVideoBitRate != oldDisplayData.m_iRecordingVideoBitRate)
            {
                comRecordingScreenSettings.SetVideoRate(newDisplayData.m_iRecordingVideoBitRate);
                fSuccess = comRecordingScreenSettings.isOk();
            }
            /* Save capture options: */
            if (fSuccess && newDisplayData.m_strRecordingVideoOptions != oldDisplayData.m_strRecordingVideoOptions)
            {
                comRecordingScreenSettings.SetOptions(newDisplayData.m_strRecordingVideoOptions);
                fSuccess = comRecordingScreenSettings.isOk();

                /** @todo The m_strRecordingVideoOptions must not be used for enabling / disabling the recording features;
                 *        instead, SetFeatures() has to be used for 7.0.
                 *
                 *        Also for the audio profile settings (Hz rate, bits, ++)
                 *        there are now audioHz, audioBits, audioChannels and so on.
                 *
                 *        So it's probably best to get rid of m_strRecordingVideoOptions altogether.
                 */
                QVector<KRecordingFeature> vecFeatures;
                if (UIDataSettingsMachineDisplay::isRecordingOptionEnabled(newDisplayData.m_strRecordingVideoOptions,
                                                                           UIDataSettingsMachineDisplay::RecordingOption_VC))
                    vecFeatures.append(KRecordingFeature_Video);

                if (UIDataSettingsMachineDisplay::isRecordingOptionEnabled(newDisplayData.m_strRecordingVideoOptions,
                                                                           UIDataSettingsMachineDisplay::RecordingOption_AC))
                    vecFeatures.append(KRecordingFeature_Audio);

                comRecordingScreenSettings.SetFeatures(vecFeatures);
            }

            /* Finally, save the screen's recording state: */
            /* Note: Must come last, as modifying options with an enabled recording state is not possible. */
            if (fSuccess && newDisplayData.m_vecRecordingScreens != oldDisplayData.m_vecRecordingScreens)
            {
                comRecordingScreenSettings.SetEnabled(newDisplayData.m_vecRecordingScreens[iScreenIndex]);
                fSuccess = comRecordingScreenSettings.isOk();
            }

            if (!fSuccess)
            {
                notifyOperationProgressError(UIErrorString::formatErrorInfo(comRecordingScreenSettings));
                break; /* No point trying to handle the other screens (if any). */
            }
        }

        /* Save whether recording is enabled:
         * Do this last, as after enabling recording no changes via API aren't allowed anymore. */
        if (fSuccess && newDisplayData.m_fRecordingEnabled != oldDisplayData.m_fRecordingEnabled)
        {
            recordingSettings.SetEnabled(newDisplayData.m_fRecordingEnabled);
            fSuccess = recordingSettings.isOk();
        }
    }

    /* Show error message if necessary: */
    if (!fSuccess)
    {
        if (!recordingSettings.isOk())
            notifyOperationProgressError(UIErrorString::formatErrorInfo(recordingSettings));
        else if (!m_machine.isOk()) /* Machine could indicate an error when saving the settings. */
            notifyOperationProgressError(UIErrorString::formatErrorInfo(m_machine));
    }

    /* Return result: */
    return fSuccess;
}

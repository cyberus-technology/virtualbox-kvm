/* $Id: UIConverterBackend.h $ */
/** @file
 * VBox Qt GUI - UIConverterBackend declaration.
 */

/*
 * Copyright (C) 2012-2023 Oracle and/or its affiliates.
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

#ifndef FEQT_INCLUDED_SRC_converter_UIConverterBackend_h
#define FEQT_INCLUDED_SRC_converter_UIConverterBackend_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* Qt includes: */
#include <QColor>
#include <QIcon>
#include <QPixmap>
#include <QString>

/* GUI includes: */
#include "UIDefs.h"
#include "UILibraryDefs.h"
#include "UIMediumDefs.h"
#include "UIExtraDataDefs.h"
#include "UISettingsDefs.h"

/* Other VBox includes: */
#include <iprt/assert.h>


/* Determines if 'Object of type X' can be converted to object of other type.
 * This function always returns 'false' until re-determined for specific object type. */
template<class X> bool canConvert() { return false; }

/* Converts passed 'Object X' to QColor.
 * This function returns null QColor for any object type until re-determined for specific one. */
template<class X> QColor toColor(const X & /* xobject */) { AssertFailed(); return QColor(); }

/* Converts passed 'Object X' to QIcon.
 * This function returns null QIcon for any object type until re-determined for specific one. */
template<class X> QIcon toIcon(const X & /* xobject */) { AssertFailed(); return QIcon(); }

/* Converts passed 'Object X' to QPixmap.
 * This function returns null QPixmap for any object type until re-determined for specific one. */
template<class X> QPixmap toWarningPixmap(const X & /* xobject */) { AssertFailed(); return QPixmap(); }

/* Converts passed 'Object of type X' to QString.
 * This function returns null QString for any object type until re-determined for specific one. */
template<class X> QString toString(const X & /* xobject */) { AssertFailed(); return QString(); }
/* Converts passed QString to 'Object of type X'.
 * This function returns default constructed object for any object type until re-determined for specific one. */
template<class X> X fromString(const QString & /* strData */) { AssertFailed(); return X(); }

/* Converts passed 'Object of type X' to non-translated QString.
 * This function returns null QString for any object type until re-determined for specific one. */
template<class X> QString toInternalString(const X & /* xobject */) { AssertFailed(); return QString(); }
/* Converts passed non-translated QString to 'Object of type X'.
 * This function returns default constructed object for any object type until re-determined for specific one. */
template<class X> X fromInternalString(const QString & /* strData */) { AssertFailed(); return X(); }

/* Converts passed 'Object of type X' to abstract integer.
 * This function returns 0 for any object type until re-determined for specific one. */
template<class X> int toInternalInteger(const X & /* xobject */) { AssertFailed(); return 0; }
/* Converts passed abstract integer to 'Object of type X'.
 * This function returns default constructed object for any object type until re-determined for specific one. */
template<class X> X fromInternalInteger(const int & /* iData */) { AssertFailed(); return X(); }


/* Declare global canConvert specializations: */
template<> SHARED_LIBRARY_STUFF bool canConvert<Qt::Alignment>();
template<> SHARED_LIBRARY_STUFF bool canConvert<Qt::SortOrder>();
template<> SHARED_LIBRARY_STUFF bool canConvert<SizeSuffix>();
template<> SHARED_LIBRARY_STUFF bool canConvert<StorageSlot>();
template<> SHARED_LIBRARY_STUFF bool canConvert<DesktopWatchdogPolicy_SynthTest>();
template<> SHARED_LIBRARY_STUFF bool canConvert<UIExtraDataMetaDefs::DialogType>();
template<> SHARED_LIBRARY_STUFF bool canConvert<UIExtraDataMetaDefs::MenuType>();
template<> SHARED_LIBRARY_STUFF bool canConvert<UIExtraDataMetaDefs::MenuApplicationActionType>();
template<> SHARED_LIBRARY_STUFF bool canConvert<UIExtraDataMetaDefs::MenuHelpActionType>();
template<> SHARED_LIBRARY_STUFF bool canConvert<UIExtraDataMetaDefs::RuntimeMenuMachineActionType>();
template<> SHARED_LIBRARY_STUFF bool canConvert<UIExtraDataMetaDefs::RuntimeMenuViewActionType>();
template<> SHARED_LIBRARY_STUFF bool canConvert<UIExtraDataMetaDefs::RuntimeMenuInputActionType>();
template<> SHARED_LIBRARY_STUFF bool canConvert<UIExtraDataMetaDefs::RuntimeMenuDevicesActionType>();
#ifdef VBOX_WITH_DEBUGGER_GUI
template<> SHARED_LIBRARY_STUFF bool canConvert<UIExtraDataMetaDefs::RuntimeMenuDebuggerActionType>();
#endif /* VBOX_WITH_DEBUGGER_GUI */
#ifdef VBOX_WS_MAC
template<> SHARED_LIBRARY_STUFF bool canConvert<UIExtraDataMetaDefs::MenuWindowActionType>();
#endif /* VBOX_WS_MAC */
template<> SHARED_LIBRARY_STUFF bool canConvert<UIExtraDataMetaDefs::DetailsElementOptionTypeGeneral>();
template<> SHARED_LIBRARY_STUFF bool canConvert<UIExtraDataMetaDefs::DetailsElementOptionTypeSystem>();
template<> SHARED_LIBRARY_STUFF bool canConvert<UIExtraDataMetaDefs::DetailsElementOptionTypeDisplay>();
template<> SHARED_LIBRARY_STUFF bool canConvert<UIExtraDataMetaDefs::DetailsElementOptionTypeStorage>();
template<> SHARED_LIBRARY_STUFF bool canConvert<UIExtraDataMetaDefs::DetailsElementOptionTypeAudio>();
template<> SHARED_LIBRARY_STUFF bool canConvert<UIExtraDataMetaDefs::DetailsElementOptionTypeNetwork>();
template<> SHARED_LIBRARY_STUFF bool canConvert<UIExtraDataMetaDefs::DetailsElementOptionTypeSerial>();
template<> SHARED_LIBRARY_STUFF bool canConvert<UIExtraDataMetaDefs::DetailsElementOptionTypeUsb>();
template<> SHARED_LIBRARY_STUFF bool canConvert<UIExtraDataMetaDefs::DetailsElementOptionTypeSharedFolders>();
template<> SHARED_LIBRARY_STUFF bool canConvert<UIExtraDataMetaDefs::DetailsElementOptionTypeUserInterface>();
template<> SHARED_LIBRARY_STUFF bool canConvert<UIExtraDataMetaDefs::DetailsElementOptionTypeDescription>();
template<> SHARED_LIBRARY_STUFF bool canConvert<UIColorThemeType>();
template<> SHARED_LIBRARY_STUFF bool canConvert<UILaunchMode>();
template<> SHARED_LIBRARY_STUFF bool canConvert<UIToolType>();
template<> SHARED_LIBRARY_STUFF bool canConvert<UIVisualStateType>();
template<> SHARED_LIBRARY_STUFF bool canConvert<DetailsElementType>();
template<> SHARED_LIBRARY_STUFF bool canConvert<PreviewUpdateIntervalType>();
template<> SHARED_LIBRARY_STUFF bool canConvert<UIDiskEncryptionCipherType>();
template<> SHARED_LIBRARY_STUFF bool canConvert<GUIFeatureType>();
template<> SHARED_LIBRARY_STUFF bool canConvert<GlobalSettingsPageType>();
template<> SHARED_LIBRARY_STUFF bool canConvert<MachineSettingsPageType>();
template<> SHARED_LIBRARY_STUFF bool canConvert<UIRemoteMode>();
template<> SHARED_LIBRARY_STUFF bool canConvert<WizardType>();
template<> SHARED_LIBRARY_STUFF bool canConvert<IndicatorType>();
template<> SHARED_LIBRARY_STUFF bool canConvert<MachineCloseAction>();
template<> SHARED_LIBRARY_STUFF bool canConvert<MouseCapturePolicy>();
template<> SHARED_LIBRARY_STUFF bool canConvert<GuruMeditationHandlerType>();
template<> SHARED_LIBRARY_STUFF bool canConvert<ScalingOptimizationType>();
#ifndef VBOX_WS_MAC
template<> SHARED_LIBRARY_STUFF bool canConvert<MiniToolbarAlignment>();
#endif
template<> SHARED_LIBRARY_STUFF bool canConvert<InformationElementType>();
template<> SHARED_LIBRARY_STUFF bool canConvert<MaximumGuestScreenSizePolicy>();
template<> SHARED_LIBRARY_STUFF bool canConvert<UIMediumFormat>();
template<> SHARED_LIBRARY_STUFF bool canConvert<UISettingsDefs::RecordingMode>();
template<> SHARED_LIBRARY_STUFF bool canConvert<VMActivityOverviewColumn>();

/* Declare COM canConvert specializations: */
template<> SHARED_LIBRARY_STUFF bool canConvert<KCloudMachineState>();
template<> SHARED_LIBRARY_STUFF bool canConvert<KMachineState>();
template<> SHARED_LIBRARY_STUFF bool canConvert<KSessionState>();
template<> SHARED_LIBRARY_STUFF bool canConvert<KParavirtProvider>();
template<> SHARED_LIBRARY_STUFF bool canConvert<KDeviceType>();
template<> SHARED_LIBRARY_STUFF bool canConvert<KClipboardMode>();
template<> SHARED_LIBRARY_STUFF bool canConvert<KDnDMode>();
template<> SHARED_LIBRARY_STUFF bool canConvert<KPointingHIDType>();
template<> SHARED_LIBRARY_STUFF bool canConvert<KGraphicsControllerType>();
template<> SHARED_LIBRARY_STUFF bool canConvert<KMediumType>();
template<> SHARED_LIBRARY_STUFF bool canConvert<KMediumVariant>();
template<> SHARED_LIBRARY_STUFF bool canConvert<KNetworkAttachmentType>();
template<> SHARED_LIBRARY_STUFF bool canConvert<KNetworkAdapterType>();
template<> SHARED_LIBRARY_STUFF bool canConvert<KNetworkAdapterPromiscModePolicy>();
template<> SHARED_LIBRARY_STUFF bool canConvert<KPortMode>();
template<> SHARED_LIBRARY_STUFF bool canConvert<KUSBControllerType>();
template<> SHARED_LIBRARY_STUFF bool canConvert<KUSBDeviceState>();
template<> SHARED_LIBRARY_STUFF bool canConvert<KUSBDeviceFilterAction>();
template<> SHARED_LIBRARY_STUFF bool canConvert<KAudioDriverType>();
template<> SHARED_LIBRARY_STUFF bool canConvert<KAudioControllerType>();
template<> SHARED_LIBRARY_STUFF bool canConvert<KAuthType>();
template<> SHARED_LIBRARY_STUFF bool canConvert<KStorageBus>();
template<> SHARED_LIBRARY_STUFF bool canConvert<KStorageControllerType>();
template<> SHARED_LIBRARY_STUFF bool canConvert<KChipsetType>();
template<> SHARED_LIBRARY_STUFF bool canConvert<KTpmType>();
template<> SHARED_LIBRARY_STUFF bool canConvert<KNATProtocol>();
template<> SHARED_LIBRARY_STUFF bool canConvert<KGuestSessionStatus>();
template<> SHARED_LIBRARY_STUFF bool canConvert<KProcessStatus>();


/* Declare global conversion specializations: */
template<> SHARED_LIBRARY_STUFF QString toInternalString(const Qt::Alignment &enmAlignment);
template<> SHARED_LIBRARY_STUFF Qt::Alignment fromInternalString<Qt::Alignment>(const QString &strAlignment);
template<> SHARED_LIBRARY_STUFF QString toInternalString(const Qt::SortOrder &enmSortOrder);
template<> SHARED_LIBRARY_STUFF Qt::SortOrder fromInternalString<Qt::SortOrder>(const QString &strSortOrder);
template<> SHARED_LIBRARY_STUFF QString toString(const SizeSuffix &sizeSuffix);
template<> SHARED_LIBRARY_STUFF SizeSuffix fromString<SizeSuffix>(const QString &strSizeSuffix);
template<> SHARED_LIBRARY_STUFF QString toString(const StorageSlot &storageSlot);
template<> SHARED_LIBRARY_STUFF StorageSlot fromString<StorageSlot>(const QString &strStorageSlot);
template<> SHARED_LIBRARY_STUFF DesktopWatchdogPolicy_SynthTest fromInternalString<DesktopWatchdogPolicy_SynthTest>(const QString &strPolicyType);
template<> SHARED_LIBRARY_STUFF QString toInternalString(const UIExtraDataMetaDefs::DialogType &enmDialogType);
template<> SHARED_LIBRARY_STUFF UIExtraDataMetaDefs::DialogType fromInternalString<UIExtraDataMetaDefs::DialogType>(const QString &strDialogType);
template<> SHARED_LIBRARY_STUFF QString toInternalString(const UIExtraDataMetaDefs::MenuType &menuType);
template<> SHARED_LIBRARY_STUFF UIExtraDataMetaDefs::MenuType fromInternalString<UIExtraDataMetaDefs::MenuType>(const QString &strMenuType);
template<> SHARED_LIBRARY_STUFF QString toInternalString(const UIExtraDataMetaDefs::MenuApplicationActionType &menuApplicationActionType);
template<> SHARED_LIBRARY_STUFF UIExtraDataMetaDefs::MenuApplicationActionType fromInternalString<UIExtraDataMetaDefs::MenuApplicationActionType>(const QString &strMenuApplicationActionType);
template<> SHARED_LIBRARY_STUFF QString toInternalString(const UIExtraDataMetaDefs::MenuHelpActionType &menuHelpActionType);
template<> SHARED_LIBRARY_STUFF UIExtraDataMetaDefs::MenuHelpActionType fromInternalString<UIExtraDataMetaDefs::MenuHelpActionType>(const QString &strMenuHelpActionType);
template<> SHARED_LIBRARY_STUFF QString toInternalString(const UIExtraDataMetaDefs::RuntimeMenuMachineActionType &runtimeMenuMachineActionType);
template<> SHARED_LIBRARY_STUFF UIExtraDataMetaDefs::RuntimeMenuMachineActionType fromInternalString<UIExtraDataMetaDefs::RuntimeMenuMachineActionType>(const QString &strRuntimeMenuMachineActionType);
template<> SHARED_LIBRARY_STUFF QString toInternalString(const UIExtraDataMetaDefs::RuntimeMenuViewActionType &runtimeMenuViewActionType);
template<> SHARED_LIBRARY_STUFF UIExtraDataMetaDefs::RuntimeMenuViewActionType fromInternalString<UIExtraDataMetaDefs::RuntimeMenuViewActionType>(const QString &strRuntimeMenuViewActionType);
template<> SHARED_LIBRARY_STUFF QString toInternalString(const UIExtraDataMetaDefs::RuntimeMenuInputActionType &runtimeMenuInputActionType);
template<> SHARED_LIBRARY_STUFF UIExtraDataMetaDefs::RuntimeMenuInputActionType fromInternalString<UIExtraDataMetaDefs::RuntimeMenuInputActionType>(const QString &strRuntimeMenuInputActionType);
template<> SHARED_LIBRARY_STUFF QString toInternalString(const UIExtraDataMetaDefs::RuntimeMenuDevicesActionType &runtimeMenuDevicesActionType);
template<> SHARED_LIBRARY_STUFF UIExtraDataMetaDefs::RuntimeMenuDevicesActionType fromInternalString<UIExtraDataMetaDefs::RuntimeMenuDevicesActionType>(const QString &strRuntimeMenuDevicesActionType);
#ifdef VBOX_WITH_DEBUGGER_GUI
template<> SHARED_LIBRARY_STUFF QString toInternalString(const UIExtraDataMetaDefs::RuntimeMenuDebuggerActionType &runtimeMenuDebuggerActionType);
template<> SHARED_LIBRARY_STUFF UIExtraDataMetaDefs::RuntimeMenuDebuggerActionType fromInternalString<UIExtraDataMetaDefs::RuntimeMenuDebuggerActionType>(const QString &strRuntimeMenuDebuggerActionType);
#endif /* VBOX_WITH_DEBUGGER_GUI */
#ifdef VBOX_WS_MAC
template<> SHARED_LIBRARY_STUFF QString toInternalString(const UIExtraDataMetaDefs::MenuWindowActionType &menuWindowActionType);
template<> SHARED_LIBRARY_STUFF UIExtraDataMetaDefs::MenuWindowActionType fromInternalString<UIExtraDataMetaDefs::MenuWindowActionType>(const QString &strMenuWindowActionType);
#endif /* VBOX_WS_MAC */
template<> SHARED_LIBRARY_STUFF QString toString(const UIExtraDataMetaDefs::DetailsElementOptionTypeGeneral &enmDetailsElementOptionTypeGeneral);
template<> SHARED_LIBRARY_STUFF QString toInternalString(const UIExtraDataMetaDefs::DetailsElementOptionTypeGeneral &enmDetailsElementOptionTypeGeneral);
template<> SHARED_LIBRARY_STUFF UIExtraDataMetaDefs::DetailsElementOptionTypeGeneral fromInternalString<UIExtraDataMetaDefs::DetailsElementOptionTypeGeneral>(const QString &strDetailsElementOptionTypeGeneral);
template<> SHARED_LIBRARY_STUFF QString toString(const UIExtraDataMetaDefs::DetailsElementOptionTypeSystem &enmDetailsElementOptionTypeSystem);
template<> SHARED_LIBRARY_STUFF QString toInternalString(const UIExtraDataMetaDefs::DetailsElementOptionTypeSystem &enmDetailsElementOptionTypeSystem);
template<> SHARED_LIBRARY_STUFF UIExtraDataMetaDefs::DetailsElementOptionTypeSystem fromInternalString<UIExtraDataMetaDefs::DetailsElementOptionTypeSystem>(const QString &strDetailsElementOptionTypeSystem);
template<> SHARED_LIBRARY_STUFF QString toString(const UIExtraDataMetaDefs::DetailsElementOptionTypeDisplay &enmDetailsElementOptionTypeDisplay);
template<> SHARED_LIBRARY_STUFF QString toInternalString(const UIExtraDataMetaDefs::DetailsElementOptionTypeDisplay &enmDetailsElementOptionTypeDisplay);
template<> SHARED_LIBRARY_STUFF UIExtraDataMetaDefs::DetailsElementOptionTypeDisplay fromInternalString<UIExtraDataMetaDefs::DetailsElementOptionTypeDisplay>(const QString &strDetailsElementOptionTypeDisplay);
template<> SHARED_LIBRARY_STUFF QString toString(const UIExtraDataMetaDefs::DetailsElementOptionTypeStorage &enmDetailsElementOptionTypeStorage);
template<> SHARED_LIBRARY_STUFF QString toInternalString(const UIExtraDataMetaDefs::DetailsElementOptionTypeStorage &enmDetailsElementOptionTypeStorage);
template<> SHARED_LIBRARY_STUFF UIExtraDataMetaDefs::DetailsElementOptionTypeStorage fromInternalString<UIExtraDataMetaDefs::DetailsElementOptionTypeStorage>(const QString &strDetailsElementOptionTypeStorage);
template<> SHARED_LIBRARY_STUFF QString toString(const UIExtraDataMetaDefs::DetailsElementOptionTypeAudio &enmDetailsElementOptionTypeAudio);
template<> SHARED_LIBRARY_STUFF QString toInternalString(const UIExtraDataMetaDefs::DetailsElementOptionTypeAudio &enmDetailsElementOptionTypeAudio);
template<> SHARED_LIBRARY_STUFF UIExtraDataMetaDefs::DetailsElementOptionTypeAudio fromInternalString<UIExtraDataMetaDefs::DetailsElementOptionTypeAudio>(const QString &strDetailsElementOptionTypeAudio);
template<> SHARED_LIBRARY_STUFF QString toString(const UIExtraDataMetaDefs::DetailsElementOptionTypeNetwork &enmDetailsElementOptionTypeNetwork);
template<> SHARED_LIBRARY_STUFF QString toInternalString(const UIExtraDataMetaDefs::DetailsElementOptionTypeNetwork &enmDetailsElementOptionTypeNetwork);
template<> SHARED_LIBRARY_STUFF UIExtraDataMetaDefs::DetailsElementOptionTypeNetwork fromInternalString<UIExtraDataMetaDefs::DetailsElementOptionTypeNetwork>(const QString &strDetailsElementOptionTypeNetwork);
template<> SHARED_LIBRARY_STUFF QString toString(const UIExtraDataMetaDefs::DetailsElementOptionTypeSerial &enmDetailsElementOptionTypeSerial);
template<> SHARED_LIBRARY_STUFF QString toInternalString(const UIExtraDataMetaDefs::DetailsElementOptionTypeSerial &enmDetailsElementOptionTypeSerial);
template<> SHARED_LIBRARY_STUFF UIExtraDataMetaDefs::DetailsElementOptionTypeSerial fromInternalString<UIExtraDataMetaDefs::DetailsElementOptionTypeSerial>(const QString &strDetailsElementOptionTypeSerial);
template<> SHARED_LIBRARY_STUFF QString toString(const UIExtraDataMetaDefs::DetailsElementOptionTypeUsb &enmDetailsElementOptionTypeUsb);
template<> SHARED_LIBRARY_STUFF QString toInternalString(const UIExtraDataMetaDefs::DetailsElementOptionTypeUsb &enmDetailsElementOptionTypeUsb);
template<> SHARED_LIBRARY_STUFF UIExtraDataMetaDefs::DetailsElementOptionTypeUsb fromInternalString<UIExtraDataMetaDefs::DetailsElementOptionTypeUsb>(const QString &strDetailsElementOptionTypeUsb);
template<> SHARED_LIBRARY_STUFF QString toString(const UIExtraDataMetaDefs::DetailsElementOptionTypeSharedFolders &enmDetailsElementOptionTypeSharedFolders);
template<> SHARED_LIBRARY_STUFF QString toInternalString(const UIExtraDataMetaDefs::DetailsElementOptionTypeSharedFolders &enmDetailsElementOptionTypeSharedFolders);
template<> SHARED_LIBRARY_STUFF UIExtraDataMetaDefs::DetailsElementOptionTypeSharedFolders fromInternalString<UIExtraDataMetaDefs::DetailsElementOptionTypeSharedFolders>(const QString &strDetailsElementOptionTypeSharedFolders);
template<> SHARED_LIBRARY_STUFF QString toString(const UIExtraDataMetaDefs::DetailsElementOptionTypeUserInterface &enmDetailsElementOptionTypeUserInterface);
template<> SHARED_LIBRARY_STUFF QString toInternalString(const UIExtraDataMetaDefs::DetailsElementOptionTypeUserInterface &enmDetailsElementOptionTypeUserInterface);
template<> SHARED_LIBRARY_STUFF UIExtraDataMetaDefs::DetailsElementOptionTypeUserInterface fromInternalString<UIExtraDataMetaDefs::DetailsElementOptionTypeUserInterface>(const QString &strDetailsElementOptionTypeUserInterface);
template<> SHARED_LIBRARY_STUFF QString toString(const UIExtraDataMetaDefs::DetailsElementOptionTypeDescription &enmDetailsElementOptionTypeDescription);
template<> SHARED_LIBRARY_STUFF QString toInternalString(const UIExtraDataMetaDefs::DetailsElementOptionTypeDescription &enmDetailsElementOptionTypeDescription);
template<> SHARED_LIBRARY_STUFF UIExtraDataMetaDefs::DetailsElementOptionTypeDescription fromInternalString<UIExtraDataMetaDefs::DetailsElementOptionTypeDescription>(const QString &strDetailsElementOptionTypeDescription);
template<> SHARED_LIBRARY_STUFF QString toString(const UIColorThemeType &colorThemeType);
template<> SHARED_LIBRARY_STUFF QString toInternalString(const UIColorThemeType &colorThemeType);
template<> SHARED_LIBRARY_STUFF UIColorThemeType fromInternalString<UIColorThemeType>(const QString &strColorThemeType);
template<> SHARED_LIBRARY_STUFF UILaunchMode fromInternalString<UILaunchMode>(const QString &strDefaultFrontendType);
template<> SHARED_LIBRARY_STUFF QString toInternalString(const UIToolType &enmToolType);
template<> SHARED_LIBRARY_STUFF UIToolType fromInternalString<UIToolType>(const QString &strToolType);
template<> SHARED_LIBRARY_STUFF QString toString(const UIVisualStateType &visualStateType);
template<> SHARED_LIBRARY_STUFF QString toInternalString(const UIVisualStateType &visualStateType);
template<> SHARED_LIBRARY_STUFF UIVisualStateType fromInternalString<UIVisualStateType>(const QString &strVisualStateType);
template<> SHARED_LIBRARY_STUFF QString toString(const DetailsElementType &detailsElementType);
template<> SHARED_LIBRARY_STUFF DetailsElementType fromString<DetailsElementType>(const QString &strDetailsElementType);
template<> SHARED_LIBRARY_STUFF QString toInternalString(const DetailsElementType &detailsElementType);
template<> SHARED_LIBRARY_STUFF DetailsElementType fromInternalString<DetailsElementType>(const QString &strDetailsElementType);
template<> SHARED_LIBRARY_STUFF QIcon toIcon(const DetailsElementType &detailsElementType);
template<> SHARED_LIBRARY_STUFF QString toInternalString(const PreviewUpdateIntervalType &previewUpdateIntervalType);
template<> SHARED_LIBRARY_STUFF PreviewUpdateIntervalType fromInternalString<PreviewUpdateIntervalType>(const QString &strPreviewUpdateIntervalType);
template<> SHARED_LIBRARY_STUFF int toInternalInteger(const PreviewUpdateIntervalType &previewUpdateIntervalType);
template<> SHARED_LIBRARY_STUFF PreviewUpdateIntervalType fromInternalInteger<PreviewUpdateIntervalType>(const int &iPreviewUpdateIntervalType);
template<> SHARED_LIBRARY_STUFF QString toInternalString(const UIDiskEncryptionCipherType &enmDiskEncryptionCipherType);
template<> SHARED_LIBRARY_STUFF UIDiskEncryptionCipherType fromInternalString<UIDiskEncryptionCipherType>(const QString &strDiskEncryptionCipherType);
template<> SHARED_LIBRARY_STUFF QString toString(const UIDiskEncryptionCipherType &enmDiskEncryptionCipherType);
template<> SHARED_LIBRARY_STUFF UIDiskEncryptionCipherType fromString<UIDiskEncryptionCipherType>(const QString &strDiskEncryptionCipherType);
template<> SHARED_LIBRARY_STUFF QString toInternalString(const GUIFeatureType &guiFeatureType);
template<> SHARED_LIBRARY_STUFF GUIFeatureType fromInternalString<GUIFeatureType>(const QString &strGuiFeatureType);
template<> SHARED_LIBRARY_STUFF QString toInternalString(const GlobalSettingsPageType &globalSettingsPageType);
template<> SHARED_LIBRARY_STUFF GlobalSettingsPageType fromInternalString<GlobalSettingsPageType>(const QString &strGlobalSettingsPageType);
template<> SHARED_LIBRARY_STUFF QPixmap toWarningPixmap(const GlobalSettingsPageType &globalSettingsPageType);
template<> SHARED_LIBRARY_STUFF QString toInternalString(const MachineSettingsPageType &machineSettingsPageType);
template<> SHARED_LIBRARY_STUFF MachineSettingsPageType fromInternalString<MachineSettingsPageType>(const QString &strMachineSettingsPageType);
template<> SHARED_LIBRARY_STUFF QPixmap toWarningPixmap(const MachineSettingsPageType &machineSettingsPageType);
template<> SHARED_LIBRARY_STUFF QString toString(const UIRemoteMode &enmMode);
template<> SHARED_LIBRARY_STUFF QString toInternalString(const WizardType &wizardType);
template<> SHARED_LIBRARY_STUFF WizardType fromInternalString<WizardType>(const QString &strWizardType);
template<> SHARED_LIBRARY_STUFF QString toInternalString(const IndicatorType &indicatorType);
template<> SHARED_LIBRARY_STUFF IndicatorType fromInternalString<IndicatorType>(const QString &strIndicatorType);
template<> SHARED_LIBRARY_STUFF QString toString(const IndicatorType &indicatorType);
template<> SHARED_LIBRARY_STUFF QIcon toIcon(const IndicatorType &indicatorType);
template<> SHARED_LIBRARY_STUFF QString toInternalString(const MachineCloseAction &machineCloseAction);
template<> SHARED_LIBRARY_STUFF MachineCloseAction fromInternalString<MachineCloseAction>(const QString &strMachineCloseAction);
template<> SHARED_LIBRARY_STUFF QString toInternalString(const MouseCapturePolicy &mouseCapturePolicy);
template<> SHARED_LIBRARY_STUFF MouseCapturePolicy fromInternalString<MouseCapturePolicy>(const QString &strMouseCapturePolicy);
template<> SHARED_LIBRARY_STUFF QString toInternalString(const GuruMeditationHandlerType &guruMeditationHandlerType);
template<> SHARED_LIBRARY_STUFF GuruMeditationHandlerType fromInternalString<GuruMeditationHandlerType>(const QString &strGuruMeditationHandlerType);
template<> SHARED_LIBRARY_STUFF QString toInternalString(const ScalingOptimizationType &optimizationType);
template<> SHARED_LIBRARY_STUFF ScalingOptimizationType fromInternalString<ScalingOptimizationType>(const QString &strOptimizationType);
#ifndef VBOX_WS_MAC
template<> SHARED_LIBRARY_STUFF QString toInternalString(const MiniToolbarAlignment &miniToolbarAlignment);
template<> SHARED_LIBRARY_STUFF MiniToolbarAlignment fromInternalString<MiniToolbarAlignment>(const QString &strMiniToolbarAlignment);
#endif
template<> SHARED_LIBRARY_STUFF QString toString(const InformationElementType &informationElementType);
template<> SHARED_LIBRARY_STUFF InformationElementType fromString<InformationElementType>(const QString &strInformationElementType);
template<> SHARED_LIBRARY_STUFF QString toInternalString(const InformationElementType &informationElementType);
template<> SHARED_LIBRARY_STUFF InformationElementType fromInternalString<InformationElementType>(const QString &strInformationElementType);
template<> SHARED_LIBRARY_STUFF QIcon toIcon(const InformationElementType &informationElementType);
template<> SHARED_LIBRARY_STUFF QString toString(const MaximumGuestScreenSizePolicy &enmMaximumGuestScreenSizePolicy);
template<> SHARED_LIBRARY_STUFF QString toInternalString(const MaximumGuestScreenSizePolicy &enmMaximumGuestScreenSizePolicy);
template<> SHARED_LIBRARY_STUFF MaximumGuestScreenSizePolicy fromInternalString<MaximumGuestScreenSizePolicy>(const QString &strMaximumGuestScreenSizePolicy);
template<> SHARED_LIBRARY_STUFF QString toString(const UIMediumFormat &enmUIMediumFormat);
template<> SHARED_LIBRARY_STUFF QString toInternalString(const UIMediumFormat &enmUIMediumFormat);
template<> SHARED_LIBRARY_STUFF UIMediumFormat fromInternalString<UIMediumFormat>(const QString &strUIMediumFormat);
template<> SHARED_LIBRARY_STUFF QString toString(const UISettingsDefs::RecordingMode &enmRecordingMode);
template<> SHARED_LIBRARY_STUFF QString toInternalString(const VMActivityOverviewColumn &enmVMActivityOverviewColumn);
template<> SHARED_LIBRARY_STUFF VMActivityOverviewColumn fromInternalString<VMActivityOverviewColumn>(const QString &strVMActivityOverviewColumn);

/* Declare COM conversion specializations: */
template<> SHARED_LIBRARY_STUFF QIcon toIcon(const KCloudMachineState &state);
template<> SHARED_LIBRARY_STUFF QString toString(const KCloudMachineState &state);
template<> SHARED_LIBRARY_STUFF QColor toColor(const KMachineState &state);
template<> SHARED_LIBRARY_STUFF QIcon toIcon(const KMachineState &state);
template<> SHARED_LIBRARY_STUFF QString toString(const KMachineState &state);
template<> SHARED_LIBRARY_STUFF QString toString(const KSessionState &state);
template<> SHARED_LIBRARY_STUFF QString toString(const KParavirtProvider &type);
template<> SHARED_LIBRARY_STUFF QString toString(const KDeviceType &type);
template<> SHARED_LIBRARY_STUFF QString toString(const KClipboardMode &mode);
template<> SHARED_LIBRARY_STUFF QString toString(const KDnDMode &mode);
template<> SHARED_LIBRARY_STUFF QString toString(const KPointingHIDType &type);
template<> SHARED_LIBRARY_STUFF QString toString(const KGraphicsControllerType &type);
template<> SHARED_LIBRARY_STUFF KGraphicsControllerType fromString<KGraphicsControllerType>(const QString &strType);
template<> SHARED_LIBRARY_STUFF QString toString(const KMediumType &type);
template<> SHARED_LIBRARY_STUFF QString toString(const KMediumVariant &variant);
template<> SHARED_LIBRARY_STUFF QString toString(const KNetworkAttachmentType &type);
template<> SHARED_LIBRARY_STUFF QString toString(const KNetworkAdapterType &type);
template<> SHARED_LIBRARY_STUFF QString toString(const KNetworkAdapterPromiscModePolicy &policy);
template<> SHARED_LIBRARY_STUFF QString toString(const KPortMode &mode);
template<> SHARED_LIBRARY_STUFF KPortMode fromString<KPortMode>(const QString &strMode);
template<> SHARED_LIBRARY_STUFF QString toString(const KUSBControllerType &type);
template<> SHARED_LIBRARY_STUFF QString toString(const KUSBDeviceState &state);
template<> SHARED_LIBRARY_STUFF QString toString(const KUSBDeviceFilterAction &action);
template<> SHARED_LIBRARY_STUFF KUSBDeviceFilterAction fromString<KUSBDeviceFilterAction>(const QString &strAction);
template<> SHARED_LIBRARY_STUFF QString toString(const KAudioDriverType &type);
template<> SHARED_LIBRARY_STUFF KAudioDriverType fromString<KAudioDriverType>(const QString &strType);
template<> SHARED_LIBRARY_STUFF QString toString(const KAudioControllerType &type);
template<> SHARED_LIBRARY_STUFF KAudioControllerType fromString<KAudioControllerType>(const QString &strType);
template<> SHARED_LIBRARY_STUFF QString toString(const KAuthType &type);
template<> SHARED_LIBRARY_STUFF KAuthType fromString<KAuthType>(const QString &strType);
template<> SHARED_LIBRARY_STUFF QString toString(const KStorageBus &bus);
template<> SHARED_LIBRARY_STUFF KStorageBus fromString<KStorageBus>(const QString &strType);
template<> SHARED_LIBRARY_STUFF QString toString(const KStorageControllerType &type);
template<> SHARED_LIBRARY_STUFF KStorageControllerType fromString<KStorageControllerType>(const QString &strType);
template<> SHARED_LIBRARY_STUFF QString toString(const KChipsetType &type);
template<> SHARED_LIBRARY_STUFF QString toString(const KTpmType &type);
template<> SHARED_LIBRARY_STUFF QString toString(const KNATProtocol &protocol);
template<> SHARED_LIBRARY_STUFF QString toInternalString(const KNATProtocol &protocol);
template<> SHARED_LIBRARY_STUFF KNATProtocol fromInternalString<KNATProtocol>(const QString &strProtocol);
template<> SHARED_LIBRARY_STUFF QString toString(const KGuestSessionStatus &status);
template<> SHARED_LIBRARY_STUFF KGuestSessionStatus fromString<KGuestSessionStatus>(const QString &strStatus);
template<> SHARED_LIBRARY_STUFF QString toString(const KProcessStatus &status);
template<> SHARED_LIBRARY_STUFF KProcessStatus fromString<KProcessStatus>(const QString &strStatus);


#endif /* !FEQT_INCLUDED_SRC_converter_UIConverterBackend_h */

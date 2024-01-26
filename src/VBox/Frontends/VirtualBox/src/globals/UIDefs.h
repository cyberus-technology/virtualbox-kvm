/* $Id: UIDefs.h $ */
/** @file
 * VBox Qt GUI - Global definitions.
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

#ifndef FEQT_INCLUDED_SRC_globals_UIDefs_h
#define FEQT_INCLUDED_SRC_globals_UIDefs_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* Define GUI log group: */
// WORKAROUND:
// This define should go *before* VBox/log.h include!
#ifndef LOG_GROUP
# define LOG_GROUP LOG_GROUP_GUI
#endif

/* Qt includes: */
#include <QEvent>
#include <QStringList>

/* GUI includes: */
#include "UILibraryDefs.h"

/* COM includes: */
#include "COMEnums.h"

/* Other VBox includes: */
#include <VBox/log.h>
#include <VBox/com/defs.h>

/* Defines: */
#ifdef RT_STRICT
# define AssertWrapperOk(w)         AssertMsg(w.isOk(), (#w " is not okay (RC=0x%08X)", w.lastRC()))
# define AssertWrapperOkMsg(w, m)   AssertMsg(w.isOk(), (#w ": " m " (RC=0x%08X)", w.lastRC()))
#else
# define AssertWrapperOk(w)         do {} while (0)
# define AssertWrapperOkMsg(w, m)   do {} while (0)
#endif


/** Global namespace. */
namespace UIDefs
{
    /** Additional Qt event types. */
    enum UIEventType
    {
        ActivateActionEventType = QEvent::User + 101,
#ifdef VBOX_WS_MAC
        ShowWindowEventType,
#endif
    };

    /** Size formatting types. */
    enum FormatSize
    {
        FormatSize_Round,
        FormatSize_RoundDown,
        FormatSize_RoundUp
    };

    /** Default guest additions image name. */
    SHARED_LIBRARY_STUFF extern const char* GUI_GuestAdditionsName;
    /** Default extension pack name. */
    SHARED_LIBRARY_STUFF extern const char* GUI_ExtPackName;

    /** Allowed VBox file extensions. */
    SHARED_LIBRARY_STUFF extern QStringList VBoxFileExts;
    /** Allowed VBox Extension Pack file extensions. */
    SHARED_LIBRARY_STUFF extern QStringList VBoxExtPackFileExts;
    /** Allowed OVF file extensions. */
    SHARED_LIBRARY_STUFF extern QStringList OVFFileExts;

    /** Holds environment variable name for Desktop Watchdog / Synthetic Test policy type. */
    SHARED_LIBRARY_STUFF extern const char *VBox_DesktopWatchdogPolicy_SynthTest;
}
using namespace UIDefs /* if header included */;


/** Size suffixes. */
enum SizeSuffix
{
    SizeSuffix_Byte = 0,
    SizeSuffix_KiloByte,
    SizeSuffix_MegaByte,
    SizeSuffix_GigaByte,
    SizeSuffix_TeraByte,
    SizeSuffix_PetaByte,
    SizeSuffix_Max
};


/** VM launch modes. */
enum UILaunchMode
{
    UILaunchMode_Invalid,
    UILaunchMode_Default,
    UILaunchMode_Headless,
    UILaunchMode_Separate
};


/** Storage-slot struct. */
struct StorageSlot
{
    StorageSlot() : bus(KStorageBus_Null), port(0), device(0) {}
    StorageSlot(const StorageSlot &other) : bus(other.bus), port(other.port), device(other.device) {}
    StorageSlot(KStorageBus otherBus, LONG iPort, LONG iDevice) : bus(otherBus), port(iPort), device(iDevice) {}
    StorageSlot& operator=(const StorageSlot &other) { bus = other.bus; port = other.port; device = other.device; return *this; }
    bool operator==(const StorageSlot &other) const { return bus == other.bus && port == other.port && device == other.device; }
    bool operator!=(const StorageSlot &other) const { return bus != other.bus || port != other.port || device != other.device; }
    bool operator<(const StorageSlot &other) const { return (bus <  other.bus) ||
                                                            (bus == other.bus && port <  other.port) ||
                                                            (bus == other.bus && port == other.port && device < other.device); }
    bool operator>(const StorageSlot &other) const { return (bus >  other.bus) ||
                                                            (bus == other.bus && port >  other.port) ||
                                                            (bus == other.bus && port == other.port && device > other.device); }
    bool isNull() const { return bus == KStorageBus_Null; }
    KStorageBus bus; LONG port; LONG device;
};
Q_DECLARE_METATYPE(StorageSlot);


/** Storage-slot struct extension with exact controller name. */
struct ExactStorageSlot : public StorageSlot
{
    ExactStorageSlot(const QString &strController,
                     KStorageBus enmBus, LONG iPort, LONG iDevice)
        : StorageSlot(enmBus, iPort, iDevice)
        , controller(strController)
    {}
    QString controller;
};


/** Desktop Watchdog / Synthetic Test policy type. */
enum DesktopWatchdogPolicy_SynthTest
{
    DesktopWatchdogPolicy_SynthTest_Disabled,
    DesktopWatchdogPolicy_SynthTest_ManagerOnly,
    DesktopWatchdogPolicy_SynthTest_MachineOnly,
    DesktopWatchdogPolicy_SynthTest_Both
};
Q_DECLARE_METATYPE(DesktopWatchdogPolicy_SynthTest);


#endif /* !FEQT_INCLUDED_SRC_globals_UIDefs_h */

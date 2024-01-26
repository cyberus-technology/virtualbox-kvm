/* $Id: UIDetailsGenerator.h $ */
/** @file
 * VBox Qt GUI - UIDetailsGenerator declaration.
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

#ifndef FEQT_INCLUDED_SRC_globals_UIDetailsGenerator_h
#define FEQT_INCLUDED_SRC_globals_UIDetailsGenerator_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* GUI includes: */
#include "UIExtraDataDefs.h"
#include "UITextTable.h"

/* Forward declarations: */
class CCloudMachine;
class CFormValue;
class CMachine;

/** Details generation namespace. */
namespace UIDetailsGenerator
{
    SHARED_LIBRARY_STUFF UITextTable generateMachineInformationGeneral(CMachine &comMachine,
                                                                       const UIExtraDataMetaDefs::DetailsElementOptionTypeGeneral &fOptions);

    SHARED_LIBRARY_STUFF UITextTable generateMachineInformationGeneral(CCloudMachine &comCloudMachine,
                                                                       const UIExtraDataMetaDefs::DetailsElementOptionTypeGeneral &fOptions);
    SHARED_LIBRARY_STUFF QString generateFormValueInformation(const CFormValue &comFormValue, bool fFull = false);

    SHARED_LIBRARY_STUFF UITextTable generateMachineInformationSystem(CMachine &comMachine,
                                                                      const UIExtraDataMetaDefs::DetailsElementOptionTypeSystem &fOptions);

    SHARED_LIBRARY_STUFF UITextTable generateMachineInformationDisplay(CMachine &comMachine,
                                                                       const UIExtraDataMetaDefs::DetailsElementOptionTypeDisplay &fOptions);

    SHARED_LIBRARY_STUFF UITextTable generateMachineInformationStorage(CMachine &comMachine,
                                                                       const UIExtraDataMetaDefs::DetailsElementOptionTypeStorage &fOptions,
                                                                       bool fLink = true);

    SHARED_LIBRARY_STUFF UITextTable generateMachineInformationAudio(CMachine &comMachine,
                                                                     const UIExtraDataMetaDefs::DetailsElementOptionTypeAudio &fOptions);

    SHARED_LIBRARY_STUFF UITextTable generateMachineInformationNetwork(CMachine &comMachine,
                                                                       const UIExtraDataMetaDefs::DetailsElementOptionTypeNetwork &fOptions);

    SHARED_LIBRARY_STUFF UITextTable generateMachineInformationSerial(CMachine &comMachine,
                                                                      const UIExtraDataMetaDefs::DetailsElementOptionTypeSerial &fOptions);

    SHARED_LIBRARY_STUFF UITextTable generateMachineInformationUSB(CMachine &comMachine,
                                                                   const UIExtraDataMetaDefs::DetailsElementOptionTypeUsb &fOptions);

    SHARED_LIBRARY_STUFF UITextTable generateMachineInformationSharedFolders(CMachine &comMachine,
                                                                             const UIExtraDataMetaDefs::DetailsElementOptionTypeSharedFolders &fOptions);

    SHARED_LIBRARY_STUFF UITextTable generateMachineInformationUI(CMachine &comMachine,
                                                                  const UIExtraDataMetaDefs::DetailsElementOptionTypeUserInterface &fOptions);

    SHARED_LIBRARY_STUFF UITextTable generateMachineInformationDescription(CMachine &comMachine,
                                                                           const UIExtraDataMetaDefs::DetailsElementOptionTypeDescription &fOptions);
}

#endif /* !FEQT_INCLUDED_SRC_globals_UIDetailsGenerator_h */

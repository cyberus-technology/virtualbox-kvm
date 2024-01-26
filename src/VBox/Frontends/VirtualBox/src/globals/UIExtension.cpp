/* $Id: UIExtension.cpp $ */
/** @file
 * VBox Qt GUI - UIExtension namespace implementation.
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

/* GUI includes: */
#include "UICommon.h"
#include "UIExtension.h"
#include "UINotificationCenter.h"
#include "UIMessageCenter.h"
#include "VBoxLicenseViewer.h"

/* COM includes: */
#include "CExtPack.h"


void UIExtension::install(QString const &strFilePath,
                          QString const &strDigest,
                          QWidget *pParent,
                          QString *pstrExtPackName)
{
    /* If the extension pack manager isn't available, skip any attempts to install: */
    CExtPackManager comExtPackManager = uiCommon().virtualBox().GetExtensionPackManager();
    if (comExtPackManager.isNull())
        return;
    /* Open the extpack tarball via IExtPackManager: */
    CExtPackFile comExtPackFile;
    if (strDigest.isEmpty())
        comExtPackFile = comExtPackManager.OpenExtPackFile(strFilePath);
    else
    {
        QString strFileAndHash = QString("%1::SHA-256=%2").arg(strFilePath).arg(strDigest);
        comExtPackFile = comExtPackManager.OpenExtPackFile(strFileAndHash);
    }
    if (!comExtPackManager.isOk())
    {
        UINotificationMessage::cannotOpenExtPack(comExtPackManager, strFilePath);
        return;
    }

    if (!comExtPackFile.GetUsable())
    {
        UINotificationMessage::cannotReadExtPack(comExtPackFile, strFilePath);
        return;
    }

    const QString strPackName = comExtPackFile.GetName();
    const QString strPackDescription = comExtPackFile.GetDescription();
    const QString strPackVersion = QString("%1r%2%3").arg(comExtPackFile.GetVersion()).arg(comExtPackFile.GetRevision()).arg(comExtPackFile.GetEdition());

    /* Check if there is a version of the extension pack already
     * installed on the system and let the user decide what to do about it. */
    CExtPack comExtPackCur = comExtPackManager.Find(strPackName);
    bool fReplaceIt = comExtPackCur.isOk();
    if (fReplaceIt)
    {
        QString strPackVersionCur = QString("%1r%2%3").arg(comExtPackCur.GetVersion()).arg(comExtPackCur.GetRevision()).arg(comExtPackCur.GetEdition());
        if (!msgCenter().confirmReplaceExtensionPack(strPackName, strPackVersion, strPackVersionCur, strPackDescription, pParent))
            return;
    }
    /* If it's a new package just ask for general confirmation. */
    else
    {
        if (!msgCenter().confirmInstallExtensionPack(strPackName, strPackVersion, strPackDescription, pParent))
            return;
    }

    /* Display the license dialog if required by the extension pack. */
    if (comExtPackFile.GetShowLicense())
    {
        QString strLicense = comExtPackFile.GetLicense();
        VBoxLicenseViewer licenseViewer(pParent);
        if (licenseViewer.showLicenseFromString(strLicense) != QDialog::Accepted)
            return;
    }

    /* Install the selected package.
     * Set the package name return value before doing
     * this as the caller should do a refresh even on failure. */
    QString strDisplayInfo;
#ifdef VBOX_WS_WIN
    if (pParent)
        strDisplayInfo.sprintf("hwnd=%#llx", (uint64_t)(uintptr_t)pParent->winId());
#endif

    /* Install extension pack: */
    UINotificationProgressExtensionPackInstall *pNotification =
            new UINotificationProgressExtensionPackInstall(comExtPackFile,
                                                           fReplaceIt,
                                                           strPackName,
                                                           strDisplayInfo);
    QObject::connect(pNotification, &UINotificationProgressExtensionPackInstall::sigExtensionPackInstalled,
                     &uiCommon(), &UICommon::sigExtensionPackInstalled);
    gpNotificationCenter->append(pNotification);

    /* Store the name: */
    if (pstrExtPackName)
        *pstrExtPackName = strPackName;
}

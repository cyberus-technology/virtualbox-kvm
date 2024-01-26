/* $Id: UIDesktopServices_x11.cpp $ */
/** @file
 * VBox Qt GUI - Qt GUI - Utility Classes and Functions specific to X11..
 */

/*
 * Copyright (C) 2010-2023 Oracle and/or its affiliates.
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

/* VBox includes */
#include "UIDesktopServices.h"

/* Qt includes */
#include <QCoreApplication>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QUrl>


bool UIDesktopServices::createMachineShortcut(const QString & /* strSrcFile */, const QString &strDstPath, const QString &strName, const QUuid &uUuid)
{
    QFile link(strDstPath + QDir::separator() + strName + ".desktop");
    if (link.open(QFile::WriteOnly | QFile::Truncate))
    {
        const QString strVBox = QDir::toNativeSeparators(QCoreApplication::applicationDirPath() + "/" + VBOX_GUI_VMRUNNER_IMAGE);
        QTextStream out(&link);
#ifndef VBOX_IS_QT6_OR_LATER /* QTextStream defaults to UTF-8 only since qt6 */
        out.setCodec("UTF-8");
#endif
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
# define QT_ENDL Qt::endl
#else
# define QT_ENDL endl
#endif
        /* Create a link which starts VirtualBox with the machine uuid. */
        out << "[Desktop Entry]" << QT_ENDL
            << "Encoding=UTF-8" << QT_ENDL
            << "Version=1.0" << QT_ENDL
            << "Name=" << strName << QT_ENDL
            << "Comment=Starts the VirtualBox machine " << strName << QT_ENDL
            << "Type=Application" << QT_ENDL
            << "Exec=" << strVBox << " --comment \"" << strName << "\" --startvm \"" << uUuid.toString() << "\"" << QT_ENDL
            << "Icon=virtualbox-vbox.png" << QT_ENDL;
        /* This would be a real file link entry, but then we could also simply
         * use a soft link (on most UNIX fs):
        out << "[Desktop Entry]" << QT_ENDL
            << "Encoding=UTF-8" << QT_ENDL
            << "Version=1.0" << QT_ENDL
            << "Name=" << strName << QT_ENDL
            << "Type=Link" << QT_ENDL
            << "Icon=virtualbox-vbox.png" << QT_ENDL
        */
        link.setPermissions(link.permissions() | QFile::ExeOwner);
        /** @todo r=bird: check status here perhaps, might've run out of disk space or
         *        some such thing... */
        return true;
    }
    return false;
}

bool UIDesktopServices::openInFileManager(const QString &strFile)
{
    QFileInfo fi(strFile);
    return QDesktopServices::openUrl(QUrl("file://" + QDir::toNativeSeparators(fi.absolutePath()), QUrl::TolerantMode));
}


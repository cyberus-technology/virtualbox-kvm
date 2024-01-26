/* $Id: UIWizardNewVD.h $ */
/** @file
 * VBox Qt GUI - UIWizardNewVD class declaration.
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

#ifndef FEQT_INCLUDED_SRC_wizards_newvd_UIWizardNewVD_h
#define FEQT_INCLUDED_SRC_wizards_newvd_UIWizardNewVD_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* GUI includes: */
#include "UINativeWizard.h"

/* COM includes: */
#include "COMEnums.h"
#include "CMedium.h"
#include "CMediumFormat.h"

/** New Virtual Disk wizard. */
class SHARED_LIBRARY_STUFF UIWizardNewVD : public UINativeWizard
{
    Q_OBJECT;

public:

    UIWizardNewVD(QWidget *pParent,
                  const QString &strDefaultName, const QString &strDefaultPath,
                  qulonglong uDefaultSize, WizardMode mode = WizardMode_Auto);

    bool createVirtualDisk();

    /** Creates and shows a UIWizardNewVD wizard.
      * @param  pParent                   Passes the parent of the wizard,
      * @param  strMachineFolder          Passes the machine folder,
      * @param  strMachineName            Passes the name of the machine,
      * @param  strMachineGuestOSTypeId   Passes the string of machine's guest OS type ID,
      * returns QUuid of the created medium. */
    static QUuid createVDWithWizard(QWidget *pParent,
                                    const QString &strMachineFolder = QString(),
                                    const QString &strMachineName = QString(),
                                    const QString &strMachineGuestOSTypeId = QString());

    /** @name Setter/getters for virtual disk parameters
     * @{ */
       qulonglong mediumVariant() const;
       void setMediumVariant(qulonglong uMediumVariant);

       const CMediumFormat &mediumFormat();
       void setMediumFormat(const CMediumFormat &mediumFormat);

       const QString &mediumPath() const;
       void setMediumPath(const QString &strMediumPath);

       qulonglong mediumSize() const;
       void setMediumSize(qulonglong mediumSize);

       QUuid mediumId() const;
    /** @} */

protected:

    virtual void populatePages() /* final override */;

private:

    void retranslateUi();
    /** Check medium capabilities and decide if medium variant page should be hidden. */
    void setMediumVariantPageVisibility();
    qulonglong m_uMediumVariant;
    CMediumFormat m_comMediumFormat;
    QString m_strMediumPath;
    qulonglong m_uMediumSize;
    QString     m_strDefaultName;
    QString     m_strDefaultPath;
    qulonglong  m_uDefaultSize;
    int m_iMediumVariantPageIndex;
    QUuid m_uMediumId;
};

typedef QPointer<UIWizardNewVD> UISafePointerWizardNewVD;

#endif /* !FEQT_INCLUDED_SRC_wizards_newvd_UIWizardNewVD_h */

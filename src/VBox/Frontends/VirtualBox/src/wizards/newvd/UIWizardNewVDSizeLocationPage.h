/* $Id: UIWizardNewVDSizeLocationPage.h $ */
/** @file
 * VBox Qt GUI - UIWizardNewVDSizeLocationPage class declaration.
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

#ifndef FEQT_INCLUDED_SRC_wizards_newvd_UIWizardNewVDSizeLocationPage_h
#define FEQT_INCLUDED_SRC_wizards_newvd_UIWizardNewVDSizeLocationPage_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* GUI includes: */
#include "UINativeWizardPage.h"

/* Forward declarations: */
class UIMediumSizeAndPathGroupBox;

class SHARED_LIBRARY_STUFF UIWizardNewVDSizeLocationPage : public UINativeWizardPage
{
    Q_OBJECT;

public:

    UIWizardNewVDSizeLocationPage(const QString &strDefaultName, const QString &strDefaultPath, qulonglong uDefaultSize);

private slots:

    void sltSelectLocationButtonClicked();
    void sltMediumSizeChanged(qulonglong uSize);
    void sltMediumPathChanged(const QString &strPath);

private:

    void retranslateUi();
    void initializePage();
    bool isComplete() const;
    bool validatePage();
    void prepare();

    UIMediumSizeAndPathGroupBox *m_pMediumSizePathGroup;
    qulonglong m_uMediumSizeMin;
    qulonglong m_uMediumSizeMax;
    QString m_strDefaultName;
    QString m_strDefaultPath;
    qulonglong m_uDefaultSize;
    QSet<QString> m_userModifiedParameters;
};


#endif /* !FEQT_INCLUDED_SRC_wizards_newvd_UIWizardNewVDSizeLocationPage_h */

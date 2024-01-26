/* $Id: UIWizardCloneVDPathSizePage.h $ */
/** @file
 * VBox Qt GUI - UIWizardCloneVDPathSizePage class declaration.
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

#ifndef FEQT_INCLUDED_SRC_wizards_clonevd_UIWizardCloneVDPathSizePage_h
#define FEQT_INCLUDED_SRC_wizards_clonevd_UIWizardCloneVDPathSizePage_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* Qt includes: */
#include <QVariant>
#include <QSet>

/* GUI includes: */
#include "UINativeWizardPage.h"

/* COM includes: */
#include "COMEnums.h"

/* Forward declarations: */
class UIMediumSizeAndPathGroupBox;

/** 4th page of the Clone Virtual Disk Image wizard (basic extension): */
class UIWizardCloneVDPathSizePage : public UINativeWizardPage
{
    Q_OBJECT;

public:

    /** Constructs basic page. */
    UIWizardCloneVDPathSizePage(qulonglong uSourceDiskLogicaSize);

private slots:

    /** Handles command to open target disk. */
    void sltSelectLocationButtonClicked();
    void sltMediumPathChanged(const QString &strPath);
    void sltMediumSizeChanged(qulonglong uSize);

private:

    /** Handles translation event. */
    virtual void retranslateUi() RT_OVERRIDE;
    void prepare(qulonglong uSourceDiskLogicaSize);

    /** Prepares the page. */
    virtual void initializePage() RT_OVERRIDE;

    /** Returns whether the page is complete. */
    virtual bool isComplete() const RT_OVERRIDE;

    /** Returns whether the page is valid. */
    virtual bool validatePage() RT_OVERRIDE;

    UIMediumSizeAndPathGroupBox *m_pMediumSizePathGroupBox;
    QSet<QString> m_userModifiedParameters;
};

#endif /* !FEQT_INCLUDED_SRC_wizards_clonevd_UIWizardCloneVDPathSizePage_h */

/* $Id: UIWizardCloneVD.h $ */
/** @file
 * VBox Qt GUI - UIWizardCloneVD class declaration.
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

#ifndef FEQT_INCLUDED_SRC_wizards_clonevd_UIWizardCloneVD_h
#define FEQT_INCLUDED_SRC_wizards_clonevd_UIWizardCloneVD_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* GUI includes: */
#include "UINativeWizard.h"

/* COM includes: */
#include "COMEnums.h"
#include "CMedium.h"
#include "CMediumFormat.h"

/** Clone Virtual Disk wizard: */
class UIWizardCloneVD : public UINativeWizard
{
    Q_OBJECT;

public:

    /** Constructs wizard to clone @a comSourceVirtualDisk passing @a pParent to the base-class. */
    UIWizardCloneVD(QWidget *pParent, const CMedium &comSourceVirtualDisk);

    /** Returns source virtual-disk. */
    const CMedium &sourceVirtualDisk() const;

    /** Makes a copy of source virtual-disk. */
    bool copyVirtualDisk();

    /** @name Parameter setter/getters
      * @{ */
        /** Returns the source virtual-disk device type. */
        KDeviceType deviceType() const;

        const CMediumFormat &mediumFormat() const;
        void setMediumFormat(const CMediumFormat &comMediumFormat);

        qulonglong mediumVariant() const;
        void setMediumVariant(qulonglong uMediumVariant);

        qulonglong mediumSize() const;
        void setMediumSize(qulonglong uMediumSize);

        const QString &mediumPath() const;
        void setMediumPath(const QString &strPath);

        qulonglong sourceDiskLogicalSize() const;
        QString sourceDiskFilePath() const;
        QString sourceDiskName() const;
   /** @} */

protected:

    virtual void populatePages() /* final override */;

private:

    /** Handles translation event. */
    virtual void retranslateUi() RT_OVERRIDE;
    void setMediumVariantPageVisibility();

    /** @name Parameters needed during medium cloning
      * @{ */
        CMediumFormat m_comMediumFormat;
        qulonglong m_uMediumVariant;
        /** Holds the source virtual disk wrapper. */
        CMedium m_comSourceVirtualDisk;

        /** Holds the source virtual-disk device type. */
        KDeviceType m_enmDeviceType;
        int m_iMediumVariantPageIndex;
        qulonglong m_uMediumSize;
        QString m_strMediumPath;
        QString m_strSourceDiskPath;
    /** @} */
};

#endif /* !FEQT_INCLUDED_SRC_wizards_clonevd_UIWizardCloneVD_h */

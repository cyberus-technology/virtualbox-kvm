/* $Id: UIWizardCloneVDVariantPage.h $ */
/** @file
 * VBox Qt GUI - UIWizardCloneVDVariantPage class declaration.
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

#ifndef FEQT_INCLUDED_SRC_wizards_clonevd_UIWizardCloneVDVariantPage_h
#define FEQT_INCLUDED_SRC_wizards_clonevd_UIWizardCloneVDVariantPage_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* GUI includes: */
#include "UINativeWizardPage.h"

/* Forward declarations: */
class QIRichTextLabel;
class CMediumFormat;
class UIDiskVariantWidget;

class UIWizardCloneVDVariantPage : public UINativeWizardPage
{
    Q_OBJECT;

public:

    /** Constructs basic page. */
    UIWizardCloneVDVariantPage();

private slots:

    void sltMediumVariantChanged(qulonglong uVariant);

private:

    /** Handles translation event. */
    virtual void retranslateUi() RT_OVERRIDE;
    void prepare();

    /** Prepares the page. */
    virtual void initializePage() RT_OVERRIDE;

    /** Returns whether the page is complete. */
    virtual bool isComplete() const RT_OVERRIDE;
    void setWidgetVisibility(const CMediumFormat &mediumFormat);

    /** Holds the description label instance. */
    QIRichTextLabel *m_pDescriptionLabel;
    /** Holds the 'Dynamic' description label instance. */
    QIRichTextLabel *m_pDynamicLabel;
    /** Holds the 'Fixed' description label instance. */
    QIRichTextLabel *m_pFixedLabel;
    /** Holds the 'Split to 2GB files' description label instance. */
    QIRichTextLabel *m_pSplitLabel;
    UIDiskVariantWidget *m_pVariantWidget;
};

#endif /* !FEQT_INCLUDED_SRC_wizards_clonevd_UIWizardCloneVDVariantPage_h */

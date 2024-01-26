/* $Id: UICloudMachineSettingsDialogPage.h $ */
/** @file
 * VBox Qt GUI - UICloudMachineSettingsDialogPage class declaration.
 */

/*
 * Copyright (C) 2020-2023 Oracle and/or its affiliates.
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

#ifndef FEQT_INCLUDED_SRC_cloud_machinesettings_UICloudMachineSettingsDialogPage_h
#define FEQT_INCLUDED_SRC_cloud_machinesettings_UICloudMachineSettingsDialogPage_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* Qt includes: */
#include <QPointer>
#include <QWidget>

/* GUI includes: */
#include "QIWithRetranslateUI.h"
#include "UIFormEditorWidget.h"

/* COM includes: */
#include "COMEnums.h"
#include "CForm.h"

/* Forward declarations: */
class UICloudMachineSettingsDialog;

/** Cloud machine settings dialog page. */
class UICloudMachineSettingsDialogPage : public QIWithRetranslateUI<QWidget>
{
    Q_OBJECT;

signals:

    /** Notifies listeners about value has became @a fValid. */
    void sigValidChanged(bool fValid);

public:

    /** Constructs cloud machine settings dialog page passing @a pParent to the base-class.
      * @param  fFullScale  Brings whether this page is full-scale and should reflect at least 12 fields. */
    UICloudMachineSettingsDialogPage(QWidget *pParent, bool fFullScale = true);

    /** Returns page form. */
    CForm form() const { return m_comForm; }
    /** Returns page filter. */
    QString filter() const { return m_strFilter; }

public slots:

    /** Defines page @a comForm. */
    void setForm(const CForm &comForm);
    /** Defines page @a strFilter. */
    void setFilter(const QString &strFilter);

    /** Makes sure page data committed. */
    void makeSureDataCommitted();

protected:

    /** Handles translation event. */
    virtual void retranslateUi() RT_OVERRIDE;

private:

    /** Prepares all. */
    void prepare();

    /** Updates editor. */
    void updateEditor();

    /** Holds the parent cloud machine settings dialog reference. */
    UICloudMachineSettingsDialog *m_pParent;

    /** Holds whether this page is full-scale and should reflect at least 12 fields. */
    bool  m_fFullScale;

    /** Holds the form editor widget instance. */
    UIFormEditorWidgetPointer  m_pFormEditor;

    /** Holds the page form. */
    CForm    m_comForm;
    /** Holds the page filter. */
    QString  m_strFilter;
};

/** Safe pointer to Form Editor widget. */
typedef QPointer<UICloudMachineSettingsDialogPage> UISafePointerCloudMachineSettingsDialogPage;

#endif /* !FEQT_INCLUDED_SRC_cloud_machinesettings_UICloudMachineSettingsDialogPage_h */

/* $Id: UIMachineSettingsPortForwardingDlg.h $ */
/** @file
 * VBox Qt GUI - UIMachineSettingsPortForwardingDlg class declaration.
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

#ifndef FEQT_INCLUDED_SRC_settings_machine_UIMachineSettingsPortForwardingDlg_h
#define FEQT_INCLUDED_SRC_settings_machine_UIMachineSettingsPortForwardingDlg_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* GUI includes: */
#include "QIWithRetranslateUI.h"
#include "QIDialog.h"
#include "UIPortForwardingTable.h"

/* Forward declarations: */
class QIDialogButtonBox;

/* Machine settings / Network page / NAT attachment / Port forwarding dialog: */
class SHARED_LIBRARY_STUFF UIMachineSettingsPortForwardingDlg : public QIWithRetranslateUI<QIDialog>
{
    Q_OBJECT;

public:

    /* Constructor/destructor: */
    UIMachineSettingsPortForwardingDlg(QWidget *pParent, const UIPortForwardingDataList &rules);

    /* API: Rules stuff: */
    const UIPortForwardingDataList rules() const;

private slots:

    /* Handlers: Dialog stuff: */
    void accept();
    void reject();

private:

    /* Handler: Translation stuff: */
    void retranslateUi();

    /* Widgets: */
    UIPortForwardingTable *m_pTable;
    QIDialogButtonBox *m_pButtonBox;
};

#endif /* !FEQT_INCLUDED_SRC_settings_machine_UIMachineSettingsPortForwardingDlg_h */

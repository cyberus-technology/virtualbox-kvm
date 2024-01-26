/* $Id: UIInformationRuntime.h $ */
/** @file
 * VBox Qt GUI - UIInformationRuntime class declaration.
 */

/*
 * Copyright (C) 2016-2023 Oracle and/or its affiliates.
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

#ifndef FEQT_INCLUDED_SRC_runtime_information_UIInformationRuntime_h
#define FEQT_INCLUDED_SRC_runtime_information_UIInformationRuntime_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* Qt includes: */
#include <QWidget>

/* COM includes: */
#include "COMEnums.h"
#include "CConsole.h"
#include "CGuest.h"
#include "CMachine.h"

/* GUI includes: */
#include "QIWithRetranslateUI.h"

/* Forward declarations: */
class QAction;
class QVBoxLayout;
class UISession;
class UIRuntimeInfoWidget;

/** UIInformationRuntime class displays a table including some
  * run time attributes. */
class UIInformationRuntime : public QIWithRetranslateUI<QWidget>
{
    Q_OBJECT;

public:

    /** Constructs information-tab passing @a pParent to the QWidget base-class constructor.
      * @param machine is machine reference.
      * @param console is machine console reference. */
    UIInformationRuntime(QWidget *pParent, const CMachine &machine, const CConsole &console, const UISession *pSession);

protected:

    void retranslateUi();

private slots:

    /** @name These functions are connected to API events and implement necessary updates on the table.
      * @{ */
        void sltGuestAdditionsStateChange();
        void sltGuestMonitorChange(KGuestMonitorChangedEventType changeType, ulong uScreenId, QRect screenGeo);
        void sltVRDEChange();
        void sltClipboardChange(KClipboardMode enmMode);
        void sltDnDModeChange(KDnDMode enmMode);
    /** @} */
    void sltHandleTableContextMenuRequest(const QPoint &position);
    void sltHandleCopyWholeTable();

private:

    void prepareObjects();

    CMachine m_machine;
    CConsole m_console;
    CGuest m_comGuest;

    /** Holds the instance of layout we create. */
    QVBoxLayout *m_pMainLayout;
    UIRuntimeInfoWidget *m_pRuntimeInfoWidget;
    QAction *m_pCopyWholeTableAction;
};

#endif /* !FEQT_INCLUDED_SRC_runtime_information_UIInformationRuntime_h */

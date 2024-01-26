/* $Id: UITools.h $ */
/** @file
 * VBox Qt GUI - UITools class declaration.
 */

/*
 * Copyright (C) 2012-2023 Oracle and/or its affiliates.
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

#ifndef FEQT_INCLUDED_SRC_manager_tools_UITools_h
#define FEQT_INCLUDED_SRC_manager_tools_UITools_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* Qt includes: */
#include <QWidget>

/* GUI icludes: */
#include "UIExtraDataDefs.h"

/* Forward declarations: */
class QVBoxLayout;
class UIActionPool;
class UIToolsItem;
class UIToolsModel;
class UIToolsView;
class UIVirtualBoxManagerWidget;

/** QWidget extension used as VM Tools-pane. */
class UITools : public QWidget
{
    Q_OBJECT;

signals:

    /** @name General stuff.
      * @{ */
        /** Notifies listeners about selection changed. */
        void sigSelectionChanged();

        /** Notifies listeners about expanding started. */
        void sigExpandingStarted();
        /** Notifies listeners about expanding finished. */
        void sigExpandingFinished();
    /** @} */

public:

    /** Constructs Tools-pane passing @a pParent to the base-class. */
    UITools(UIVirtualBoxManagerWidget *pParent = 0);

    /** @name General stuff.
      * @{ */
        /** Returns the manager-widget reference. */
        UIVirtualBoxManagerWidget *managerWidget() const { return m_pManagerWidget; }

        /** Returns the action-pool reference. */
        UIActionPool *actionPool() const;

        /** Return the Tools-model instance. */
        UIToolsModel *model() const { return m_pToolsModel; }
        /** Return the Tools-view instance. */
        UIToolsView *view() const { return m_pToolsView; }

        /** Defines current tools @a enmClass. */
        void setToolsClass(UIToolClass enmClass);
        /** Returns current tools class. */
        UIToolClass toolsClass() const;

        /** Defines current tools @a enmType. */
        void setToolsType(UIToolType enmType);
        /** Returns current tools type. */
        UIToolType toolsType() const;

        /** Returns last selected global tool. */
        UIToolType lastSelectedToolGlobal() const;
        /** Returns last selected machine tool. */
        UIToolType lastSelectedToolMachine() const;

        /** Defines whether certain @a enmClass of tools is @a fEnabled.*/
        void setToolClassEnabled(UIToolClass enmClass, bool fEnabled);
        /** Returns whether certain class of tools is enabled.*/
        bool toolClassEnabled(UIToolClass enmClass) const;

        /** Defines restructed tool @a types. */
        void setRestrictedToolTypes(const QList<UIToolType> &types);
        /** Returns restricted tool types. */
        QList<UIToolType> restrictedToolTypes() const;
    /** @} */

    /** @name Current item stuff.
      * @{ */
        /** Returns current item. */
        UIToolsItem *currentItem() const;
    /** @} */

private:

    /** @name Prepare/Cleanup cascade.
      * @{ */
        /** Prepares all. */
        void prepare();
        /** Prepares contents. */
        void prepareContents();
        /** Prepares model. */
        void prepareModel();
        /** Prepares view. */
        void prepareView();
        /** Prepares connections. */
        void prepareConnections();
        /** Inits model. */
        void initModel();
    /** @} */

    /** @name General stuff.
      * @{ */
        /** Holds the manager-widget reference. */
        UIVirtualBoxManagerWidget *m_pManagerWidget;

        /** Holds the main layout instane. */
        QVBoxLayout  *m_pMainLayout;
        /** Holds the Tools-model instane. */
        UIToolsModel *m_pToolsModel;
        /** Holds the Tools-view instane. */
        UIToolsView  *m_pToolsView;
    /** @} */
};

#endif /* !FEQT_INCLUDED_SRC_manager_tools_UITools_h */

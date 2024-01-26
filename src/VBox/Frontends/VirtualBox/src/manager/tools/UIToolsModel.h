/* $Id: UIToolsModel.h $ */
/** @file
 * VBox Qt GUI - UIToolsModel class declaration.
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

#ifndef FEQT_INCLUDED_SRC_manager_tools_UIToolsModel_h
#define FEQT_INCLUDED_SRC_manager_tools_UIToolsModel_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* Qt includes: */
#include <QMap>
#include <QObject>
#include <QPointer>
#include <QTransform>

/* GUI includes: */
#include "QIWithRetranslateUI.h"
#include "UIToolsItem.h"

/* COM includes: */
#include "COMEnums.h"

/* Forward declaration: */
class QGraphicsItem;
class QGraphicsScene;
class QGraphicsSceneContextMenuEvent;
class QMenu;
class QPaintDevice;
class QTimer;
class UIActionPool;
class UITools;
class UIToolsHandlerMouse;
class UIToolsHandlerKeyboard;

/** QObject extension used as VM Tools-pane model: */
class UIToolsModel : public QIWithRetranslateUI3<QObject>
{
    Q_OBJECT;

signals:

    /** @name Selection stuff.
      * @{ */
        /** Notifies about selection changed. */
        void sigSelectionChanged();
        /** Notifies about focus changed. */
        void sigFocusChanged();

        /** Notifies about group expanding started. */
        void sigExpandingStarted();
        /** Notifies about group expanding finished. */
        void sigExpandingFinished();
    /** @} */

    /** @name Layout stuff.
      * @{ */
        /** Notifies about item minimum width @a iHint changed. */
        void sigItemMinimumWidthHintChanged(int iHint);
        /** Notifies about item minimum height @a iHint changed. */
        void sigItemMinimumHeightHintChanged(int iHint);
    /** @} */

public:

    /** Constructs Tools-model passing @a pParent to the base-class. */
    UIToolsModel(UITools *pParent);
    /** Destructs Tools-model. */
    virtual ~UIToolsModel() RT_OVERRIDE;

    /** @name General stuff.
      * @{ */
        /** Inits model. */
        void init();

        /** Returns the Tools reference. */
        UITools *tools() const;
        /** Returns the action-pool reference. */
        UIActionPool *actionPool() const;
        /** Returns the scene reference. */
        QGraphicsScene *scene() const;
        /** Returns the paint device reference. */
        QPaintDevice *paintDevice() const;

        /** Returns item at @a position, taking into account possible @a deviceTransform. */
        QGraphicsItem *itemAt(const QPointF &position, const QTransform &deviceTransform = QTransform()) const;

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

        /** Closes parent. */
        void closeParent();
    /** @} */

    /** @name Children stuff.
      * @{ */
        /** Returns the item list. */
        QList<UIToolsItem*> items() const;

        /** Returns the item of passed @a enmType. */
        UIToolsItem *item(UIToolType enmType) const;
    /** @} */

    /** @name Selection stuff.
      * @{ */
        /** Defines current @a pItem. */
        void setCurrentItem(UIToolsItem *pItem);
        /** Returns current item. */
        UIToolsItem *currentItem() const;

        /** Defines focus @a pItem. */
        void setFocusItem(UIToolsItem *pItem);
        /** Returns focus item. */
        UIToolsItem *focusItem() const;
    /** @} */

    /** @name Navigation stuff.
      * @{ */
        /** Returns navigation item list. */
        const QList<UIToolsItem*> &navigationList() const;
        /** Removes @a pItem from navigation list. */
        void removeFromNavigationList(UIToolsItem *pItem);
        /** Updates navigation list. */
        void updateNavigation();
    /** @} */

    /** @name Layout stuff.
      * @{ */
        /** Updates layout. */
        void updateLayout();
    /** @} */

public slots:

    /** @name General stuff.
      * @{ */
        /** Handles Tools-view resize. */
        void sltHandleViewResized();
    /** @} */

    /** @name Children stuff.
      * @{ */
        /** Handles minimum width hint change. */
        void sltItemMinimumWidthHintChanged();
        /** Handles minimum height hint change. */
        void sltItemMinimumHeightHintChanged();
    /** @} */

protected:

    /** @name Event handling stuff.
      * @{ */
        /** Preprocesses Qt @a pEvent for passed @a pObject. */
        virtual bool eventFilter(QObject *pObject, QEvent *pEvent) RT_OVERRIDE;

        /** Handles translation event. */
        virtual void retranslateUi() RT_OVERRIDE;
    /** @} */

private slots:

    /** @name Selection stuff.
      * @{ */
        /** Handles focus item destruction. */
        void sltFocusItemDestroyed();
    /** @} */

private:

    /** Data field types. */
    enum ToolsModelData
    {
        /* Layout hints: */
        ToolsModelData_Margin,
        ToolsModelData_Spacing,
    };

    /** @name Prepare/Cleanup cascade.
      * @{ */
        /** Prepares all. */
        void prepare();
        /** Prepares scene. */
        void prepareScene();
        /** Prepares items. */
        void prepareItems();
        /** Prepares handlers. */
        void prepareHandlers();
        /** Prepares connections. */
        void prepareConnections();
        /** Loads settings. */
        void loadSettings();

        /** Cleanups connections. */
        void cleanupConnections();
        /** Cleanups connections. */
        void cleanupHandlers();
        /** Cleanups items. */
        void cleanupItems();
        /** Cleanups scene. */
        void cleanupScene();
        /** Cleanups all. */
        void cleanup();
    /** @} */

    /** @name General stuff.
      * @{ */
        /** Returns abstractly stored data value for certain @a iKey. */
        QVariant data(int iKey) const;
    /** @} */

    /** @name General stuff.
      * @{ */
        /** Holds the Tools reference. */
        UITools *m_pTools;

        /** Holds the scene reference. */
        QGraphicsScene *m_pScene;

        /** Holds the mouse handler instance. */
        UIToolsHandlerMouse    *m_pMouseHandler;
        /** Holds the keyboard handler instance. */
        UIToolsHandlerKeyboard *m_pKeyboardHandler;

        /** Holds current tools class. */
        UIToolClass  m_enmCurrentClass;

        /** Holds whether tools of particular class are enabled. */
        QMap<UIToolClass, bool>  m_enabledToolClasses;

        /** Holds a list of restricted tool types. */
        QList<UIToolType>  m_restrictedToolTypes;
    /** @} */

    /** @name Children stuff.
      * @{ */
        /** Holds the root stack. */
        QList<UIToolsItem*>  m_items;
    /** @} */

    /** @name Selection stuff.
      * @{ */
        /** Holds the selected item reference. */
        QPointer<UIToolsItem> m_pCurrentItem;
        /** Holds the focus item reference. */
        QPointer<UIToolsItem> m_pFocusItem;
    /** @} */

    /** @name Navigation stuff.
      * @{ */
        /** Holds the navigation list. */
        QList<UIToolsItem*>  m_navigationList;

        /** Holds the last chosen navigation item of global class. */
        QPointer<UIToolsItem> m_pLastItemGlobal;
        /** Holds the last chosen navigation item of machine class. */
        QPointer<UIToolsItem> m_pLastItemMachine;
    /** @} */
};

#endif /* !FEQT_INCLUDED_SRC_manager_tools_UIToolsModel_h */

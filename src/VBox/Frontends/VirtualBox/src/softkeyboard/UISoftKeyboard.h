/* $Id: UISoftKeyboard.h $ */
/** @file
 * VBox Qt GUI - UISoftKeyboard class declaration.
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

#ifndef FEQT_INCLUDED_SRC_softkeyboard_UISoftKeyboard_h
#define FEQT_INCLUDED_SRC_softkeyboard_UISoftKeyboard_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* Qt includes: */
#include <QMainWindow>

/* COM includes: */
#include "COMDefs.h"

/* GUI includes: */
#include "QIWithRestorableGeometry.h"
#include "QIWithRetranslateUI.h"

/* Forward declarations: */
class CKeyboard;
class QHBoxLayout;
class QToolButton;
class UIKeyboardLayoutEditor;
class UILayoutSelector;
class UISession;
class UISoftKeyboardKey;
class UISoftKeyboardSettingsWidget;
class UISoftKeyboardStatusBarWidget;
class UISoftKeyboardWidget;
class QSplitter;
class QStackedWidget;

/* Type definitions: */
typedef QIWithRestorableGeometry<QMainWindow> QMainWindowWithRestorableGeometry;
typedef QIWithRetranslateUI<QMainWindowWithRestorableGeometry> QMainWindowWithRestorableGeometryAndRetranslateUi;

class UISoftKeyboard : public QMainWindowWithRestorableGeometryAndRetranslateUi
{
    Q_OBJECT;

signals:

    void sigHelpRequested(const QString &strHelpKeyword);
    void sigClose();

public:

    UISoftKeyboard(QWidget *pParent, UISession *pSession, QWidget *pCenterWidget,
                   QString strMachineName = QString());
    ~UISoftKeyboard();

protected:

    virtual void retranslateUi() RT_OVERRIDE;
    virtual bool shouldBeMaximized() const RT_OVERRIDE;
    virtual void closeEvent(QCloseEvent *event) RT_OVERRIDE;
    bool event(QEvent *pEvent) RT_OVERRIDE;

private slots:

    void sltKeyboardLedsChange();
    void sltPutKeyboardSequence(QVector<LONG> sequence);
    void sltPutUsageCodesPress(QVector<QPair<LONG, LONG> > sequence);
    void sltPutUsageCodesRelease(QVector<QPair<LONG, LONG> > sequence);

    /** Handles the signal we get from the layout selector widget.
      * Selection changed is forwarded to the keyboard widget. */
    void sltLayoutSelectionChanged(const QUuid &layoutUid);
    /** Handles the signal we get from the keyboard widget. */
    void sltCurentLayoutChanged();
    void sltShowLayoutSelector();
    void sltShowLayoutEditor();
    void sltKeyToEditChanged(UISoftKeyboardKey* pKey);
    void sltLayoutEdited();
    /** Make th necessary changes to data structures when th key captions updated. */
    void sltKeyCaptionsEdited(UISoftKeyboardKey* pKey);
    void sltShowHideSidePanel();
    void sltShowHideSettingsWidget();
    void sltHandleColorThemeListSelection(const QString &strColorThemeName);
    void sltHandleKeyboardWidgetColorThemeChange();
    void sltCopyLayout();
    void sltSaveLayout();
    void sltDeleteLayout();
    void sltStatusBarMessage(const QString &strMessage);
    void sltShowHideOSMenuKeys(bool fShow);
    void sltShowHideNumPad(bool fShow);
    void sltShowHideMultimediaKeys(bool fHide);
    void sltHandleColorCellClick(int iColorRow);
    void sltResetKeyboard();
    void sltHandleHelpRequest();
    void sltSaveSettings();
    void sltReleaseKeys();

private:

    void prepareObjects();
    void prepareConnections();
    void loadSettings();
    void saveCustomColorTheme();
    void saveSelectedColorThemeName();
    void saveCurrentLayout();
    void saveDialogGeometry();
    void configure();
    void updateStatusBarMessage(const QString &strLayoutName);
    void updateLayoutSelectorList();
    CKeyboard& keyboard() const;

    UISession     *m_pSession;
    QWidget       *m_pCenterWidget;
    QHBoxLayout   *m_pMainLayout;
    QString        m_strMachineName;
    QSplitter      *m_pSplitter;
    QStackedWidget *m_pSidePanelWidget;
    UISoftKeyboardWidget   *m_pKeyboardWidget;
    UIKeyboardLayoutEditor *m_pLayoutEditor;
    UILayoutSelector       *m_pLayoutSelector;

    UISoftKeyboardSettingsWidget  *m_pSettingsWidget;
    UISoftKeyboardStatusBarWidget *m_pStatusBarWidget;
    int m_iGeometrySaveTimerId;
};

#endif /* !FEQT_INCLUDED_SRC_softkeyboard_UISoftKeyboard_h */

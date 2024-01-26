/* $Id: UISettingsDialog.h $ */
/** @file
 * VBox Qt GUI - UISettingsDialog class declaration.
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

#ifndef FEQT_INCLUDED_SRC_settings_UISettingsDialog_h
#define FEQT_INCLUDED_SRC_settings_UISettingsDialog_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* Qt includes: */
#include <QMainWindow>
#include <QPointer>
#include <QVariant>

/* GUI includes: */
#include "QIWithRetranslateUI.h"
#include "UISettingsDefs.h"

/* Forward declarations: */
class QEvent;
class QObject;
class QLabel;
class QProgressBar;
class QShowEvent;
class QStackedWidget;
class QTimer;
class QIDialogButtonBox;
class UIPageValidator;
class UISettingsPage;
class UISettingsSelector;
class UISettingsSerializer;
class UIWarningPane;

/* Using declarations: */
using namespace UISettingsDefs;

/** QMainWindow subclass used as
  * base dialog class for both Global Preferences & Machine Settings
  * dialogs, which encapsulates most of their common functionality. */
class SHARED_LIBRARY_STUFF UISettingsDialog : public QIWithRetranslateUI<QMainWindow>
{
    Q_OBJECT;

signals:

    /** Notifies listeners about dialog should be closed. */
    void sigClose();

public:

    /** Dialog types. */
    enum DialogType { DialogType_Global, DialogType_Machine };

    /** Constructs settings dialog passing @a pParent to the base-class.
      * @param  strCategory  Brings the name of category to be opened.
      * @param  strControl   Brings the name of control to be focused. */
    UISettingsDialog(QWidget *pParent,
                     const QString &strCategory,
                     const QString &strControl);
    /** Destructs settings dialog. */
    virtual ~UISettingsDialog() RT_OVERRIDE;

    /** Returns dialog type. */
    virtual DialogType dialogType() const = 0;

    /** Loads the dialog data. */
    virtual void load() = 0;
    /** Saves the dialog data. */
    virtual void save() = 0;

protected slots:

    /** Hides the modal dialog and sets the result code to Accepted. */
    virtual void accept();
    /** Hides the modal dialog and sets the result code to Rejected. */
    virtual void reject();

    /** Handles category change to @a cId. */
    virtual void sltCategoryChanged(int cId);

    /** Marks dialog loaded. */
    virtual void sltMarkLoaded();
    /** Marks dialog saved. */
    virtual void sltMarkSaved();

    /** Handles process start. */
    void sltHandleProcessStarted();
    /** Handles process progress change to @a iValue. */
    void sltHandleProcessProgressChange(int iValue);

protected:

    /** Preprocesses any Qt @a pEvent for passed @a pObject. */
    virtual bool eventFilter(QObject *pObject, QEvent *pEvent) RT_OVERRIDE;
    /** Handles translation event. */
    virtual void retranslateUi() RT_OVERRIDE;
    /** Handles show @a pEvent. */
    virtual void showEvent(QShowEvent *pEvent) RT_OVERRIDE;
    /** Handles first show @a pEvent. */
    virtual void polishEvent(QShowEvent *pEvent);
    /** Handles close @a pEvent. */
    virtual void closeEvent(QCloseEvent *pEvent) RT_OVERRIDE;

    /** Selects page and tab.
      * @param  fKeepPreviousByDefault  Brings whether we should keep current page/tab by default. */
    void choosePageAndTab(bool fKeepPreviousByDefault = false);

    /** Loads the dialog @a data. */
    void loadData(QVariant &data);
    /** Saves the dialog @a data. */
    void saveData(QVariant &data);

    /** Returns configuration access level. */
    ConfigurationAccessLevel configurationAccessLevel() const { return m_enmConfigurationAccessLevel; }
    /** Defines configuration access level. */
    void setConfigurationAccessLevel(ConfigurationAccessLevel enmConfigurationAccessLevel);

    /** Returns the serialize process instance. */
    UISettingsSerializer *serializeProcess() const { return m_pSerializeProcess; }
    /** Returns whether the serialization is in progress. */
    bool isSerializationInProgress() const { return m_fSerializationIsInProgress; }

    /** Returns the dialog title extension. */
    virtual QString titleExtension() const = 0;
    /** Returns the dialog title. */
    virtual QString title() const = 0;

    /** Adds an item (page).
      * @param  strBigIcon     Brings the big icon.
      * @param  strMediumIcon  Brings the medium icon.
      * @param  strSmallIcon   Brings the small icon.
      * @param  cId            Brings the page ID.
      * @param  strLink        Brings the page link.
      * @param  pSettingsPage  Brings the page reference.
      * @param  iParentId      Brings the page parent ID. */
    void addItem(const QString &strBigIcon, const QString &strMediumIcon, const QString &strSmallIcon,
                 int cId, const QString &strLink,
                 UISettingsPage* pSettingsPage = 0, int iParentId = -1);

    /** Verifies data integrity between certain @a pSettingsPage and other pages. */
    virtual void recorrelate(UISettingsPage *pSettingsPage) { Q_UNUSED(pSettingsPage); }

    /** Inserts an item to the map m_pageHelpKeywords. */
    void addPageHelpKeyword(int iPageType, const QString &strHelpKeyword);

    /** Validates data correctness using certain @a pValidator. */
    void revalidate(UIPageValidator *pValidator);
    /** Validates data correctness. */
    void revalidate();

    /** Returns whether settings were changed. */
    bool isSettingsChanged();

    /** Holds the name of category to be opened. */
    QString  m_strCategory;
    /** Holds the name of control to be focused. */
    QString  m_strControl;

    /** Holds the page selector instance. */
    UISettingsSelector *m_pSelector;
    /** Holds the page stack instance. */
    QStackedWidget     *m_pStack;

private slots:

    /** Handles validity change for certain @a pValidator. */
    void sltHandleValidityChange(UIPageValidator *pValidator);

    /** Handles hover enter for warning pane specified by @a pValidator. */
    void sltHandleWarningPaneHovered(UIPageValidator *pValidator);
    /** Handles hover leave for warning pane specified by @a pValidator. */
    void sltHandleWarningPaneUnhovered(UIPageValidator *pValidator);

    /** Updates watch this information depending on whether we have @a fGotFocus. */
    void sltUpdateWhatsThis(bool fGotFocus);
    /** Updates watch this information. */
    void sltUpdateWhatsThisNoFocus() { sltUpdateWhatsThis(false); }

private:

    /** Prepares all. */
    void prepare();
    /** Prepares widgets. */
    void prepareWidgets();
    /** Assigns validater for passed @a pPage. */
    void assignValidator(UISettingsPage *pPage);

    /** Holds configuration access level. */
    ConfigurationAccessLevel  m_enmConfigurationAccessLevel;

    /** Holds the serialize process instance. */
    UISettingsSerializer *m_pSerializeProcess;

    /** Holds whether dialog is polished. */
    bool  m_fPolished;
    /** Holds whether the serialization is in progress. */
    bool  m_fSerializationIsInProgress;
    /** Holds whether there were no serialization errors. */
    bool  m_fSerializationClean;
    /** Holds whether the dialod had emitted signal to be closed. */
    bool  m_fClosed;

    /** Holds the status-bar widget instance. */
    QStackedWidget *m_pStatusBar;
    /** Holds the process-bar widget instance. */
    QProgressBar   *m_pProcessBar;
    /** Holds the warning-pane instance. */
    UIWarningPane  *m_pWarningPane;

    /** Holds whether settings dialog is valid (no errors, can be warnings). */
    bool  m_fValid;
    /** Holds whether settings dialog is silent (no errors and no warnings). */
    bool  m_fSilent;

    /** Holds the warning hint. */
    QString  m_strWarningHint;

    /** Holds the what's this hover timer instance. */
    QTimer            *m_pWhatsThisTimer;
    /** Holds the what's this hover timer instance. */
    QPointer<QWidget>  m_pWhatsThisCandidate;

    /** Holds the map of settings pages. */
    QMap<int, int>      m_pages;
    /** Stores the help tag per page. Key is the page type (either GlobalSettingsPageType or MachineSettingsPageType)
      * and value is the help tag. Used in context sensitive help: */
    QMap<int, QString>  m_pageHelpKeywords;

#ifdef VBOX_WS_MAC
    /** Holds the list of settings page sizes for animation purposes. */
    QList<QSize>  m_sizeList;
#endif

    /** @name Widgets
     * @{ */
       QLabel            *m_pLabelTitle;
       QIDialogButtonBox *m_pButtonBox;
       QWidget           *m_pWidgetStackHandler;
    /** @} */
};

#endif /* !FEQT_INCLUDED_SRC_settings_UISettingsDialog_h */

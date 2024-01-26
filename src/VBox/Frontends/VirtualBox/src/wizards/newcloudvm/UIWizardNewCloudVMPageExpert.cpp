/* $Id: UIWizardNewCloudVMPageExpert.cpp $ */
/** @file
 * VBox Qt GUI - UIWizardNewCloudVMPageExpert class implementation.
 */

/*
 * Copyright (C) 2009-2023 Oracle and/or its affiliates.
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

/* Qt includes: */
#include <QHeaderView>
#include <QListWidget>
#include <QPushButton>
#include <QTabBar>
#include <QVBoxLayout>

/* GUI includes: */
#include "QIComboBox.h"
#include "QIToolButton.h"
#include "UICloudNetworkingStuff.h"
#include "UIFormEditorWidget.h"
#include "UIIconPool.h"
#include "UINotificationCenter.h"
#include "UIToolBox.h"
#include "UIVirtualBoxEventHandler.h"
#include "UIVirtualBoxManager.h"
#include "UIWizardNewCloudVM.h"
#include "UIWizardNewCloudVMPageExpert.h"

/* Namespaces: */
using namespace UIWizardNewCloudVMSource;
using namespace UIWizardNewCloudVMProperties;


UIWizardNewCloudVMPageExpert::UIWizardNewCloudVMPageExpert()
    : m_pToolBox(0)
    , m_pProviderComboBox(0)
    , m_pProfileComboBox(0)
    , m_pProfileToolButton(0)
    , m_pSourceTabBar(0)
    , m_pSourceImageList(0)
    , m_pFormEditor(0)
{
    /* Prepare main layout: */
    QVBoxLayout *pLayoutMain = new QVBoxLayout(this);
    if (pLayoutMain)
    {
        /* Prepare tool-box: */
        m_pToolBox = new UIToolBox(this);
        if (m_pToolBox)
        {
            /* Prepare location widget: */
            QWidget *pWidgetLocation = new QWidget(m_pToolBox);
            if (pWidgetLocation)
            {
                /* Prepare location layout: */
                QVBoxLayout *pLayoutLocation = new QVBoxLayout(pWidgetLocation);
                if (pLayoutLocation)
                {
                    pLayoutLocation->setContentsMargins(0, 0, 0, 0);

                    /* Prepare provider combo-box: */
                    m_pProviderComboBox = new QIComboBox(pWidgetLocation);
                    if (m_pProviderComboBox)
                        pLayoutLocation->addWidget(m_pProviderComboBox);

                    /* Prepare profile layout: */
                    QHBoxLayout *pLayoutProfile = new QHBoxLayout;
                    if (pLayoutProfile)
                    {
                        pLayoutProfile->setContentsMargins(0, 0, 0, 0);
                        pLayoutProfile->setSpacing(1);

                        /* Prepare profile combo-box: */
                        m_pProfileComboBox = new QIComboBox(pWidgetLocation);
                        if (m_pProfileComboBox)
                            pLayoutProfile->addWidget(m_pProfileComboBox);

                        /* Prepare profile tool-button: */
                        m_pProfileToolButton = new QIToolButton(pWidgetLocation);
                        if (m_pProfileToolButton)
                        {
                            m_pProfileToolButton->setIcon(UIIconPool::iconSet(":/cloud_profile_manager_16px.png",
                                                                              ":/cloud_profile_manager_disabled_16px.png"));
                            pLayoutProfile->addWidget(m_pProfileToolButton);
                        }

                        /* Add into layout: */
                        pLayoutLocation->addLayout(pLayoutProfile);
                    }
                }

                /* Add into tool-box: */
                m_pToolBox->insertPage(0, pWidgetLocation, QString());
            }

            /* Prepare source widget: */
            QWidget *pWidgetSource = new QWidget(m_pToolBox);
            if (pWidgetSource)
            {
                /* Prepare source layout: */
                QVBoxLayout *pLayoutSource = new QVBoxLayout(pWidgetSource);
                if (pLayoutSource)
                {
                    pLayoutSource->setContentsMargins(0, 0, 0, 0);
                    pLayoutSource->setSpacing(0);

                    /* Prepare source tab-bar: */
                    m_pSourceTabBar = new QTabBar(pWidgetSource);
                    if (m_pSourceTabBar)
                    {
                        m_pSourceTabBar->addTab(QString());
                        m_pSourceTabBar->addTab(QString());

                        /* Add into layout: */
                        pLayoutSource->addWidget(m_pSourceTabBar);
                    }

                    /* Prepare source image list: */
                    m_pSourceImageList = new QListWidget(pWidgetSource);
                    if (m_pSourceImageList)
                    {
                        /* We want to have sorting enabled: */
                        m_pSourceImageList->setSortingEnabled(true);
                        /* A bit of look&feel: */
                        m_pSourceImageList->setAlternatingRowColors(true);

                        /* Add into layout: */
                        pLayoutSource->addWidget(m_pSourceImageList);
                    }
                }

                /* Add into tool-box: */
                m_pToolBox->insertPage(1, pWidgetSource, QString());
            }

            /* Prepare settings widget: */
            QWidget *pWidgetSettings = new QWidget(m_pToolBox);
            if (pWidgetSettings)
            {
                /* Prepare settings layout: */
                QVBoxLayout *pLayoutSettings = new QVBoxLayout(pWidgetSettings);
                if (pLayoutSettings)
                {
                    pLayoutSettings->setContentsMargins(0, 0, 0, 0);

                    /* Prepare form editor widget: */
                    m_pFormEditor = new UIFormEditorWidget(pWidgetSettings);
                    if (m_pFormEditor)
                    {
                        /* Add into layout: */
                        pLayoutSettings->addWidget(m_pFormEditor);
                    }
                }

                /* Add into tool-box: */
                m_pToolBox->insertPage(2, pWidgetSettings, QString());
            }

            /* Add into layout: */
            pLayoutMain->addWidget(m_pToolBox);
        }
    }

    /* Setup connections: */
    connect(gVBoxEvents, &UIVirtualBoxEventHandler::sigCloudProfileRegistered,
            this, &UIWizardNewCloudVMPageExpert::sltHandleProviderComboChange);
    connect(gVBoxEvents, &UIVirtualBoxEventHandler::sigCloudProfileChanged,
            this, &UIWizardNewCloudVMPageExpert::sltHandleProviderComboChange);
    connect(m_pProviderComboBox, &QIComboBox::activated,
            this, &UIWizardNewCloudVMPageExpert::sltHandleProviderComboChange);
    connect(m_pProfileComboBox, static_cast<void(QIComboBox::*)(int)>(&QIComboBox::currentIndexChanged),
            this, &UIWizardNewCloudVMPageExpert::sltHandleProfileComboChange);
    connect(m_pProfileToolButton, &QIToolButton::clicked,
            this, &UIWizardNewCloudVMPageExpert::sltHandleProfileButtonClick);
    connect(m_pSourceTabBar, &QTabBar::currentChanged,
            this, &UIWizardNewCloudVMPageExpert::sltHandleSourceTabBarChange);
    connect(m_pSourceImageList, &QListWidget::currentRowChanged,
            this, &UIWizardNewCloudVMPageExpert::sltHandleSourceImageChange);
}

UIWizardNewCloudVM *UIWizardNewCloudVMPageExpert::wizard() const
{
    return qobject_cast<UIWizardNewCloudVM*>(UINativeWizardPage::wizard());
}

void UIWizardNewCloudVMPageExpert::retranslateUi()
{
    /* Translate tool-box: */
    if (m_pToolBox)
    {
        m_pToolBox->setPageTitle(0, UIWizardNewCloudVM::tr("Location"));
        m_pToolBox->setPageTitle(1, UIWizardNewCloudVM::tr("Source"));
        m_pToolBox->setPageTitle(2, UIWizardNewCloudVM::tr("Settings"));
    }

    /* Translate received values of Location combo-box.
     * We are enumerating starting from 0 for simplicity: */
    if (m_pProviderComboBox)
        for (int i = 0; i < m_pProviderComboBox->count(); ++i)
        {
            m_pProviderComboBox->setItemText(i, m_pProviderComboBox->itemData(i, ProviderData_Name).toString());
            m_pProviderComboBox->setItemData(i, UIWizardNewCloudVM::tr("Create VM for cloud service provider."), Qt::ToolTipRole);
        }

    /* Translate source tab-bar: */
    if (m_pSourceTabBar)
    {
        m_pSourceTabBar->setTabText(0, UIWizardNewCloudVM::tr("&Images"));
        m_pSourceTabBar->setTabText(1, UIWizardNewCloudVM::tr("&Boot Volumes"));
    }

    /* Translate profile stuff: */
    if (m_pProfileToolButton)
        m_pProfileToolButton->setToolTip(UIWizardNewCloudVM::tr("Open Cloud Profile Manager..."));

    /* Update tool-tips: */
    updateComboToolTip(m_pProviderComboBox);
}

void UIWizardNewCloudVMPageExpert::initializePage()
{
    /* Choose 1st tool to be chosen initially: */
    m_pToolBox->setCurrentPage(0);
    /* Make sure form-editor knows notification-center: */
    m_pFormEditor->setNotificationCenter(wizard()->notificationCenter());
    /* Populate providers: */
    populateProviders(m_pProviderComboBox, wizard()->notificationCenter());
    /* Translate providers: */
    retranslateUi();
    /* Make image list focused by default: */
    m_pSourceImageList->setFocus();
    /* Fetch it, asynchronously: */
    QMetaObject::invokeMethod(this, "sltHandleProviderComboChange", Qt::QueuedConnection);
}

bool UIWizardNewCloudVMPageExpert::isComplete() const
{
    /* Initial result: */
    bool fResult = true;

    /* Check cloud settings: */
    fResult =    wizard()->client().isNotNull()
              && wizard()->vsd().isNotNull();

    /* Return result: */
    return fResult;
}

bool UIWizardNewCloudVMPageExpert::validatePage()
{
    /* Initial result: */
    bool fResult = true;

    /* Make sure table has own data committed: */
    m_pFormEditor->makeSureEditorDataCommitted();

    /* Check whether we have proper VSD form: */
    CVirtualSystemDescriptionForm comForm = wizard()->vsdForm();
    /* Give changed VSD back: */
    if (comForm.isNotNull())
    {
        comForm.GetVirtualSystemDescription();
        fResult = comForm.isOk();
        if (!fResult)
            UINotificationMessage::cannotAcquireVirtualSystemDescriptionFormParameter(comForm, wizard()->notificationCenter());
    }

    /* Try to create cloud VM: */
    if (fResult)
    {
        fResult = wizard()->createCloudVM();

        /* If the final step failed we could try
         * sugest user more valid form this time: */
        if (!fResult)
        {
            wizard()->setVSDForm(CVirtualSystemDescriptionForm());
            wizard()->createVSDForm();
            updatePropertiesTable();
            emit completeChanged();
        }
    }

    /* Return result: */
    return fResult;
}

void UIWizardNewCloudVMPageExpert::sltHandleProviderComboChange()
{
    /* Update combo tool-tip: */
    updateComboToolTip(m_pProviderComboBox);

    /* Update wizard fields: */
    wizard()->setProviderShortName(m_pProviderComboBox->currentData(ProviderData_ShortName).toString());

    /* Update profiles: */
    populateProfiles(m_pProfileComboBox, wizard()->notificationCenter(), wizard()->providerShortName(), wizard()->profileName());
    sltHandleProfileComboChange();

    /* Notify about changes: */
    emit completeChanged();
}

void UIWizardNewCloudVMPageExpert::sltHandleProfileComboChange()
{
    /* Update wizard fields: */
    wizard()->setProfileName(m_pProfileComboBox->currentData(ProfileData_Name).toString());
    wizard()->setClient(cloudClientByName(wizard()->providerShortName(), wizard()->profileName(), wizard()->notificationCenter()));

    /* Update source: */
    sltHandleSourceTabBarChange();

    /* Notify about changes: */
    emit completeChanged();
}

void UIWizardNewCloudVMPageExpert::sltHandleProfileButtonClick()
{
    if (gpManager)
        gpManager->openCloudProfileManager();
}

void UIWizardNewCloudVMPageExpert::sltHandleSourceTabBarChange()
{
    /* Update source type: */
    wizard()->wizardButton(WizardButtonType_Expert)->setEnabled(false);
    populateSourceImages(m_pSourceImageList, m_pSourceTabBar, wizard()->notificationCenter(), wizard()->client());
    wizard()->wizardButton(WizardButtonType_Expert)->setEnabled(true);
    sltHandleSourceImageChange();

    /* Notify about changes: */
    emit completeChanged();
}

void UIWizardNewCloudVMPageExpert::sltHandleSourceImageChange()
{
    /* Update source image & VSD form: */
    m_strSourceImageId = currentListWidgetData(m_pSourceImageList);
    wizard()->setVSD(createVirtualSystemDescription(wizard()->notificationCenter()));
    populateFormProperties(wizard()->vsd(), wizard(), m_pSourceTabBar, m_strSourceImageId);
    wizard()->createVSDForm();
    updatePropertiesTable();

    /* Notify about changes: */
    emit completeChanged();
}

void UIWizardNewCloudVMPageExpert::updatePropertiesTable()
{
    refreshFormPropertiesTable(m_pFormEditor, wizard()->vsdForm());
}

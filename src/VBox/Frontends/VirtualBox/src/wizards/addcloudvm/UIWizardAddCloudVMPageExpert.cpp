/* $Id: UIWizardAddCloudVMPageExpert.cpp $ */
/** @file
 * VBox Qt GUI - UIWizardAddCloudVMPageExpert class implementation.
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
#include <QHBoxLayout>
#include <QHeaderView>
#include <QListWidget>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>

/* GUI includes: */
#include "QIComboBox.h"
#include "QIToolButton.h"
#include "UICloudNetworkingStuff.h"
#include "UIIconPool.h"
#include "UIToolBox.h"
#include "UIVirtualBoxEventHandler.h"
#include "UIVirtualBoxManager.h"
#include "UIWizardAddCloudVM.h"
#include "UIWizardAddCloudVMPageExpert.h"

/* Namespaces: */
using namespace UIWizardAddCloudVMSource;


UIWizardAddCloudVMPageExpert::UIWizardAddCloudVMPageExpert()
    : m_pToolBox(0)
    , m_pProviderLabel(0)
    , m_pProviderComboBox(0)
    , m_pProfileLabel(0)
    , m_pProfileComboBox(0)
    , m_pProfileToolButton(0)
    , m_pSourceInstanceLabel(0)
    , m_pSourceInstanceList(0)
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

                    /* Prepare source instances table: */
                    m_pSourceInstanceList = new QListWidget(pWidgetSource);
                    if (m_pSourceInstanceList)
                    {
                        /* A bit of look&feel: */
                        m_pSourceInstanceList->setAlternatingRowColors(true);
                        /* Allow to select more than one item to add: */
                        m_pSourceInstanceList->setSelectionMode(QAbstractItemView::ExtendedSelection);

                        /* Add into layout: */
                        pLayoutSource->addWidget(m_pSourceInstanceList);
                    }
                }

                /* Add into tool-box: */
                m_pToolBox->insertPage(1, pWidgetSource, QString());
            }

            /* Add into layout: */
            pLayoutMain->addWidget(m_pToolBox);
        }
    }

    /* Setup connections: */
    connect(gVBoxEvents, &UIVirtualBoxEventHandler::sigCloudProfileRegistered,
            this, &UIWizardAddCloudVMPageExpert::sltHandleProviderComboChange);
    connect(gVBoxEvents, &UIVirtualBoxEventHandler::sigCloudProfileChanged,
            this, &UIWizardAddCloudVMPageExpert::sltHandleProviderComboChange);
    connect(m_pProviderComboBox, &QIComboBox::activated,
            this, &UIWizardAddCloudVMPageExpert::sltHandleProviderComboChange);
    connect(m_pProfileComboBox, static_cast<void(QIComboBox::*)(int)>(&QIComboBox::currentIndexChanged),
            this, &UIWizardAddCloudVMPageExpert::sltHandleProfileComboChange);
    connect(m_pProfileToolButton, &QIToolButton::clicked,
            this, &UIWizardAddCloudVMPageExpert::sltHandleProfileButtonClick);
    connect(m_pSourceInstanceList, &QListWidget::itemSelectionChanged,
            this, &UIWizardAddCloudVMPageExpert::sltHandleSourceInstanceChange);
}

UIWizardAddCloudVM *UIWizardAddCloudVMPageExpert::wizard() const
{
    return qobject_cast<UIWizardAddCloudVM*>(UINativeWizardPage::wizard());
}

void UIWizardAddCloudVMPageExpert::retranslateUi()
{
    /* Translate tool-box: */
    if (m_pToolBox)
    {
        m_pToolBox->setPageTitle(0, UIWizardAddCloudVM::tr("Location"));
        m_pToolBox->setPageTitle(1, UIWizardAddCloudVM::tr("Source"));
    }

    /* Translate profile stuff: */
    if (m_pProfileToolButton)
        m_pProfileToolButton->setToolTip(UIWizardAddCloudVM::tr("Open Cloud Profile Manager..."));

    /* Translate received values of Source combo-box.
     * We are enumerating starting from 0 for simplicity: */
    if (m_pProviderComboBox)
        for (int i = 0; i < m_pProviderComboBox->count(); ++i)
        {
            m_pProviderComboBox->setItemText(i, m_pProviderComboBox->itemData(i, ProviderData_Name).toString());
            m_pProviderComboBox->setItemData(i, UIWizardAddCloudVM::tr("Add VM from cloud service provider."), Qt::ToolTipRole);
        }

    /* Update tool-tips: */
    updateComboToolTip(m_pProviderComboBox);
}

void UIWizardAddCloudVMPageExpert::initializePage()
{
    /* Choose 1st tool to be chosen initially: */
    m_pToolBox->setCurrentPage(0);
    /* Populate providers: */
    populateProviders(m_pProviderComboBox, wizard()->notificationCenter());
    /* Translate providers: */
    retranslateUi();
    /* Fetch it, asynchronously: */
    QMetaObject::invokeMethod(this, "sltHandleProviderComboChange", Qt::QueuedConnection);
    /* Make image list focused by default: */
    m_pSourceInstanceList->setFocus();
}

bool UIWizardAddCloudVMPageExpert::isComplete() const
{
    /* Initial result: */
    bool fResult = true;

    /* Make sure client is not NULL and
     * at least one instance is selected: */
    fResult =    wizard()->client().isNotNull()
              && !wizard()->instanceIds().isEmpty();

    /* Return result: */
    return fResult;
}

bool UIWizardAddCloudVMPageExpert::validatePage()
{
    /* Initial result: */
    bool fResult = true;

    /* Try to add cloud VMs: */
    fResult = wizard()->addCloudVMs();

    /* Return result: */
    return fResult;
}

void UIWizardAddCloudVMPageExpert::sltHandleProviderComboChange()
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

void UIWizardAddCloudVMPageExpert::sltHandleProfileComboChange()
{
    /* Update wizard fields: */
    wizard()->setProfileName(m_pProfileComboBox->currentData(ProfileData_Name).toString());
    wizard()->setClient(cloudClientByName(wizard()->providerShortName(), wizard()->profileName(), wizard()->notificationCenter()));

    /* Update profile instances: */
    wizard()->wizardButton(WizardButtonType_Expert)->setEnabled(false);
    populateProfileInstances(m_pSourceInstanceList, wizard()->notificationCenter(), wizard()->client());
    wizard()->wizardButton(WizardButtonType_Expert)->setEnabled(true);
    sltHandleSourceInstanceChange();

    /* Notify about changes: */
    emit completeChanged();
}

void UIWizardAddCloudVMPageExpert::sltHandleProfileButtonClick()
{
    if (gpManager)
        gpManager->openCloudProfileManager();
}

void UIWizardAddCloudVMPageExpert::sltHandleSourceInstanceChange()
{
    /* Update wizard fields: */
    wizard()->setInstanceIds(currentListWidgetData(m_pSourceInstanceList));

    /* Notify about changes: */
    emit completeChanged();
}

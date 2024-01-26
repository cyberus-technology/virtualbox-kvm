/* $Id: UIHostnameDomainNameEditor.cpp $ */
/** @file
 * VBox Qt GUI - UIHostnameDomainNameEditor class implementation.
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

/* Qt includes: */
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QRegularExpressionValidator>
#include <QStyle>
#include <QVBoxLayout>

/* GUI includes: */
#include "QILineEdit.h"
#include "QIRichTextLabel.h"
#include "QIToolButton.h"
#include "UICommon.h"
#include "UIIconPool.h"
#include "UIHostnameDomainNameEditor.h"
#include "UIWizardNewVM.h"



UIHostnameDomainNameEditor::UIHostnameDomainNameEditor(QWidget *pParent /*  = 0 */)
    : QIWithRetranslateUI<QWidget>(pParent)
    , m_pHostnameLineEdit(0)
    , m_pDomainNameLineEdit(0)
    , m_pHostnameLabel(0)
    , m_pDomainNameLabel(0)
    , m_pMainLayout(0)
{
    prepare();
}

QString UIHostnameDomainNameEditor::hostname() const
{
    if (m_pHostnameLineEdit)
        return m_pHostnameLineEdit->text();
    return QString();
}

bool UIHostnameDomainNameEditor::isComplete() const
{
    return m_pHostnameLineEdit && m_pHostnameLineEdit->hasAcceptableInput() &&
        m_pDomainNameLineEdit && m_pDomainNameLineEdit->hasAcceptableInput();
}

void UIHostnameDomainNameEditor::mark()
{
    if (m_pHostnameLineEdit)
        m_pHostnameLineEdit->mark(!m_pHostnameLineEdit->hasAcceptableInput(),
                                  tr("Hostname should be at least 2 character long. "
                                     "Allowed characters are alphanumerics, \"-\" and \".\""));
    if (m_pDomainNameLineEdit)
        m_pDomainNameLineEdit->mark(!m_pDomainNameLineEdit->hasAcceptableInput(),
                                    tr("Domain name should be at least 2 character long. "
                                       "Allowed characters are alphanumerics, \"-\" and \".\""));
}

void UIHostnameDomainNameEditor::setHostname(const QString &strHostname)
{
    if (m_pHostnameLineEdit)
        m_pHostnameLineEdit->setText(strHostname);
}

QString UIHostnameDomainNameEditor::domainName() const
{
    if (m_pDomainNameLineEdit)
        return m_pDomainNameLineEdit->text();
    return QString();
}

void UIHostnameDomainNameEditor::setDomainName(const QString &strDomain)
{
    if (m_pDomainNameLineEdit)
        m_pDomainNameLineEdit->setText(strDomain);
}

QString UIHostnameDomainNameEditor::hostnameDomainName() const
{
    if (m_pHostnameLineEdit && m_pDomainNameLineEdit)
        return QString("%1.%2").arg(m_pHostnameLineEdit->text()).arg(m_pDomainNameLineEdit->text());
    return QString();
}

int UIHostnameDomainNameEditor::firstColumnWidth() const
{
    int iWidth = 0;
    if (m_pHostnameLabel)
        iWidth = qMax(iWidth, m_pHostnameLabel->minimumSizeHint().width());
    if (m_pDomainNameLabel)
        iWidth = qMax(iWidth, m_pDomainNameLabel->minimumSizeHint().width());
    return iWidth;
}

void UIHostnameDomainNameEditor::setFirstColumnWidth(int iWidth)
{
    if (m_pMainLayout)
        m_pMainLayout->setColumnMinimumWidth(0, iWidth);
}

void UIHostnameDomainNameEditor::retranslateUi()
{
    if (m_pHostnameLabel)
        m_pHostnameLabel->setText(tr("Hostna&me:"));
    if (m_pHostnameLineEdit)
        m_pHostnameLineEdit->setToolTip(tr("Holds the hostname."));
    if (m_pDomainNameLabel)
        m_pDomainNameLabel->setText(tr("&Domain Name:"));
    if (m_pDomainNameLineEdit)
        m_pDomainNameLineEdit->setToolTip(tr("Holds the domain name."));
}

template<class T>
void UIHostnameDomainNameEditor::addLineEdit(int &iRow, QLabel *&pLabel, T *&pLineEdit, QGridLayout *pLayout)
{
    AssertReturnVoid(pLayout);
    if (pLabel || pLineEdit)
        return;

    pLabel = new QLabel;
    AssertReturnVoid(pLabel);
    pLabel->setAlignment(Qt::AlignRight);
    //pLabel->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);

    pLayout->addWidget(pLabel, iRow, 0, 1, 1);

    pLineEdit = new T;
    AssertReturnVoid(pLineEdit);

    pLayout->addWidget(pLineEdit, iRow, 1, 1, 3);
    pLabel->setBuddy(pLineEdit);
    ++iRow;
    return;
}

void UIHostnameDomainNameEditor::prepare()
{
    m_pMainLayout = new QGridLayout;
    m_pMainLayout->setColumnStretch(0, 0);
    m_pMainLayout->setColumnStretch(1, 1);
    if (!m_pMainLayout)
        return;
    setLayout(m_pMainLayout);
    int iRow = 0;
    addLineEdit<UIMarkableLineEdit>(iRow, m_pHostnameLabel, m_pHostnameLineEdit, m_pMainLayout);
    addLineEdit<QILineEdit>(iRow, m_pDomainNameLabel, m_pDomainNameLineEdit, m_pMainLayout);

    /* Host name and domain should be strings of minimum length of 2 and composed of alpha numerics, '-', and '.'
     * Exclude strings with . at the end: */
    m_pHostnameLineEdit->setValidator(new QRegularExpressionValidator(QRegularExpression("^[a-zA-Z0-9-.]{2,}[$a-zA-Z0-9-]"), this));
    m_pDomainNameLineEdit->setValidator(new QRegularExpressionValidator(QRegularExpression("^[a-zA-Z0-9-.]{2,}[$a-zA-Z0-9-]"), this));

    connect(m_pHostnameLineEdit, &UIMarkableLineEdit::textChanged,
            this, &UIHostnameDomainNameEditor::sltHostnameChanged);
    connect(m_pDomainNameLineEdit, &QILineEdit::textChanged,
            this, &UIHostnameDomainNameEditor::sltDomainChanged);

    retranslateUi();
}

void UIHostnameDomainNameEditor::sltHostnameChanged()
{
    m_pHostnameLineEdit->mark(!m_pHostnameLineEdit->hasAcceptableInput(),
                              tr("Hostname should be at least 2 character long. "
                                 "Allowed characters are alphanumerics, \"-\" and \".\""));
    emit sigHostnameDomainNameChanged(hostnameDomainName(), isComplete());
}

void UIHostnameDomainNameEditor::sltDomainChanged()
{
    m_pDomainNameLineEdit->mark(!m_pDomainNameLineEdit->hasAcceptableInput(),
                                tr("Domain name should be at least 2 character long. "
                                   "Allowed characters are alphanumerics, \"-\" and \".\""));
    emit sigHostnameDomainNameChanged(hostnameDomainName(), isComplete());
}

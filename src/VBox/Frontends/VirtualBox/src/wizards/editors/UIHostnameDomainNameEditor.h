/* $Id: UIHostnameDomainNameEditor.h $ */
/** @file
 * VBox Qt GUI - UIHostnameDomainNameEditor class declaration.
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

#ifndef FEQT_INCLUDED_SRC_wizards_editors_UIHostnameDomainNameEditor_h
#define FEQT_INCLUDED_SRC_wizards_editors_UIHostnameDomainNameEditor_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* Qt includes: */
#include <QIcon>
#include <QWidget>

/* Local includes: */
#include "QIWithRetranslateUI.h"

/* Forward declarations: */
class QGridLayout;
class QLabel;
class QILineEdit;
class UIMarkableLineEdit;
class UIPasswordLineEdit;

class UIHostnameDomainNameEditor : public QIWithRetranslateUI<QWidget>
{

    Q_OBJECT;

signals:

    void sigHostnameDomainNameChanged(const QString &strHostNameDomain, bool fIsComplete);

public:

    UIHostnameDomainNameEditor(QWidget *pParent = 0);

    QString hostname() const;
    void setHostname(const QString &strHostname);

    QString domainName() const;
    void setDomainName(const QString &strDomain);

    QString hostnameDomainName() const;

    bool isComplete() const;
    void mark();

    int firstColumnWidth() const;
    void setFirstColumnWidth(int iWidth);


protected:

    void retranslateUi();

private slots:

    void sltHostnameChanged();
    void sltDomainChanged();

private:

    void prepare();
    template<class T>
    void addLineEdit(int &iRow, QLabel *&pLabel, T *&pLineEdit, QGridLayout *pLayout);

    UIMarkableLineEdit *m_pHostnameLineEdit;
    QILineEdit         *m_pDomainNameLineEdit;

    QLabel *m_pHostnameLabel;
    QLabel *m_pDomainNameLabel;
    QGridLayout *m_pMainLayout;
};

#endif /* !FEQT_INCLUDED_SRC_wizards_editors_UIHostnameDomainNameEditor_h */

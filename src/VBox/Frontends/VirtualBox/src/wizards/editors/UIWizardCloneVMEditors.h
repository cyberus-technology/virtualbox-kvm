/* $Id: UIWizardCloneVMEditors.h $ */
/** @file
 * VBox Qt GUI - UIWizardDiskEditors class declaration.
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

#ifndef FEQT_INCLUDED_SRC_wizards_editors_UIWizardCloneVMEditors_h
#define FEQT_INCLUDED_SRC_wizards_editors_UIWizardCloneVMEditors_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* Qt includes: */
#include <QGroupBox>

/* Local includes: */
#include "QIWithRetranslateUI.h"

/* Other VBox includes: */
#include "COMEnums.h"


/* Forward declarations: */
class QAbstractButton;
class QButtonGroup;
class QCheckBox;
class QGridLayout;
class QComboBox;
class QLabel;
class QRadioButton;
class QILineEdit;
class UIFilePathSelector;
class UIMarkableLineEdit;

/** MAC address policies. */
enum MACAddressClonePolicy
{
    MACAddressClonePolicy_KeepAllMACs,
    MACAddressClonePolicy_KeepNATMACs,
    MACAddressClonePolicy_StripAllMACs,
    MACAddressClonePolicy_MAX
};
Q_DECLARE_METATYPE(MACAddressClonePolicy);

class UICloneVMNamePathEditor : public QIWithRetranslateUI<QGroupBox>
{
    Q_OBJECT;

signals:

    void sigCloneNameChanged(const QString &strCloneName);
    void sigClonePathChanged(const QString &strClonePath);

public:

    UICloneVMNamePathEditor(const QString &strOriginalName, const QString &strDefaultPath, QWidget *pParent = 0);

    void setFirstColumnWidth(int iWidth);
    int firstColumnWidth() const;

    void setLayoutContentsMargins(int iLeft, int iTop, int iRight, int iBottom);

    QString cloneName() const;
    void setCloneName(const QString &strName);

    QString clonePath() const;
    void setClonePath(const QString &strPath);

    bool isComplete(const QString &strMachineGroup);

private:

    void prepare();
    virtual void retranslateUi() /* override final */;

    QGridLayout *m_pContainerLayout;
    UIMarkableLineEdit  *m_pNameLineEdit;
    UIFilePathSelector  *m_pPathSelector;
    QLabel      *m_pNameLabel;
    QLabel      *m_pPathLabel;

    QString      m_strOriginalName;
    QString      m_strDefaultPath;
};


class UICloneVMAdditionalOptionsEditor : public QIWithRetranslateUI<QGroupBox>
{
    Q_OBJECT;

signals:

    void sigMACAddressClonePolicyChanged(MACAddressClonePolicy enmMACAddressClonePolicy);
    void sigKeepDiskNamesToggled(bool fKeepDiskNames);
    void sigKeepHardwareUUIDsToggled(bool fKeepHardwareUUIDs);

public:

    UICloneVMAdditionalOptionsEditor(QWidget *pParent = 0);

    bool isComplete();

    void setLayoutContentsMargins(int iLeft, int iTop, int iRight, int iBottom);

    MACAddressClonePolicy macAddressClonePolicy() const;
    void setMACAddressClonePolicy(MACAddressClonePolicy enmMACAddressClonePolicy);

    void setFirstColumnWidth(int iWidth);
    int firstColumnWidth() const;

    bool keepHardwareUUIDs() const;
    bool keepDiskNames() const;

private slots:

    void sltMACAddressClonePolicyChanged();

private:

    void prepare();
    virtual void retranslateUi() /* override final */;
    void populateMACAddressClonePolicies();
    void updateMACAddressClonePolicyComboToolTip();

    QGridLayout *m_pContainerLayout;
    QLabel *m_pMACComboBoxLabel;
    QComboBox *m_pMACComboBox;
    QLabel *m_pAdditionalOptionsLabel;
    QCheckBox   *m_pKeepDiskNamesCheckBox;
    QCheckBox   *m_pKeepHWUUIDsCheckBox;
};

class UICloneVMCloneTypeGroupBox : public QIWithRetranslateUI<QGroupBox>
{
    Q_OBJECT;

signals:

    void sigFullCloneSelected(bool fSelected);

public:

    UICloneVMCloneTypeGroupBox(QWidget *pParent = 0);
    bool isFullClone() const;

private slots:

    void sltButtonClicked(QAbstractButton *);

private:

    void prepare();
    virtual void retranslateUi() /* override final */;

    QButtonGroup *m_pButtonGroup;
    QRadioButton *m_pFullCloneRadio;
    QRadioButton *m_pLinkedCloneRadio;
};


class UICloneVMCloneModeGroupBox : public QIWithRetranslateUI<QGroupBox>
{
    Q_OBJECT;

signals:

    void sigCloneModeChanged(KCloneMode enmCloneMode);

public:

    UICloneVMCloneModeGroupBox(bool fShowChildsOption, QWidget *pParent = 0);
    KCloneMode cloneMode() const;

private slots:

    void sltButtonClicked();

private:

    void prepare();
    virtual void retranslateUi() /* override final */;

    bool m_fShowChildsOption;
    QRadioButton *m_pMachineRadio;
    QRadioButton *m_pMachineAndChildsRadio;
    QRadioButton *m_pAllRadio;
};



#endif /* !FEQT_INCLUDED_SRC_wizards_editors_UIWizardCloneVMEditors_h */

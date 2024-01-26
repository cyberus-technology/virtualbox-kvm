/* $Id: UIMediumSizeEditor.cpp $ */
/** @file
 * VBox Qt GUI - UIMediumSizeEditor class implementation.
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
#include <QRegularExpressionValidator>
#include <QSlider>

/* GUI includes: */
#include "QILineEdit.h"
#include "UICommon.h"
#include "UIConverter.h"
#include "UIMediumSizeEditor.h"
#include "UITranslator.h"

/* COM includes: */
#include "CSystemProperties.h"


const qulonglong UIMediumSizeEditor::m_uSectorSize = 512;

UIMediumSizeEditor::UIMediumSizeEditor(QWidget *pParent, qulonglong uMinimumSize /* = _4M */)
    : QIWithRetranslateUI<QWidget>(pParent)
    , m_uSizeMin(uMinimumSize)
    , m_uSizeMax(uiCommon().virtualBox().GetSystemProperties().GetInfoVDSize())
    , m_iSliderScale(calculateSliderScale(m_uSizeMax))
    , m_uSize(0)
    , m_enmSizeSuffix(SizeSuffix_Byte)
    , m_pSlider(0)
    , m_pLabelMinSize(0)
    , m_pLabelMaxSize(0)
    , m_pEditor(0)
{
    /* Prepare: */
    prepare();
    QString strRegEx = QString("[^\\d%1]").arg(UITranslator::decimalSep());
    m_regExNonDigitOrSeparator = QRegularExpression(strRegEx);
}

void UIMediumSizeEditor::setMediumSize(qulonglong uSize)
{
    /* Remember the new size: */
    m_uSize = uSize;

    /* And assign it to the slider & editor: */
    m_pSlider->blockSignals(true);
    m_pSlider->setValue(sizeMBToSlider(m_uSize, m_iSliderScale));
    m_pSlider->blockSignals(false);
    m_pEditor->blockSignals(true);
    m_pEditor->setText(UITranslator::formatSize(m_uSize));
    m_enmSizeSuffix = UITranslator::parseSizeSuffix(m_pEditor->text());
    m_pEditor->blockSignals(false);
}

void UIMediumSizeEditor::retranslateUi()
{
    /* Translate labels: */
    m_pLabelMinSize->setText(UITranslator::formatSize(m_uSizeMin));
    m_pLabelMaxSize->setText(UITranslator::formatSize(m_uSizeMax));

    /* Translate fields: */
    m_pSlider->setToolTip(tr("Holds the size of this medium."));
    m_pEditor->setToolTip(tr("Holds the size of this medium."));
    m_pLabelMinSize->setToolTip(tr("Minimum size for this medium."));
    m_pLabelMaxSize->setToolTip(tr("Maximum size for this medium."));
}

void UIMediumSizeEditor::sltSizeSliderChanged(int iValue)
{
    /* Update the current size: */
    m_uSize = sliderToSizeMB(iValue, m_iSliderScale);
    /* Update the other widget: */
    m_pEditor->blockSignals(true);
    m_pEditor->setText(UITranslator::formatSize(m_uSize));
    m_enmSizeSuffix = UITranslator::parseSizeSuffix(m_pEditor->text());
    m_pEditor->blockSignals(false);
    /* Notify the listeners: */
    emit sigSizeChanged(m_uSize);
}

void UIMediumSizeEditor::sltSizeEditorTextChanged()
{
    QString strSizeString = ensureSizeSuffix(m_pEditor->text());


    m_pEditor->blockSignals(true);
    int iCursorPosition = m_pEditor->cursorPosition();
    m_pEditor->setText(strSizeString);
    m_pEditor->setCursorPosition(iCursorPosition);
    m_pEditor->blockSignals(false);

    /* Update the current size: */
    m_uSize = checkSectorSizeAlignment(UITranslator::parseSize(strSizeString));

    /* Update the other widget: */
    m_pSlider->blockSignals(true);
    m_pSlider->setValue(sizeMBToSlider(m_uSize, m_iSliderScale));
    m_pSlider->blockSignals(false);
    /* Notify the listeners: */
    emit sigSizeChanged(m_uSize);
}

QString UIMediumSizeEditor::ensureSizeSuffix(const QString &strSizeString)
{
    /* Try to update the m_enmSizeSuffix: */
    if (UITranslator::hasSizeSuffix(strSizeString))
        m_enmSizeSuffix = UITranslator::parseSizeSuffix(strSizeString);

    QString strOnlyDigits(strSizeString);
    /* Remove any chars from the string except digits and decimal separator and then add a space and size suffix: */
    return QString("%1 %2").arg(strOnlyDigits.remove(m_regExNonDigitOrSeparator)).arg(gpConverter->toString(m_enmSizeSuffix));
}

void UIMediumSizeEditor::prepare()
{
    /* Create layout: */
    QGridLayout *pLayout = new QGridLayout(this);
    if (pLayout)
    {
        /* Configure layout: */
        pLayout->setContentsMargins(0, 0, 0, 0);
        pLayout->setColumnStretch(0, 1);
        pLayout->setColumnStretch(1, 1);
        pLayout->setColumnStretch(2, 0);

        /* Create size slider: */
        m_pSlider = new QSlider;
        if (m_pSlider)
        {
            /* Configure slider: */
            m_pSlider->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
            m_pSlider->setOrientation(Qt::Horizontal);
            m_pSlider->setTickPosition(QSlider::TicksBelow);
            m_pSlider->setFocusPolicy(Qt::StrongFocus);
            m_pSlider->setPageStep(m_iSliderScale);
            m_pSlider->setSingleStep(m_iSliderScale / 8);
            m_pSlider->setTickInterval(0);
            m_pSlider->setMinimum(sizeMBToSlider(m_uSizeMin, m_iSliderScale));
            m_pSlider->setMaximum(sizeMBToSlider(m_uSizeMax, m_iSliderScale));
            connect(m_pSlider, &QSlider::valueChanged,
                    this, &UIMediumSizeEditor::sltSizeSliderChanged);

            /* Add into layout: */
            pLayout->addWidget(m_pSlider, 0, 0, 1, 2, Qt::AlignTop);
        }

        /* Create minimum size label: */
        m_pLabelMinSize = new QLabel;
        if (m_pLabelMinSize)
        {
            /* Configure label: */
            m_pLabelMinSize->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

            /* Add into layout: */
            pLayout->addWidget(m_pLabelMinSize, 1, 0);
        }

        /* Create maximum size label: */
        m_pLabelMaxSize = new QLabel;
        if (m_pLabelMaxSize)
        {
            /* Configure label: */
            m_pLabelMaxSize->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

            /* Add into layout: */
            pLayout->addWidget(m_pLabelMaxSize, 1, 1);
        }

        /* Create size editor: */
        m_pEditor = new QILineEdit;
        if (m_pEditor)
        {
            /* Configure editor: */
            m_pEditor->installEventFilter(this);
            m_pEditor->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
            m_pEditor->setFixedWidthByText("88888.88 MB");
            m_pEditor->setAlignment(Qt::AlignRight);
            m_pEditor->setValidator(new QRegularExpressionValidator(QRegularExpression(UITranslator::sizeRegexp()), this));
            connect(m_pEditor, &QILineEdit::textChanged,
                    this, &UIMediumSizeEditor::sltSizeEditorTextChanged);

            /* Add into layout: */
            pLayout->addWidget(m_pEditor, 0, 2, Qt::AlignTop);
        }
    }

    /* Apply language settings: */
    retranslateUi();
}

/* static */
int UIMediumSizeEditor::calculateSliderScale(qulonglong uMaximumMediumSize)
{
    /* Detect how many steps to recognize between adjacent powers of 2
     * to ensure that the last slider step is exactly that we need: */
    int iSliderScale = 0;
    int iPower = log2i(uMaximumMediumSize);
    qulonglong uTickMB = (qulonglong)1 << iPower;
    if (uTickMB < uMaximumMediumSize)
    {
        qulonglong uTickMBNext = (qulonglong)1 << (iPower + 1);
        qulonglong uGap = uTickMBNext - uMaximumMediumSize;
        iSliderScale = (int)((uTickMBNext - uTickMB) / uGap);
#ifdef VBOX_WS_MAC
        // WORKAROUND:
        // There is an issue with Qt5 QSlider under OSX:
        // Slider tick count (maximum - minimum) is limited with some
        // "magical number" - 588351, having it more than that brings
        // unpredictable results like slider token jumping and disappearing,
        // so we are limiting tick count by lowering slider-scale 128 times.
        iSliderScale /= 128;
#endif /* VBOX_WS_MAC */
    }
    return qMax(iSliderScale, 8);
}

/* static */
int UIMediumSizeEditor::log2i(qulonglong uValue)
{
    if (!uValue)
        return 0;
    int iPower = -1;
    while (uValue)
    {
        ++iPower;
        uValue >>= 1;
    }
    return iPower;
}

/* static */
int UIMediumSizeEditor::sizeMBToSlider(qulonglong uValue, int iSliderScale)
{
    /* Make sure *any* slider value is multiple of m_uSectorSize: */
    uValue /= m_uSectorSize;

    /* Calculate result: */
    int iPower = log2i(uValue);
    qulonglong uTickMB = qulonglong (1) << iPower;
    qulonglong uTickMBNext = qulonglong (1) << (iPower + 1);
    int iStep = (uValue - uTickMB) * iSliderScale / (uTickMBNext - uTickMB);
    int iResult = iPower * iSliderScale + iStep;

    /* Return result: */
    return iResult;
}

/* static */
qulonglong UIMediumSizeEditor::sliderToSizeMB(int uValue, int iSliderScale)
{
    /* Calculate result: */
    int iPower = uValue / iSliderScale;
    int iStep = uValue % iSliderScale;
    qulonglong uTickMB = qulonglong (1) << iPower;
    qulonglong uTickMBNext = qulonglong (1) << (iPower + 1);
    qulonglong uResult = uTickMB + (uTickMBNext - uTickMB) * iStep / iSliderScale;

    /* Make sure *any* slider value is multiple of m_uSectorSize: */
    uResult *= m_uSectorSize;

    /* Return result: */
    return uResult;
}

void UIMediumSizeEditor::updateSizeToolTips(qulonglong uSize)
{
    const QString strToolTip = tr("<nobr>%1 (%2 B)</nobr>").arg(UITranslator::formatSize(uSize)).arg(uSize);
    m_pSlider->setToolTip(strToolTip);
    m_pEditor->setToolTip(strToolTip);
}

qulonglong UIMediumSizeEditor::checkSectorSizeAlignment(qulonglong uSize)
{
    if (m_uSectorSize == 0 || uSize % m_uSectorSize == 0)
        return uSize;
    qulonglong uNewSize = (uSize / m_uSectorSize) * m_uSectorSize;
    return uNewSize;
}

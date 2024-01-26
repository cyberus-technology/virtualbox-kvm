/* $Id: UIVMLogViewerFilterPanel.cpp $ */
/** @file
 * VBox Qt GUI - UIVMLogViewer class implementation.
 */

/*
 * Copyright (C) 2010-2023 Oracle and/or its affiliates.
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
#include <QButtonGroup>
#include <QComboBox>
#include <QFrame>
#include <QHBoxLayout>
#if defined(RT_OS_SOLARIS)
# include <QFontDatabase>
#endif
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QRegularExpression>
#include <QTextCursor>
#include <QRadioButton>
#include <QScrollArea>

/* GUI includes: */
#include "QIToolButton.h"
#include "UIIconPool.h"
#include "UIVMLogPage.h"
#include "UIVMLogViewerFilterPanel.h"
#include "UIVMLogViewerWidget.h"
#ifdef VBOX_WS_MAC
# include "VBoxUtils-darwin.h"
#endif


/*********************************************************************************************************************************
*   UIVMFilterLineEdit definition.                                                                                               *
*********************************************************************************************************************************/

/** UIVMFilterLineEdit class is used to display and modify the list of filter terms.
 *  the terms are displayed as words with spaces in between and it is possible to
 *  remove these terms one by one by selecting them or completely by the clearAll button
 *  located on the right side of the line edit: */
class UIVMFilterLineEdit : public QLineEdit
{
    Q_OBJECT;

signals:

    void sigFilterTermRemoved(QString removedString);
    void sigClearAll();

public:

    UIVMFilterLineEdit(QWidget *parent = 0);
    void addFilterTerm(const QString& filterTermString);
    void clearAll();

protected:

    /* Delete mouseDoubleClick and mouseMoveEvent implementations of the base class */
    virtual void        mouseDoubleClickEvent(QMouseEvent *) RT_OVERRIDE {}
    virtual void        mouseMoveEvent(QMouseEvent *) RT_OVERRIDE {}
    /* Override the mousePressEvent to control how selection is done: */
    virtual void        mousePressEvent(QMouseEvent * event) RT_OVERRIDE;
    virtual void        mouseReleaseEvent(QMouseEvent *){}
    virtual void        paintEvent(QPaintEvent *event) RT_OVERRIDE;

private slots:

    /* Nofifies the listeners that selected word (filter term) has been removed: */
    void sltRemoveFilterTerm();
    /* The whole content is removed. Listeners are notified: */
    void sltClearAll();

private:

    void          createButtons();
    QToolButton   *m_pRemoveTermButton;
    QToolButton   *m_pClearAllButton;
    const int      m_iRemoveTermButtonSize;
    int            m_iTrailingSpaceCount;
};


/*********************************************************************************************************************************
*   UIVMFilterLineEdit implementation.                                                                                           *
*********************************************************************************************************************************/

UIVMFilterLineEdit::UIVMFilterLineEdit(QWidget *parent /*= 0*/)
    :QLineEdit(parent)
    , m_pRemoveTermButton(0)
    , m_pClearAllButton(0)
    , m_iRemoveTermButtonSize(16)
    , m_iTrailingSpaceCount(1)
{
    setReadOnly(true);
    home(false);
    setContextMenuPolicy(Qt::NoContextMenu);
    createButtons();
    /** Try to guess the width of the space between filter terms so that remove button
        we display when a term is selected does not hide the next/previous word: */
#if QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
    int spaceWidth = fontMetrics().horizontalAdvance(' ');
#else
    int spaceWidth = fontMetrics().width(' ');
#endif
    if (spaceWidth != 0)
        m_iTrailingSpaceCount = (m_iRemoveTermButtonSize / spaceWidth) + 1;
}

void UIVMFilterLineEdit::addFilterTerm(const QString& filterTermString)
{
    if (text().isEmpty())
        insert(filterTermString);
    else
    {
        QString newString(filterTermString);
        QString space(m_iTrailingSpaceCount, QChar(' '));
        insert(newString.prepend(space));
    }
}

void UIVMFilterLineEdit::clearAll()
{
    if (text().isEmpty())
        return;
    sltClearAll();
}

void UIVMFilterLineEdit::mousePressEvent(QMouseEvent * event)
{
    /* Simulate double mouse click to select a word with a single click: */
    QLineEdit::mouseDoubleClickEvent(event);
}

void UIVMFilterLineEdit::paintEvent(QPaintEvent *event)
{
    /* Call to base-class: */
    QLineEdit::paintEvent(event);

    if (!m_pClearAllButton || !m_pRemoveTermButton)
        createButtons();
    int clearButtonSize = height();

    int deltaHeight = 0.5 * (height() - m_pClearAllButton->height());
#ifdef VBOX_WS_MAC
    m_pClearAllButton->setGeometry(width() - clearButtonSize - 2, deltaHeight, clearButtonSize, clearButtonSize);
#else
    m_pClearAllButton->setGeometry(width() - clearButtonSize - 1, deltaHeight, clearButtonSize, clearButtonSize);
#endif

    /* If we have a selected term move the m_pRemoveTermButton to the end of the
       or start of the word (depending on the location of the word within line edit itself: */
    if (hasSelectedText())
    {
        //int deltaHeight = 0.5 * (height() - m_pClearAllButton->height());
        m_pRemoveTermButton->show();
        int buttonSize = m_iRemoveTermButtonSize;
#if QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
        int charWidth = fontMetrics().horizontalAdvance('x');
#else
        int charWidth = fontMetrics().width('x');
#endif
#ifdef VBOX_WS_MAC
        int buttonLeft = cursorRect().left() + 1;
#else
        int buttonLeft = cursorRect().right() - 0.9 * charWidth;
#endif
        /* If buttonLeft is in far right of the line edit, move the
           button to left side of the selected word: */
        if (buttonLeft + buttonSize  >=  width() - clearButtonSize)
        {
            int selectionWidth = charWidth * selectedText().length();
            buttonLeft -= (selectionWidth + buttonSize);
        }
        m_pRemoveTermButton->setGeometry(buttonLeft, deltaHeight, buttonSize, buttonSize);
    }
    else
        m_pRemoveTermButton->hide();
}

void UIVMFilterLineEdit::sltRemoveFilterTerm()
{
    if (!hasSelectedText())
        return;
    emit sigFilterTermRemoved(selectedText());
    /* Remove the string from text() including the trailing space: */
    setText(text().remove(selectionStart(), selectedText().length() + m_iTrailingSpaceCount));
}

void UIVMFilterLineEdit::sltClearAll()
{
    /* Check if we have some text to avoid recursive calls: */
    if (text().isEmpty())
        return;

    clear();
    emit sigClearAll();
}

void UIVMFilterLineEdit::createButtons()
{
    if (!m_pRemoveTermButton)
    {
        m_pRemoveTermButton = new QToolButton(this);
        if (m_pRemoveTermButton)
        {
            m_pRemoveTermButton->setIcon(UIIconPool::iconSet(":/log_viewer_delete_filter_16px.png"));
            m_pRemoveTermButton->hide();
            connect(m_pRemoveTermButton, &QIToolButton::clicked, this, &UIVMFilterLineEdit::sltRemoveFilterTerm);
            const QSize sh = m_pRemoveTermButton->sizeHint();
            m_pRemoveTermButton->setStyleSheet("QToolButton { border: 0px none black; margin: 0px 0px 0px 0px; } QToolButton::menu-indicator {image: none;}");
            m_pRemoveTermButton->setFixedSize(sh);
        }
    }

    if (!m_pClearAllButton)
    {
        m_pClearAllButton = new QToolButton(this);
        if (m_pClearAllButton)
        {
            m_pClearAllButton->setIcon(UIIconPool::iconSet(":/log_viewer_delete_all_filters_16px.png"));
            connect(m_pClearAllButton, &QIToolButton::clicked, this, &UIVMFilterLineEdit::sltClearAll);
            const QSize sh = m_pClearAllButton->sizeHint();
            m_pClearAllButton->setStyleSheet("QToolButton { border: 0px none black; margin: 0px 0px 0px 0px; } QToolButton::menu-indicator {image: none;}");
            m_pClearAllButton->setFixedSize(sh);
        }
    }
    if (m_pRemoveTermButton && m_pClearAllButton)
        setMinimumHeight(qMax(m_pRemoveTermButton->minimumHeight(), m_pClearAllButton->minimumHeight()));
    else if (m_pRemoveTermButton)
        setMinimumHeight(m_pRemoveTermButton->minimumHeight());
    else if (m_pClearAllButton)
        setMinimumHeight(m_pClearAllButton->minimumHeight());
}


/*********************************************************************************************************************************
*   UIVMLogViewerFilterPanel implementation.                                                                                     *
*********************************************************************************************************************************/

UIVMLogViewerFilterPanel::UIVMLogViewerFilterPanel(QWidget *pParent, UIVMLogViewerWidget *pViewer)
    : UIVMLogViewerPanel(pParent, pViewer)
    , m_pFilterLabel(0)
    , m_pFilterComboBox(0)
    , m_pButtonGroup(0)
    , m_pAndRadioButton(0)
    , m_pOrRadioButton(0)
    , m_pRadioButtonContainer(0)
    , m_pAddFilterTermButton(0)
    , m_eFilterOperatorButton(AndButton)
    , m_pFilterTermsLineEdit(0)
    , m_pResultLabel(0)
    , m_iUnfilteredLineCount(0)
    , m_iFilteredLineCount(0)
{
    prepare();
}

QString UIVMLogViewerFilterPanel::panelName() const
{
    return "FilterPanel";
}

void UIVMLogViewerFilterPanel::applyFilter()
{
    if (isVisible())
        filter();
    else
        resetFiltering();
    retranslateUi();
    emit sigFilterApplied();
}

void UIVMLogViewerFilterPanel::filter()
{
    if (!viewer())
        return;
    QPlainTextEdit *pCurrentTextEdit = textEdit();
    if (!pCurrentTextEdit)
        return;

    UIVMLogPage *logPage = viewer()->currentLogPage();
    if (!logPage)
        return;

    const QString* originalLogString = logString();
    m_iUnfilteredLineCount = 0;
    m_iFilteredLineCount = 0;
    if (!originalLogString || originalLogString->isNull())
        return;
    QTextDocument *document = textDocument();
    if (!document)
        return;
    QStringList stringLines = originalLogString->split("\n");
    m_iUnfilteredLineCount = stringLines.size();

    if (m_filterTermSet.empty())
        resetFiltering();

    /* Prepare filter-data: */
    QString strFilteredText;
    int count = 0;
    for (int lineIdx = 0; lineIdx < stringLines.size(); ++lineIdx)
    {
        const QString& currentLineString = stringLines[lineIdx];
        if (currentLineString.isEmpty())
            continue;
        if (applyFilterTermsToString(currentLineString))
        {
            strFilteredText.append(currentLineString).append("\n");
            ++count;
        }
    }

    document->setPlainText(strFilteredText);
    m_iFilteredLineCount = document->lineCount();

    /* Move the cursor position to end: */
    QTextCursor cursor = pCurrentTextEdit->textCursor();
    cursor.movePosition(QTextCursor::End, QTextCursor::MoveAnchor);
    pCurrentTextEdit->setTextCursor(cursor);
    logPage->scrollToEnd();
}

void UIVMLogViewerFilterPanel::resetFiltering()
{
    UIVMLogPage *logPage = viewer()->currentLogPage();
    QTextDocument *document = textDocument();
    if (!logPage || !document)
        return;

    document->setPlainText(logPage->logString());
    m_iFilteredLineCount = document->lineCount();
    m_iUnfilteredLineCount = document->lineCount();
    logPage->scrollToEnd();
}

bool UIVMLogViewerFilterPanel::applyFilterTermsToString(const QString& string)
{
    /* Number of the filter terms contained with the @p string: */
    int hitCount = 0;

    for (QSet<QString>::const_iterator iterator = m_filterTermSet.begin();
        iterator != m_filterTermSet.end(); ++iterator)
    {
        /* Disregard empty and invalid filter terms: */
        const QString& filterTerm = *iterator;
        if (filterTerm.isEmpty())
            continue;
        const QRegularExpression rxFilterExp(filterTerm, QRegularExpression::CaseInsensitiveOption);
        if (!rxFilterExp.isValid())
            continue;

        if (string.contains(rxFilterExp))
        {
            ++hitCount;
            /* Early return */
            if (m_eFilterOperatorButton == OrButton)
                return true;
        }

        /* Early return */
        if (!string.contains(rxFilterExp) && m_eFilterOperatorButton == AndButton )
            return false;
    }
    /* All the terms are found within the @p string. To catch AND case: */
    if (hitCount == m_filterTermSet.size())
        return true;
    return false;
}


void UIVMLogViewerFilterPanel::sltAddFilterTerm()
{
    if (!m_pFilterComboBox)
        return;
    if (m_pFilterComboBox->currentText().isEmpty())
        return;

    /* Continue only if the term is new. */
    if (m_filterTermSet.contains(m_pFilterComboBox->currentText()))
        return;
    m_filterTermSet.insert(m_pFilterComboBox->currentText());

    /* Add the new filter term to line edit: */
    if (m_pFilterTermsLineEdit)
        m_pFilterTermsLineEdit->addFilterTerm(m_pFilterComboBox->currentText());

    /* Clear the content of the combo box: */
    m_pFilterComboBox->setCurrentText(QString());
    applyFilter();
}

void UIVMLogViewerFilterPanel::sltClearFilterTerms()
{
    if (m_filterTermSet.empty())
        return;
    m_filterTermSet.clear();
    applyFilter();
    if (m_pFilterTermsLineEdit)
        m_pFilterTermsLineEdit->clearAll();
}

void UIVMLogViewerFilterPanel::sltOperatorButtonChanged(QAbstractButton *pButton)
{
    int buttonId = m_pButtonGroup->id(pButton);
    if (buttonId < 0 || buttonId >= ButtonEnd)
        return;
    m_eFilterOperatorButton = static_cast<FilterOperatorButton>(buttonId);
    applyFilter();
}

void UIVMLogViewerFilterPanel::sltRemoveFilterTerm(const QString &termString)
{
    m_filterTermSet.remove(termString);
    applyFilter();
}

void UIVMLogViewerFilterPanel::prepareWidgets()
{
    if (!mainLayout())
        return;

    prepareRadioButtonGroup();

    /* Create combo/button layout: */
    QHBoxLayout *pComboButtonLayout = new QHBoxLayout;
    if (pComboButtonLayout)
    {
        pComboButtonLayout->setContentsMargins(0, 0, 0, 0);
#ifdef VBOX_WS_MAC
        pComboButtonLayout->setSpacing(5);
#else
        pComboButtonLayout->setSpacing(qApp->style()->pixelMetric(QStyle::PM_LayoutHorizontalSpacing) / 2);
#endif

        /* Create filter combo-box: */
        m_pFilterComboBox = new QComboBox;
        if (m_pFilterComboBox)
        {
            m_pFilterComboBox->setEditable(true);
            QStringList strFilterPresets;
            strFilterPresets << "" << "GUI" << "NAT" << "AHCI" << "VD"
                             << "Audio" << "VUSB" << "SUP" << "PGM" << "HDA"
                             << "HM" << "VMM" << "GIM" << "CPUM";
            strFilterPresets.sort();
            m_pFilterComboBox->addItems(strFilterPresets);
            pComboButtonLayout->addWidget(m_pFilterComboBox);
        }

        /* Create add filter-term button: */
        m_pAddFilterTermButton = new QIToolButton;
        if (m_pAddFilterTermButton)
        {
            m_pAddFilterTermButton->setIcon(UIIconPool::iconSet(":/log_viewer_filter_add_16px.png"));
            pComboButtonLayout->addWidget(m_pAddFilterTermButton);
        }

        mainLayout()->addLayout(pComboButtonLayout, 1);
    }

    /* Create filter-term line-edit: */
    m_pFilterTermsLineEdit = new UIVMFilterLineEdit;
    if (m_pFilterTermsLineEdit)
    {
        m_pFilterTermsLineEdit->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
        mainLayout()->addWidget(m_pFilterTermsLineEdit, 3);
    }

    /* Create result label: */
    m_pResultLabel = new QLabel;
    if (m_pResultLabel)
    {
        m_pResultLabel->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
        mainLayout()->addWidget(m_pResultLabel, 0);
    }
}

void UIVMLogViewerFilterPanel::prepareRadioButtonGroup()
{
    /* Create radio-button container: */
    m_pRadioButtonContainer = new QFrame;
    if (m_pRadioButtonContainer)
    {
        /* Configure container: */
        m_pRadioButtonContainer->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        m_pRadioButtonContainer->setFrameStyle(QFrame::StyledPanel | QFrame::Sunken);

        /* Create container layout: */
        QHBoxLayout *pContainerLayout = new QHBoxLayout(m_pRadioButtonContainer);
        if (pContainerLayout)
        {
            /* Configure layout: */
#ifdef VBOX_WS_MAC
            pContainerLayout->setContentsMargins(5, 0, 0, 7);
            pContainerLayout->setSpacing(5);
#else
            pContainerLayout->setContentsMargins(qApp->style()->pixelMetric(QStyle::PM_LayoutLeftMargin) / 2, 0,
                                                 qApp->style()->pixelMetric(QStyle::PM_LayoutRightMargin) / 2, 0);
            pContainerLayout->setSpacing(qApp->style()->pixelMetric(QStyle::PM_LayoutHorizontalSpacing) / 2);
#endif

            /* Create button-group: */
            m_pButtonGroup = new QButtonGroup(this);
            if (m_pButtonGroup)
            {
                /* Create 'Or' radio-button: */
                m_pOrRadioButton = new QRadioButton;
                if (m_pOrRadioButton)
                {
                    /* Configure radio-button: */
                    m_pButtonGroup->addButton(m_pOrRadioButton, static_cast<int>(OrButton));
                    m_pOrRadioButton->setChecked(true);
                    m_pOrRadioButton->setText("Or");

                    /* Add into layout: */
                    pContainerLayout->addWidget(m_pOrRadioButton);
                }

                /* Create 'And' radio-button: */
                m_pAndRadioButton = new QRadioButton;
                if (m_pAndRadioButton)
                {
                    /* Configure radio-button: */
                    m_pButtonGroup->addButton(m_pAndRadioButton, static_cast<int>(AndButton));
                    m_pAndRadioButton->setText("And");

                    /* Add into layout: */
                    pContainerLayout->addWidget(m_pAndRadioButton);
                }
            }
        }

        /* Add into layout: */
        mainLayout()->addWidget(m_pRadioButtonContainer);
    }

    /* Initialize other related stuff: */
    m_eFilterOperatorButton = OrButton;
}

void UIVMLogViewerFilterPanel::prepareConnections()
{
    connect(m_pAddFilterTermButton, &QIToolButton::clicked, this,  &UIVMLogViewerFilterPanel::sltAddFilterTerm);
    connect(m_pButtonGroup, static_cast<void (QButtonGroup::*)(QAbstractButton *)>(&QButtonGroup::buttonClicked),
            this, &UIVMLogViewerFilterPanel::sltOperatorButtonChanged);
    connect(m_pFilterComboBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &UIVMLogViewerFilterPanel::sltAddFilterTerm);
    connect(m_pFilterTermsLineEdit, &UIVMFilterLineEdit::sigFilterTermRemoved,
            this, &UIVMLogViewerFilterPanel::sltRemoveFilterTerm);
    connect(m_pFilterTermsLineEdit, &UIVMFilterLineEdit::sigClearAll,
            this, &UIVMLogViewerFilterPanel::sltClearFilterTerms);
}


void UIVMLogViewerFilterPanel::retranslateUi()
{
    UIVMLogViewerPanel::retranslateUi();

    m_pFilterComboBox->setToolTip(UIVMLogViewerWidget::tr("Select or enter a term which will be used in filtering the log text"));
    m_pAddFilterTermButton->setToolTip(UIVMLogViewerWidget::tr("Add the filter term to the set of filter terms"));
    m_pResultLabel->setText(UIVMLogViewerWidget::tr("Showing %1/%2").arg(m_iFilteredLineCount).arg(m_iUnfilteredLineCount));
    m_pFilterTermsLineEdit->setToolTip(UIVMLogViewerWidget::tr("The filter terms list, select one to remove or click "
                                                               "the button on the right side to remove them all"));
    m_pRadioButtonContainer->setToolTip(UIVMLogViewerWidget::tr("The type of boolean operator for filter operation"));
}

bool UIVMLogViewerFilterPanel::eventFilter(QObject *pObject, QEvent *pEvent)
{
    /* Handle only events sent to viewer(): */
    if (pObject != viewer())
        return UIVMLogViewerPanel::eventFilter(pObject, pEvent);

    /* Depending on event-type: */
    switch (pEvent->type())
    {
        /* Process key press only: */
        case QEvent::KeyPress:
        {
            /* Cast to corresponding key press event: */
            QKeyEvent *pKeyEvent = static_cast<QKeyEvent*>(pEvent);

            /* Handle Ctrl+T key combination as a shortcut to focus search field: */
            if (pKeyEvent->QInputEvent::modifiers() == Qt::ControlModifier &&
                pKeyEvent->key() == Qt::Key_T)
            {
                if (isHidden())
                    show();
                m_pFilterComboBox->setFocus();
                return true;
            }
            else if (pKeyEvent->key() == Qt::Key_Return && m_pFilterComboBox && m_pFilterComboBox->hasFocus())
                sltAddFilterTerm();

            break;
        }
        default:
            break;
    }

    /* Call to base-class: */
    return UIVMLogViewerPanel::eventFilter(pObject, pEvent);
}

/** Handles the Qt show @a pEvent. */
void UIVMLogViewerFilterPanel::showEvent(QShowEvent *pEvent)
{
    UIVMLogViewerPanel::showEvent(pEvent);
    /* Set focus to combo-box: */
    m_pFilterComboBox->setFocus();
    applyFilter();
}

void UIVMLogViewerFilterPanel::hideEvent(QHideEvent *pEvent)
{
    UIVMLogViewerPanel::hideEvent(pEvent);
    applyFilter();
}

#include "UIVMLogViewerFilterPanel.moc"

/* $Id: UIVMLogViewerTextEdit.cpp $ */
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
#if defined(RT_OS_SOLARIS)
# include <QFontDatabase>
#endif
#include <QMenu>
#include <QPainter>
#include <QPlainTextEdit>
#include <QScrollBar>
#include <QStyle>
#include <QTextBlock>

/* GUI includes: */
#include "UIIconPool.h"
#include "UIVMLogViewerTextEdit.h"
#include "UIVMLogViewerWidget.h"

/** We use a modified scrollbar style for our QPlainTextEdits to get the
    markings on the scrollbars correctly. The default scrollbarstyle does not
    reveal the height of the pushbuttons on the scrollbar (on either side of it, with arrow on them)
    to compute the marking locations correctly. Thus we turn these push buttons off: */
const QString verticalScrollBarStyle("QScrollBar:vertical {"
                                     "border: 1px ridge grey; "
                                     "margin: 0px 0px 0 0px;}"
                                     "QScrollBar::handle:vertical {"
                                     "min-height: 10px;"
                                     "background: grey;}"
                                     "QScrollBar::add-line:vertical {"
                                     "width: 0px;}"
                                     "QScrollBar::sub-line:vertical {"
                                     "width: 0px;}");

const QString horizontalScrollBarStyle("QScrollBar:horizontal {"
                                       "border: 1px ridge grey; "
                                       "margin: 0px 0px 0 0px;}"
                                       "QScrollBar::handle:horizontal {"
                                       "min-height: 10px;"
                                       "background: grey;}"
                                       "QScrollBar::add-line:horizontal {"
                                       "height: 0px;}"
                                       "QScrollBar::sub-line:horizontal {"
                                       "height: 0px;}");


/*********************************************************************************************************************************
*   UIIndicatorScrollBar definition.                                                                                             *
*********************************************************************************************************************************/

class UIIndicatorScrollBar : public QScrollBar
{
    Q_OBJECT;

public:

    UIIndicatorScrollBar(QWidget *parent = 0);
    void setMarkingsVector(const QVector<float> &vector);
    void clearMarkingsVector();

protected:

    virtual void paintEvent(QPaintEvent *pEvent) RT_OVERRIDE;

private:

    /* Stores the relative (to scrollbar's height) positions of markings,
       where we draw a horizontal line. Values are in [0.0, 1.0]*/
    QVector<float> m_markingsVector;
};


/*********************************************************************************************************************************
*   UIIndicatorScrollBar implemetation.                                                                                          *
*********************************************************************************************************************************/

UIIndicatorScrollBar::UIIndicatorScrollBar(QWidget *parent /*= 0 */)
    :QScrollBar(parent)
{
    setStyleSheet(verticalScrollBarStyle);
}

void UIIndicatorScrollBar::setMarkingsVector(const QVector<float> &vector)
{
    m_markingsVector = vector;
}

void UIIndicatorScrollBar::clearMarkingsVector()
{
    m_markingsVector.clear();
}

void UIIndicatorScrollBar::paintEvent(QPaintEvent *pEvent) /* override */
{
    QScrollBar::paintEvent(pEvent);
    /* Put a red line to mark the bookmark positions: */
    for (int i = 0; i < m_markingsVector.size(); ++i)
    {
        QPointF p1 = QPointF(0, m_markingsVector[i] * height());
        QPointF p2 = QPointF(width(), m_markingsVector[i] * height());

        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setPen(QPen(QColor(255, 0, 0, 75), 1.1f));
        painter.drawLine(p1, p2);
    }
}


/*********************************************************************************************************************************
*   UILineNumberArea definition.                                                                                                 *
*********************************************************************************************************************************/

class UILineNumberArea : public QWidget
{
public:
    UILineNumberArea(UIVMLogViewerTextEdit *textEdit);
    QSize sizeHint() const;

protected:

    void paintEvent(QPaintEvent *event);
    void mouseMoveEvent(QMouseEvent *pEvent);
    void mousePressEvent(QMouseEvent *pEvent);

private:
    UIVMLogViewerTextEdit *m_pTextEdit;
};


/*********************************************************************************************************************************
*   UILineNumberArea implemetation.                                                                                              *
*********************************************************************************************************************************/

UILineNumberArea::UILineNumberArea(UIVMLogViewerTextEdit *textEdit)
    :QWidget(textEdit)
    , m_pTextEdit(textEdit)
{
    setMouseTracking(true);
}

QSize UILineNumberArea::sizeHint() const
{
    if (!m_pTextEdit)
        return QSize();
    return QSize(m_pTextEdit->lineNumberAreaWidth(), 0);
}

void UILineNumberArea::paintEvent(QPaintEvent *event)
{
    if (m_pTextEdit)
        m_pTextEdit->lineNumberAreaPaintEvent(event);
}

void UILineNumberArea::mouseMoveEvent(QMouseEvent *pEvent)
{
    if (m_pTextEdit)
        m_pTextEdit->setMouseCursorLine(m_pTextEdit->lineNumberForPos(pEvent->pos()));
    update();
}

void UILineNumberArea::mousePressEvent(QMouseEvent *pEvent)
{
    if (m_pTextEdit)
        m_pTextEdit->toggleBookmark(m_pTextEdit->bookmarkForPos(pEvent->pos()));
}


/*********************************************************************************************************************************
*   UIVMLogViewerTextEdit implemetation.                                                                                         *
*********************************************************************************************************************************/

UIVMLogViewerTextEdit::UIVMLogViewerTextEdit(QWidget* parent /* = 0 */)
    : QIWithRetranslateUI<QPlainTextEdit>(parent)
    , m_pLineNumberArea(0)
    , m_mouseCursorLine(-1)
    , m_bShownTextIsFiltered(false)
    , m_bShowLineNumbers(true)
    , m_bWrapLines(true)
    , m_bHasContextMenu(false)
    , m_iVerticalScrollBarValue(0)
{
    configure();
    prepare();
}

void UIVMLogViewerTextEdit::configure()
{
    setMouseTracking(true);

    /* Prepare modified standard palette: */
    QPalette pal = QApplication::palette();
    pal.setColor(QPalette::Inactive, QPalette::Highlight, pal.color(QPalette::Active, QPalette::Highlight));
    pal.setColor(QPalette::Inactive, QPalette::HighlightedText, pal.color(QPalette::Active, QPalette::HighlightedText));
    setPalette(pal);

    /* Configure this' wrap mode: */
    setWrapLines(false);
    setReadOnly(true);
}

void UIVMLogViewerTextEdit::prepare()
{
    prepareWidgets();
    retranslateUi();
}

void UIVMLogViewerTextEdit::prepareWidgets()
{
    m_pLineNumberArea = new UILineNumberArea(this);

    connect(this, &UIVMLogViewerTextEdit::blockCountChanged, this, &UIVMLogViewerTextEdit::sltUpdateLineNumberAreaWidth);
    connect(this, &UIVMLogViewerTextEdit::updateRequest, this, &UIVMLogViewerTextEdit::sltHandleUpdateRequest);
    sltUpdateLineNumberAreaWidth(0);

    setVerticalScrollBar(new UIIndicatorScrollBar());
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    QScrollBar *pHorizontalScrollBar = horizontalScrollBar();
    if (pHorizontalScrollBar)
        pHorizontalScrollBar->setStyleSheet(horizontalScrollBarStyle);
}

void UIVMLogViewerTextEdit::setCurrentFont(QFont font)
{
    setFont(font);
    if (m_pLineNumberArea)
        m_pLineNumberArea->setFont(font);
}

void UIVMLogViewerTextEdit::saveScrollBarPosition()
{
    if (verticalScrollBar())
        m_iVerticalScrollBarValue = verticalScrollBar()->value();
}

void UIVMLogViewerTextEdit::restoreScrollBarPosition()
{
    QScrollBar *pBar = verticalScrollBar();
    if (pBar && pBar->maximum() >= m_iVerticalScrollBarValue && pBar->minimum() <= m_iVerticalScrollBarValue)
        pBar->setValue(m_iVerticalScrollBarValue);
}

void UIVMLogViewerTextEdit::setCursorPosition(int iPosition)
{
    QTextCursor cursor = textCursor();
    cursor.setPosition(iPosition);
    setTextCursor(cursor);
    centerCursor();
}

int UIVMLogViewerTextEdit::lineNumberAreaWidth()
{
    if (!m_bShowLineNumbers)
        return 0;

    int digits = 1;
    int max = qMax(1, blockCount());
    while (max >= 10) {
        max /= 10;
        ++digits;
    }

#if QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
    int space = 3 + fontMetrics().horizontalAdvance(QLatin1Char('9')) * digits;
#else
    int space = 3 + fontMetrics().width(QLatin1Char('9')) * digits;
#endif

    return space;
}

void UIVMLogViewerTextEdit::lineNumberAreaPaintEvent(QPaintEvent *event)
{
    if (!m_bShowLineNumbers)
        return;
    QPainter painter(m_pLineNumberArea);
    painter.fillRect(event->rect(), Qt::lightGray);
    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top = (int) blockBoundingGeometry(block).translated(contentOffset()).top();
    int bottom = top + (int) blockBoundingRect(block).height();
    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) {
            QString number = QString::number(blockNumber + 1);
            /* Mark this line if it is bookmarked, but only if the text is not filtered. */
            if (m_bookmarkLineSet.contains(blockNumber + 1) && !m_bShownTextIsFiltered)
            {
                QPainterPath path;
                path.addRect(0, top, m_pLineNumberArea->width(), m_pLineNumberArea->fontMetrics().lineSpacing());
                painter.fillPath(path, QColor(204, 255, 51, 125));
                painter.drawPath(path);
            }
            /* Draw a unfilled red rectangled around the line number to indicate line the mouse cursor is currently
               hovering on. Do this only if mouse is over the ext edit or the context menu is around: */
            if ((blockNumber + 1) == m_mouseCursorLine && (underMouse() || m_bHasContextMenu))
            {
                painter.setPen(Qt::red);
                painter.drawRect(0, top, m_pLineNumberArea->width(), m_pLineNumberArea->fontMetrics().lineSpacing());
            }

            painter.setPen(Qt::black);
            painter.drawText(0, top, m_pLineNumberArea->width(), m_pLineNumberArea->fontMetrics().lineSpacing(),
                             Qt::AlignRight, number);
        }
        block = block.next();
        top = bottom;
        bottom = top + (int) blockBoundingRect(block).height();
        ++blockNumber;
    }
}

void UIVMLogViewerTextEdit::retranslateUi()
{
    m_strBackgroungText = QString(UIVMLogViewerWidget::tr("Filtered"));
}

void UIVMLogViewerTextEdit::contextMenuEvent(QContextMenuEvent *pEvent)
{
    /* If shown text is filtered, do not create Bookmark action since
       we disable all bookmarking related functionalities in this case. */
    if (m_bShownTextIsFiltered)
    {
        QPlainTextEdit::contextMenuEvent(pEvent);
        return;
    }
    m_bHasContextMenu = true;
    QMenu *menu = createStandardContextMenu();


    QAction *pAction = menu->addAction(UIVMLogViewerWidget::tr("Bookmark"));
    if (pAction)
    {
        pAction->setCheckable(true);
        UIVMLogBookmark menuBookmark = bookmarkForPos(pEvent->pos());
        pAction->setChecked(m_bookmarkLineSet.contains(menuBookmark.m_iLineNumber));
        if (pAction->isChecked())
            pAction->setIcon(UIIconPool::iconSet(":/log_viewer_bookmark_on_16px.png"));
        else
            pAction->setIcon(UIIconPool::iconSet(":/log_viewer_bookmark_off_16px.png"));

        m_iContextMenuBookmark = menuBookmark;
        connect(pAction, &QAction::triggered, this, &UIVMLogViewerTextEdit::sltBookmark);

    }
    menu->exec(pEvent->globalPos());

    if (pAction)
        disconnect(pAction, &QAction::triggered, this, &UIVMLogViewerTextEdit::sltBookmark);

    delete menu;
    m_bHasContextMenu = false;
}

void UIVMLogViewerTextEdit::resizeEvent(QResizeEvent *pEvent)
{
    QPlainTextEdit::resizeEvent(pEvent);
    if (m_pLineNumberArea)
    {
        QRect cr = contentsRect();
        m_pLineNumberArea->setGeometry(QRect(cr.left(), cr.top(), lineNumberAreaWidth(), cr.height()));
    }
}

void UIVMLogViewerTextEdit::mouseMoveEvent(QMouseEvent *pEvent)
{
    setMouseCursorLine(lineNumberForPos(pEvent->pos()));
    if (m_pLineNumberArea)
        m_pLineNumberArea->update();
    QPlainTextEdit::mouseMoveEvent(pEvent);
}

void UIVMLogViewerTextEdit::leaveEvent(QEvent * pEvent)
{
    QPlainTextEdit::leaveEvent(pEvent);
    /* Force a redraw as mouse leaves this to remove the mouse
       cursor track rectangle (the red rectangle we draw on the line number area). */
    update();
}

void UIVMLogViewerTextEdit::sltUpdateLineNumberAreaWidth(int /* newBlockCount */)
{
    setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
}

void UIVMLogViewerTextEdit::sltHandleUpdateRequest(const QRect &rect, int dy)
{
    if (dy)
        m_pLineNumberArea->scroll(0, dy);
    else
        m_pLineNumberArea->update(0, rect.y(), m_pLineNumberArea->width(), rect.height());

    if (rect.contains(viewport()->rect()))
        sltUpdateLineNumberAreaWidth(0);

    if (viewport())
        viewport()->update();
}

void UIVMLogViewerTextEdit::sltBookmark()
{
    toggleBookmark(m_iContextMenuBookmark);
}

void UIVMLogViewerTextEdit::setScrollBarMarkingsVector(const QVector<float> &vector)
{
    UIIndicatorScrollBar* vScrollBar = qobject_cast<UIIndicatorScrollBar*>(verticalScrollBar());
    if (vScrollBar)
        vScrollBar->setMarkingsVector(vector);
}

void UIVMLogViewerTextEdit::clearScrollBarMarkingsVector()
{
    UIIndicatorScrollBar* vScrollBar = qobject_cast<UIIndicatorScrollBar*>(verticalScrollBar());
    if (vScrollBar)
        vScrollBar->clearMarkingsVector();
}

void UIVMLogViewerTextEdit::scrollToLine(int lineNumber)
{
    QTextDocument* pDocument = document();
    if (!pDocument)
        return;

    moveCursor(QTextCursor::End);
    int halfPageLineCount = 0.5 * visibleLineCount() ;
    QTextCursor cursor(pDocument->findBlockByLineNumber(qMax(lineNumber - halfPageLineCount, 0)));
    setTextCursor(cursor);
}

void UIVMLogViewerTextEdit::scrollToEnd()
{
    moveCursor(QTextCursor::End);
    ensureCursorVisible();
}

int UIVMLogViewerTextEdit::visibleLineCount()
{
    int height = 0;
    if (viewport())
        height = viewport()->height();
    if (verticalScrollBar() && verticalScrollBar()->isVisible())
        height -= horizontalScrollBar()->height();
    int singleLineHeight = fontMetrics().lineSpacing();
    if (singleLineHeight == 0)
        return 0;
    return height / singleLineHeight;
}

void UIVMLogViewerTextEdit::setBookmarkLineSet(const QSet<int>& lineSet)
{
    m_bookmarkLineSet = lineSet;
    update();
}

int  UIVMLogViewerTextEdit::lineNumberForPos(const QPoint &position)
{
    QTextCursor cursor = cursorForPosition(position);
    QTextBlock block = cursor.block();
    return block.blockNumber() + 1;
}

UIVMLogBookmark UIVMLogViewerTextEdit::bookmarkForPos(const QPoint &position)
{
    QTextCursor cursor = cursorForPosition(position);
    QTextBlock block = cursor.block();
    return UIVMLogBookmark(block.blockNumber() + 1, cursor.position(), block.text());
}

void UIVMLogViewerTextEdit::setMouseCursorLine(int lineNumber)
{
    m_mouseCursorLine = lineNumber;
}

void UIVMLogViewerTextEdit::toggleBookmark(const UIVMLogBookmark& bookmark)
{
    if (m_bShownTextIsFiltered)
        return;

    if (m_bookmarkLineSet.contains(bookmark.m_iLineNumber))
        emit sigDeleteBookmark(bookmark);
    else
        emit sigAddBookmark(bookmark);
}

void UIVMLogViewerTextEdit::setShownTextIsFiltered(bool warning)
{
    if (m_bShownTextIsFiltered == warning)
        return;
    m_bShownTextIsFiltered = warning;
    if (viewport())
        viewport()->update();
}

void UIVMLogViewerTextEdit::setShowLineNumbers(bool bShowLineNumbers)
{
    if (m_bShowLineNumbers == bShowLineNumbers)
        return;
    m_bShowLineNumbers = bShowLineNumbers;
    emit updateRequest(viewport()->rect(), 0);
}

bool UIVMLogViewerTextEdit::showLineNumbers() const
{
    return m_bShowLineNumbers;
}

void UIVMLogViewerTextEdit::setWrapLines(bool bWrapLines)
{
    if (m_bWrapLines == bWrapLines)
        return;
    m_bWrapLines = bWrapLines;
    if (m_bWrapLines)
    {
        setLineWrapMode(QPlainTextEdit::WidgetWidth);
        setWordWrapMode(QTextOption::WordWrap);
    }
    else
    {
        setWordWrapMode(QTextOption::NoWrap);
        setWordWrapMode(QTextOption::NoWrap);
    }
    update();
}

bool UIVMLogViewerTextEdit::wrapLines() const
{
    return m_bWrapLines;
}

int  UIVMLogViewerTextEdit::currentVerticalScrollBarValue() const
{
    if (!verticalScrollBar())
        return -1;
    return verticalScrollBar()->value();
}

void UIVMLogViewerTextEdit::setCurrentVerticalScrollBarValue(int value)
{
    if (!verticalScrollBar())
        return;

    setCenterOnScroll(true);

    verticalScrollBar()->setValue(value);
    verticalScrollBar()->setSliderPosition(value);
    viewport()->update();
    update();
}

#include "UIVMLogViewerTextEdit.moc"

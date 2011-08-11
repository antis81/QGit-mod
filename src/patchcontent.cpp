/*
    Author: Marco Costalba (C) 2005-2007

    Copyright: See COPYING file that comes with this distribution

*/
#include <QScrollBar>
#include <QTextCharFormat>
#include "common.h"
#include "domain.h"
#include "git.h"
#include "myprocess.h"
#include "patchcontent.h"
#include "stdio.h"
#include <QPainter>
#include "linenumberarea.h"
#include "patchcontentfindsupport.h"
#include "patchtextblockuserdata.h"

PatchContent::PatchContent(QWidget* parent) : QPlainTextEdit(parent) {
    fitted_height = 0;
    diffLoaded = seekTarget = false;
    curFilter = prevFilter = VIEW_ALL;

    m_findSupport = new PatchContentFindSupport(this);

    setFont(QGit::TYPE_WRITER_FONT);
    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
    lineNumberArea = new LineNumberArea(this);

    connect(this, SIGNAL(blockCountChanged(int)), this, SLOT(updateLineNumberAreaWidth(int)));
    connect(this, SIGNAL(updateRequest(QRect,int)), this, SLOT(updateLineNumberArea(QRect,int)));
    connect(this, SIGNAL(cursorPositionChanged()), this, SLOT(highlightCurrentLine()));
    connect(this, SIGNAL(textChanged()), this, SLOT(onTextChanged()));

    updateLineNumberAreaWidth(0);
    highlightCurrentLine();
}

PatchContent::~PatchContent()
{
    if (m_findSupport) {
        delete m_findSupport;
        m_findSupport = NULL;
    }
}

void PatchContent::setup(Domain*, Git* g) {
    git = g;
}

void PatchContent::clear() {

    git->cancelProcess(proc);
    QPlainTextEdit::clear();
    patchRowData.clear();
    halfLine = "";
    diffLoaded = false;
    seekTarget = !target.isEmpty();
}

void PatchContent::refresh() {

    int topPara = topToLineNum();
    setUpdatesEnabled(false);
    QByteArray tmp(patchRowData);
    clear();
    patchRowData = tmp;
    processData(patchRowData, &topPara);
    scrollLineToTop(topPara);
    fitHeightToDocument();
    setUpdatesEnabled(true);
}

void PatchContent::scrollCursorToTop() {
    QTextCursor cursor = textCursor();
    verticalScrollBar()->setValue(cursor.blockNumber());
}

void PatchContent::scrollLineToTop(int lineNum) {

    QTextCursor tc = textCursor();
    tc.movePosition(QTextCursor::Start);
    tc.movePosition(QTextCursor::NextBlock, QTextCursor::MoveAnchor, lineNum);
    setTextCursor(tc);
    scrollCursorToTop();
}

int PatchContent::positionToLineNum(int pos) {

    QTextCursor tc = textCursor();
    tc.setPosition(pos);
    return tc.blockNumber();
}

int PatchContent::topToLineNum() {

    return cursorForPosition(QPoint(1, 1)).blockNumber();
}

bool PatchContent::centerTarget(SCRef target) {

    moveCursor(QTextCursor::Start);

    // find() updates cursor position
    if (!find(target, QTextDocument::FindCaseSensitively | QTextDocument::FindWholeWords))
        return false;

    // move to the beginning of the line
    moveCursor(QTextCursor::StartOfLine);

    // grap copy of current cursor state
    QTextCursor tc = textCursor();

    // move the target line to the top
    moveCursor(QTextCursor::End);
    setTextCursor(tc);

    return true;
}

void PatchContent::centerOnFileHeader(StateInfo& st) {

    if (st.fileName().isEmpty())
        return;

    target = st.fileName();
    bool combined = (st.isMerge() && !st.allMergeFiles());
    git->formatPatchFileHeader(&target, st.sha(), st.diffToSha(), combined, st.allMergeFiles());
    seekTarget = !target.isEmpty();
    if (seekTarget)
        seekTarget = !centerTarget(target);
}

void PatchContent::centerMatch(int id) {

    if (m_findSupport->matches.count() <= id)
        return;

    PatchContentFindSupport::MatchSelection selection = m_findSupport->matches[id];
    QTextCursor cursor = QTextCursor(textCursor());
    cursor.setPosition(selection.from);
    cursor.setPosition(selection.to, QTextCursor::KeepAnchor);
    setTextCursor(cursor);
    scrollCursorToTop();
}

void PatchContent::procReadyRead(const QByteArray& data) {

    patchRowData.append(data);
    if (document()->isEmpty() && isVisible())
        processData(data);
}

void PatchContent::typeWriterFontChanged() {

    setFont(QGit::TYPE_WRITER_FONT);
    setPlainText(toPlainText());
}

void PatchContent::processData(const QByteArray& fileChunk, int* prevLineNum) {

    QString newLines;
    if (!QGit::stripPartialParaghraps(fileChunk, &newLines, &halfLine))
        return;

    const QStringList sl(newLines.split('\n', QString::KeepEmptyParts));

    setUpdatesEnabled(false);
    long* lastStartNumbers = NULL;
    int partCount = 0;
    lineNumberColumnCount = 0;
    bool partHeaderDetected = false;

    FOREACH_SL (row, sl) {
        const QString& r = *row;
        RowType rowType = getRowType(r);

        if (rowType == ROW_PART_HEADER) {
            partHeaderDetected = true;
            partCount = 0;
            QString header(*row);

            int stringLength = header.length();
            while(header[partCount] == '@' && partCount < stringLength) {
                partCount++;
            }
            if (partCount > 0) {
                int eoh = header.indexOf('@', partCount);
                QStringList parts(header.mid(partCount, eoh-partCount).split(' ', QString::SkipEmptyParts));
                if (lastStartNumbers) {
                    delete[] lastStartNumbers;
                }
                partCount = parts.length();
                if (partCount > lineNumberColumnCount) lineNumberColumnCount = partCount;

                lastStartNumbers = new long[partCount];
                for(int part = 0; part < partCount; part++) {
                    QStringList partData(parts[part].split(','));
                    lastStartNumbers[part] = partData[0].toLong();
                    if (lastStartNumbers[part] < 0) lastStartNumbers[part] = -lastStartNumbers[part];
                }
            }
        }

        QTextCursor tc(textCursor());
        tc.movePosition(QTextCursor::End, QTextCursor::MoveAnchor);
        tc.insertBlock();
        tc.insertText(*row);
        QTextBlock block(tc.block());


        PatchTextBlockUserData* ud = new PatchTextBlockUserData();
        ud->rowType = rowType;
        if (partHeaderDetected && rowType != ROW_PART_HEADER) {
            QString text(*row);
            long* rowNumbers = new long[partCount];
            int* mask = new int[partCount];
            int maskElement = 0;

            rowType = ROW_CONTEXT;
            int maskSum = 0;
            int part;
            for (part = 0; part < partCount-1 && part < text.length(); part++) {
                QChar c = text.at(part);
                if (c == '+') {
                    maskElement = 1;
                } else if (c == '-') {
                    maskElement = -1;
                } else {
                    maskElement = 0;
                }
                maskSum += maskElement;
                mask[part] = maskElement;
            }

            if (maskSum > 0) {
                for (part = 0; part < partCount-1; part++) {
                    if (mask[part] == 0) {
                        rowNumbers[part] = lastStartNumbers[part];
                        lastStartNumbers[part]++;
                    } else {
                        rowNumbers[part] = -1;
                    }
                }
                rowNumbers[part] = lastStartNumbers[part];
                lastStartNumbers[part]++;
                rowType = ROW_ADDED;
            } else if (maskSum < 0) {
                for (part = 0; part < partCount-1; part++) {
                    if (mask[part] < 0) {
                        rowNumbers[part] = lastStartNumbers[part];
                        lastStartNumbers[part]++;
                    } else {
                        rowNumbers[part] = -1;
                    }
                }
                rowNumbers[part] = -1;
                rowType = ROW_REMOVED;
            } else {
                rowType = ROW_CONTEXT;
                for (part = 0; part < partCount; part++) {
                    rowNumbers[part] = lastStartNumbers[part];
                    lastStartNumbers[part]++;
                }
            }
            delete mask;

            ud->rowType = rowType;
            ud->rowNumbers = rowNumbers;
            ud->partCount = partCount;
        }

        block.setUserData(ud);
        formatBlock(tc, block);
    }
    if (lastStartNumbers) {
        delete[] lastStartNumbers;
    }

    QScrollBar* vsb = verticalScrollBar();
    vsb->setValue(vsb->value() + cursorRect().top());
    setUpdatesEnabled(true);
}

void PatchContent::procFinished() {

    if (!patchRowData.endsWith("\n"))
        patchRowData.append('\n'); // flush pending half lines

    refresh(); // show patchRowData content

    if (seekTarget)
                seekTarget = !centerTarget(target);

    diffLoaded = true;

    if (m_findSupport->find()) {
        updateMatchesHighlight();
        centerMatch();
    }
}


void PatchContent::updateMatchesHighlight()
{
    if (!m_findSupport || m_findSupport->matches.isEmpty()) {
        return;
    }

    QTextCharFormat cf;
    cf.setBackground(QGit::ORANGE);
    cf.setForeground(Qt::white);
    FOREACH(PatchContentFindSupport::Matches, it, m_findSupport->matches) {
        QTextCursor cursor(textCursor());
        PatchContentFindSupport::MatchSelection ms = *it;
        cursor.setPosition(ms.from, QTextCursor::MoveAnchor);
        cursor.setPosition(ms.to, QTextCursor::KeepAnchor);
        cursor.mergeCharFormat(cf);
    }
}

void PatchContent::on_highlightPatch(const QString& exp, bool re) {
    m_findSupport->setText(exp, re);
    if (diffLoaded)
        procFinished();
}

void PatchContent::update(StateInfo& st) {

    bool combined = (st.isMerge() && !st.allMergeFiles());
    clear();
    proc = git->getDiff(st.sha(), this, st.diffToSha(), combined); // non blocking
}

PatchContent::RowType PatchContent::getRowType(const QString& row) {
    if (row.isEmpty())
        return ROW_OTHER;

    const char firstChar = row.at(0).toLatin1();
    switch (firstChar) {
        case '@':
            return ROW_PART_HEADER;
        case '+':
            if (row.startsWith("+++")) {
                return ROW_FILE_NEW;
            }
            return ROW_ADDED;
        case '-':
            if (row.startsWith("---")) {
                return ROW_FILE_OLD;
            }
            return ROW_REMOVED;
        case 'c':
        case 'd':
        case 'i':
        case 'n':
        case 'o':
        case 'r':
        case 's':
            if (row.startsWith("diff --git a/")) {
                return ROW_FILE_HEADER;
            }

            if (row.startsWith("copy ")
                    || row.startsWith("index ")
                    || row.startsWith("new ")
                    || row.startsWith("old ")
                    || row.startsWith("rename ")
                    || row.startsWith("similarity ")) {
                return ROW_FILE_HEADER;
            }

            if (row.startsWith("diff --combined")) {
                return ROW_DIFF_COMBINED;
            }
            break;
        case ' ':
            return ROW_CONTEXT;
    }
    return ROW_OTHER;
}

void PatchContent::formatBlock(QTextCursor& cursor, QTextBlock& block) {
    PatchTextBlockUserData* ud = static_cast<PatchTextBlockUserData*>(block.userData());
    formatBlock(cursor, block, ud->rowType);
}


void PatchContent::formatRow(QTextCursor tc) {
    //tc.select(QTextCursor::BlockUnderCursor);
    QTextBlock block(tc.block());
    const QString& text = block.text();
    RowType rowType = getRowType(text);
    formatBlock(tc, block, rowType);
}

void PatchContent::formatBlock(QTextCursor& cursor, QTextBlock& block, PatchContent::RowType rowType) {
    // TODO: make configurable color constants
    // TODO: make colormap { type => color and others font styles}
    QColor lr(255, 220, 220);
    QColor lg(220, 255, 220);
    QTextCharFormat charFormat;
    QTextBlockFormat blockFormat;
    charFormat.setForeground(Qt::black);
    blockFormat.setBackground(QGit::PATCH_BACKGROUND);
    switch (rowType) {
    case ROW_PART_HEADER:
        charFormat.setForeground(Qt::white);
        blockFormat.setBackground(QGit::LIGHT_BLUE);
        break;
    case ROW_ADDED:
    case ROW_FILE_NEW:
        charFormat.setForeground(Qt::darkGreen);
        blockFormat.setBackground(lg);
        break;
    case ROW_REMOVED:
    case ROW_FILE_OLD:
        charFormat.setForeground(Qt::red);
        blockFormat.setBackground(lr);
        break;
    case ROW_FILE_HEADER:
    case ROW_DIFF_COMBINED:
        charFormat.setForeground(Qt::white);
        blockFormat.setBackground(QGit::DARK_ORANGE);
        break;
    case ROW_OTHER:        
        break;
    case ROW_CONTEXT:
        charFormat.setForeground(Qt::blue);
        break;
    }
    if (blockFormat.isValid()) {
        cursor.mergeBlockFormat(blockFormat);
    }
    if (charFormat.isValid()) {
        cursor.select(QTextCursor::BlockUnderCursor);
        cursor.setCharFormat(charFormat);
    }
}


int PatchContent::lineNumberAreaWidth()
{

    int digits = 4;
    int space = 3 + (fontMetrics().width(QLatin1Char('9')) * digits + 3) * lineNumberColumnCount;

    return space;
}



void PatchContent::updateLineNumberAreaWidth(int /* newBlockCount */)
{
    setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
}



void PatchContent::updateLineNumberArea(const QRect &rect, int dy)
{
    if (dy)
        lineNumberArea->scroll(0, dy);
    else
        lineNumberArea->update(0, rect.y(), lineNumberArea->width(), rect.height());

    if (rect.contains(viewport()->rect()))
        updateLineNumberAreaWidth(0);
}



void PatchContent::resizeEvent(QResizeEvent *e)
{
    QPlainTextEdit::resizeEvent(e);

    QRect cr = contentsRect();
    lineNumberArea->setGeometry(QRect(cr.left(), cr.top(), lineNumberAreaWidth(), cr.height()));
}



void PatchContent::highlightCurrentLine()
{
    QList<QTextEdit::ExtraSelection> extraSelections;

    if (!isReadOnly()) {
        QTextEdit::ExtraSelection selection;

        QColor lineColor = QColor(Qt::yellow).lighter(160);

        selection.format.setBackground(lineColor);
        selection.format.setProperty(QTextFormat::FullWidthSelection, true);
        selection.cursor = textCursor();
        selection.cursor.clearSelection();
        extraSelections.append(selection);
    }

    setExtraSelections(extraSelections);
}


void PatchContent::lineNumberAreaPaintEvent(QPaintEvent *event)
{
    QPainter painter(lineNumberArea);

    const int shadowStrength = 20;
    QColor backColorLight(QGit::LINE_NUMBERS_BACKGROUND.red() + shadowStrength,
                          QGit::LINE_NUMBERS_BACKGROUND.green() + shadowStrength,
                          QGit::LINE_NUMBERS_BACKGROUND.blue() + shadowStrength);
    QColor backColorDark(QGit::LINE_NUMBERS_BACKGROUND.red() - shadowStrength,
                          QGit::LINE_NUMBERS_BACKGROUND.green() - shadowStrength,
                          QGit::LINE_NUMBERS_BACKGROUND.blue() - shadowStrength);

    painter.fillRect(event->rect(), QGit::LINE_NUMBERS_BACKGROUND);

    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top = (int) blockBoundingGeometry(block).translated(contentOffset()).top();
    int bottom = top + (int) blockBoundingRect(block).height();
    int width = lineNumberArea->width();

    for (int c = 1; c < lineNumberColumnCount; c++) {
        int x = c * width / lineNumberColumnCount;
        painter.setPen(backColorLight);
        painter.drawLine(x, event->rect().top(), x, event->rect().bottom());
        painter.setPen(backColorDark);
        painter.drawLine(x + 1, event->rect().top(), x + 1, event->rect().bottom());
    }


    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) {

            PatchTextBlockUserData* ud = static_cast<PatchTextBlockUserData*>(block.userData());
            if (ud && ud->partCount > 0 && ud->rowNumbers) {
                for (int c = 0; c < ud->partCount; c++) {
                    QString number;
                    if (ud->rowNumbers[c] >= 0) {
                        number = QString::number(ud->rowNumbers[c]);

                        painter.setPen(QGit::LINE_NUMBERS_FOREGROUND);
                        painter.drawText(0, top, (c + 1) * lineNumberArea->width() / lineNumberColumnCount - 3, fontMetrics().height(),
                                        Qt::AlignRight, number);
                    }
                }
            }
        }

        block = block.next();
        top = bottom;
        bottom = top + (int) blockBoundingRect(block).height();
        ++blockNumber;
    }
}


void PatchContent::setFilter(PatchFilter filter)
{
    prevFilter = curFilter;
    curFilter = filter;
    refresh();
}

QSize PatchContent::sizeHint() const {
  QSize sizehint = QPlainTextEdit::sizeHint();
  sizehint.setHeight(this->fitted_height);
  return sizehint;
}

void PatchContent::fitHeightToDocument() {
      this->document()->setTextWidth(this->viewport()->width());
      QSize document_size(this->document()->size().toSize());

      this->fitted_height = qRound( (document_size.height() + 2) * fontMetrics().lineSpacing() + document()->documentMargin() * 2 );
      this->updateGeometry();
}

void PatchContent::onTextChanged()
{
    //fitHeightToDocument();
}


FindSupport* PatchContent::findSupport()
{
    return m_findSupport;
}


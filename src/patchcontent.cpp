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

PatchContent::PatchContent(QWidget* parent) : QPlainTextEdit(parent) {
    fitted_height = 0;
    diffLoaded = seekTarget = false;
    curFilter = prevFilter = VIEW_ALL;

    pickAxeRE.setMinimal(true);
    pickAxeRE.setCaseSensitivity(Qt::CaseInsensitive);

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

void PatchContent::setup(Domain*, Git* g) {

    git = g;
}

void PatchContent::clear() {

    git->cancelProcess(proc);
    QPlainTextEdit::clear();
    patchRowData.clear();
    halfLine = "";
    matches.clear();
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

    QRect r = cursorRect();
    QScrollBar* vsb = verticalScrollBar();
    vsb->setValue(vsb->value() + r.top());
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

    if (matches.count() <= id)
        return;
//FIXME
//    patchTab->textEditDiff->setSelection(matches[id].paraFrom, matches[id].indexFrom,
//                                         matches[id].paraTo, matches[id].indexTo);
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
    FOREACH_SL (row, sl) {
        RowType rowType = getRowType(*row);
        if (rowType == ROW_PART_HEADER) {
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
                lastStartNumbers = new long[partCount];
                for(int part = 0; part < partCount; part++) {
                    QStringList partData(parts[part].split(','));
                    lastStartNumbers[part] = partData[0].toLong();
                    if (lastStartNumbers[part] < 0) lastStartNumbers[part] = -lastStartNumbers[part];
                }
            }
        }

        QTextCursor tc(textCursor());
        tc.movePosition(QTextCursor::End, QTextCursor::KeepAnchor);
        tc.insertBlock();
        tc.insertText(*row);
        QTextBlock block(tc.block());


        PatchTextBlockUserData* ud = new PatchTextBlockUserData();
        ud->rowType = rowType;
        if (lastStartNumbers) {
            if (rowType == ROW_REMOVED || rowType == ROW_ADDED || rowType == ROW_CONTEXT) {
                long* rowNumbers = new long[partCount];
                for(int part = 0; part < partCount; part++) {
                    rowNumbers[part] = lastStartNumbers[part];
                }
                ud->rowNumbers = rowNumbers;
                ud->partCount = partCount;
                if (rowType == ROW_REMOVED) {
                    ud->rowNumbers[1] = -1;
                }
                if (rowType == ROW_ADDED) {
                    ud->rowNumbers[0] = -1;
                }
            }
            if (rowType == ROW_REMOVED || rowType == ROW_CONTEXT) {
                lastStartNumbers[0]++;
            }
            if (rowType == ROW_ADDED || rowType == ROW_CONTEXT) {
                lastStartNumbers[1]++;
            }
        }

        block.setUserData(ud);
        formatBlock(tc, block);
    }
    if (lastStartNumbers) {
        delete[] lastStartNumbers;
    }

//    if (prevLineNum || curFilter != VIEW_ALL)
//    {

//        QString filteredLines;
//        int notNegCnt = 0, notPosCnt = 0;
//        QVector<int> toAdded(1), toRemoved(1); // lines count from 1

//        // prevLineNum will be set to the number of corresponding
//        // line in full patch. Number is negative just for algorithm
//        // reasons, prevLineNum counts lines from 1
//        if (prevLineNum && prevFilter == VIEW_ALL)
//            *prevLineNum = -(*prevLineNum); // set once


//        FOREACH_SL (it, sl) {
//            RowType rowType = getRowType(it);
//            // do not remove diff header because of centerTarget
//            bool n = rowType == ROW_REMOVED;
//            bool p = rowType == ROW_ADDED;

//            if (!p)
//                notPosCnt++;
//            if (!n)
//                notNegCnt++;

//            toAdded.append(notNegCnt);
//            toRemoved.append(notPosCnt);

//            int curLineNum = toAdded.count() - 1;

//            bool toRemove = (n && curFilter == VIEW_ADDED) || (p && curFilter == VIEW_REMOVED);
//            if (!toRemove)
//                filteredLines.append(*it).append('\n');

//            if (prevLineNum && *prevLineNum == notNegCnt && prevFilter == VIEW_ADDED)
//                *prevLineNum = -curLineNum; // set once

//            if (prevLineNum && *prevLineNum == notPosCnt && prevFilter == VIEW_REMOVED)
//                *prevLineNum = -curLineNum; // set once
//        }
//        if (prevLineNum && *prevLineNum <= 0) {
//            if (curFilter == VIEW_ALL)
//                *prevLineNum = -(*prevLineNum);

//            else if (curFilter == VIEW_ADDED)
//                *prevLineNum = toAdded.at(-(*prevLineNum));

//            else if (curFilter == VIEW_REMOVED)
//                *prevLineNum = toRemoved.at(-(*prevLineNum));

//            if (*prevLineNum < 0)
//                *prevLineNum = 0;
//        }
//        newLines = filteredLines;

//    }




//    if (prevLineNum || document()->isEmpty()) { // use the faster setPlainText()

//        setPlainText(newLines);
//        moveCursor(QTextCursor::Start);
//    } else {
//        int topLine = cursorForPosition(QPoint(1, 1)).blockNumber();
//                appendPlainText(newLines);
//        if (topLine > 0)
//            scrollLineToTop(topLine);
//    }
//    QTextCursor tc(textCursor());
//    tc.movePosition(QTextCursor::Start, QTextCursor::MoveAnchor);
//    do {
//        formatRow(tc);
//        for(QTextBlock::iterator it = tc.block().begin(); (!it.atEnd()); ++it) {

//        }

//    } while(tc.movePosition(QTextCursor::NextBlock, QTextCursor::MoveAnchor));
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
    if (computeMatches()) {
//                diffHighlighter->rehighlight(); // slow on big data
        centerMatch();
    }
}

int PatchContent::doSearch(SCRef txt, int pos) {

    if (isRegExp)
        return txt.indexOf(pickAxeRE, pos);

    return txt.indexOf(pickAxeRE.pattern(), pos, Qt::CaseInsensitive);
}

bool PatchContent::computeMatches() {

    matches.clear();
    if (pickAxeRE.isEmpty())
        return false;

    SCRef txt = toPlainText();
    int pos, lastPos = 0, lastPara = 0;

    // must be at the end to catch patterns across more the one chunk
    while ((pos = doSearch(txt, lastPos)) != -1) {

        matches.append(MatchSelection());
        MatchSelection& s = matches.last();

        s.paraFrom = txt.mid(lastPos, pos - lastPos).count('\n');
        s.paraFrom += lastPara;
        s.indexFrom = pos - txt.lastIndexOf('\n', pos) - 1; // index starts from 0

        lastPos = pos;
        pos += (isRegExp ? pickAxeRE.matchedLength() : pickAxeRE.pattern().length());
        pos--;

        s.paraTo = s.paraFrom + txt.mid(lastPos, pos - lastPos).count('\n');
                s.indexTo = pos - txt.lastIndexOf('\n', pos) - 1;
        s.indexTo++; // in QTextEdit::setSelection() indexTo is not included

        lastPos = pos;
        lastPara = s.paraTo;
    }
    return !matches.isEmpty();
}

bool PatchContent::getMatch(int para, int* indexFrom, int* indexTo) {

    for (int i = 0; i < matches.count(); i++)
        if (matches[i].paraFrom <= para && matches[i].paraTo >= para) {

            *indexFrom = (para == matches[i].paraFrom ? matches[i].indexFrom : 0);
            *indexTo = (para == matches[i].paraTo ? matches[i].indexTo : 0);
            return true;
        }
    return false;
}

void PatchContent::on_highlightPatch(const QString& exp, bool re) {

        pickAxeRE.setPattern(exp);
    isRegExp = re;
    if (diffLoaded)
        procFinished();
}

void PatchContent::update(StateInfo& st) {

    bool combined = (st.isMerge() && !st.allMergeFiles());
//        if (combined) {
//                const Rev* r = git->revLookup(st.sha());
//                if (r)
//                        diffHighlighter->setCombinedLength(r->parentsCount());
//        } else
//                diffHighlighter->setCombinedLength(0);

    clear();
    proc = git->getDiff(st.sha(), this, st.diffToSha(), combined); // non blocking
}

PatchContent::RowType PatchContent::getRowType(QString row) {
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
                return ROW_OTHER;
            }
            break;
        case ' ':
            return ROW_CONTEXT;
            // TODO: restore combined length functionality
//            if (text.left(cl).contains('+'))
//                    charFormat.setForeground(Qt::darkGreen);
//            else if (text.left(cl).contains('-'))
//                    charFormat.setForeground(Qt::red);
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

    int digits = 9;
    int space = 3 + fontMetrics().width(QLatin1Char('9')) * digits + 3;

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

    painter.setPen(backColorLight);
    painter.drawLine(width/2, event->rect().top(), width/2, event->rect().bottom());
    painter.setPen(backColorDark);
    painter.drawLine(width/2+1, event->rect().top(), width/2+1, event->rect().bottom());


    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) {

            PatchTextBlockUserData* ud = static_cast<PatchTextBlockUserData*>(block.userData());
            if (ud && ud->partCount > 0 && ud->rowNumbers) {
                QString number;
                if (ud->rowNumbers[0] >= 0) {
                    number = QString::number(ud->rowNumbers[0]);

                    painter.setPen(QGit::LINE_NUMBERS_FOREGROUND);
                    painter.drawText(0, top, lineNumberArea->width()/2 - 3, fontMetrics().height(),
                                    Qt::AlignRight, number);
                }
                if (ud->rowNumbers[1] >= 0) {
                    number = QString::number(ud->rowNumbers[1]);

                    painter.setPen(QGit::LINE_NUMBERS_FOREGROUND);
                    painter.drawText(0, top, lineNumberArea->width() - 3, fontMetrics().height(),
                                     Qt::AlignRight, number);
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

      this->fitted_height = (document_size.height() + 2) * fontMetrics().lineSpacing() + document()->documentMargin() * 2;
      this->updateGeometry();
}

void PatchContent::onTextChanged()
{
    //fitHeightToDocument();
}

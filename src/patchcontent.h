/*
	Author: Marco Costalba (C) 2005-2007

	Copyright: See COPYING file that comes with this distribution

*/
#ifndef PATCHCONTENT_H
#define PATCHCONTENT_H

#include <QPointer>
#include <QPlainTextEdit>
#include <QSyntaxHighlighter>
#include "common.h"

class Domain;
class Git;
class MyProcess;
class StateInfo;

class DiffHighlighter : public QSyntaxHighlighter
{
public:
    DiffHighlighter(QTextEdit* p) : QSyntaxHighlighter(p), cl(0) {}
    void setCombinedLength(uint c) { cl = c; }
    virtual void highlightBlock(const QString& text);
private:
    uint cl;
};

class BlockData {
public:
    enum RowType {
        ROW_FILE_HEADER,
        ROW_PART_HEADER,
        ROW_ADDED,
        ROW_REMOVED,
        ROW_CONTEXT,
        ROW_OTHER
    };

    RowType type;
};

class PatchContent: public QPlainTextEdit
{
    Q_OBJECT
public:

    PatchContent(QWidget* parent);
    void setup(Domain* parent, Git* git);
    void clear();
    void centerOnFileHeader(StateInfo& st);
    void refresh();
    void update(StateInfo& st);

        void lineNumberAreaPaintEvent(QPaintEvent *event);
        int lineNumberAreaWidth();

    enum PatchFilter {
        VIEW_ALL,
        VIEW_ADDED,
        VIEW_REMOVED
    };
    PatchFilter curFilter, prevFilter;
protected:
        void resizeEvent(QResizeEvent *event);

public slots:
    void on_highlightPatch(const QString&, bool);
    void typeWriterFontChanged();
    void procReadyRead(const QByteArray& data);
    void procFinished();

        // line numbers
        void updateLineNumberAreaWidth(int newBlockCount);
        void highlightCurrentLine();
        void updateLineNumberArea(const QRect &, int);

private:
    friend class DiffHighlighter;

    void scrollCursorToTop();
    void scrollLineToTop(int lineNum);
    int positionToLineNum(int pos);
    int topToLineNum();
    void saveRestoreSizes(bool startup = false);
    int doSearch(const QString& txt, int pos);
    bool computeMatches();
    bool getMatch(int para, int* indexFrom, int* indexTo);
    void centerMatch(int id = 0);
    bool centerTarget(SCRef target);
    void processData(const QByteArray& data, int* prevLineNum = NULL);
        void formatRow(QTextCursor tc);
    Git* git;
    DiffHighlighter* diffHighlighter;
    QPointer<MyProcess> proc;
    bool diffLoaded;
    QByteArray patchRowData;
    QString halfLine;
    bool isRegExp;
    QRegExp pickAxeRE;
    QString target;
    bool seekTarget;
        QWidget *lineNumberArea;

    struct MatchSelection {
        int paraFrom;
        int indexFrom;
        int paraTo;
        int indexTo;
    };
    typedef QVector<MatchSelection> Matches;
    Matches matches;
};

class LineNumberArea : public QWidget
{
public:
    LineNumberArea(PatchContent *editor) : QWidget(editor) {
        codeEditor = editor;
    }

    QSize sizeHint() const {
        return QSize(codeEditor->lineNumberAreaWidth(), 0);
    }

protected:
    void paintEvent(QPaintEvent *event) {
        codeEditor->lineNumberAreaPaintEvent(event);
    }

private:
    PatchContent *codeEditor;
};
#endif

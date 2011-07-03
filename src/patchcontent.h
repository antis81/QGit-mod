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

    PatchFilter filter() { return curFilter; };
    void setFilter(PatchFilter filter);

    QSize sizeHint() const;

    enum RowType {
        ROW_FILE_HEADER,
        ROW_PART_HEADER,
        ROW_FILE_OLD,
        ROW_FILE_NEW,
        ROW_ADDED,
        ROW_REMOVED,
        ROW_CONTEXT,
        ROW_OTHER
    };

protected:
    PatchFilter curFilter, prevFilter;

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
    void formatBlock(QTextCursor& cursor, QTextBlock& block, RowType rowType);
    void formatBlock(QTextCursor& cursor, QTextBlock& block);
    RowType getRowType(QString row);
    Git* git;
    QPointer<MyProcess> proc;
    bool diffLoaded;
    QByteArray patchRowData;
    QString halfLine;
    bool isRegExp;
    QRegExp pickAxeRE;
    QString target;
    bool seekTarget;
    QWidget *lineNumberArea;

private:
    int fitted_height;
    void fitHeightToDocument();

    struct MatchSelection {
        int paraFrom;
        int indexFrom;
        int paraTo;
        int indexTo;
    };
    typedef QVector<MatchSelection> Matches;
    Matches matches;

private slots:
    void onTextChanged();
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

class PatchTextBlockUserData : public QTextBlockUserData
{
public:
    PatchContent::RowType rowType;

    long* rowNumbers;
    int partCount;

    PatchTextBlockUserData() : rowNumbers(NULL), partCount(0), rowType(PatchContent::ROW_OTHER) {}
    ~PatchTextBlockUserData() {
        if (rowNumbers) {
            delete[] rowNumbers;
            rowNumbers = NULL;
        }
    }
};

#endif

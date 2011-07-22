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
#include "findsupport.h"

class Domain;
class Git;
class MyProcess;
class StateInfo;
class PatchContentFindSupport;

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

    PatchContentFindSupport* m_findSupport;

    ~PatchContent();

protected:

    void resizeEvent(QResizeEvent *event);

public slots:
    void on_highlightPatch(const QString&, bool);
    void typeWriterFontChanged();
    void procReadyRead(const QByteArray& data);
    void procFinished();


private:
    friend class DiffHighlighter;

    void scrollCursorToTop();
    void scrollLineToTop(int lineNum);
    int positionToLineNum(int pos);
    int topToLineNum();
    void saveRestoreSizes(bool startup = false);
    void centerMatch(int id = 0);
    bool centerTarget(SCRef target);
    void processData(const QByteArray& data, int* prevLineNum = NULL);
    void formatRow(QTextCursor tc);
    void formatBlock(QTextCursor& cursor, QTextBlock& block, RowType rowType);
    void formatBlock(QTextCursor& cursor, QTextBlock& block);
    RowType getRowType(const QString& row);
    Git* git;
    QPointer<MyProcess> proc;
    bool diffLoaded;
    QByteArray patchRowData;
    QString halfLine;
    QString target;
    bool seekTarget;


private slots:
    void onTextChanged();

// Filter
public:
    enum PatchFilter {
        VIEW_ALL,
        VIEW_ADDED,
        VIEW_REMOVED
    };

    PatchFilter filter() { return curFilter; };
    void setFilter(PatchFilter filter);

protected:
    PatchFilter curFilter;
    PatchFilter prevFilter;


// Line numbers
public:
    void lineNumberAreaPaintEvent(QPaintEvent *event);
    int lineNumberAreaWidth();

private:
    QWidget *lineNumberArea;

private slots:
    void updateLineNumberAreaWidth(int newBlockCount);
    void highlightCurrentLine();
    void updateLineNumberArea(const QRect &, int);


// Auto size
private:
    int fitted_height;
    void fitHeightToDocument();


// Search
public:
    FindSupport* findSupport();

private:
    friend class PatchContentFindSupport;
    void updateMatchesHighlight();
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

class PatchContentFindSupport : public FindSupport
{
public:
    PatchContentFindSupport(PatchContent* patchContent);

    bool find();
    bool findNext();

    void setText(QString text, bool re);
    void setText(QString text);
private:
    friend class PatchContent;

    PatchContent* m_patchContent;
    bool isRegExp;
    QRegExp pickAxeRE;

    struct MatchSelection {
        int from;
        int to;
    };
    typedef QVector<MatchSelection> Matches;

    Matches matches;

    QTextCursor findNextMatch(const QTextDocument* document, QTextCursor& cursor);
    bool computeMatches();
};

#endif

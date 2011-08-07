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
//#include "patchcontentfindsupport.h"

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

    enum RowType
    {
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

    // Search
    FindSupport* findSupport();

    // Line numbers
    void lineNumberAreaPaintEvent(QPaintEvent *event);
    int lineNumberAreaWidth();

    // Filter
    enum PatchFilter
    {
        VIEW_ALL,
        VIEW_ADDED,
        VIEW_REMOVED
    };

    PatchFilter filter() { return curFilter; };
    void setFilter(PatchFilter filter);

public slots:
    void on_highlightPatch(const QString&, bool);
    void typeWriterFontChanged();
    void procReadyRead(const QByteArray& data);
    void procFinished();

protected:
    void resizeEvent(QResizeEvent *event);

    PatchFilter curFilter;
    PatchFilter prevFilter;

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

    // Auto size
    int fitted_height;
    void fitHeightToDocument();

    friend class PatchContentFindSupport;
    void updateMatchesHighlight();

    QWidget *lineNumberArea;

private slots:
    void onTextChanged();

    void updateLineNumberAreaWidth(int newBlockCount);
    void highlightCurrentLine();
    void updateLineNumberArea(const QRect &, int);
};

#endif

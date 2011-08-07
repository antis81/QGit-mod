/*
    Author: Marco Costalba (C) 2005-2007

    Copyright: See COPYING file that comes with this distribution
*/
#ifndef LISTVIEW_H
#define LISTVIEW_H

#include <QTreeView>
#include <QItemDelegate>
#include <QSortFilterProxyModel>
#include <QRegExp>
#include "common.h"
#include "listviewproxy.h"

class Git;
class StateInfo;
class Domain;
class FileHistory;
class ListViewProxy;

class ListView: public QTreeView
{
    Q_OBJECT
public:
    ListView(QWidget* parent);
    ~ListView();
    void setup(Domain* d, Git* g);
    const QString shaFromAnnId(uint id);
    void showIdValues();
    void scrollToCurrent(ScrollHint hint = EnsureVisible);
    void scrollToNextHighlighted(int direction);
    void getSelectedItems(QStringList& selectedItems);
    bool update();
    void addNewRevs(const QVector<QString>& shaVec);
    const QString currentText(int col);
    int filterRows(bool, bool, SCRef = QString(), int = -1, ShaSet* = NULL);
    const QString sha(int row) const;
    int row(SCRef sha) const;

signals:
    void lanesContextMenuRequested(const QStringList&, const QStringList&);
    void revisionsDragged(const QStringList&);
    void revisionsDropped(const QStringList&);
    void contextMenu(const QString&, int);
    void diffTargetChanged(int); // used by new model_view integration

public slots:
    void on_changeFont(const QFont& f);
    void on_keyUp();
    void on_keyDown();

protected:
    virtual void mousePressEvent(QMouseEvent* e);
    virtual void mouseMoveEvent(QMouseEvent* e);
    virtual void mouseReleaseEvent(QMouseEvent* e);
    virtual void dragEnterEvent(QDragEnterEvent* e);
    virtual void dragMoveEvent(QDragMoveEvent* e);
    virtual void dropEvent(QDropEvent* e);

private slots:
    void on_customContextMenuRequested(const QPoint&);
    virtual void currentChanged(const QModelIndex&, const QModelIndex&);

private:
    void setupGeometry();
    bool filterRightButtonPressed(QMouseEvent* e);
    bool getLaneParentsChilds(SCRef sha, int x, SList p, SList c);
    LaneType getLaneType(SCRef sha, int pos) const;

    Domain* d;
    Git* git;
    StateInfo* st;
    FileHistory* fh;
    ListViewProxy* lp;
    unsigned long secs;
    bool filterNextContextMenuRequest;
};

#endif

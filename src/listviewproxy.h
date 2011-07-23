#ifndef LISTVIEWPROXY_H
#define LISTVIEWPROXY_H

#include <QSortFilterProxyModel>
#include "common.h"
#include "filehistory.h"
#include "listview.h"

class Git;
class StateInfo;
class Domain;

class ListViewProxy : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    ListViewProxy(QObject* parent, Domain* d, Git* g);
    int setFilter(bool isOn, bool highlight, SCRef filter, int colNum, ShaSet* s);
    bool isHighlighted(int row) const;

protected:
    virtual bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const;

private:
    bool isMatch(int row) const;
    bool isMatch(SCRef sha) const;

    Domain* d;
    Git* git;
    bool isHighLight;
    QRegExp filter;
    int colNum;
    ShaSet shaSet;
};

#endif // LISTVIEWPROXY_H

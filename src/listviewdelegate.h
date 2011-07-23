#ifndef LISTVIEWDELEGATE_H
#define LISTVIEWDELEGATE_H

#include <QItemDelegate>
#include "git.h"
#include "listviewproxy.h"
#include <QPainter>

using namespace QGit;

class ListViewDelegate : public QItemDelegate
{
    Q_OBJECT
public:
    ListViewDelegate(Git* git, ListViewProxy* lp, QObject* parent);

    virtual void paint(QPainter* p, const QStyleOptionViewItem& o, const QModelIndex &i) const;
    virtual QSize sizeHint(const QStyleOptionViewItem& o, const QModelIndex &i) const;
    int laneWidth() const { return 3 * laneHeight / 4; }
    void setLaneHeight(int h) { laneHeight = h; }

signals:
    void updateView();

public slots:
    void diffTargetChanged(int);

private:
    const Rev* revLookup(int row, FileHistory** fhPtr = NULL) const;
    void paintLog(QPainter* p, const QStyleOptionViewItem& o, const QModelIndex &i) const;
    void paintGraph(QPainter* p, const QStyleOptionViewItem& o, const QModelIndex &i) const;
    void paintGraphLane(QPainter* p, int type, int x1, int x2, const QColor& col, const QColor& activeCol, const QBrush& back) const;
    QPixmap* getTagMarks(SCRef sha, const QStyleOptionViewItem& opt) const;
    void addRefPixmap(QPixmap** pp, SCRef sha, int type, QStyleOptionViewItem opt) const;
    void addTextPixmap(QPixmap** pp, SCRef txt, const QStyleOptionViewItem& opt) const;
    bool changedFiles(SCRef sha) const;

    Git* git;
    ListViewProxy* lp;
    int laneHeight;
    int diffTargetRow;
};

#endif // LISTVIEWDELEGATE_H

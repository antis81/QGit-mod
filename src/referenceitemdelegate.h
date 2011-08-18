/*
Author: Nils Fenner (c) 2011

Copyright: See COPYING file that comes with this distribution
*/

#ifndef REFERENCEITEMDELEGATE_H
#define REFERENCEITEMDELEGATE_H

#include <QItemDelegate>

class ReferenceTreeItem;


class ReferenceItemDelegate : public QItemDelegate
{
    Q_OBJECT
public:
    explicit ReferenceItemDelegate(QObject *parent = 0);

    void paint ( QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index ) const;

    bool editorEvent (
            QEvent * event, QAbstractItemModel * model, const QStyleOptionViewItem & option, const QModelIndex & index );

private:
    void menuForReferenceItem(ReferenceTreeItem *item, const QPoint &pos);
};

#endif // REFERENCEITEMDELEGATE_H

/*
Author: Nils Fenner (c) 2011

Copyright: See COPYING file that comes with this distribution
*/

#ifndef REFERENCETREEVIEWITEMDELEGATE_H
#define REFERENCETREEVIEWITEMDELEGATE_H

#include <QItemDelegate>

class ReferenceTreeViewItem;

class ReferenceTreeViewItemDelegate : public QItemDelegate
{
    Q_OBJECT

public:
    explicit ReferenceTreeViewItemDelegate(QObject* parent = 0);

    void paint(QPainter* painter, const QStyleOptionViewItem& option,
               const QModelIndex& index) const;

    bool editorEvent(QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option,
                     const QModelIndex& index);

private:
    void menuForReferenceItem(ReferenceTreeViewItem* item, const QPoint& pos);
};

#endif // REFERENCETREEVIEWITEMDELEGATE_H

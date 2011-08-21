/*
Author: Nils Fenner (c) 2011

Copyright: See COPYING file that comes with this distribution
*/

#include "referenceitemdelegate.h"

#include <QEvent>
#include <QMouseEvent>
#include <QMenu>
#include <QTreeView>
#include <QPaintEngine>

#include "referencetreeitem.h"


ReferenceItemDelegate::ReferenceItemDelegate(QObject*parent)
    : QItemDelegate(parent)
{
}

bool ReferenceItemDelegate::editorEvent(QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option, const QModelIndex& index)
{
    bool result = false;
    ReferenceTreeItem* item = static_cast<ReferenceTreeItem*>(index.internalPointer());

    QMouseEvent* me = static_cast<QMouseEvent*>(event);

    switch (event->type()) {
    case QEvent::MouseButtonDblClick:
            // show the branch in the revisions view
            item->showRevision();
            result = true;
        break;

    case QEvent::MouseButtonPress:
        if (me && (me->button() == Qt::RightButton)) {
            menuForReferenceItem(item, me->pos());
            result = true;
        }
        break;

    default:
        break;
    }

    return result;
}

/**
A reference item has requested a context menu.
*/
void ReferenceItemDelegate::menuForReferenceItem(ReferenceTreeItem* item, const QPoint& pos)
{
    if (item->type() != ReferenceTreeItem::LeafBranch && item->type() != ReferenceTreeItem::LeafTag) {
        return;
    }

    QMenu*  m = new QMenu();

    m->addAction("Checkout", item, SLOT(checkout()));
    m->addAction(tr("Delete"), item, SLOT(removeReference()));

    m->popup(QCursor::pos());
}

/**
Do custom painting here.
*/
void ReferenceItemDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    QItemDelegate::paint(painter, option, index);
}


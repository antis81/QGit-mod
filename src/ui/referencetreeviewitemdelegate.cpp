/*
Author: Nils Fenner (c) 2011

Copyright: See COPYING file that comes with this distribution
*/

#include "referencetreeviewitemdelegate.h"

#include <QEvent>
#include <QMouseEvent>
#include <QMenu>
#include <QTreeView>
#include <QPaintEngine>

#include "referencetreeviewitem.h"


ReferenceTreeViewItemDelegate::ReferenceTreeViewItemDelegate(QObject*parent)
    : QItemDelegate(parent)
{
}

bool ReferenceTreeViewItemDelegate::editorEvent(QEvent* event, QAbstractItemModel* model,
                                                const QStyleOptionViewItem& option,
                                                const QModelIndex& index)
{
    Q_UNUSED(model);
    Q_UNUSED(option);

    bool result = false;
    ReferenceTreeViewItem* item = static_cast<ReferenceTreeViewItem*>(index.internalPointer());

    QMouseEvent* me = static_cast<QMouseEvent*>(event);

    switch (event->type()) {
    case QEvent::MouseButtonDblClick:
            // show the branch in the revisions view
            // item->showRevision();
        //! \todo off because this slot is disabled
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
void ReferenceTreeViewItemDelegate::menuForReferenceItem(ReferenceTreeViewItem* item,
                                                         const QPoint& pos)
{
    if (item->type() != ReferenceTreeViewItem::LeafBranch && item->type()
            != ReferenceTreeViewItem::LeafTag) {
        return;
    }

    QMenu*  m = new QMenu();

//    m->addAction("Checkout", item, SLOT(checkout()));
//    m->addAction(tr("Delete"), item, SLOT(removeReference()));

//    m->popup(QCursor::pos());
}

/**
Do custom painting here.
*/
void ReferenceTreeViewItemDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option,
                                          const QModelIndex& index) const
{
    QItemDelegate::paint(painter, option, index);
}


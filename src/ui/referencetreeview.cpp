#include "referencetreeview.h"
#include "referencetreeviewitemdelegate.h"

#include <qdebug.h>

ReferenceTreeView::ReferenceTreeView(QWidget* parent) : QTreeView(parent)
{
    setItemDelegate(new ReferenceTreeViewItemDelegate());
    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, SIGNAL(customContextMenuRequested(const QPoint&)), this,
            SLOT(contextMenu(const QPoint&)));
}

ReferenceTreeView::~ReferenceTreeView()
{
    delete m_delegate;
    m_delegate = NULL;
}

void ReferenceTreeView::setDelegate(ReferenceTreeViewDelegate* delegate)
{
    m_delegate = delegate;
}

ReferenceTreeViewDelegate* ReferenceTreeView::delegate() const
{
    return m_delegate;
}

void ReferenceTreeView::showAllItems()
{
    for (int i = 0; i < model()->rowCount(QModelIndex()); i++) {
        setRowHidden(i, QModelIndex() , false);
    }
}

void ReferenceTreeView::showSearchedItems(QString inputText)
{
    expandAll();
    showAllItems();

    if (!(inputText.simplified().isEmpty())) {
        for (int i = 0; i < model()->rowCount(QModelIndex()); i++) {
            if (isRegExpConformed(inputText, model()->index(i, 0, QModelIndex()).data().toString())) {
                setRowHidden(i, QModelIndex(), false);
            } else {
                setRowHidden(i, QModelIndex(), true);
            }
        }
    }
}

void ReferenceTreeView::mouseDoubleClickEvent(QMouseEvent* event)
{
    const QModelIndex& modelIndex = indexAt(event->pos());
    if (!modelIndex.isValid()) {
        return;
    }

    const QVariant& referenceName = modelIndex.data(Qt::UserRole);
    if (referenceName.isNull()) {
        return;
    }

    m_delegate->processDblClick(referenceName.toString());
}

void ReferenceTreeView::contextMenu(const QPoint& pos)
{
    QPoint globalPos = viewport()->mapToGlobal(pos);
    globalPos += QPoint(10, 10);

    const QModelIndex& modelIndex = indexAt(pos);
    if (!modelIndex.isValid()) {
        return;
    }

    this->selectionModel()->select(modelIndex, QItemSelectionModel::ClearAndSelect);
    const QVariant& referenceName = modelIndex.data(Qt::UserRole);
    if (referenceName.isNull()) {
        return;
    }

    m_delegate->processContextMenu(globalPos, referenceName.toString());
}

//void ReferenceTreeView::showSearchedItems(QString inputText)
//{
///*
//    bool f = false;
//    showAllItems();
//    if (inputText.contains("/")) {
//        QString firstPart = inputText.left(inputText.indexOf("/"));
//        QString lastPart = inputText.mid(inputText.indexOf("/") + 1);
//        // top level item named Remotes for tree is 1 index

//        // hide all, exept Remotes
//        for (int i = 0; i < model()->rowCount(QModelIndex()); i++) {
//            if (i != 1)
//                f = isBranchesTreeItemShown(topLevelItem(i), inputText);
//        }
//        // hide header in "header/lower"
//        f = isBranchesTreeItemShown(topLevelItem(1), firstPart);
//        // find "lower"
//        for (int i = 0; i < topLevelItem(1)->childCount(); i++) {
//            if (topLevelItem(1)->child(i)->text(0) == firstPart) {
//                f = isBranchesTreeItemShown(topLevelItem(1)->child(i), lastPart);
//            }
//        }
//    } else {

//*/
//    // THINKME: Change to load/save statement of tree
//        } // else load condition
////->    }

//}

//bool ReferenceTreeView::isItemShown(QModelIndex* item, QString currentString)
//{
//    if (isRegExpConformed(currentString, model()->index())) {
//        item->setHidden(false);
//        return true;
//    } else {
//        if (item->childCount() > 0) {
//            bool flag = false; // FIXME: bad name
//            for (int i = 0; i < item->childCount(); i++) {
//                if (isBranchesTreeItemShown(item->child(i), currentString)) {
//                    if (flag == false) {
//                        flag = true;
//                    }
//                }
//            }
//            item->setHidden(!flag);
//            return flag;
//        } else {
//            item->setHidden(true);
//            return false;
//        }
//    }
//}

bool ReferenceTreeView::isRegExpConformed(QString currentString, QString originalString)
{
    if (originalString.indexOf(currentString, 0, Qt::CaseInsensitive) == 0)
        return true;
    else
        return false;
}

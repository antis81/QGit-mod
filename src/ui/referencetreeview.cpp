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

void ReferenceTreeView::showAllItems(QModelIndex modelIndex)
{
    int currentRowCount = model()->rowCount(modelIndex);
    for (int i = 0; i < currentRowCount; i++) {
        setRowHidden(i, modelIndex, false);
        showAllItems(model()->index(i, 0, modelIndex));
    }
}

void ReferenceTreeView::showSearchedItems(QString inputText)
{
    showAllItems();
    expandAll();

    int rowCount = model()->rowCount(QModelIndex());

    for (int i = 0; i < rowCount; i++) {
        showNode(QModelIndex(), i, inputText);
    }
}

bool ReferenceTreeView::showNode(QModelIndex modelIndex, int iPos, QString inputText)
{
    QModelIndex subIndex = model()->index(iPos, 0, modelIndex);
    bool isMustBeShown = isRegExpConformed(inputText, subIndex.data(Qt::UserRole).toString());

    if (!isMustBeShown) {
        int rowCount = model()->rowCount(subIndex);
        for (int i = 0; i < rowCount; i++) {
            isMustBeShown |= showNode(subIndex, i, inputText);
        }
    }
    setRowHidden(iPos, modelIndex, !isMustBeShown);
    return isMustBeShown;
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

bool ReferenceTreeView::isRegExpConformed(QString currentString, QString originalString)
{
    if (originalString.indexOf(currentString, 0, Qt::CaseInsensitive) == -1)
        return false;
    else
        return true;
}

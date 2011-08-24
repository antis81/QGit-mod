#include "referencetreeview.h"
#include "referencetreeviewitemdelegate.h"

ReferenceTreeView::ReferenceTreeView(QWidget* parent) : QTreeView(parent)
{
    setItemDelegate(new ReferenceTreeViewItemDelegate());
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

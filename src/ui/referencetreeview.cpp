#include "referencetreeview.h"
#include "referencetreeviewdelegate.h"

#include <QMouseEvent>

ReferenceTreeView::ReferenceTreeView(QWidget* parent) : QTreeView(parent)
{
}

void ReferenceTreeView::setDelegate(ReferenceTreeViewDelegate* delegate)
{
    m_delegate = delegate;
}

void ReferenceTreeView::mouseDoubleClickEvent(QMouseEvent* event)
{
    QString referenceName = indexAt(event->pos()).data(Qt::UserRole).toString();
    m_delegate->changeReference(referenceName);
}

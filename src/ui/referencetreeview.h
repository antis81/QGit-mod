#ifndef REFERENCETREEVIEW_H
#define REFERENCETREEVIEW_H

#include <QTreeWidget>

class ReferenceTreeViewDelegate;

class ReferenceTreeView : public QTreeView
{
public:
    explicit ReferenceTreeView(QWidget* parent = 0);
    void setDelegate(ReferenceTreeViewDelegate* delegate);
protected:
    void mouseDoubleClickEvent(QMouseEvent* event);
private:
    ReferenceTreeViewDelegate* m_delegate;
};

#endif // REFERENCETREEVIEW_H

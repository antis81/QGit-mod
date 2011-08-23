#ifndef REFERENCETREEVIEW_H
#define REFERENCETREEVIEW_H

#include <QTreeWidget>

#include <qdebug.h>

class ReferenceTreeViewDelegate;

class ReferenceTreeView : public QTreeView
{
    Q_OBJECT
public:
    explicit ReferenceTreeView(QWidget* parent = 0);
    void setDelegate(ReferenceTreeViewDelegate* delegate);
protected:
    void mouseDoubleClickEvent(QMouseEvent *event);
private:
    ReferenceTreeViewDelegate* m_delegate;
};

#endif // REFERENCETREEVIEW_H

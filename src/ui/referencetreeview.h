#ifndef REFERENCETREEVIEW_H
#define REFERENCETREEVIEW_H

#include <QWidget>
#include <QTreeView>
#include <QMouseEvent>
#include "referencetreeviewdelegate.h"

class ReferenceTreeViewDelegate;

class ReferenceTreeView : public QTreeView
{
    Q_OBJECT
public:
    explicit ReferenceTreeView(QWidget* parent = 0);
    virtual ~ReferenceTreeView();
    void setDelegate(ReferenceTreeViewDelegate* delegate);
    ReferenceTreeViewDelegate* delegate() const;
protected:
    void mouseDoubleClickEvent(QMouseEvent* event);
private:
    ReferenceTreeViewDelegate* m_delegate;
};

#endif // REFERENCETREEVIEW_H

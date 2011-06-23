#ifndef BRANCHTREEITEM_H
#define BRANCHTREEITEM_H

#include <QTreeWidgetItem>

class BranchTreeItem : public QTreeWidgetItem
{
public:
    BranchTreeItem(QString branchNameValue, int type);
    QString branchName();
private:;
    QString myBranchName;
};

#endif // BRANCHTREEITEM_H

#include "branchtreeitem.h"

BranchTreeItem::BranchTreeItem(QString branchNameValue, int type)
    : QTreeWidgetItem(type)
{
    myBranchName = branchNameValue;
    setData(0, Qt::UserRole, branchNameValue);
}

QString BranchTreeItem::branchName()
{
    return myBranchName;
}

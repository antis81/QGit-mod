#include "branchestreeitem.h"

BranchesTreeItem::BranchesTreeItem(QTreeWidgetItem *parent, const QStringList &strings, int type)
    : QTreeWidgetItem(parent, strings, type)
{
    setupStyle();
}

BranchesTreeItem::BranchesTreeItem(QTreeWidget *view, const QStringList &strings, int type)
    : QTreeWidgetItem(view, strings, type)
{
    setupStyle();
}

void BranchesTreeItem::setupStyle()
{
    if (treeWidget()) {
        QSize size = sizeHint(0);
        size.setHeight(treeWidget()->fontMetrics().lineSpacing() + 7);
        setSizeHint(0, size);
    }
}

QString BranchesTreeItem::branch()
{
    return m_branch;
}

void BranchesTreeItem::setBranch(QString branch)
{
    m_branch = branch;
}

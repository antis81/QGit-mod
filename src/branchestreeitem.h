#ifndef BRANCHESTREEITEM_H
#define BRANCHESTREEITEM_H

#include <QTreeWidgetItem>

class BranchesTreeItem : public QTreeWidgetItem
{
public:
    BranchesTreeItem(QTreeWidgetItem *parent, const QStringList &strings, int type = Type);
    BranchesTreeItem(QTreeWidget *view, const QStringList &strings, int type = Type);
    void setupStyle();
    QString branch();
    void setBranch(QString branch);
private:
    QString m_branch;
};

#endif // BRANCHESTREEITEM_H

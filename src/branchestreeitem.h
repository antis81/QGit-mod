#ifndef BRANCHESTREEITEM_H
#define BRANCHESTREEITEM_H

#include <QTreeWidgetItem>

// Скорее всего этот класс не нужен, объяснение смотри в branchestree.h

// THINKME: Кстати, не переименовать ли всё это дело в BranchTree. так разумнее

class BranchesTreeItem : public QTreeWidgetItem
{
public:
    // TODO: Why class have two constructors? Must be explained
    BranchesTreeItem(QTreeWidgetItem *parent, const QStringList &strings, int type = Type);
    BranchesTreeItem(QTreeWidget *view, const QStringList &strings, int type = Type);
    void setupStyle();
    QString branch(); // FIXME: May be use postfix "const"
    void setBranch(QString branch); // FIXME: Nonclear name
private:
    QString m_branch;
};

#endif // BRANCHESTREEITEM_H

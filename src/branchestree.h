#ifndef BRANCHESTREE_H
#define BRANCHESTREE_H

#include <QTreeWidget>
#include "git.h"
#include "domain.h"

class BranchesTree : public QTreeWidget
{
    Q_OBJECT
public:
    enum BranchTreeItemType
    {
        BranchTreeLeaf = 257,
        BranchTreeNode = 258
    };
    BranchesTree(QWidget *parent = 0);
    void setup(Domain *domain, Git *git);
    void update();

private:
    Git *g;
    Domain *d;
    void addNode(QString header, Git::RefType type);

public slots:
    void changeBranch(QTreeWidgetItem *item, int column);
};

#endif // BRANCHESTREE_H

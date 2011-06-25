#ifndef BRANCHESTREE_H
#define BRANCHESTREE_H

#include <QTreeWidget>
#include "git.h"
#include "domain.h"

class BranchesTree : public QTreeWidget
{
    Q_OBJECT
public:
    enum BranchTreeItemTypes
    {
        HeaderBranch = 257,
        HeaderRemote = 258,
        HeaderTag = 259,
        LeafBranch = 260,
        LeafRemote = 261,
        LeafTag = 262
    };
    BranchesTree(QWidget *parent = 0);
    void setup(Domain *domain, Git *git);
    void update();

public slots:
    void changeBranch(QTreeWidgetItem *item, int column);
    void contextMenu(const QPoint & pos);
    void checkout();
    void removeTag();

private:
    Git *g;
    Domain *d;
    QAction *collapseAllAction;
    QAction *expandAllAction;
    QAction *checkoutAction;
    QAction *removeTagAction;
    void addNode(BranchTreeItemTypes headerType, Git::RefType type);

};

#endif // BRANCHESTREE_H

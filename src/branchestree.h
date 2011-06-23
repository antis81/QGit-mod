#ifndef BRANCHESTREE_H
#define BRANCHESTREE_H

#include <QTreeWidget>
#include "branchtreeitem.h"
#include "git.h"
#include "domain.h"

class BranchesTree : public QTreeWidget
{
    Q_OBJECT
public:
    explicit BranchesTree(QWidget *parent = 0);
    void setup(Domain *domain, Git *git);
    void update();

private:
    Git *g;
    Domain *d;
    void addNode(QString header, Git::RefType type);

public slots:
    void itemDoubleClicked(QTreeWidgetItem *item, int column = 0);
};

#endif // BRANCHESTREE_H

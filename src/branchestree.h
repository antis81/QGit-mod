#ifndef BRANCHESTREE_H
#define BRANCHESTREE_H

#include <QTreeWidget>
#include "git.h"
#include "domain.h"

class BranchesTree : public QTreeWidget
{
    Q_OBJECT
public:
    enum ItemType
    {
        HeaderBranches = 257,
        HeaderRemotes = 258,
        HeaderTags = 259,
        LeafBranch = 260,
        LeafRemote = 261,
        LeafTag = 262,
        HeaderRemote = 263
    };
    BranchesTree(QWidget *parent = 0);
    void setup(Domain *domain, Git *git);
    void update();

public slots:
    void changeBranch(QTreeWidgetItem *item, int column);
    void contextMenu(const QPoint & pos);
    void checkout();
    void removeTag();

public:
    void selectBranch(const QString& branch);
private:
    Git *g;
    Domain *d;
    QAction *collapseAllAction;
    QAction *expandAllAction;
    QAction *checkoutAction;
    QAction *removeTagAction;
    void addNode(ItemType headerType, Git::RefType type);
    void addRemotesNodes();

    QTreeWidgetItem* recursiveFindBranch(const QString& branch);
    QTreeWidgetItem* recursiveFindBranch(QTreeWidgetItem* parent, const QString& branch);

};

class BranchesTreeItem : public QTreeWidgetItem
{
private:
    QString m_branch;

public:
    BranchesTreeItem(QTreeWidgetItem *parent, const QStringList &strings, int type = Type)
        : QTreeWidgetItem(parent, strings, type) {};

    QString branch() { return m_branch; };

    void setBranch(QString branch) {
        m_branch = branch;
    }
};

#endif // BRANCHESTREE_H

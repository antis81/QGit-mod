#include "branchestree.h"
#include "mainimpl.h"

BranchesTree::BranchesTree(QWidget *parent) : QTreeWidget(parent)
{
    QObject::connect(this,
                     SIGNAL(itemDoubleClicked(QTreeWidgetItem*,int)),
                     this,
                     SLOT(itemDoubleClicked(QTreeWidgetItem*,int)));
}

void BranchesTree::setup(Domain *domain, Git *git)
{
    d = domain;
    g = git;
}

void BranchesTree::update()
{
    clear();
    setColumnCount(1);
    setHeaderLabel("Branches Tree");
    addNode("Branches", Git::BRANCH);
    addNode("Remotes", Git::RMT_BRANCH);
    addNode("Tags", Git::TAG);
}

void BranchesTree::addNode(QString header, Git::RefType type)
{
    QStringList list = g->getAllRefNames(type, !Git::optOnlyLoaded);

    QTreeWidgetItem *node = new QTreeWidgetItem(this, QStringList(header));
    addTopLevelItem(node);

    QTreeWidgetItem *li;

    FOREACH_SL (it, list) {
        li = new QTreeWidgetItem(QStringList(QString(*it)));
        node->addChild(li);
    }
}

void BranchesTree::itemDoubleClicked(QTreeWidgetItem *item, int column)
{
    if (!item->data(column, Qt::UserRole).isNull()) {
        //QTreeWidgetItem* branchLI = (QTreeWidgetItem*)item;
        BranchTreeItem* branchLI = (BranchTreeItem*)item;
        d->m()->changeBranch(branchLI->branchName());
    }
}

//void BranchesList::addList(QString header, Git::RefType type)
//{
//    QStringList list = git->getAllRefNames(type, !Git::optOnlyLoaded);
//    QListWidgetItem* li;
//    if (list.isEmpty()) {
//        return;
//    }

//    if (this->count() > 0) {
//        li = new QListWidgetItem();
//        this->addItem(li);
//    }

//    li = new QListWidgetItem();
//    li->setText(header);
//    QFont font = li->font();
//    font.setBold(true);
//    li->setFont(font);

//    QColor white(255, 255, 255);
//    QColor black(128, 128, 128);
//    QLinearGradient g(0, 0, 0, 20);
//    g.setColorAt(0, white);
//    g.setColorAt(1, black);
//    li->setBackground(g);
//    this->addItem(li);

//    list.sort();
//    FOREACH_SL (it, list) {
//        li = new BranchListItem(QString(*it));
//        this->addItem(li);
//    }
//}

//void BranchesList::itemDoubleClicked(QListWidgetItem *item)
//{
//    if (!item->data(Qt::UserRole).isNull()) {
//        BranchListItem* branchLI = (BranchListItem*)item;
//        d->m()->changeBranch(branchLI->getBranchName());
//    }
//}

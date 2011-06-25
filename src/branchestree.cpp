#include "branchestree.h"
#include "mainimpl.h"
#include <QDebug>

BranchesTree::BranchesTree(QWidget *parent) : QTreeWidget(parent)
{
    setContextMenuPolicy(Qt::CustomContextMenu);

    QObject::connect(this, SIGNAL(itemClicked(QTreeWidgetItem*, int)),
                     this, SLOT(changeBranch(QTreeWidgetItem*, int)));

    QObject::connect(this, SIGNAL(customContextMenuRequested(QPoint)),
                     this, SLOT(contextMenu(QPoint)));

    collapseAllAction = new QAction(tr("Collapse all"), this);
    QObject::connect(collapseAllAction, SIGNAL(triggered()),
                     this, SLOT(collapseAll()));

    expandAllAction = new QAction(tr("Expand all"), this);
    QObject::connect(expandAllAction, SIGNAL(triggered()),
                     this, SLOT(expandAll()));

    checkoutAction = new QAction(tr("Checkout"), this);
    QObject::connect(checkoutAction, SIGNAL(triggered()),
                     this, SLOT(checkout()));

    removeTagAction = new QAction(tr("Remove"), this);
    QObject::connect(removeTagAction, SIGNAL(triggered()),
                     this, SLOT(removeTag()));
}

void BranchesTree::setup(Domain *domain, Git *git)
{
    d = domain;
    g = git;
}

void BranchesTree::update()
{
    clear();
    addNode(BranchesTree::HeaderBranch, Git::BRANCH);
    addNode(BranchesTree::HeaderRemote, Git::RMT_BRANCH);
    addNode(BranchesTree::HeaderTag, Git::TAG);
    expandAll();
}

void BranchesTree::addNode(BranchTreeItemTypes headerType, Git::RefType type)
{
    // получаем нужную инфу по типу
    QStringList tempList = g->getAllRefNames(type, !Git::optOnlyLoaded);

    // делаем хедер и добавляем на верхний уровень
    QTreeWidgetItem *node;
    switch (headerType) {
    case (BranchesTree::HeaderBranch):
        node = new QTreeWidgetItem(this, QStringList("Branches"), headerType);
        break;
    case (BranchesTree::HeaderRemote):
        node = new QTreeWidgetItem(this, QStringList("Remotes"), headerType);
        node->setIcon(0, QIcon(QString::fromUtf8(":/icons/resources/branch_master.png")));
        break;
    case (BranchesTree::HeaderTag):
        node = new QTreeWidgetItem(this, QStringList("Tags"), headerType);
        node->setIcon(0, QIcon(QString::fromUtf8(":/icons/resources/tag.png")));
        break;
    }

    tempList.sort();

    QFont font = node->font(0);
    font.setBold(true);
    node->setFont(0, font);

//    QColor white(255, 255, 255);
//    QColor black(128, 128, 128);
//    QLinearGradient localGradient(0, 0, 0, 20);
//    localGradient.setColorAt(0, white);
//    localGradient.setColorAt(1, black);
//    node->setBackground(0, localGradient);

    addTopLevelItem(node);

    BranchesTreeItem *tempItemList;

    // заполняем дерево потомками
    FOREACH_SL (it, tempList) {
        bool isCurrent = (g->currentBranch().compare(*it) == 0);
        switch (headerType) {
        case (HeaderBranch):
            // имеет значение, что node, а не this. это важно!
            tempItemList = new BranchesTreeItem(node, QStringList(QString(*it)),
                                               LeafBranch);
            if (isCurrent) {
                tempItemList->setText(0, tempItemList->text(0) + " *");
            }
            tempItemList->setBranch(QString(*it));
            break;
        case (HeaderRemote):
            tempItemList = new BranchesTreeItem(node, QStringList(QString(*it)),
                                               LeafRemote);
            tempItemList->setBranch(QString(*it));
            break;
        case (HeaderTag):
            tempItemList = new BranchesTreeItem(node, QStringList(QString(*it)),
                                               LeafTag);
            if (isCurrent) {
                tempItemList->setText(0, tempItemList->text(0) + " *");
            }
            tempItemList->setBranch(QString(*it));
            break;
        }
        tempItemList->setIcon(0, QIcon(QString::fromUtf8(":/icons/resources/branch.png")));
        node->addChild(tempItemList);
    }
}

void BranchesTree::changeBranch(QTreeWidgetItem *item, int column)
{
    if ((item->type() != BranchesTree::HeaderBranch)
            && (item->type() != BranchesTree::HeaderRemote)
            && (item->type() != BranchesTree::HeaderTag)) {
        // запоминаем состояние закрытости/открытости хедеров
        // и текст выделенного узла
        bool stateTree[topLevelItemCount()];
        BranchesTreeItem* branchItem = static_cast<BranchesTreeItem*>(item);
        const QString& branch = branchItem->branch();

        for (int i = 0; i < topLevelItemCount(); i++) {
            stateTree[i] = this->topLevelItem(i)->isExpanded();
        }

        // перестраиваем дерево
        d->m()->changeBranch(branch);

        // возвращаем назад состояние
        for (int i = 0; i < topLevelItemCount(); i++) {
            this->topLevelItem(i)->setExpanded(stateTree[i]);
        }

        clearSelection();
        selectBranch(branch);
    }
}

void BranchesTree::selectBranch(const QString& branch) {
    QTreeWidgetItem* item = recursiveFindBranch(branch);
    if (item) setCurrentItem(item);
}

QTreeWidgetItem* BranchesTree::recursiveFindBranch(const QString& branch) {
    for (int i = 0; i < topLevelItemCount(); i++) {
        QTreeWidgetItem* item = recursiveFindBranch(topLevelItem(i), branch);
        if (item) return item;
    }
}

QTreeWidgetItem* BranchesTree::recursiveFindBranch(QTreeWidgetItem* parent, const QString& branch) {
    if (parent->type() == LeafBranch || parent->type() == LeafRemote || parent->type() == LeafTag) {
        BranchesTreeItem* branchItem = static_cast<BranchesTreeItem*>(parent);
        if (branchItem->branch().compare(branch) == 0) {
            return parent;
        }
    }

    for (int j = 0; j < parent->childCount(); j++) {
        QTreeWidgetItem* foundItem = recursiveFindBranch(parent->child(j), branch);
        if (foundItem) return foundItem;
    }

    return NULL;
}

void BranchesTree::contextMenu(const QPoint & pos)
{
    QPoint globalPos = viewport()->mapToGlobal(pos);
    globalPos += QPoint(10, 10);

    QMenu branchesTreeContextMenu(tr("Context menu"), this);
    QTreeWidgetItem *item = selectedItems().first();    
    BranchesTreeItem* branchItem;
    switch (item->type()) {
    case HeaderBranch:
        ;
    case HeaderRemote:
        ;
    case HeaderTag:
        branchesTreeContextMenu.addAction(collapseAllAction);
        branchesTreeContextMenu.addAction(expandAllAction);
        break;
    case LeafBranch:
        branchItem = static_cast<BranchesTreeItem* >(item);
        checkoutAction->setData(branchItem->branch());
        branchesTreeContextMenu.addAction(checkoutAction);
        break;
    case LeafRemote:
        break;
    case LeafTag:
        branchItem = static_cast<BranchesTreeItem* >(item);
        checkoutAction->setData(branchItem->branch());
        branchesTreeContextMenu.addAction(checkoutAction);
        branchesTreeContextMenu.addAction(removeTagAction);
        break;
    }
    if (!branchesTreeContextMenu.isEmpty()) {
        branchesTreeContextMenu.exec(globalPos);
    }
}

void BranchesTree::checkout()
{
    QString branch = (checkoutAction->data()).toString();
    d->m()->checkout(branch);
    update();
}

void BranchesTree::removeTag()
{

}


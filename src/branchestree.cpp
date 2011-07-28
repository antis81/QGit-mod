#include "branchestree.h"
#include "mainimpl.h"
#include <QDebug>

BranchesTree::BranchesTree(QWidget *parent) : QTreeWidget(parent),
    branchIcon(QString::fromUtf8(":/icons/resources/branch.png")),
    masterBranchIcon(QString::fromUtf8(":/icons/resources/branch_master.png")),
    tagIcon(QString::fromUtf8(":/icons/resources/tag.png"))
{
    setContextMenuPolicy(Qt::CustomContextMenu);

    QObject::connect(this, SIGNAL(itemDoubleClicked(QTreeWidgetItem*,int)),
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
    this->setRootIsDecorated(false);
    this->setIndentation(10);

    QPalette p = this->palette();
    p.setColor(QPalette::Base, p.color(QPalette::Window));
    this->setPalette(p);
}

void BranchesTree::setup(Domain *domain, Git *git)
{
    d = domain;
    g = git;
}

//void BranchesTree::update()
//{
//    clear();
//    addNode(BranchesTree::HeaderBranches, Reference::BRANCH);
//    addRemotesNodes();
//    addNode(BranchesTree::HeaderTags, Reference::TAG);
//    expandAll();
//}

void BranchesTree::update(QString inputText)
{
    clear();
    addNode(BranchesTree::HeaderBranches, Reference::BRANCH, inputText);
    addRemotesNodes(inputText);
    addNode(BranchesTree::HeaderTags, Reference::TAG, inputText);
    expandAll();
}

//void BranchesTree::addNode(ItemType headerType, Reference::Type type)
//{
//    // получаем нужную инфу по типу
//    QStringList tempList = g->getAllRefNames(type, !Git::optOnlyLoaded);

//    // делаем хедер и добавляем на верхний уровень
//    BranchesTreeItem *node;
//    switch (headerType) {
//    case (BranchesTree::HeaderBranches):
//        node = new BranchesTreeItem(this, QStringList("Branches"), headerType);
//        break;
//    case (BranchesTree::HeaderRemotes):
//        node = new BranchesTreeItem(this, QStringList("Remotes"), headerType);
//        break;
//    case (BranchesTree::HeaderTags):
//        node = new BranchesTreeItem(this, QStringList("Tags"), headerType);
//        break;
//    }

//    tempList.sort();

//    QFont font = node->font(0);
//    font.setBold(true);
//    node->setFont(0, font);


//    addTopLevelItem(node);

//    BranchesTreeItem *tempItemList;

//    // заполняем дерево потомками
//    FOREACH_SL (it, tempList) {
//        bool isCurrent = (g->currentBranch().compare(*it) == 0);
//        switch (headerType) {
//        case (HeaderBranches):

//            tempItemList = new BranchesTreeItem(node, QStringList(QString(*it)), LeafBranch);
//            if (isCurrent) {
//                QFont font = tempItemList->font(0);
//                font.setBold(true);
//                tempItemList->setFont(0, font);
//                tempItemList->setForeground(0, Qt::red);
//            }
//            tempItemList->setIcon(0, branchIcon);
//            if (*it == "master") {
//                tempItemList->setIcon(0, masterBranchIcon);
//            }
//            break;
//        case (HeaderRemotes):
//            tempItemList = new BranchesTreeItem(node, QStringList(QString(*it)), LeafRemote);
//            tempItemList->setIcon(0, branchIcon);
//            break;
//        case (HeaderTags):
//            tempItemList = new BranchesTreeItem(node, QStringList(QString(*it)), LeafTag);
//            tempItemList->setIcon(0, tagIcon);
//            break;
//        }
//        tempItemList->setBranch(QString(*it));
//    }
//}

void BranchesTree::addNode(ItemType headerType, Reference::Type type, QString textValue)
{
    // get info by type
    QStringList tempList = g->getAllRefNames(type, !Git::optOnlyLoaded);

    // make header and put up to tree
    BranchesTreeItem *node;
    switch (headerType) {
    case (BranchesTree::HeaderBranches):
        node = new BranchesTreeItem(this, QStringList("Branches"), headerType);
        break;
    case (BranchesTree::HeaderRemotes):
        node = new BranchesTreeItem(this, QStringList("Remotes"), headerType);
        break;
    case (BranchesTree::HeaderTags):
        node = new BranchesTreeItem(this, QStringList("Tags"), headerType);
        break;
    }

    tempList.sort();

    QFont font = node->font(0);
    font.setBold(true);
    node->setFont(0, font);


    addTopLevelItem(node);

    BranchesTreeItem *tempItemList;

    // заполняем дерево потомками
    FOREACH_SL (it, tempList) {
        bool isCurrent = (g->currentBranch().compare(*it) == 0);
        if ((textValue == QString(*it))
                || textValue == "") {
            switch (headerType) {
            case (HeaderBranches):
                tempItemList = new BranchesTreeItem(node, QStringList(QString(*it)), LeafBranch);
                if (isCurrent) {
                    QFont font = tempItemList->font(0);
                    font.setBold(true);
                    tempItemList->setFont(0, font);
                    tempItemList->setForeground(0, Qt::red);
                }
                tempItemList->setIcon(0, branchIcon);
                if (*it == "master") {
                    tempItemList->setIcon(0, masterBranchIcon);
                }
                break;
            case (HeaderRemotes):
                tempItemList = new BranchesTreeItem(node, QStringList(QString(*it)), LeafRemote);
                tempItemList->setIcon(0, branchIcon);
                break;
            case (HeaderTags):
                tempItemList = new BranchesTreeItem(node, QStringList(QString(*it)), LeafTag);
                tempItemList->setIcon(0, tagIcon);
                break;
            }
            tempItemList->setBranch(QString(*it));
        }
    }
}

void BranchesTree::addRemotesNodes(QString textValue)
{
    // делаем хедер и добавляем на верхний уровень
    QTreeWidgetItem *headerNode;
    headerNode = new QTreeWidgetItem(this, QStringList("Remotes"), HeaderRemotes);
    QFont font = headerNode->font(0);
    font.setBold(true);
    headerNode->setFont(0, font);

    // получаем нужную инфу по типу
    QStringList tempList = g->getAllRefNames(Reference::REMOTE_BRANCH, !Git::optOnlyLoaded);
    tempList.sort();


    BranchesTreeItem *tempItemList;

    QString lastRemoteName;
    QString remoteName;
    QTreeWidgetItem* parentNode = headerNode;
    QString text;

    // заполняем дерево потомками
    FOREACH_SL (it, tempList) {
        const QString& branchName = *it;
        if ((textValue == branchName)
                || (textValue == "")) {
            int i = branchName.indexOf("/");
            if (i > 0) {
                remoteName = branchName.left(i);
                text = branchName.mid(i + 1);
                if (remoteName.compare(lastRemoteName) != 0) {
                    parentNode = new BranchesTreeItem(headerNode, QStringList(remoteName), HeaderRemote);
                    lastRemoteName = remoteName;
                }
            } else {
                parentNode = headerNode;
                text = branchName;
                lastRemoteName = "";
            }
            tempItemList = new BranchesTreeItem(parentNode, QStringList(text), LeafRemote);
            tempItemList->setIcon(0, QIcon(QString::fromUtf8(":/icons/resources/branch.png")));
            tempItemList->setBranch(QString(*it));
        }
    }
}

void BranchesTree::changeBranch(QTreeWidgetItem *item, int column)
{
    if ((item->type() != LeafBranch)
            && (item->type() != LeafRemote)
            && (item->type() != LeafTag)) {
        return;
    }

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
    case HeaderBranches:
        ;
    case HeaderRemotes:
        ;
    case HeaderTags:
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

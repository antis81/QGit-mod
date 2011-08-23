#include "branchestree.h"
#include "mainimpl.h"
#include <QDebug>
#include <QKeyEvent>

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

void BranchesTree::update()
{
    clear();
    addNode(BranchesTree::HeaderBranches, Reference::BRANCH);
    addRemotesNodes();
    addNode(BranchesTree::HeaderTags, Reference::TAG);
    expandAll();
}

void BranchesTree::addNode(ItemType headerType, Reference::Type type)
{
    // получаем нужную инфу по типу
    QStringList tempList = g->getAllRefNames(type, !Git::optOnlyLoaded);

    // делаем хедер и добавляем на верхний уровень
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
    default:
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
        default:
            break;
        }
        tempItemList->setBranch(QString(*it));
    }
}

void BranchesTree::addRemotesNodes()
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

void BranchesTree::setAllItemsShown()
{
    for (int i = 0; i < topLevelItemCount(); i++) {
        setShownItem(topLevelItem(i));
    }
}

void BranchesTree::setShownItem(QTreeWidgetItem *item)
{
    item->setHidden(false);
    if (item->childCount() > 0) {
        for (int i = 0; i < item->childCount(); i++) {
            setShownItem(item->child(i));
        }
    }
}

void BranchesTree::changeBranch(QTreeWidgetItem *item, int column)  // REMEMBER: use this princip
                                                                    // of column to avoid magic numbers
                                                                    // see at this class acurately
{
    if ((item->type() != LeafBranch)
            && (item->type() != LeafRemote)
            && (item->type() != LeafTag)) {
        return;
    }

    // запоминаем состояние закрытости/открытости хедеров
    // и текст выделенного узла
    bool* stateTree = new bool[topLevelItemCount()];
    BranchesTreeItem* branchItem = static_cast<BranchesTreeItem*>(item);
    const QString& branch = branchItem->branch();

    for (int i = 0; i < topLevelItemCount(); i++) {
        stateTree[i] = this->topLevelItem(i)->isExpanded();
    }

    // rebuild tree
    d->m()->changeBranch(branch);

    // set back statement
    for (int i = 0; i < topLevelItemCount(); i++) {
        this->topLevelItem(i)->setExpanded(stateTree[i]);
    }

    delete[] stateTree;
    clearSelection();
    selectBranch(branch);
}

void BranchesTree::selectBranch(const QString& branch)
{
    QTreeWidgetItem* item = recursiveFindBranch(branch);
    if (item) setCurrentItem(item);
}

QTreeWidgetItem* BranchesTree::recursiveFindBranch(const QString& branch)
{
    for (int i = 0; i < topLevelItemCount(); i++) {
        QTreeWidgetItem* item = recursiveFindBranch(topLevelItem(i), branch);
        if (item) return item;
    }

    return NULL;
}

QTreeWidgetItem* BranchesTree::recursiveFindBranch(QTreeWidgetItem* parent, const QString& branch)
{
    if (parent->type() == LeafBranch
            || parent->type() == LeafRemote
            || parent->type() == LeafTag) {
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
    QTreeWidgetItem* item = selectedItems().first();
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

void BranchesTree::keyPressEvent(QKeyEvent *event)
{
    if ((event->key() >= Qt::Key_A) && (event->key() <= Qt::Key_Z)) {
        // FIXME: Bad code
        d->m()->searchBranchLineEdit->show();
        d->m()->searchBranchLineEdit->setFocus();
        d->m()->searchBranchLineEdit->setText(event->text());
    }
}

void BranchesTree::showSearchBranchesItems(QString inputText)
{
    bool f = false;
    setAllItemsShown();
    if (inputText.contains("/")) {
        QString firstPart = inputText.left(inputText.indexOf("/"));
        QString lastPart = inputText.mid(inputText.indexOf("/") + 1);
        // top level item named Remotes for tree is 1 index

        // hide all, exept Remotes
        for (int i = 0; i < topLevelItemCount(); i++) {
            if (i != 1)
                f = isBranchesTreeItemShown(topLevelItem(i), inputText);
        }
        // hide header in "header/lower"
        f = isBranchesTreeItemShown(topLevelItem(1), firstPart);
        // find "lower"
        for (int i = 0; i < topLevelItem(1)->childCount(); i++) {
            if (topLevelItem(1)->child(i)->text(0) == firstPart) {
                f = isBranchesTreeItemShown(topLevelItem(1)->child(i), lastPart);
            }
        }
    } else {
    // THINKME: Change to load/save statement of tree
        if (!(inputText.simplified().isEmpty())) {
            for (int i = 0; i < topLevelItemCount(); i++) {
                f = isBranchesTreeItemShown(topLevelItem(i), inputText);
            }
        } // else load condition
    }

}

bool BranchesTree::isBranchesTreeItemShown(QTreeWidgetItem *item, QString currentString)
{
    // Use it, if don't remove branchestreeitem class {
    //      BranchesTreeItem* branchItem = static_cast<BranchesTreeItem*>(item);
    // }
    if (isRegExpConformed(currentString, item->text(0))) { // FIXME: in class must be reimplement magic number "0"
        item->setHidden(false);
        return true;
    } else {
        if (item->childCount() > 0) {
            bool flag = false; // FIXME: bad name
            for (int i = 0; i < item->childCount(); i++) {
                if (isBranchesTreeItemShown(item->child(i), currentString)) {
                    if (flag == false) {
                        flag = true;
                    }
                }
            }
            item->setHidden(!flag);
            return flag;
        } else {
            item->setHidden(true);
            return false;
        }
    }
}

bool BranchesTree::isRegExpConformed(QString currentString, QString originalString)
{
    if (originalString.indexOf(currentString, 0, Qt::CaseInsensitive) == 0)
        return true;
    else
        return false;
}

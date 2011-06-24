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

    collapseHeaderAction = new QAction(tr("Collapse"), this);
    QObject::connect(collapseHeaderAction, SIGNAL(triggered()),
                     this, SLOT(collapseHeader()));

    expandHeaderAction = new QAction(tr("Expand"), this);
    QObject::connect(expandHeaderAction, SIGNAL(triggered()),
                     this, SLOT(expandHeader()));

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
        node = new QTreeWidgetItem(this, QStringList("branches"), headerType);
        break;
    case (BranchesTree::HeaderRemote):
        node = new QTreeWidgetItem(this, QStringList("remotes"), headerType);
        break;
    case (BranchesTree::HeaderTag):
        node = new QTreeWidgetItem(this, QStringList("tags"), headerType);
        break;
    }

    tempList.sort();

    QFont font = node->font(0);
    font.setBold(true);
    node->setFont(0, font);

    QColor white(255, 255, 255);
    QColor black(128, 128, 128);
    QLinearGradient localGradient(0, 0, 0, 20);
    localGradient.setColorAt(0, white);
    localGradient.setColorAt(1, black);
    node->setBackground(0, localGradient);

    addTopLevelItem(node);

    QTreeWidgetItem *tempItemList;

    // заполняем дерево потомками
    FOREACH_SL (it, tempList) {
        switch (headerType) {
        case (BranchesTree::HeaderBranch):
            // имеет значение, что node, а не this. это важно!
            tempItemList = new QTreeWidgetItem(node, QStringList(QString(*it)),
                                               BranchesTree::LeafBranch);
            break;
        case (BranchesTree::HeaderRemote):
            tempItemList = new QTreeWidgetItem(node, QStringList(QString(*it)),
                                               BranchesTree::LeafRemote);
            break;
        case (BranchesTree::HeaderTag):
            tempItemList = new QTreeWidgetItem(node, QStringList(QString(*it)),
                                               BranchesTree::LeafTag);
            break;
        }
        node->addChild(tempItemList);
    }
}

void BranchesTree::changeBranch(QTreeWidgetItem *item, int column)
{
    // !!!  Осторожно! КОСТЫЛЬ!  !!!
    // проверяем не заголовок ли дерева. если нет, то переходим.
    // примечание: ->childCount() == 0) { - альтернативный вариант, забракованый Скальмом
    // потому что у верхнего уровня может и не быть детей.
    // Кстати, вместо числа надо запилить константу (объявляется в addNode)
    if ((item->type() != BranchesTree::HeaderBranch)
            && (item->type() != BranchesTree::HeaderRemote)
            && (item->type() != BranchesTree::HeaderTag)) {
        // запоминаем состояние закрытости/открытости хедеров
        // и текст выделенного узла
        bool stateTree[topLevelItemCount()];
        QString tempItemText = item->text(column);

        for (int i = 0; i < topLevelItemCount(); i++) {
            stateTree[i] = this->topLevelItem(i)->isExpanded();
        }

        // перестраиваем дерево
        d->m()->changeBranch(item->text(column));

        // возвращаем назад состояние
        for (int i = 0; i < topLevelItemCount(); i++) {
            this->topLevelItem(i)->setExpanded(stateTree[i]);
        }

        clearSelection();
        QTreeWidgetItem *tempItem;
        // не будет корректно работать, если существуют одинаковые имена
        // (хер знает, как этого избежать)
        // поиск ОБЯЗАТЕЛЬНО рекурсивный
        tempItem = findItems(tempItemText, Qt::MatchRecursive, column).first();

        if (tempItem) // кажись прокатывает такой вариант
            tempItem->setSelected(true);
    }
}

void BranchesTree::contextMenu(const QPoint & pos)
{
    QMenu branchesTreeContextMenu(tr("Context menu"), this);
    QTreeWidgetItem *item = selectedItems().first();

    switch (item->type()) {
    case BranchesTree::HeaderBranch:
        ;
    case BranchesTree::HeaderRemote:
        ;
    case BranchesTree::HeaderTag:
        branchesTreeContextMenu.addAction(collapseHeaderAction);
        branchesTreeContextMenu.addAction(expandHeaderAction);
        branchesTreeContextMenu.exec(viewport()->mapToGlobal(pos));
        break;
    case BranchesTree::LeafBranch:
        branchesTreeContextMenu.addAction(checkoutAction);
        branchesTreeContextMenu.exec(viewport()->mapToGlobal(pos));
        break;
    case BranchesTree::LeafRemote:
        break;
    case BranchesTree::LeafTag:
        branchesTreeContextMenu.addAction(checkoutAction);
        branchesTreeContextMenu.addAction(removeTagAction);
        branchesTreeContextMenu.exec(viewport()->mapToGlobal(pos));
        break;
    }
}

void BranchesTree::collapseHeader()
{
    QTreeWidgetItem *item = selectedItems().first();
    item->setExpanded(false);
    update();
}

void BranchesTree::expandHeader()
{
    QTreeWidgetItem *item = selectedItems().first();
    item->setExpanded(true);
}

void BranchesTree::checkout()
{
    //
}

void BranchesTree::removeTag()
{

}

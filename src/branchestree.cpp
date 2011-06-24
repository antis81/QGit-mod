#include "branchestree.h"
#include "mainimpl.h"
#include <QDebug>

BranchesTree::BranchesTree(QWidget *parent) : QTreeWidget(parent)
{
    setColumnCount(1);
    setHeaderLabel("Branches Tree");
    QObject::connect(this, SIGNAL(itemClicked(QTreeWidgetItem*, int)),
                     this, SLOT(changeBranch(QTreeWidgetItem*, int)));
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
    // получаем нужную инфу по типу
    QStringList tempList = g->getAllRefNames(type, !Git::optOnlyLoaded);

    // делаем хедер и добавляем на верхний уровень
    QTreeWidgetItem *node = new QTreeWidgetItem(this, QStringList(header), BranchTreeNode);

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
        tempItemList = new QTreeWidgetItem(node, QStringList(QString(*it)));
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
    if (item->type() != BranchTreeNode) {
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

#ifndef BRANCHESTREE_H
#define BRANCHESTREE_H

#include <QTreeWidget>
#include "git.h"
#include "domain.h"
#include "branchestreeitem.h"

// Стоит учесть, что уникальное имя в branchesTreeItem не нужно, т.к. поиск мы всё равно производим
// по видимому имени. Или нужно иметь заранее оговорки, что они могут отличаться только наличием []
// или = = по краям. Иначе мы введём пользователя в заблуждение. Я бы лично отказался от них совсем,
// потому что KISS. Чем проще, тем лучше. И кастовать друг в друга не надо будет.

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
    void setup(Domain *domain, Git *git); // FIXME: Move to constructor
    void update();                        // FIXME: Not update, but updateStatement!

public slots:
    void showSearchBranchesItems(QString inputText = ""); // not change statement, but hide items
    void changeBranch(QTreeWidgetItem *item, int column);
    void contextMenu(const QPoint &pos);
    void checkout();
    void removeTag();

public:
    void selectBranch(const QString &branch);
private:
    void setShownItem(QTreeWidgetItem *item);
    Git *g;     // FIXME: Too short names
    Domain *d;  //
    QAction *collapseAllAction;
    QAction *expandAllAction;
    QAction *checkoutAction;
    QAction *removeTagAction;
    QIcon branchIcon;
    QIcon masterBranchIcon;
    QIcon tagIcon;
    void addNode(ItemType headerType, Reference::Type type);
    void addRemotesNodes();
    void setAllItemsShown();
    QTreeWidgetItem *recursiveFindBranch(const QString &branch);
    QTreeWidgetItem *recursiveFindBranch(QTreeWidgetItem *parent, const QString &branch);
    bool isRegExpConformed(QString currentString, QString originalString); // ATTENTION: Don't used
    bool isBranchesTreeItemContainedSearchString(QTreeWidgetItem *item, QString currentString);
};

#endif // BRANCHESTREE_H

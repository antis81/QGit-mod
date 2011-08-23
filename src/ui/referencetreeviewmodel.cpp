/*
Author: Nils Fenner (c) 2011

Copyright: See COPYING file that comes with this distribution
*/

#include <QBrush>
#include <QMenu>
#include "referencetreeviewmodel.h"
#include "referencetreeviewitem.h"
#include "git.h"

ReferenceTreeViewModel::ReferenceTreeViewModel(QObject* parent)
    : QAbstractItemModel(parent), m_git(NULL), m_rootItem(NULL)
{
}

ReferenceTreeViewModel::~ReferenceTreeViewModel()
{
}

QVariant ReferenceTreeViewModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid())
        return QVariant();

    ReferenceTreeViewItem* item = static_cast<ReferenceTreeViewItem*>(index.internalPointer());

    switch(role) {
    case Qt::DisplayRole:
        return item->text();// must be text() //->data("title");
        break;
    case Qt::FontRole:
        if (item->isHeaderItem()) {
            QFont headFont;
            headFont.setBold(true);
            return headFont;
        }
        break;
    case Qt::ForegroundRole:
        if ( (item->type() == ReferenceTreeViewItem::LeafBranch)
                && (item->name() == m_git->currentBranch()) ) {
            QBrush textColor(Qt::red);
            return textColor;
        }
        break;
    }

    return QVariant();
}

Qt::ItemFlags ReferenceTreeViewModel::flags(const QModelIndex& index) const
{
    if (!index.isValid())
        return 0;

    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

QVariant ReferenceTreeViewModel::headerData(int section, Qt::Orientation orientation,
                                            int role) const
{
    Q_UNUSED(section)

    if ((orientation == Qt::Horizontal)
            && (role == Qt::DisplayRole))
        return m_rootItem->text();

    return QVariant();
}

QModelIndex ReferenceTreeViewModel::index(int row, int column, const QModelIndex& parent) const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    ReferenceTreeViewItem* parentItem;

    if (!parent.isValid())
        parentItem = m_rootItem;
    else
        parentItem = static_cast<ReferenceTreeViewItem*>(parent.internalPointer());

    ReferenceTreeViewItem* childItem = parentItem->children().at(row);

    if (childItem)
        return createIndex(row, column, childItem);
    else
        return QModelIndex();
}

QModelIndex ReferenceTreeViewModel::parent(const QModelIndex& index) const
{
    if (!index.isValid())
        return QModelIndex();

    ReferenceTreeViewItem* childItem;
    childItem = static_cast<ReferenceTreeViewItem*>(index.internalPointer());

    if (!childItem) {
        return QModelIndex();
    }

    ReferenceTreeViewItem* parentItem = childItem->parent();
    if (!parentItem || parentItem == m_rootItem) {
        return QModelIndex();
    }

    return createIndex(parentItem->row(), 0, parentItem);
}

int ReferenceTreeViewModel::rowCount(const QModelIndex& parent) const
{
    if (parent.column() > 0)
        return 0;

    ReferenceTreeViewItem* parentItem;

    if (!parent.isValid())
        parentItem = m_rootItem;
    else
        parentItem = static_cast<ReferenceTreeViewItem*>(parent.internalPointer());

    return parentItem ? parentItem->children().count() : 0;
}

int ReferenceTreeViewModel::columnCount(const QModelIndex& parent) const
{
    return 1;
}

/**
    Set the root item. The item is not owned by the model.

    @return The previous root item or NULL, if no root item was set before.
*/
void ReferenceTreeViewModel::setRootItem(ReferenceTreeViewItem *root)
{
    delete m_rootItem;

    m_rootItem = root;
}

/**
    Initializes the repository tree.
*/
void ReferenceTreeViewModel::setup(Git* git)
{
    //setRootItem(git);
    m_git = git;

    ReferenceTreeViewItem* root;
    root = new ReferenceTreeViewItem(NULL, ReferenceTreeViewItem::HeaderBranches,
                                     "References", "References");

    m_rootItem = root;

    addNode(ReferenceTreeViewItem::HeaderBranches, Reference::BRANCH);
    addNode(ReferenceTreeViewItem::HeaderRemotes, Reference::REMOTE_BRANCH);
    addNode(ReferenceTreeViewItem::HeaderTags, Reference::TAG);
}


void ReferenceTreeViewModel::addNode(ReferenceTreeViewItem::ItemType headerType, Reference::Type type)
{
    QStringList tempList = m_git->getAllRefNames(type, !Git::optOnlyLoaded);

    ReferenceTreeViewItem *headerNode;

    switch (headerType) {
    case (ReferenceTreeViewItem::HeaderBranches):
        headerNode = new ReferenceTreeViewItem(m_rootItem, headerType, "Branches", "Branches");
        break;
    case (ReferenceTreeViewItem::HeaderRemotes):
        headerNode = new ReferenceTreeViewItem(m_rootItem, headerType, "Remotes", "Remotes");
        break;
    case (ReferenceTreeViewItem::HeaderTags):
        headerNode = new ReferenceTreeViewItem(m_rootItem, headerType, "Tags", "Tags");
        break;
    default:
        break;
    }

    tempList.sort();

//    QFont font = node->font(0);
//    font.setBold(true);
//    node->setFont(0, font);
//    addTopLevelItem(node);

    ReferenceTreeViewItem *tempItemList;

    if (headerType == ReferenceTreeViewItem::HeaderRemotes) {
 //       QStringList tempList = git->getAllRefNames(Reference::REMOTE_BRANCH, !Git::optOnlyLoaded);
 //       tempList.sort();

 //       ReferenceTreeViewItem *tempItemList;
//        ReferenceTreeViewItem* headerNode = headerNode;

        QString lastRemoteName;
        QString remoteName;
        ReferenceTreeViewItem* parentNode = headerNode;
        QString text;

        // заполняем дерево потомками
        FOREACH_SL (it, tempList) {
            const QString& branchName = *it;
            int i = branchName.indexOf("/");
            if (i > 0) {
                remoteName = branchName.left(i);
                text = branchName.mid(i + 1);
                if (remoteName.compare(lastRemoteName) != 0) {
                    parentNode = new ReferenceTreeViewItem(headerNode, ReferenceTreeViewItem::HeaderRemote, remoteName, remoteName);
    //                addNodes(headerNode, remoteName);
                    lastRemoteName = remoteName;
                }
            } else {
                parentNode = headerNode;
                text = branchName;
                lastRemoteName = "";
            }
            tempItemList = new ReferenceTreeViewItem(parentNode, ReferenceTreeViewItem::LeafRemote, text, branchName);
        }
    }


    FOREACH_SL (it, tempList) {
        //bool isCurrent = (m_git->currentBranch().compare(*it) == 0);
        switch (headerType) {
        case (ReferenceTreeViewItem::HeaderBranches):
            tempItemList = new ReferenceTreeViewItem(headerNode, ReferenceTreeViewItem::LeafBranch, QString(*it), QString(*it));
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
            break;
        case (ReferenceTreeViewItem::HeaderRemotes):
//            tempItemList = new ReferenceTreeViewItem(headerNode, ReferenceTreeViewItem::LeafRemote, QString(*it));
            //tempItemList->setIcon(0, branchIcon);
            break;
        case (ReferenceTreeViewItem::HeaderTags):
            tempItemList = new ReferenceTreeViewItem(headerNode, ReferenceTreeViewItem::LeafTag, QString(*it), QString(*it));
                    //new BranchesTreeItem(node, QStringList(QString(*it)), LeafTag);
            //tempItemList->setIcon(0, tagIcon);
            break;
        default:
            break;
        }
        //tempItemList->setBranch(QString(*it));
    }
/**/
}

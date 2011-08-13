/*
Author: Nils Fenner (c) 2011

Copyright: See COPYING file that comes with this distribution
*/

#include "referencetreemodel.h"

#include <QBrush>

#include "referencetreeitem.h"

#include "git.h"


ReferenceTreeModel::ReferenceTreeModel(QObject* parent)
    : QAbstractItemModel(parent),
      m_git(NULL),
      m_rootItem(NULL)
{
}

ReferenceTreeModel::~ReferenceTreeModel()
{
}

QVariant ReferenceTreeModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid())
        return QVariant();

    ReferenceTreeItem* item = static_cast<ReferenceTreeItem*>(index.internalPointer());

    switch(role)
    {
    case Qt::DisplayRole:
        {
            return item->data("title");
        }
        break;
    case Qt::FontRole:
        if (item->isHeaderItem())
        {
            QFont headFont;
            headFont.setBold(true);
            return headFont;
        }
        break;
    case Qt::ForegroundRole:
        if ( (item->type() == ReferenceTreeItem::LeafBranch) && (item->title() == m_git->currentBranch()) )
        {
            QBrush textColor(Qt::red);
            return textColor;
        }
        break;
    }

    return QVariant();
}

Qt::ItemFlags ReferenceTreeModel::flags(const QModelIndex& index) const
{
    if (!index.isValid())
        return 0;

    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

QVariant ReferenceTreeModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
        return m_rootItem->data("title");

    return QVariant();
}

QModelIndex ReferenceTreeModel::index(int row, int column, const QModelIndex& parent) const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    ReferenceTreeItem* parentItem;

    if (!parent.isValid())
        parentItem = m_rootItem;
    else
        parentItem = static_cast<ReferenceTreeItem*>(parent.internalPointer());

    ReferenceTreeItem* childItem = parentItem->children().at(row);
    if (childItem)
        return createIndex(row, column, childItem);
    else
        return QModelIndex();
}

QModelIndex ReferenceTreeModel::parent(const QModelIndex& index) const
{
    if (!index.isValid())
        return QModelIndex();

    ReferenceTreeItem* childItem = static_cast<ReferenceTreeItem*>(index.internalPointer());
    ReferenceTreeItem* parentItem = childItem->parent();

    if (parentItem == m_rootItem)
        return QModelIndex();

    return createIndex(parentItem->row(), 0, parentItem);
}

int ReferenceTreeModel::rowCount(const QModelIndex& parent) const
{
    ReferenceTreeItem* parentItem;
    if (parent.column() > 0)
        return 0;

    if (!parent.isValid())
        parentItem = m_rootItem;
    else
        parentItem = static_cast<ReferenceTreeItem*>(parent.internalPointer());

    return parentItem->children().count();
}

int ReferenceTreeModel::columnCount(const QModelIndex& parent) const
{
    if (parent.isValid())
        return static_cast<ReferenceTreeItem*>(parent.internalPointer())->data().count();
    else
        return m_rootItem->data().count();
}

/**
    Set the root item. The item is not owned by the model.

    @return The previous root item or NULL, if no root item was set before.
*/
void ReferenceTreeModel::setRootItem(ReferenceTreeItem *root)
{
    delete m_rootItem;

    m_rootItem = root;
}

/**
    Initializes the repository tree.
*/
void ReferenceTreeModel::setup(Git* git)
{
    m_git = git;

    // make header and add the top level items
    ReferenceTreeItem* root = new ReferenceTreeItem(NULL, ReferenceTreeItem::HeaderBranches, "Repository");

    ReferenceTreeItem* branchesItem = new ReferenceTreeItem(root, ReferenceTreeItem::HeaderBranches, "Branches");
    branchesItem->setIsHeaderItem(true);
    addNodes( branchesItem, git->getAllRefNames(Reference::BRANCH, !Git::optOnlyLoaded) );

    ReferenceTreeItem* remoteItems = new ReferenceTreeItem(root, ReferenceTreeItem::HeaderTags, "Remotes");
    remoteItems->setIsHeaderItem(true);
    addNodes( remoteItems, git->getAllRefNames(Reference::REMOTE_BRANCH, !Git::optOnlyLoaded) );

    ReferenceTreeItem* tagItems = new ReferenceTreeItem(root, ReferenceTreeItem::HeaderTags, "Tags");
    tagItems->setIsHeaderItem(true);
    addNodes( tagItems, git->getAllRefNames(Reference::TAG, !Git::optOnlyLoaded));

    //! @todo Implement functionality for stashes and submodules
    ReferenceTreeItem* stashesItem = new ReferenceTreeItem(root, ReferenceTreeItem::HeaderBranches, "Stashes");
    ReferenceTreeItem* subRepoItem = new ReferenceTreeItem(root, ReferenceTreeItem::HeaderTags, "Submodules");

    setRootItem(root); // delete previous root and set new one
}


/**
    Adds child nodes to the repository tree. The parent node must not be NULL. The root node is the repository node.
*/
void ReferenceTreeModel::addNodes(ReferenceTreeItem* parent, const QStringList& titles, bool sorted)
{
    if (parent == NULL)
        return;

    QStringList list(titles);
    if (sorted) {
        list.sort();
    }

    ReferenceTreeItem* tempItemList = NULL;

    foreach (const QString& it, list)
    {
        ReferenceTreeItem::ItemType type;
        switch (parent->type())
        {
        case ReferenceTreeItem::HeaderBranches:
            type = ReferenceTreeItem::LeafBranch;

            break;

        case ReferenceTreeItem::HeaderRemotes:
            type = ReferenceTreeItem::LeafRemote;

            break;

        case ReferenceTreeItem::HeaderTags:
            type = ReferenceTreeItem::LeafTag;

            break;

        default:
            type = ReferenceTreeItem::LeafBranch;

            break;
        }

        tempItemList = new ReferenceTreeItem(parent, type, it);
    }
}

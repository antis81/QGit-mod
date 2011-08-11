/*
Author: Nils Fenner (c) 2011

Copyright: See COPYING file that comes with this distribution
*/

#include "repomodel.h"

#include "repotreeitem.h"

#include "git.h"
#include "domain.h"


RepoModel::RepoModel(QObject* parent)
    : QAbstractItemModel(parent),
      m_rootItem(NULL)
{
}

RepoModel::~RepoModel()
{
}

QVariant RepoModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (role != Qt::DisplayRole)
        return QVariant();

    RepoTreeItem* item = static_cast<RepoTreeItem*>(index.internalPointer());

    switch (item->type())
    {
    case RepoTreeItem::HeaderBranches:
    case RepoTreeItem::HeaderRemotes:
    case RepoTreeItem::HeaderRemote:
    case RepoTreeItem::HeaderTags:
//        QFont font = tempItemList->font();
//        font.setBold(true);
//        setFont(0, font);
//        (0, Qt::red);
        break;

    default: break;
    }

    return item->data("title");
}

Qt::ItemFlags RepoModel::flags(const QModelIndex& index) const
{
    if (!index.isValid())
        return 0;

    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

QVariant RepoModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
        return m_rootItem->data("title");

    return QVariant();
}

QModelIndex RepoModel::index(int row, int column, const QModelIndex& parent) const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    RepoTreeItem* parentItem;

    if (!parent.isValid())
        parentItem = m_rootItem;
    else
        parentItem = static_cast<RepoTreeItem*>(parent.internalPointer());

    RepoTreeItem* childItem = parentItem->children().at(row);
    if (childItem)
        return createIndex(row, column, childItem);
    else
        return QModelIndex();
}

QModelIndex RepoModel::parent(const QModelIndex& index) const
{
    if (!index.isValid())
        return QModelIndex();

    RepoTreeItem* childItem = static_cast<RepoTreeItem*>(index.internalPointer());
    RepoTreeItem* parentItem = childItem->parent();

    if (parentItem == m_rootItem)
        return QModelIndex();

    return createIndex(parentItem->row(), 0, parentItem);
}

int RepoModel::rowCount(const QModelIndex& parent) const
{
    RepoTreeItem* parentItem;
    if (parent.column() > 0)
        return 0;

    if (!parent.isValid())
        parentItem = m_rootItem;
    else
        parentItem = static_cast<RepoTreeItem*>(parent.internalPointer());

    return parentItem->children().count();
}

int RepoModel::columnCount(const QModelIndex& parent) const
{
    if (parent.isValid())
        return static_cast<RepoTreeItem*>(parent.internalPointer())->data().count();
    else
        return m_rootItem->data().count();
}

/**
    Set the root item. The item is not owned by the model.

    @return The previous root item or NULL, if no root item was set before.
*/
void RepoModel::setRootItem(RepoTreeItem *root)
{
    delete m_rootItem;

    m_rootItem = root;
}

/**
    Initializes the repository tree.
*/
void RepoModel::setup(Git* git)
{
    // make header and add the top level items
    RepoTreeItem* root = new RepoTreeItem(NULL, RepoTreeItem::HeaderBranches, "Repository");

    RepoTreeItem* branchesItem = new RepoTreeItem(root, RepoTreeItem::HeaderBranches, "Branches");
    addNodes(branchesItem, git.getAllRefNames(Reference::BRANCH, !Git::optOnlyLoaded) );

    RepoTreeItem* remoteItems = new RepoTreeItem(root, RepoTreeItem::HeaderTags, "Remotes");
    addNodes(remoteItems, git.getAllRefNames(Reference::REMOTE_BRANCH, !Git::optOnlyLoaded) );

    RepoTreeItem* tagItems = new RepoTreeItem(root, RepoTreeItem::HeaderTags, "Tags");
    addNodes(tagItems, git.getAllRefNames(Reference::TAG, !Git::optOnlyLoaded));

    RepoTreeItem* subRepoItem = new RepoTreeItem(root, RepoTreeItem::HeaderTags, "Submodules");

    setRootItem(root); // delete previous root and set new one
}


/**
    Adds child nodes to the repository tree. The parent node must not be NULL. The root node is the repository node.
*/
void RepoModel::addNodes(RepoTreeItem* parent, const QStringList& titles, bool sorted)
{
    if (parent == NULL)
        return;

    QStringList list(titles);
    if (sorted) {
        list.sort();
    }

    RepoTreeItem* tempItemList = NULL;

    foreach (const QString& it, list)
    {
        RepoTreeItem::ItemType type;
        switch (parent->type())
        {
        case RepoTreeItem::HeaderBranches:
            type = RepoTreeItem::LeafBranch;

            break;

        case RepoTreeItem::HeaderRemotes:
            type = RepoTreeItem::LeafRemote;

            break;

        case RepoTreeItem::HeaderTags:
            type = RepoTreeItem::LeafTag;

            break;

        default:
            type = RepoTreeItem::LeafBranch;

            break;
        }

        tempItemList = new RepoTreeItem(parent, type, it);
    }
}

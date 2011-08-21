/*
Author: Nils Fenner (c) 2011

Copyright: See COPYING file that comes with this distribution
*/

#include "referencetreeviewmodel.h"

#include <QBrush>
#include <QMenu>

#include "referencetreeviewitem.h"

#include "git.h"


ReferenceTreeViewModel::ReferenceTreeViewModel(QObject* parent)
    : QAbstractItemModel(parent),
      m_git(NULL),
      m_rootItem(NULL)
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
        return item->data("title");
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
                && (item->title() == m_git->currentBranch()) ) {
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

    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
        return m_rootItem->data("title");

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
    ReferenceTreeViewItem* parentItem = childItem->parent();

    if (parentItem == m_rootItem)
        return QModelIndex();

    return createIndex(parentItem->row(), 0, parentItem);
}

int ReferenceTreeViewModel::rowCount(const QModelIndex& parent) const
{
    ReferenceTreeViewItem* parentItem;
    if (parent.column() > 0)
        return 0;

    if (!parent.isValid())
        parentItem = m_rootItem;
    else
        parentItem = static_cast<ReferenceTreeViewItem*>(parent.internalPointer());

    return parentItem->children().count();
}

int ReferenceTreeViewModel::columnCount(const QModelIndex& parent) const
{
    if (parent.isValid())
        return static_cast<ReferenceTreeViewItem*>(parent.internalPointer())->data().count();
    else
        return m_rootItem->data().count();
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
    // FIXME: Ugly code. Maybe a singleton would do here.
    m_git = git;

    // make header and add the top level items
    ReferenceTreeViewItem* root;
    root = new ReferenceTreeViewItem(NULL, ReferenceTreeViewItem::HeaderBranches,
                                     "Repository", git);

    ReferenceTreeViewItem* branchesItem;
    branchesItem = new ReferenceTreeViewItem(root, ReferenceTreeViewItem::HeaderBranches,
                                             "Branches", git);
    branchesItem->setIsHeaderItem(true);
    addNodes(branchesItem, git->getAllRefNames(Reference::BRANCH, !Git::optOnlyLoaded));

    ReferenceTreeViewItem* remoteItems;
    remoteItems = new ReferenceTreeViewItem(root, ReferenceTreeViewItem::HeaderTags,
                                            "Remotes", git);
    remoteItems->setIsHeaderItem(true);
    addNodes(remoteItems, git->getAllRefNames(Reference::REMOTE_BRANCH, !Git::optOnlyLoaded));

    ReferenceTreeViewItem* tagItems;
    tagItems = new ReferenceTreeViewItem(root, ReferenceTreeViewItem::HeaderTags,
                                         "Tags", git);
    tagItems->setIsHeaderItem(true);
    addNodes(tagItems, git->getAllRefNames(Reference::TAG, !Git::optOnlyLoaded));

    //! @todo Implement functionality for stashes and submodules
    ReferenceTreeViewItem* stashesItem;
    stashesItem = new ReferenceTreeViewItem(root, ReferenceTreeViewItem::HeaderBranches,
                                            "Stashes", git);

    ReferenceTreeViewItem* subRepoItem;
    subRepoItem= new ReferenceTreeViewItem(root, ReferenceTreeViewItem::HeaderTags,
                                           "Submodules", git);

    setRootItem(root); // delete previous root and set new one
}


/**
    Adds child nodes to the repository tree. The parent node must not be NULL.
    The root node is the repository node.
*/
void ReferenceTreeViewModel::addNodes(ReferenceTreeViewItem* parent, const QStringList& titles,
                                      bool sorted)
{
    if (parent == NULL)
        return;

    QStringList list(titles);

    if (sorted) {
        list.sort();
    }

    ReferenceTreeViewItem* tempItemList = NULL;

    foreach (const QString& it, list) {
        ReferenceTreeViewItem::ItemType type;
        switch (parent->type()) {
        case ReferenceTreeViewItem::HeaderBranches:
            type = ReferenceTreeViewItem::LeafBranch;
            break;
        case ReferenceTreeViewItem::HeaderRemotes:
            type = ReferenceTreeViewItem::LeafRemote;
            break;
        case ReferenceTreeViewItem::HeaderTags:
            type = ReferenceTreeViewItem::LeafTag;
            break;
        default:
            type = ReferenceTreeViewItem::LeafBranch;
            break;
        }

        tempItemList = new ReferenceTreeViewItem(parent, type, it, m_git);
    }
}

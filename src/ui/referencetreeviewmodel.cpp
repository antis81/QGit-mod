/*
Author: Nils Fenner (c) 2011

Copyright: See COPYING file that comes with this distribution
*/

#include <QBrush>
#include <QMenu>
#include "referencetreeviewmodel.h"
#include "referencetreeviewitem.h"

ReferenceTreeViewModel::ReferenceTreeViewModel(QObject* parent)
    : QAbstractItemModel(parent), m_references(NULL), m_rootItem(NULL)
{
    m_rootItem = new ReferenceTreeViewItem(NULL, ReferenceTreeViewItem::HeaderBranches,
                                     "References", "References");
}

ReferenceTreeViewModel::~ReferenceTreeViewModel()
{
    delete m_rootItem;
    m_rootItem = NULL;
}

void ReferenceTreeViewModel::clear()
{
    beginResetModel();
    m_rootItem->removeAllChildren();
    endResetModel();
}

void ReferenceTreeViewModel::update()
{
    clear();
    beginInsertRows(index(0, 0), 0, 2);
    addNode(ReferenceTreeViewItem::HeaderBranches, Reference::BRANCH);
    addNode(ReferenceTreeViewItem::HeaderRemotes, Reference::REMOTE_BRANCH);
    addNode(ReferenceTreeViewItem::HeaderTags, Reference::TAG);
    endInsertRows();
}

void ReferenceTreeViewModel::addNode(ReferenceTreeViewItem::ItemType headerType, Reference::Type type)
{
    if (!m_references) {
        return;
    }

    QStringList references = m_references->getNames(type);

    ReferenceTreeViewItem* headerNode;

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

    if (!headerNode) {
        return;
    }

    references.sort();

    ReferenceTreeViewItem* item;

    if (headerType == ReferenceTreeViewItem::HeaderRemotes) {
        QString lastRemoteName;
        QString remoteName;
        ReferenceTreeViewItem* parentNode = headerNode;
        QString text;

        // заполняем дерево потомками
        foreach (const QString& branchName, references) {
            int i = branchName.indexOf("/");
            remoteName = (i > 0) ? branchName.left(i) : "";
            text = branchName.mid(i + 1);

            if (remoteName.compare(lastRemoteName) != 0) {
                parentNode = new ReferenceTreeViewItem(headerNode,
                                                       ReferenceTreeViewItem::HeaderRemote,
                                                       remoteName,
                                                       remoteName);
                lastRemoteName = remoteName;
            } else {
                parentNode = headerNode;
            }
            item = new ReferenceTreeViewItem(parentNode,
                                             ReferenceTreeViewItem::LeafRemote,
                                             text,
                                             branchName);
        }
    }


    Reference* ref = NULL;
    foreach (const QString& reference, references) {
        switch (headerType) {
        case (ReferenceTreeViewItem::HeaderBranches):
            ref = m_references->byName(reference);
            item = new ReferenceTreeViewItem(headerNode,
                                             ReferenceTreeViewItem::LeafBranch,
                                             reference,
                                             reference);
            item->setCurrent(ref->type() & Reference::CUR_BRANCH);
            break;
        case (ReferenceTreeViewItem::HeaderTags):
            item = new ReferenceTreeViewItem(headerNode,
                                             ReferenceTreeViewItem::LeafTag,
                                             reference,
                                             reference);
            break;
        default:
            break;
        }
    }
}

QVariant ReferenceTreeViewModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }

    ReferenceTreeViewItem* item = static_cast<ReferenceTreeViewItem*>(index.internalPointer());

    if (!item) {
        return QVariant();
    }

    switch(role) {
    case Qt::DisplayRole:
        return item->text();
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
                && item->current() ) {
            QBrush textColor(Qt::red);
            return textColor;
        }
        break;
    case Qt::UserRole:
        return item->name();
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

    if (!parentItem) {
        return QModelIndex();
    }

    if (row < 0 || row >= parentItem->children().count()) {
        return QModelIndex();
    }

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
    Q_UNUSED(parent);

    return 1;
}

/**
    Initializes the repository tree.
*/
void ReferenceTreeViewModel::setup(References* references)
{
    m_references = references;
}




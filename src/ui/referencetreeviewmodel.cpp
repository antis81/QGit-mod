/*
Author: Nils Fenner (c) 2011

Copyright: See COPYING file that comes with this distribution
*/

#include <QBrush>
#include <QMenu>
#include "referencetreeviewmodel.h"
#include "referencetreeviewitem.h"
#include "model/reference.h"
#include <QDebug>

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

void ReferenceTreeViewModel::removeChildren(const QModelIndex& parent)
{
    beginRemoveRows(parent, 0, rowCount(parent));
    ReferenceTreeViewItem* item = static_cast<ReferenceTreeViewItem*>(parent.internalPointer());
    item->removeAllChildren();
    endRemoveRows();
}

void ReferenceTreeViewModel::update()
{
    if (!m_references) {
        clear();
        return;
    }

    addNode(ReferenceTreeViewItem::HeaderBranches, Reference::BRANCH);
    addNode(ReferenceTreeViewItem::HeaderRemotes, Reference::REMOTE_BRANCH);
    addNode(ReferenceTreeViewItem::HeaderTags, Reference::TAG);
}

ReferenceTreeViewItem* ReferenceTreeViewModel::createOrGetHeader(ReferenceTreeViewItem::ItemType headerType)
{
    QString name;

    switch (headerType) {
    case ReferenceTreeViewItem::HeaderBranches:
        name = "Branches";
        break;
    case ReferenceTreeViewItem::HeaderRemotes:
        name = "Remotes";
        break;
    case ReferenceTreeViewItem::HeaderTags:
        name = "Tags";
        break;
    default:
        name = "Unknown";
    }

    int index = m_rootItem->findChild(name);

    if (index >= 0) return m_rootItem->children().at(index);

    int childrenCount = m_rootItem->children().count();
    this->beginInsertRows(QModelIndex(), childrenCount, childrenCount + 1);

    ReferenceTreeViewItem* header = new ReferenceTreeViewItem(m_rootItem, headerType, name, name);

    this->endInsertRows();

    return header;
}

void ReferenceTreeViewModel::addNode(ReferenceTreeViewItem::ItemType headerType, Reference::Type type)
{
    ReferenceTreeViewItem* headerNode = createOrGetHeader(headerType);

    QModelIndex headerIndex = createIndex(headerNode->row(), 0, headerNode);

    QStringList references = m_references->getNames(type);
    references.sort();

    ReferenceTreeViewItem* item;
    QString lastRemoteName;
    QString remoteName;
    QString branchName;
    ReferenceTreeViewItem* parentNode = headerNode;
    QModelIndex parentIndex = headerIndex;
    int index;
    int prevNewIndex = -1;
    int prevOldIndex = -1;
    int newIndex = 0;
    int oldIndex = 0;
    QString oldName;



    int leftIndex = 0;
    int rightIndex = 0;
    ReferenceTreeViewItem* left;
    ReferenceTreeViewItem* right;
    QString leftName;
    QString rightName;
    int compare;

    while (true) {
        if (leftIndex < headerNode->children().count()) {
            left = headerNode->children().at(leftIndex);
            leftName = left->name();
        } else {
            left = NULL;
        }
        if (rightIndex < references.count()) {
            rightName = references.at(rightIndex);
        } else {
            rightName = QString();
        }

        if (left != NULL && !rightName.isNull()) {
            compare = leftName.compare(rightName);
        } else if (left != NULL && rightName.isNull()) {
            compare = -1;
        } else if (left == NULL && !rightName.isNull()) {
            compare = 1;
        } else {
            break;
        }
        if (compare == 0) {
            leftIndex++;
            rightIndex++;
        } else if (compare < 0) {
            beginRemoveRows(headerIndex, leftIndex, leftIndex);
            delete left;
            endRemoveRows();
        } else if (compare > 0) {
            qDebug()<<"insert" << rightName;
            beginInsertRows(headerIndex, leftIndex, leftIndex);
            item = new ReferenceTreeViewItem(NULL,
                                             ReferenceTreeViewItem::LeafBranch,
                                             rightName,
                                             rightName);
            item->setParent(headerNode, leftIndex);
            endInsertRows();
            rightIndex++;
            leftIndex++;
        }
    }

//    Reference* ref = NULL;
//    foreach (const QString& referenceName, references) {
//        switch (headerType) {
//        case ReferenceTreeViewItem::HeaderBranches:
//            oldName = headerNode->children().at(oldIndex)->name();
//            if (newName == oldName) {
//                newIndex++;
//                oldIndex++;
//                break;
//            }

//            while (newName > oldName) {
//                beginRemoveRows(headerIndex, oldIndex, oldIndex);
//                oldName = headerNode->children().at(oldIndex)->name();
//            }

//            while (newName < oldName) {
//                if (newName <= oldName) break;
//                beginRemoveRows(headerIndex, );
//                oldIndex--;
//            }

//            ref = m_references->byName(referenceName);
//            beginInsertRows(headerIndex, headerNode->children().count(), headerNode->children().count() + 1);
//            item = new ReferenceTreeViewItem(parentNode,
//                                             ReferenceTreeViewItem::LeafBranch,
//                                             referenceName,
//                                             referenceName);
//            item->setCurrent(ref->type() & Reference::CUR_BRANCH);
//            endInsertRows();
//            break;

//        case ReferenceTreeViewItem::HeaderTags:
//            removeChildren(headerIndex);

//            beginInsertRows(headerIndex, headerNode->children().count(), headerNode->children().count() + 1);
//            item = new ReferenceTreeViewItem(parentNode,
//                                             ReferenceTreeViewItem::LeafTag,
//                                             referenceName,
//                                             referenceName);
//            endInsertRows();
//            break;

//        case ReferenceTreeViewItem::HeaderRemotes:
//            parseRemoteBranchName(referenceName, remoteName, branchName);
//            if (remoteName.isEmpty()) {
//                parentNode = headerNode;
//                parentIndex = headerIndex;
//            } else if (remoteName.compare(lastRemoteName) != 0) {
//                index = headerNode->findChild(remoteName);
//                if (index < 0) {
//                    beginInsertRows(headerIndex, headerNode->children().count(), headerNode->children().count() + 1);
//                    parentNode = new ReferenceTreeViewItem(headerNode,
//                                                           ReferenceTreeViewItem::HeaderRemote,
//                                                           remoteName,
//                                                           remoteName);
//                    endInsertRows();
//                } else {
//                    parentNode = headerNode->children().at(index);
//                }
//                parentIndex = createIndex(parentNode->row(), 0, parentNode);
//                lastRemoteName = remoteName;

//                removeChildren(parentIndex);
//            }
//            beginInsertRows(parentIndex, parentNode->children().count(), parentNode->children().count() + 1);
//            item = new ReferenceTreeViewItem(parentNode,
//                                             ReferenceTreeViewItem::LeafRemote,
//                                             branchName,
//                                             referenceName);
//            endInsertRows();
//        default:
//            break;
//        }
//    }
}

void ReferenceTreeViewModel::parseRemoteBranchName(const QString& remoteBranchName, QString& remoteName, QString& branchName)
{
    int i = remoteBranchName.indexOf("/");
    remoteName = (i > 0) ? remoteBranchName.left(i) : "";
    branchName = remoteBranchName.mid(i + 1);
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
        if ((item->type() == ReferenceTreeViewItem::LeafBranch)
                && item->current() ) {
            QBrush textColor(Qt::red);
            return textColor;
        }
        break;
    case Qt::DecorationRole:
        if (!item->icon().isNull())
            return item->icon();
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




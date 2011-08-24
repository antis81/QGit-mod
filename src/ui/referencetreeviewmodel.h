/*
Author: Nils Fenner (c) 2011

Copyright: See COPYING file that comes with this distribution
*/

#ifndef REFERENCETREEVIEWMODEL_H
#define REFERENCETREEVIEWMODEL_H

#include <QAbstractItemModel>
#include <QPoint>

#include "referencetreeviewitem.h"
#include "model/reference.h" //! \todo remove this dependence
#include "git/references.h"

/**
    @brief Provides a tree model managing the main elements of a git repository structure like
    barnches, tags, submodules etc.
*/

class ReferenceTreeViewModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    explicit ReferenceTreeViewModel(QObject* parent = 0);
    virtual ~ReferenceTreeViewModel();

    QVariant data(const QModelIndex& index, int role) const;
    Qt::ItemFlags flags(const QModelIndex& index) const;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const;
    QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const;
    QModelIndex parent(const QModelIndex& index) const;

    int rowCount(const QModelIndex& parent = QModelIndex()) const;
    int columnCount(const QModelIndex& parent = QModelIndex()) const;

    void setup(References* references);
    void update();

private:
    References* m_references;
    ReferenceTreeViewItem* m_rootItem;

    void clear();

    void addNode(ReferenceTreeViewItem::ItemType headerType, Reference::Type type);
};

#endif // REFERENCETREEVIEWMODEL_H

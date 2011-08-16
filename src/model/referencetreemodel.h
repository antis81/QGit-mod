/*
Author: Nils Fenner (c) 2011

Copyright: See COPYING file that comes with this distribution
*/

#ifndef REPOMODEL_H
#define REPOMODEL_H

#include <QAbstractItemModel>
#include <QPoint>

class ReferenceTreeItem;
class Domain;
class Git;


/**
    @brief Provides a tree model managing the main elements of a git repository structure like barnches, tags, submodules etc.
*/
class ReferenceTreeModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    explicit ReferenceTreeModel(QObject* parent = 0);
    virtual ~ReferenceTreeModel();

    void setRootItem(ReferenceTreeItem* root);

    QVariant data(const QModelIndex& index, int role) const;
    Qt::ItemFlags flags(const QModelIndex& index) const;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const;
    QModelIndex index(int row, int column,
                      const QModelIndex& parent = QModelIndex()) const;
    QModelIndex parent(const QModelIndex& index) const;
    int rowCount(const QModelIndex& parent = QModelIndex()) const;
    int columnCount(const QModelIndex& parent = QModelIndex()) const;

    void setup(Git* git);

// FIXME: outsource menu and actions to a proxy like thing
public slots:
    void showContextMenu(QPoint pos);

    void actionCheckout();

private:
    Git* m_git;
    ReferenceTreeItem* m_rootItem;

    void addNodes(ReferenceTreeItem* parent, const QStringList& titles, bool sorted = true);

    // FIXME: outsource git functionality to a proxy like thing
    void checkout() const;
};

#endif // REPOMODEL_H

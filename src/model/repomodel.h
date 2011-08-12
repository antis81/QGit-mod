/*
Author: Nils Fenner (c) 2011

Copyright: See COPYING file that comes with this distribution
*/

#ifndef REPOMODEL_H
#define REPOMODEL_H

#include <QAbstractItemModel>

class RepoTreeItem;
class Domain;
class Git;


/**
@brief Provides a tree model managing the main elements of a git repository structure like barnches, tags, submodules etc.
*/
class RepoModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    explicit RepoModel(QObject *parent = 0);
    virtual ~RepoModel();

    void setRootItem(RepoTreeItem * root);

    QVariant data(const QModelIndex &index, int role) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const;
    QModelIndex index(int row, int column,
                      const QModelIndex &parent = QModelIndex()) const;
    QModelIndex parent(const QModelIndex &index) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;

    void setup(Git &git);

private:
    Git *               m_git;

    RepoTreeItem *      m_rootItem;

    void addNodes(RepoTreeItem * parent, const QStringList &titles, bool sorted=true);
};

#endif // REPOMODEL_H

/*
    Author: Nils Fenner (C) 2005-2011

    Copyright: See COPYING file that comes with this distribution
*/

#ifndef REPOTREEITEM_H
#define REPOTREEITEM_H

#include <QList>
#include <QVariant>


/**
    @brief Represents a repository tree item.
*/

class RepoTreeItem
{
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

    explicit RepoTreeItem(RepoTreeItem* parent, RepoTreeItem::ItemType type, const QString& title );

    bool isHeaderItem() const;
    void setIsHeaderItem(bool yes);

    RepoTreeItem::ItemType type() const;
    //void setType(RepoTreeItem::ItemType type);

    const QString title() const;
    void setTitle(const QString& value);

    RepoTreeItem* parent();
    void setParent(RepoTreeItem* parent);

    const QList<RepoTreeItem*>& children() const;
    QList<RepoTreeItem*>& children();

    const QMap<QString, QVariant>& data() const;
    QVariant data(const QString& id) const;

    int row() const;

private:
    RepoTreeItem::ItemType      m_type;
    bool                        m_isHeader;
    RepoTreeItem*               m_parent;
    QList<RepoTreeItem*>        m_children;
    QMap<QString, QVariant>     m_itemData;
};

#endif // REPOTREEITEM_H

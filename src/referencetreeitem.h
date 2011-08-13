/*
    Author: Nils Fenner (C) 2011

    Copyright: See COPYING file that comes with this distribution
*/

#ifndef REPOTREEITEM_H
#define REPOTREEITEM_H

#include <QList>
#include <QVariant>


/**
    @brief Represents a repository reference tree item.
*/

class ReferenceTreeItem
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

    explicit ReferenceTreeItem(ReferenceTreeItem* parent, ReferenceTreeItem::ItemType type, const QString& title );

    bool isHeaderItem() const;
    void setIsHeaderItem(bool yes);

    ReferenceTreeItem::ItemType type() const;
    //void setType(RepoTreeItem::ItemType type);

    const QString title() const;
    void setTitle(const QString& value);

    ReferenceTreeItem* parent();
    void setParent(ReferenceTreeItem* parent);

    const QList<ReferenceTreeItem*>& children() const;
    QList<ReferenceTreeItem*>& children();

    const QMap<QString, QVariant>& data() const;
    QVariant data(const QString& id) const;

    int row() const;

private:
    ReferenceTreeItem::ItemType m_type;
    bool                        m_isHeader;
    ReferenceTreeItem*          m_parent;
    QList<ReferenceTreeItem*>   m_children;
    QMap<QString, QVariant>     m_itemData;
};

#endif // REPOTREEITEM_H

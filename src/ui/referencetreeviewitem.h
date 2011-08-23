/*
    Author: Nils Fenner (C) 2011

    Copyright: See COPYING file that comes with this distribution
*/

#ifndef REFERENCETREEVIEWITEM_H
#define REFERENCETREEVIEWITEM_H

#include <QList>
#include <QVariant>
#include <QString>

class Git;

/**
    @brief Represents a repository reference tree item.
*/

class ReferenceTreeViewItem
{
public:
    enum ItemType
    {
        HeaderBranches = 257,
        HeaderRemotes  = 258,
        HeaderTags     = 259,
        LeafBranch     = 260,
        LeafRemote     = 261,
        LeafTag        = 262,
        HeaderRemote   = 263
    };

    explicit ReferenceTreeViewItem(ReferenceTreeViewItem* parent,
                                   ReferenceTreeViewItem::ItemType type, const QString& text,
                                   const QString& name);
    virtual ~ReferenceTreeViewItem();

    bool isHeaderItem() const;

    ReferenceTreeViewItem::ItemType type() const;
    ReferenceTreeViewItem* parent();
    QList<ReferenceTreeViewItem*>& children();
    QString name();
    QString text();
    void setParent(ReferenceTreeViewItem* parent);

    int row() const;

//public slots:
//    void checkout();
//    void removeReference();
//    void showRevision();

private:
    ReferenceTreeViewItem*        m_parent;
    QList<ReferenceTreeViewItem*> m_children;
    ItemType                      m_type;
    QString                       m_name;
    QString                       m_text;
};

#endif // REFERENCETREEVIEWITEM_H

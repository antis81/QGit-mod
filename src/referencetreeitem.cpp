/*
    Author: Nils Fenner (C) 2011

    Copyright: See COPYING file that comes with this distribution
*/

#include "referencetreeitem.h"

ReferenceTreeItem::ReferenceTreeItem( ReferenceTreeItem * parent, ReferenceTreeItem::ItemType type, const QString & title ) :
    m_type(type)
  , m_isHeader(false)
  , m_parent(parent)
{
    setTitle(title);
    if (parent != NULL)
        parent->children().append(this);
}

ReferenceTreeItem * ReferenceTreeItem::parent()
{
    return m_parent;
}

const QList<ReferenceTreeItem *> & ReferenceTreeItem::children() const
{
    return m_children;
}

const QMap<QString, QVariant> & ReferenceTreeItem::data() const
{
    return m_itemData;
}

QVariant ReferenceTreeItem::data(const QString &id) const
{
    return m_itemData.value(id, QVariant());
}

int ReferenceTreeItem::row() const
{
    if (m_parent)
        return m_parent->children().indexOf(const_cast<ReferenceTreeItem*>(this));

    return 0;
}

const QString & ReferenceTreeItem::title() const
{
    return m_itemData.value("title", QVariant()).toString();
}

void ReferenceTreeItem::setTitle(const QString &value)
{
    m_itemData.insert("title", value);
}

ReferenceTreeItem::ItemType ReferenceTreeItem::type() const
{
    return m_type;
}

QList<ReferenceTreeItem *> & ReferenceTreeItem::children()
{
    return m_children;
}

/**
Header items are parent items with a grouping function.

@return Returns true when it is a header item.
*/
bool ReferenceTreeItem::isHeaderItem() const
{
    return m_isHeader;
}

/**
@see isHeaderItem
*/
void ReferenceTreeItem::setIsHeaderItem(bool yes)
{
    m_isHeader = yes;
}


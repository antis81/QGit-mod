/*
    Author: Nils Fenner (C) 2011

    Copyright: See COPYING file that comes with this distribution
*/

#include "referencetreeviewitem.h"

ReferenceTreeViewItem::ReferenceTreeViewItem(ReferenceTreeViewItem* parent,
                                             ReferenceTreeViewItem::ItemType type,
                                             const QString& text, const QString& name)
    : m_type(type), m_text(text), m_name(name), m_parent(parent)
{
    setParent(parent);
}

ReferenceTreeViewItem::~ReferenceTreeViewItem()
{
    setParent(NULL);
    qDeleteAll(m_children);
}

ReferenceTreeViewItem* ReferenceTreeViewItem::parent()
{
    return m_parent;
}

void ReferenceTreeViewItem::setParent(ReferenceTreeViewItem* parent)
{
    if (m_parent) {
        m_parent->children().removeOne(this);
        m_parent = NULL;
    }

    m_parent = parent;

    if (m_parent) {
        parent->children().append(this);
    }
}

int ReferenceTreeViewItem::row() const
{
    return m_parent ? m_parent->children().indexOf(const_cast<ReferenceTreeViewItem*>(this)) : 0;
}

ReferenceTreeViewItem::ItemType ReferenceTreeViewItem::type() const
{
    return m_type;
}

QList<ReferenceTreeViewItem*>& ReferenceTreeViewItem::children()
{
    return m_children;
}

QString ReferenceTreeViewItem::name()
{
    return m_name;
}

QString ReferenceTreeViewItem::text()
{
    return m_text;
}

/**
Header items are parent items with a grouping function.

@return Returns true when it is a header item.
*/
bool ReferenceTreeViewItem::isHeaderItem() const
{
    return ((type() == ReferenceTreeViewItem::HeaderBranches)
            || (type() == ReferenceTreeViewItem::HeaderRemotes)
            || (type() == ReferenceTreeViewItem::HeaderTags));
}

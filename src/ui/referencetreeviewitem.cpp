/*
    Author: Nils Fenner (C) 2011

    Copyright: See COPYING file that comes with this distribution
*/

#include "referencetreeviewitem.h"
#include <QDebug>
ReferenceTreeViewItem::ReferenceTreeViewItem(ReferenceTreeViewItem* parent,
                                             ReferenceTreeViewItem::ItemType type,
                                             const QString& text, const QString& name)
    : m_parent(NULL),
      m_type(type),
      m_name(name),
      m_text(text),
      m_current(false)
{
    setParent(parent);
}

ReferenceTreeViewItem::~ReferenceTreeViewItem()
{
    setParent(NULL);
    removeAllChildren();
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
        m_parent->children().append(this);
    }
}

void ReferenceTreeViewItem::removeAllChildren()
{
    while (m_children.count()) {
        delete m_children.at(0);
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

const QString& ReferenceTreeViewItem::name() const
{
    return m_name;
}

const QString& ReferenceTreeViewItem::text() const
{
    return m_text;
}

bool ReferenceTreeViewItem::current() const
{
    return m_current;
}

void ReferenceTreeViewItem::setCurrent(bool current)
{
    m_current = current;
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

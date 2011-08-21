/*
    Author: Nils Fenner (C) 2011

    Copyright: See COPYING file that comes with this distribution
*/

#include "referencetreeviewitem.h"
#include "git.h"

ReferenceTreeViewItem::ReferenceTreeViewItem(ReferenceTreeViewItem* parent,
                                             ReferenceTreeViewItem::ItemType type,
                                             const QString& title, Git* git )
    : QObject(0),
      m_type(type),
      m_isHeader(false),
      m_parent(parent),
      m_git(git)
{
    setTitle(title);

    if (parent != NULL) {
        parent->children().append(this);
    }
}

ReferenceTreeViewItem::~ReferenceTreeViewItem()
{
    qDeleteAll(m_children);
}

ReferenceTreeViewItem* ReferenceTreeViewItem::parent()
{
    return m_parent;
}

const QList<ReferenceTreeViewItem*> & ReferenceTreeViewItem::children() const
{
    return m_children;
}

const QMap<QString, QVariant>& ReferenceTreeViewItem::data() const
{
    return m_itemData;
}

QVariant ReferenceTreeViewItem::data(const QString& id) const
{
    return m_itemData.value(id, QVariant());
}

int ReferenceTreeViewItem::row() const
{
    return m_parent ? m_parent->children().indexOf(const_cast<ReferenceTreeViewItem*>(this)) : 0;
}

QString ReferenceTreeViewItem::title() const
{
    return m_itemData.value("title", QVariant()).toString();
}

void ReferenceTreeViewItem::setTitle(const QString& value)
{
    m_itemData.insert("title", value);
}

ReferenceTreeViewItem::ItemType ReferenceTreeViewItem::type() const
{
    return m_type;
}

QList<ReferenceTreeViewItem*>& ReferenceTreeViewItem::children()
{
    return m_children;
}

/**
Header items are parent items with a grouping function.

@return Returns true when it is a header item.
*/
bool ReferenceTreeViewItem::isHeaderItem() const
{
    return m_isHeader;
}

/**
@see isHeaderItem
*/
void ReferenceTreeViewItem::setIsHeaderItem(bool yes)
{
    m_isHeader = yes;
}

/**
Check out the item.
*/
void ReferenceTreeViewItem::checkout()
{
    if ((m_type != LeafBranch) && (m_type != LeafTag)) {
        return;
    }

    m_git->checkout( title() );
}

/**
Remove the item (branch, tag, ...) from the repo.
*/
void ReferenceTreeViewItem::removeReference()
{
    //! @todo remove reference from repo
    //m_git->removeReference( title() );
}

void ReferenceTreeViewItem::showRevision()
{
    if ((m_type != ReferenceTreeViewItem::LeafBranch) && (m_type != ReferenceTreeViewItem::LeafTag))
            return;

    //! @todo jump to revision in history view
}

/*
    Author: Nils Fenner (C) 2011

    Copyright: See COPYING file that comes with this distribution
*/

#include "referencetreeviewitem.h"
#include "git.h"

ReferenceTreeViewItem::ReferenceTreeViewItem(ReferenceTreeViewItem* parent,
                                             ReferenceTreeViewItem::ItemType type,
                                             const QString& name)//, Git* git )
    : m_type(type),
      m_name(name),
      m_parent(parent)
{
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
    if ((type() == ReferenceTreeViewItem::HeaderBranches)
            || (type() == ReferenceTreeViewItem::HeaderRemotes)
            || (type() == ReferenceTreeViewItem::HeaderTags))
        return true;
    else
        return false;
}

/**
Check out the item.
*/
//void ReferenceTreeViewItem::checkout()
//{
//    if ((m_type != LeafBranch) && (m_type != LeafTag)) {
//        return;
//    }

//    m_git->checkout(title());
//}

///**
//Remove the item (branch, tag, ...) from the repo.
//*/
//void ReferenceTreeViewItem::removeReference()
//{
//    //! @todo remove reference from repo
//    //m_git->removeReference( title() );
//}

//void ReferenceTreeViewItem::showRevision()
//{
//    if ((m_type != ReferenceTreeViewItem::LeafBranch)
//            && (m_type != ReferenceTreeViewItem::LeafTag))
//        return;

//    //! @todo jump to revision in history view
//}

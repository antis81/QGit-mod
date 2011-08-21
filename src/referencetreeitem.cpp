/*
    Author: Nils Fenner (C) 2011

    Copyright: See COPYING file that comes with this distribution
*/

#include "referencetreeitem.h"
#include "git.h"

ReferenceTreeItem::ReferenceTreeItem(ReferenceTreeItem* parent, ReferenceTreeItem::ItemType type, const QString& title, Git* git )
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

ReferenceTreeItem::~ReferenceTreeItem()
{
    qDeleteAll(m_children);
}

ReferenceTreeItem* ReferenceTreeItem::parent()
{
    return m_parent;
}

const QList<ReferenceTreeItem*> & ReferenceTreeItem::children() const
{
    return m_children;
}

const QMap<QString, QVariant>& ReferenceTreeItem::data() const
{
    return m_itemData;
}

QVariant ReferenceTreeItem::data(const QString& id) const
{
    return m_itemData.value(id, QVariant());
}

int ReferenceTreeItem::row() const
{
    return m_parent ? m_parent->children().indexOf(const_cast<ReferenceTreeItem*>(this)) : 0;
}

QString ReferenceTreeItem::title() const
{
    return m_itemData.value("title", QVariant()).toString();
}

void ReferenceTreeItem::setTitle(const QString& value)
{
    m_itemData.insert("title", value);
}

ReferenceTreeItem::ItemType ReferenceTreeItem::type() const
{
    return m_type;
}

QList<ReferenceTreeItem*>& ReferenceTreeItem::children()
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

/**
Check out the item.
*/
void ReferenceTreeItem::checkout()
{

    if ((m_type != LeafBranch) && (m_type != LeafTag)) {
        return;
    }

    m_git->checkout( title() );
}

/**
Remove the item (branch, tag, ...) from the repo.
*/
void ReferenceTreeItem::removeReference()
{
    //! @todo remove reference from repo
    //m_git->removeReference( title() );
}

void ReferenceTreeItem::showRevision()
{
    if ((m_type != ReferenceTreeItem::LeafBranch) && (m_type != ReferenceTreeItem::LeafTag))
            return;

    //! @todo jump to revision in history view
}

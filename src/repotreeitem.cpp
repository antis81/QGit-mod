/*
    Author: Nils Fenner

    Copyright: See COPYING file that comes with this distribution
*/

#include "repotreeitem.h"

RepoTreeItem::RepoTreeItem(RepoTreeItem* parent, RepoTreeItem::ItemType type, const QString& title )
    : m_type(type),
      m_isHeader(false),
      m_parent(parent)
{
    setTitle(title);
    if (parent != NULL) {
        parent->children().append(this);
    }
}

RepoTreeItem* RepoTreeItem::parent()
{
    return m_parent;
}

const QList<RepoTreeItem*>& RepoTreeItem::children() const
{
    return m_children;
}

const QMap<QString, QVariant>& RepoTreeItem::data() const
{
    return m_itemData;
}

QVariant RepoTreeItem::data(const QString& id) const
{
    return m_itemData.value(id, QVariant());
}

int RepoTreeItem::row() const
{
    return m_parent ? m_parent->children().indexOf(const_cast<RepoTreeItem*>(this)) : 0;
}

const QString RepoTreeItem::title() const
{
    return m_itemData.value("title", QVariant()).toString();
}

void RepoTreeItem::setTitle(const QString& value)
{
    m_itemData.insert("title", value);
}

RepoTreeItem::ItemType RepoTreeItem::type() const
{
    return m_type;
}

QList<RepoTreeItem*>& RepoTreeItem::children()
{
    return m_children;
}

/**
Header items are parent items with a grouping function.

@return Returns true when it is a header item.
*/
bool RepoTreeItem::isHeaderItem() const
{
    return m_isHeader;
}

/**
@see isHeaderItem
*/
void RepoTreeItem::setIsHeaderItem(bool yes)
{
    m_isHeader = yes;
}


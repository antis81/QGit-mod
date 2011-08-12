/*
**    Copyright (c) 2011 by Nils Fenner
**
**    This file is part of %TARGET%.
**
**    %TARGET% is free software: you can redistribute it and/or modify
**    it under the terms of the GNU General Public License as published by
**    the Free Software Foundation, either version 3 of the License, or
**    (at your option) any later version.
**
**    %TARGET% is distributed in the hope that it will be useful,
**    but WITHOUT ANY WARRANTY; without even the implied warranty of
**    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**    GNU General Public License for more details.
**
**    You should have received a copy of the GNU General Public License
**    along with %TARGET%.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "repotreeitem.h"

RepoTreeItem::RepoTreeItem( RepoTreeItem * parent, RepoTreeItem::ItemType type, const QString & title ) :
    m_type(type)
  , m_isHeader(false)
  , m_parent(parent)
{
    setTitle(title);
    if (parent != NULL)
        parent->children().append(this);
}

RepoTreeItem * RepoTreeItem::parent()
{
    return m_parent;
}

const QList<RepoTreeItem *> & RepoTreeItem::children() const
{
    return m_children;
}

const QMap<QString, QVariant> & RepoTreeItem::data() const
{
    return m_itemData;
}

QVariant RepoTreeItem::data(const QString &id) const
{
    return m_itemData.value(id, QVariant());
}

int RepoTreeItem::row() const
{
    if (m_parent)
        return m_parent->children().indexOf(const_cast<RepoTreeItem*>(this));

    return 0;
}

const QString & RepoTreeItem::title() const
{
    return m_itemData.value("title", QVariant()).toString();
}

void RepoTreeItem::setTitle(const QString &value)
{
    m_itemData.insert("title", value);
}

RepoTreeItem::ItemType RepoTreeItem::type() const
{
    return m_type;
}

QList<RepoTreeItem *> & RepoTreeItem::children()
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


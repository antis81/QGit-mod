/*
    Author: Nils Fenner (C) 2011

    Copyright: See COPYING file that comes with this distribution
*/

#ifndef REFERENCETREEITEM_H
#define REFERENCETREEITEM_H

#include <QList>
#include <QVariant>

class Git;


/**
    @brief Represents a repository reference tree item.
*/

class ReferenceTreeItem : public QObject
{
    Q_OBJECT
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

    explicit ReferenceTreeItem(ReferenceTreeItem* parent, ReferenceTreeItem::ItemType type, const QString& title, Git* git);
    virtual ~ReferenceTreeItem();

    bool isHeaderItem() const;
    void setIsHeaderItem(bool yes);

    ReferenceTreeItem::ItemType type() const;
    //void setType(RepoTreeItem::ItemType type);

    QString title() const;
    void setTitle(const QString& value);

    ReferenceTreeItem* parent();
    void setParent(ReferenceTreeItem* parent);

    const QList<ReferenceTreeItem*>& children() const;
    QList<ReferenceTreeItem*>& children();

    const QMap<QString, QVariant>& data() const;
    QVariant data(const QString& id) const;

    int row() const;

public slots:
    void checkout();
    void removeReference();
    void showRevision();

private:
    ReferenceTreeItem::ItemType m_type;
    bool                        m_isHeader;
    ReferenceTreeItem*          m_parent;
    QList<ReferenceTreeItem*>   m_children;
    QMap<QString, QVariant>     m_itemData;

    Git* m_git; //!< @todo Workaround - need a better solution!

};

#endif // REFERENCETREEITEM_H

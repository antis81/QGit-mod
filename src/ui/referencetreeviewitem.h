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

class ReferenceTreeViewItem : public QObject
{
//    Q_OBJECT

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
                                   ReferenceTreeViewItem::ItemType type, const QString& name);
    virtual ~ReferenceTreeViewItem();

    bool isHeaderItem() const;
//    void setIsHeaderItem(bool yes);

    ReferenceTreeViewItem::ItemType type() const;
//    void setType(RepoTreeItem::ItemType type);

//    QString title() const;
//    void setTitle(const QString& value);

    ReferenceTreeViewItem* parent();
//    void setParent(ReferenceTreeViewItem* parent);

//    const QList<ReferenceTreeViewItem*>& children() const;
    QList<ReferenceTreeViewItem*>& children();
    QString name();
    QString text();

//    const QMap<QString, QVariant>& data() const;
//    QVariant data(const QString& id) const;


    int row() const;

//public slots:
//    void checkout();
//    void removeReference();
//    void showRevision();

private:
    ReferenceTreeViewItem*        m_parent;
    QList<ReferenceTreeViewItem*> m_children;
    ItemType                      m_type;
//    bool                          m_isHeader; // m_type give this info
//    QMap<QString, QVariant>       m_itemData; // QVariant - bad solution. try to avoid it.
    QString                       m_name;
    QString                       m_text;

//    Git* m_git; //!< @todo Workaround - need a better solution!
};

#endif // REFERENCETREEVIEWITEM_H

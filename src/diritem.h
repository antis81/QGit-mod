#ifndef DIRITEM_H
#define DIRITEM_H

#include "fileitem.h"

class DirItem : public FileItem
{
public:
    DirItem(QTreeWidget *parent, SCRef ts, SCRef nm);
    DirItem(DirItem *parent, SCRef ts, SCRef nm);

protected:
    friend class TreeView;

    QString treeSha;
};

#endif // DIRITEM_H

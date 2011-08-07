#ifndef FILEITEM_H
#define FILEITEM_H

#include <QTreeWidgetItem>
#include <QStringList>
#include "common.h"

class FileItem : public QTreeWidgetItem
{
public:
    FileItem(FileItem* p, SCRef nm) : QTreeWidgetItem(p, QStringList(nm)) {}
    FileItem(QTreeWidget* p, SCRef nm) : QTreeWidgetItem(p, QStringList(nm)) {}

    virtual QString fullName() const;
    void setBold(bool b);
};

#endif // FILEITEM_H

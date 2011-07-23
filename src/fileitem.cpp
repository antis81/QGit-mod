#include "fileitem.h"

QString FileItem::fullName() const {

    QTreeWidgetItem* p = parent();
    QString s(p ? text(0) : ""); // root directory has no fullName

    while (p && p->parent()) {
        s.prepend(p->text(0) + '/');
        p = p->parent();
    }
    return s;
}

void FileItem::setBold(bool b) {

    if (font(0).bold() == b)
        return;

    QFont fnt(font(0));
    fnt.setBold(b);
    setFont(0, fnt);
}

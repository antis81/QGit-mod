#include "diritem.h"

DirItem::DirItem(DirItem* p, SCRef ts, SCRef nm) : FileItem(p, nm), treeSha(ts) {}
DirItem::DirItem(QTreeWidget* p, SCRef ts, SCRef nm) : FileItem(p, nm), treeSha(ts) {}

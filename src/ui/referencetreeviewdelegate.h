#ifndef REFERENCETREEVIEWDELEGATE_H
#define REFERENCETREEVIEWDELEGATE_H

#include <QObject>
#include <QString>
#include <QPoint>

class ReferenceTreeViewDelegate : public QObject
{
    Q_OBJECT
public:
    ReferenceTreeViewDelegate() : QObject(0) {}
    virtual void processDblClick(const QString& referenceName) = 0;
    virtual void processContextMenu(const QPoint& pos, const QString& referenceName) = 0;
};

#endif // REFERENCETREEVIEWDELEGATE_H

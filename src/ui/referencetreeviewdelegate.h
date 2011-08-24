#ifndef REFERENCETREEVIEWDELEGATE_H
#define REFERENCETREEVIEWDELEGATE_H

#include <QString>

class ReferenceTreeViewDelegate
{
public:
    virtual void processDblClick(QString referenceName) = 0;
};

#endif // REFERENCETREEVIEWDELEGATE_H

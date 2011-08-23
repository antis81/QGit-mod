#ifndef REFERENCETREEVIEWDELEGATE_H
#define REFERENCETREEVIEWDELEGATE_H

#include <QObject>
#include <QString>
#include "domain.h"
#include "mainimpl.h"

class ReferenceTreeViewDelegate : public QObject
{
public:
    ReferenceTreeViewDelegate(QObject* parent = 0);
    void setup(Domain* domain);
    void processDblClick(QString referenceName);
private:
    Domain* m_domain;
};

#endif // REFERENCETREEVIEWDELEGATE_H

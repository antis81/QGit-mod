#ifndef REFERENCETREEVIEWDELEGATE_H
#define REFERENCETREEVIEWDELEGATE_H

#include "domain.h"
#include "mainimpl.h"

class MainImpl;

class ReferenceTreeViewDelegate : public QObject
{
public:
    ReferenceTreeViewDelegate(QObject* parent = 0);
    void setup(Domain* domain);
    void changeReference(QString referenceName);
private:
    Domain* m_domain;
};

#endif // REFERENCETREEVIEWDELEGATE_H

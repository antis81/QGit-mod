#ifndef REFERENCETREEVIEWDELEGATE_H
#define REFERENCETREEVIEWDELEGATE_H

#include "domain.h"
#include "mainimpl.h"

#include <qdebug.h>
#include <QObject>

class MainImpl;

class ReferenceTreeViewDelegate : public QObject
{
    Q_OBJECT
public:
    ReferenceTreeViewDelegate(QObject* parent = 0);
    void setup(Domain* domain);
    void changeReference(QString referenceName);
signals:
    void setReference(QString referenceName);
private:
    Domain* m_domain;
};

#endif // REFERENCETREEVIEWDELEGATE_H

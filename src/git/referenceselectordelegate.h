#ifndef REFERENCESELECTORDELEGATE_H
#define REFERENCESELECTORDELEGATE_H

#include <QObject>
#include <QString>
#include <QPoint>

#include "ui/referencetreeviewdelegate.h"
#include "domain.h"
#include "mainimpl.h"

class ReferenceSelectorDelegate : public ReferenceTreeViewDelegate
{
public:
    ReferenceSelectorDelegate();
    void setup(Domain* domain);
    void processDblClick(const QString& referenceName);
    void processContextMenu(const QPoint& pos, const QString& referenceName);
private:
    Domain* m_domain;
private slots:
    void checkout();
    void removeTag();
};

#endif // REFERENCESELECTORDELEGATE_H

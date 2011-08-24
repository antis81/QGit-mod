#ifndef REFERENCESELECTORDELEGATE_H
#define REFERENCESELECTORDELEGATE_H

#include "ui/referencetreeviewdelegate.h"
#include "domain.h"
#include "mainimpl.h"

class ReferenceSelectorDelegate : public ReferenceTreeViewDelegate
{
public:
    ReferenceSelectorDelegate();
    void setup(Domain* domain);
    void processDblClick(QString referenceName);
private:
    Domain* m_domain;
};

#endif // REFERENCESELECTORDELEGATE_H

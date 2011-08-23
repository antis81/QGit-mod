#include "referencetreeviewdelegate.h"

ReferenceTreeViewDelegate::ReferenceTreeViewDelegate(QObject* parent) :QObject(parent)
{
}

void ReferenceTreeViewDelegate::setup(Domain *domain)
{
    m_domain = domain;
}


void ReferenceTreeViewDelegate::changeReference(QString referenceName)
{
    m_domain->m()->changeBranch(referenceName);
}

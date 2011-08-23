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
//    MainImpl* f = new MainImpl("");
//    f = m_domain->m(); /
//    m_domain->m()->changeBranch(referenceName);
    emit setReference(referenceName);
}

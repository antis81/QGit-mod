#include "referenceselectordelegate.h"

ReferenceSelectorDelegate::ReferenceSelectorDelegate()
{
}

void ReferenceSelectorDelegate::setup(Domain* domain)
{
    m_domain = domain;
}

void ReferenceSelectorDelegate::processDblClick(QString referenceName)
{
    m_domain->m()->changeBranch(referenceName);
}

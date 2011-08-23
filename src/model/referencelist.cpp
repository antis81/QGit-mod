#include "referencelist.h"
#include "common.h"

ReferenceList::ReferenceList()
{
}

ReferenceList ReferenceList::filter(uint typeMask) const
{
    ReferenceList list;
    FOREACH(ReferenceList, it, *this) {
        Reference* ref = *it;
        if (ref->type() & typeMask) {
            list.append(ref);
        }
    }
    return list;
}

uint ReferenceList::containsType(uint typeMask) const
{
    uint resultMask = 0;
    FOREACH(ReferenceList, it, *this) {
        Reference* ref = *it;
        resultMask |= ref->type();
    }
    return resultMask & typeMask;
}

QStringList ReferenceList::getNames() const
{
    QStringList list;
    FOREACH(ReferenceList, it, *this) {
        Reference* ref = *it;
        list.append(ref->name());
    }
    return list;
}

QStringList ReferenceList::getShas() const
{
    QStringList list;
    FOREACH(ReferenceList, it, *this) {
        Reference* ref = *it;
        list.append(ref->sha());
    }
    return list;
}


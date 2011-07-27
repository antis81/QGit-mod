#include "shamap.h"
#include "common.h"

ShaMap::ShaMap()
{
}

uint ShaMap::checkRef(const ShaString& sha, uint typeMask) const
{
    ShaMap::const_iterator it(constFind(sha));
    return (it != constEnd() ? (*it).type & typeMask : 0);
}

uint ShaMap::checkRef(SCRef sha, uint typeMask) const
{
    return checkRef(QGit::toTempSha(sha), typeMask);
}

bool ShaMap::hasType(const ShaString& sha, Reference::Type type) const
{
    return checkRef(sha, type) != 0;
}

bool ShaMap::hasType(SCRef sha, Reference::Type type) const
{
    return checkRef(sha, type) != 0;
}

const QStringList ShaMap::getRefName(SCRef sha, Reference::Type type) const
{
    return getRefName(QGit::toTempSha(sha), type);
}

const QStringList ShaMap::getRefName(const ShaString& sha, Reference::Type type) const
{
    if (!this->contains(sha)) {
        return QStringList();
    }

    const Reference& rf = (*this)[sha];

    if (rf.type & type == 0) {
        return QStringList();
    }

    if (type == Reference::TAG)
        return rf.tags;

    else if (type == Reference::BRANCH)
        return rf.branches;

    else if (type == Reference::REMOTE_BRANCH)
        return rf.remoteBranches;

    else if (type == Reference::REF)
        return rf.refs;

    else if (type == Reference::APPLIED || type == Reference::UN_APPLIED)
        return QStringList(rf.stgitPatch);

    return QStringList();
}

const QStringList ShaMap::getAllSha(uint typeMask)
{
    QStringList shas;
    FOREACH (ShaMap, it, *this) {
        if ((*it).type & typeMask)
            shas.append(it.key());
    }
    return shas;
}

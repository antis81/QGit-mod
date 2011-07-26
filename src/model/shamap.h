#ifndef REFERENCEMAP_H
#define REFERENCEMAP_H

#include <QString>
#include <QStringList>
#include <QHash>
#include "shastring.h"
#include "common.h"

struct Reference  // stores tag information associated to a revision
{
    Reference() : type(0) {};
    uint type;
    QStringList branches;
    QStringList remoteBranches;
    QStringList tags;
    QStringList refs;
    QString     tagObj; // TODO support more then one obj
    QString     tagMsg;
    QString     stgitPatch;

    enum Type
    {
        TAG        = 1,
        BRANCH     = 2,
        REMOTE_BRANCH = 4,
        CUR_BRANCH = 8,
        REF        = 16,
        APPLIED    = 32,
        UN_APPLIED = 64,
        ANY_REF    = 127
    };
};

class ShaMap : public QHash<ShaString, Reference>
{
public:
    ShaMap();
    uint checkRef(const ShaString& sha, uint typeMask = Reference::ANY_REF) const;
    uint checkRef(SCRef sha, uint typeMask = Reference::ANY_REF) const;
    bool hasType(const ShaString& sha, Reference::Type type) const;
    bool hasType(SCRef sha, Reference::Type type) const;
    const QStringList getRefName(const ShaString& sha, Reference::Type type) const;
    const QStringList getRefName(SCRef sha, Reference::Type type) const;
    const QStringList getAllSha(uint typeMask);
};

#endif // REFERENCEMAP_H

#ifndef REFERENCEINFO_H
#define REFERENCEINFO_H

#include <QString>
#include "shastring.h"

class ReferenceInfo
{
private:
    Type m_type;

public:
    ReferenceInfo();

    enum Type
    {
        UNDEFINED  = 0,
        TAG        = 1,
        BRANCH     = 2,
        REMOTE_BRANCH = 4,
        CUR_BRANCH = 8,
        REF        = 16,
        APPLIED    = 32,
        UN_APPLIED = 64,
        ANY_REF    = 127
    };

    Type type();
};

class TagInfo : public ReferenceInfo
{
private:
    QString m_message;
    ShaString m_reference;

public:
    TagInfo();
    QString& message();
};

class StgitPatchInfo : public ReferenceInfo
{
private:
    QString m_patch;

public:
    StgitPatchInfo();
    QString& patch();
};


#endif // REFERENCEINFO_H

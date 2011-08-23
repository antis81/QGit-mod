#include "stgitpatchreference.h"

StgitPatchReference::StgitPatchReference() : Reference()
{

}

StgitPatchReference::StgitPatchReference(const ShaString& sha, const QString& name, uint type)
    : Reference(sha, name, type)
{

}

const QString& StgitPatchReference::patch()
{
    return m_patch;
}

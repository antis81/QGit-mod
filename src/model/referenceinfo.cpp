#include "referenceinfo.h"

ReferenceInfo::ReferenceInfo()
    : m_type(UNDEFINED)
{

}

Type ReferenceInfo::type()
{
    return m_type;
}


TagInfo::TagInfo() : ReferenceInfo(), m_type(TAG)
{

}

QString& TagInfo::message()
{
    return m_message;
}


StgitPatchInfo::StgitPatchInfo() : ReferenceInfo()
{

}

QString& StgitPatchInfo::patch()
{
    return m_patch;
}

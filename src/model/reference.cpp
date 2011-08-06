#include "reference.h"

Reference::Reference()
    : m_type(UNDEFINED), m_name(), m_sha()
{

}

Reference::Reference(const ShaString& sha, const QString& name, uint type)
    : m_type(type), m_name(name), m_sha(sha)
{

}

bool Reference::operator==(const Reference& ref) const
{
    return (m_type == ref.m_type) && (m_name == ref.m_name);
}

uint Reference::type() const
{
    return m_type;
}

void Reference::setType(uint type)
{
    m_type = type;
}

const QString& Reference::name() const
{
    return m_name;
}

const ShaString& Reference::sha() const
{
    return m_sha;
}


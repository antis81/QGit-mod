#include "tagreference.h"

TagReference::TagReference() : Reference(), m_message()
{

}

TagReference::TagReference(const ShaString& sha, const QString& name)
    : Reference(sha, name, TAG), m_message(), m_reference()
{

}

TagReference::TagReference(const ShaString& sha, const QString& name, const ShaString& reference)
    : Reference(sha, name, TAG), m_message(), m_reference(reference)
{
}

const QString& TagReference::message() const
{
    return m_message;
}

bool TagReference::isAnnotated() const
{
    return !m_reference.isNull();
}

const ShaString& TagReference::reference() const
{
    return m_reference;
}

void TagReference::setMessage(const QString& message)
{
    m_message = message;
}

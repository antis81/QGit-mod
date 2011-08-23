#ifndef TAGREFERENCE_H
#define TAGREFERENCE_H

#include "reference.h"

//! Model of tag reference
/*!
  Incapsulate data for reference of type TAG
  \sa ReferenceInfo::Type
*/
class TagReference : public Reference
{
private:
    QString m_message;
    ShaString m_reference;

public:
    //! A default constructor
    /*!
      Constructs null-tag
    */
    TagReference();

    //! A constructor for non-annotated tag
    /*!
      Constructs non-annotated tag by sha and name
    */
    TagReference(const ShaString& sha, const QString& name);

    //! A constructor for annotated tag
    /*!
      Constructs annotated tag by sha name and reference object.
      Message of tag can be set later.
     */
    TagReference(const ShaString& sha, const QString& name, const ShaString& reference);

    //! Message of tag
    /*!
      Message of annotated tag. It is null-string when tag is not annotated
    */
    const QString& message() const;

    /*!
      \return true when tag is annotated
    */
    bool isAnnotated() const;

    /*!
      \return reference object for annotated tag, null-sha otherwise
    */
    const ShaString& reference() const;

    /*!
      Sets message for annotated tag
    */
    void setMessage(const QString& message);
};

#endif // TAGREFERENCE_H

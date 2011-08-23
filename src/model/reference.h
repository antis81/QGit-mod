#ifndef REFERENCE_H
#define REFERENCE_H

#include <QString>
#include "shastring.h"

//! Model of reference
/*!
  Incapsulates data for abstract reference
*/
class Reference
{
public:

    //! Type of reference
    /*!
      Reference type can be combination of flags
    */
    enum Type
    {
        UNDEFINED     = 0x00, /*!< Undefined type of reference. Used as NULL-reference */
        TAG           = 0x01, /*!< Tag */
        BRANCH        = 0x02, /*!< Local branch */
        REMOTE_BRANCH = 0x04, /*!< Remote branch */
        CUR_BRANCH    = 0x08, /*!< Current branch. Used with BRANCH */
        REF           = 0x10, /*!< Other reference */
        APPLIED       = 0x20, /*!< Applied StGit patch */
        UN_APPLIED    = 0x40, /*!< Unapplied StGit patch */
        ANY_TYPE      = 0xFF  /*!< Any type of reference. Used for filtering */
    };

    //! A default constructor
    /*!
      Construct null-reference
    */
    Reference();

    //! A constructor
    /*!
      Construct reference by sha, name and type.
      \sa type(), name(), sha(), Type
    */
    Reference(const ShaString& sha, const QString& name, uint type);

    //! Type
    /*!
      \return type of reference. Can be combination of constans from \c Type
      \sa Type
    */
    uint type() const;

    //! Sets new type.
    void setType(uint type);

    //! Name of reference
    /*!
      \return Short name of reference that GIT understands. For example master, origin/master
    */
    const QString& name() const;

    //! SHA of reference
    /*!
      \return SHA of reference
    */
    const ShaString& sha() const;

    //! Equality operator
    /*!
      \returns true when name and type equals name and type of given object respectively.
      */
    bool operator==(const Reference& ref) const;

protected:
    uint m_type;
    QString m_name;
    ShaString m_sha;
};

#endif // REFERENCE_H

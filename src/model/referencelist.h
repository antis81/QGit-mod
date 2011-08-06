#ifndef REFERENCELIST_H
#define REFERENCELIST_H

#include "reference.h"
#include <QList>

//! List of references
/*!
  Implements some logic for searching, filtering and field requesting
*/
class ReferenceList : public QList<Reference*>
{
public:
    //! A default constructor
    /*!
      Constructs empty list
    */
    ReferenceList();

    //! Filter list by reference type
    /*!
      Filters list by reference type and return new filtered list
    */
    ReferenceList filter(uint typeMask = Reference::ANY_TYPE) const;

    /*!
      \return combination of types of all references that contains in this list
    */
    uint containsType(uint typeMask) const;

    /*!
      \return list of names for all references in this list
    */
    QStringList getNames() const;

    /*!
      \return list of SHA for all references in this list
    */
    QStringList getShas() const;
};

#endif // REFERENCELIST_H

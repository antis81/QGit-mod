#ifndef STGITPATCHREFERENCE_H
#define STGITPATCHREFERENCE_H

#include "reference.h"

//! Model for StGIT patch
/*!
  Incapsulate data and logic for patches that StGIT generates.
  This reference uses APPLIED and UN_APPLIED type
  \as ReferenceInfo::Type
*/
class StgitPatchReference : public Reference
{
protected:
    QString m_patch;

public:
    //! A default constructor
    /*!
      Constucts null-reference
    */
    StgitPatchReference();

    //! A constructor
    /*!
      Constructs reference for patch by sha, name and type.
      \param type can be APPLIED or UN_APPLIED
    */
    StgitPatchReference(const ShaString& sha, const QString& name, uint type);

    /*!
      \return content of patch
    */
    const QString& patch();
};


#endif // STGITPATCHREFERENCE_H

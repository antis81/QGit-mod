#ifndef REFERENCES_H
#define REFERENCES_H

#include <QString>
#include <QStringList>
#include <QHash>
#include <QByteArray>
#include <QVector>

#include "model/shastring.h"
#include "model/reference.h"
#include "model/tagreference.h"
#include "model/stgitpatchreference.h"
#include "model/referencelist.h"

#include "rungit_interface.h"
//! Container for repository references
/*!
  Implements container for repository references and their fast searching
*/
class References
{
private:
    RunGitInterface* m_git;

    typedef QHash<ShaString, ReferenceList> ShaToReferenceInfoList;
    ReferenceList m_refs;
    ShaToReferenceInfoList m_shaToRef;
    QVector<QByteArray> m_shaBackupBuf;


    void parseStGitPatches(const QStringList& patchNames, const QStringList& patchShas);

public:
    // FIXME: temporary. move to STGit support class
    int patchesStillToFind;


    //! A default constructor
    References(RunGitInterface* git);

    /*!
      \return true if any reference with given sha exists.
    */
    bool containsSha(const ShaString& sha) const;

    /*!
      \return combination of type of all references that has given SHA.
    */
    uint containsType(const ShaString& sha, uint typeMask = Reference::ANY_TYPE) const;

    /*!
      Filter list of references that has given sha and type.
    */
    ReferenceList filter(const ShaString& sha, uint typeMask = Reference::ANY_TYPE) const;

    /*!
      \return List of sha of all references with given type.
    */
    QStringList getShas(uint typeMask) const;

    /*!
      \return reference by it name. Type may be used for strong search. If reference not found returns \c NULL.
    */
    Reference* byName(const QString& refName, uint typeMask = Reference::ANY_TYPE) const;

    /*!
      \return list of names of all references with given type.
    */
    QStringList getNames(uint typeMask) const;

    /*!
      Adds reference to this list.
    */
    void add(Reference* ref);

    /*!
      Removes reference from this list.
      This function free memory of given object.
    */
    void remove(Reference* ref);


    /*!
      Remove all references from this list.
      This function free memory for all objects.
    */
    void clear();

    /*!
      \return true if this list contains no references.
    */
    bool isEmpty() const;

    /*!
      Get message of tag from object. If the message has not been loaded, the message will be loaded and will be stored in object.
      \return Message of tag
    */
    const QString getTagMessage(TagReference* tagRef);


    /*!
      \return true if the given reference is StGit patch.
    */
    bool isPatchName(const QString& refName);

    /*!
      \return SHA of reference by it name and type. If reference is not known, it asks Git.
    */
    const ShaString getRefSha(const QString& refName, uint type = Reference::ANY_TYPE, bool askGit = true);

    /*!
      \return SHA of reference given from string
    */
    const ShaString toSha(const QString& shaAsString);

    // FIXME: temporary parameter
    bool load(const QString& stgCurBranch);
};

#endif // REFERENCES_H

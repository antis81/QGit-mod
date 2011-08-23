#include "references.h"
#include "common.h"

References::References(RunGitInterface* git) : m_git(git), patchesStillToFind(0)
{

}

bool References::containsSha(const ShaString& sha) const
{
    return m_shaToRef.contains(sha);
}

ReferenceList References::filter(const ShaString& sha, uint typeMask) const
{
    ShaToReferenceInfoList::const_iterator it(m_shaToRef.constFind(sha));
    if (it == m_shaToRef.constEnd()) {
        return ReferenceList();
    }

    if (typeMask == Reference::ANY_TYPE) {
        return ReferenceList(it.value());
    }

    ReferenceList list = it.value().filter(typeMask);
    return list;
}

uint References::containsType(const ShaString& sha, uint typeMask) const
{
    ShaToReferenceInfoList::const_iterator it(m_shaToRef.constFind(sha));
    if (it == m_shaToRef.constEnd()) return 0;

    const ReferenceList& list = it.value();
    return list.containsType(typeMask);
}

Reference* References::byName(const QString& refName, uint typeMask) const
{
    FOREACH(ReferenceList, it, m_refs) {
        Reference* ref = *it;
        if (!(ref->type() & typeMask)) continue;
        if (ref->name().compare(refName) == 0) {
            return ref;
        }
    }
    return NULL;
}

QStringList References::getNames(uint typeMask) const
{
    QStringList list;

    FOREACH(ReferenceList, it, m_refs) {
        const Reference* ref = *it;
        if (ref->type() & typeMask) {
            list.append(ref->name());
        }
    }

    return list;
}

QStringList References::getShas(uint typeMask) const
{
    QStringList list;

    FOREACH(ReferenceList, it, m_refs) {
        const Reference* ref = *it;
        if (ref->type() & typeMask) {
            list.append(ref->sha()); // FIXME: some branches may have the same SHA and such SHA will be duplicated
        }
    }

    return list;
}

void References::clear()
{
    FOREACH(ReferenceList, it, m_refs) {
        Reference* ref = *it;
        delete ref;
    }

    m_refs.clear();
    m_shaToRef.clear();
    m_shaBackupBuf.clear();

    patchesStillToFind = 0;
}

void References::add(Reference* ref)
{
    const ShaString& sha = ref->sha();
    m_refs.append(ref);
    ShaToReferenceInfoList::iterator it = m_shaToRef.find(sha);
    if (it != m_shaToRef.end()) {
        ReferenceList& list = it.value();
        list.append(ref);
    } else {
        ReferenceList list;
        list.append(ref);
        m_shaToRef.insert(sha, list);
    }
}

void References::remove(Reference* ref)
{
    m_refs.removeOne(ref);
    const ShaString& sha = ref->sha();

    ShaToReferenceInfoList::iterator it = m_shaToRef.find(sha);
    if (it != m_shaToRef.end()) {
        ReferenceList& list = it.value();
        list.removeOne(ref);
    }
    delete ref;
}

bool References::isEmpty() const
{
    return m_refs.isEmpty();
}

const QString References::getTagMessage(TagReference* tagRef)
{
    if (!tagRef->isAnnotated()) {
        return "";
    }

    if (!tagRef->message().isNull()) {
        return tagRef->message();
    }

    QRegExp pgp("-----BEGIN PGP SIGNATURE*END PGP SIGNATURE-----",
                Qt::CaseSensitive, QRegExp::Wildcard);

    QString ro;
    if (m_git->runGit("git cat-file tag " + tagRef->reference(), &ro)) {
        QString message = ro.section("\n\n", 1).remove(pgp).trimmed();
        tagRef->setMessage(message);
    }
    return tagRef->message();
}

bool References::isPatchName(const QString& refName)
{
    return !!byName(refName, Reference::UN_APPLIED | Reference::APPLIED);
}

const ShaString References::getRefSha(const QString& refName, uint type, bool askGit)
{
    Reference* ref = byName(refName, type);
    if (ref) return ref->sha();

    if (!askGit) return ShaString();

    // if a ref was not found perhaps is an abbreviated form
    QString runOutput;
    bool ok = m_git->runGit("git rev-parse --revs-only " + refName, &runOutput);

    if (!ok) {
        return ShaString();
    }

    // TODO: cache results
    ShaString sha = toSha(runOutput.trimmed());
    return sha;
}

const ShaString References::toSha(const QString& shaAsString)
{
    m_shaBackupBuf.append(shaAsString.toLatin1());
    return ShaString(m_shaBackupBuf.last().constData());
}

bool References::load(const QString& stgCurBranch)
{
    // read refs, normally unsorted
    QString runOutput;
    if (!m_git->runGit("git show-ref -d", &runOutput))
        return false;

    clear();

    ShaString prevRefSha;
    QStringList patchNames, patchShas;
    const QStringList rLst(runOutput.split('\n', QString::SkipEmptyParts));
    FOREACH_SL (it, rLst) {

        const ShaString revSha = toSha((*it).left(40));
        const QString refName = (*it).mid(41);

        if (refName.startsWith("refs/patches/")) {

            // save StGIT patch sha, to be used later
            SCRef patchesDir("refs/patches/" + stgCurBranch + "/");
            if (refName.startsWith(patchesDir)) {
                patchNames.append(refName.mid(patchesDir.length()));
                patchShas.append(revSha);
            }
            // StGIT patches should not be added to refs,
            // but an applied StGIT patch could be also an head or
            // a tag in this case will be added in another loop cycle
            continue;
        }
        // one rev could have many tags

        if (refName.startsWith("refs/tags/")) {
            if (refName.endsWith("^{}")) { // tag dereference

                // we assume that a tag dereference follows strictly
                // the corresponding tag object in rLst. So the
                // last added tag is a tag object, not a commit object
                QString tagName = refName.mid(10, refName.length() - 13);

                Reference* prevTag = byName(tagName, Reference::TAG);
                TagReference* tagRef = new TagReference(revSha, tagName, prevRefSha);
                remove(prevTag);
                add(tagRef);
                // tagObj must be removed from ref map

            } else {
                TagReference* tagRef = new TagReference(revSha, refName.mid(10));
                add(tagRef);
            }
        } else if (refName.startsWith("refs/heads/")) {
            Reference* ref = new Reference(revSha, refName.mid(11), Reference::BRANCH);
            add(ref);

        } else if (refName.startsWith("refs/remotes/") && !refName.endsWith("HEAD")) {
            Reference* ref = new Reference(revSha, refName.mid(13), Reference::REMOTE_BRANCH);
            add(ref);

        } else if (!refName.startsWith("refs/bases/") && !refName.endsWith("HEAD")) {
            Reference* ref = new Reference(revSha, refName, Reference::REF);
            add(ref);
        }
        prevRefSha = revSha;
    }
    if (m_git->isStGITStack() && !patchNames.isEmpty())
        parseStGitPatches(patchNames, patchShas);

    return !isEmpty();
}

void References::parseStGitPatches(const QStringList& patchNames, const QStringList& patchShas)
{
    patchesStillToFind = 0;

    // get patch names and status of current branch
    QString runOutput;
    if (!m_git->runGit("stg series", &runOutput))
        return;

    const QStringList pl(runOutput.split('\n', QString::SkipEmptyParts));
    FOREACH_SL (it, pl) {

        const QString status = (*it).left(1);
        const QString patchName = (*it).mid(2);

        bool applied = (status == "+" || status == ">");
        int pos = patchNames.indexOf(patchName);
        if (pos < 0) {
            dbp("ASSERT in Git::parseStGitPatches(), patch %1 "
                "not found in references list.", patchName);
            continue;
        }
        const ShaString& sha = toSha(patchShas.at(pos));
        StgitPatchReference* ref = new StgitPatchReference(sha, patchName, (applied ? Reference::APPLIED : Reference::UN_APPLIED));
        add(ref);

        if (applied)
            patchesStillToFind++;
    }
}

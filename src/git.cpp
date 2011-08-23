/*
    Description: interface to git programs

    Author: Marco Costalba (C) 2005-2007

    Copyright: See COPYING file that comes with this distribution

*/
#include <QApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFontMetrics>
#include <QImageReader>
#include <QRegExp>
#include <QSet>
#include <QSettings>
#include <QTextCodec>
#include <QTextDocument>
#include <QTextStream>
#include "annotate.h"
#include "cache.h"
#include "git.h"
#include "lanes.h"
#include "myprocess.h"

#include <QPair>
#include <QSettings>
#include "exceptionmanager.h"
#include "ui/rangeselectimpl.h"
#include "lanes.h"
#include "mainimpl.h"
#include "dataloader.h"
#include "model/reference.h"
#include "model/stgitpatchreference.h"
#include "model/tagreference.h"

using namespace QGit;

// ****************************************************************************

bool Git::TreeEntry::operator<(const TreeEntry& te) const
{
    if (this->type == te.type)
        return (this->name < te.name);

    // directories are smaller then files
    // to appear as first when sorted
    if (this->type == "tree")
        return true;

    if (te.type == "tree")
        return false;

    return (this->name < te.name);
}

Git::Git(QObject* p) : QObject(p), m_references(this)
{
    EM_INIT(exGitStopped, "Stopping connection with git");

    fileCacheAccessed = cacheNeedsUpdate = isMergeHead = false;
    isStGIT = isGIT = loadingUnAppliedPatches = isTextHighlighterFound = false;
    errorReportingEnabled = true; // report errors if run() fails
    curDomain = NULL;
    revData = NULL;
    revsFiles.reserve(MAX_DICT_SIZE);
}

Git::~Git()
{
}

void Git::checkEnvironment()
{
    QString version;
    if (run("git --version", &version)) {

        version = version.section(' ', -1, -1).section('.', 0, 2);
        if (version < GIT_VERSION) {

            // simply send information, the 'not compatible version'
            // policy should be implemented upstream
            const QString cmd("Current git version is " + version +
                  " but is required " + GIT_VERSION + " or better");

            const QString errorDesc("Your installed git is too old."
                  "\nPlease upgrade to avoid possible misbehaviours.");

            MainExecErrorEvent* e = new MainExecErrorEvent(cmd, errorDesc);
            QApplication::postEvent(parent(), e);
        }
    } else {
        dbs("Cannot find git files");
        return;
    }
    errorReportingEnabled = false;
    isTextHighlighterFound = run("source-highlight -V", &version);
    errorReportingEnabled = true;
    if (isTextHighlighterFound)
        dbp("Found %1", version.section('\n', 0, 0));
}

void Git::userInfo(SList info)
{
/*
  git looks for commit user information in following order:

    - GIT_AUTHOR_NAME and GIT_AUTHOR_EMAIL environment variables
    - repository config file
    - global config file
    - your name, hostname and domain
*/
    const QString env(QProcess::systemEnvironment().join(","));
    QString user(env.section("GIT_AUTHOR_NAME", 1).section(",", 0, 0).section("=", 1).trimmed());
    QString email(env.section("GIT_AUTHOR_EMAIL", 1).section(",", 0, 0).section("=", 1).trimmed());

    info.clear();
    info << "Environment" << user << email;

    errorReportingEnabled = false; // 'git config' could fail, see docs

    run("git config user.name", &user);
    run("git config user.email", &email);
    info << "Local config" << user << email;

    run("git config --global user.name", &user);
    run("git config --global user.email", &email);
    info << "Global config" << user << email;

    errorReportingEnabled = true;
}

const QStringList Git::getGitConfigList(bool global)
{
    QString runOutput;

    errorReportingEnabled = false; // 'git config' could fail, see docs

    if (global)
        run("git config --global --list", &runOutput);
    else
        run("git config --list", &runOutput);

    errorReportingEnabled = true;

    return runOutput.split('\n', QString::SkipEmptyParts);
}

bool Git::isImageFile(SCRef file)
{
    const QString ext(file.section('.', -1).toLower());
    return QImageReader::supportedImageFormats().contains(ext.toAscii());
}

// TODO: this may work not correctly for others extensions. May be use internal GIT algorithm?
bool Git::isBinaryFile(SCRef file)
{
    static const char* binaryFileExtensions[] = {"bmp", "gif", "jpeg", "jpg",
                       "png", "svg", "tiff", "pcx", "xcf", "xpm",
                       "bz", "bz2", "rar", "tar", "z", "gz", "tgz", "zip", 0};

    if (isImageFile(file))
        return true;

    const QString ext(file.section('.', -1).toLower());
    int i = 0;
    while (binaryFileExtensions[i] != 0)
        if (ext == binaryFileExtensions[i++])
            return true;
    return false;
}

void Git::setThrowOnStop(bool b)
{
    if (b)
        EM_REGISTER(exGitStopped);
    else
        EM_REMOVE(exGitStopped);
}

bool Git::isThrowOnStopRaised(int excpId, SCRef curContext)
{
    return EM_MATCH(excpId, exGitStopped, curContext);
}

void Git::setTextCodec(QTextCodec* tc)
{
    QTextCodec::setCodecForCStrings(tc); // works also with tc == 0 (Latin1)
    QTextCodec::setCodecForLocale(tc);
    QString name(tc ? tc->name() : "Latin1");

    // workaround Qt issue of mime name different from
    // standard http://www.iana.org/assignments/character-sets
    if (name == "Big5-HKSCS")
        name = "Big5";

   run("git config i18n.commitencoding " + name);
}

QTextCodec* Git::getTextCodec(bool* isGitArchive)
{
    *isGitArchive = isGIT;
    if (!isGIT) // can be called also when not in an archive
        return NULL;

    QString runOutput;
    if (!run("git config i18n.commitencoding", &runOutput))
        return NULL;

    if (runOutput.isEmpty()) // git docs says default is utf-8
        return QTextCodec::codecForName(QByteArray("utf8"));

    return QTextCodec::codecForName(runOutput.trimmed().toLatin1());
}

// TODO: make as static
const QString Git::escape(QString s) {
    return s.replace(QRegExp("([\\\\\\ \\\"\\\'])"), "\\\\1");
}

// TODO: make as static
const QString Git::unescape(QString s) {
    return s.replace(QRegExp("(\\\\([\\ \\\\\\\'\\\"]))"), "\\2");
}

// TODO: make as static
const QString Git::quote(SCRef nm) {

    return (QUOTE_CHAR + nm + QUOTE_CHAR);
}

// TODO: make as static
const QString Git::quote(SCList sl)
{
    QString q(sl.join(QUOTE_CHAR + ' ' + QUOTE_CHAR));
    q.prepend(QUOTE_CHAR).append(QUOTE_CHAR);
    return q;
}



void Git::appendNamesWithId(QStringList& names, SCRef sha, SCList data, bool onlyLoaded)
{
    const Revision* r = revLookup(sha);
    if (onlyLoaded && !r)
        return;

    if (onlyLoaded) { // prepare for later sorting
        SCRef cap = QString("%1 ").arg(r->orderIdx, 6);
        FOREACH_SL (it, data)
            names.append(cap + *it);
    } else
        names += data;
}

const QStringList Git::getAllRefNames(uint mask, bool onlyLoaded)
{
// returns reference names sorted by loading order if 'onlyLoaded' is set


    QStringList names(m_references.getNames(mask));
//    FOREACH (ShaMap, it, shaMap) {
//
//        if (mask & Reference::TAG)
//            appendNamesWithId(names, it.key(), (*it).tags, onlyLoaded);
//
//        if (mask & Reference::BRANCH)
//            appendNamesWithId(names, it.key(), (*it).branches, onlyLoaded);
//
//        if (mask & Reference::REMOTE_BRANCH)
//            appendNamesWithId(names, it.key(), (*it).remoteBranches, onlyLoaded);
//
//        if (mask & Reference::REF)
//            appendNamesWithId(names, it.key(), (*it).refs, onlyLoaded);
//
//        if ((mask & (Reference::APPLIED | Reference::UN_APPLIED)) && !onlyLoaded)
//            names.append((*it).stgitPatch); // doesn't work with 'onlyLoaded'
//    }

    if (onlyLoaded) {
        names.sort();
        QStringList::iterator itN(names.begin());
        for ( ; itN != names.end(); ++itN) // strip 'idx'
            (*itN) = (*itN).section(' ', -1, -1);
    }

    return names;
}

const QString Git::getRevInfo(const ShaString &sha)
{
    if (sha.isNull() || !m_references.containsSha(sha)) {
        return "";
    }

    QString refsInfo;
    QStringList list;

    ReferenceList curBranch = m_references.filter(sha, Reference::CUR_BRANCH);
    ReferenceList branchList = m_references.filter(sha, Reference::BRANCH);
    if (!curBranch.isEmpty()) {
        branchList.removeOne(curBranch.first());
        refsInfo.append("  HEAD: " + curBranch.first()->name());
    }
    if (!branchList.isEmpty()) {
        refsInfo.append("  Branch: " + branchList.getNames().join(" "));
    }

    list = m_references.filter(sha, Reference::REMOTE_BRANCH).getNames();
    if (!list.isEmpty()) {
        refsInfo.append("  Remote branch: " + list.join(" "));
    }

    ReferenceList tagList = m_references.filter(sha, Reference::TAG);
    list = tagList.getNames();
    if (!list.isEmpty()) {
        refsInfo.append("  Tag: " + list.join(" "));
    }

    list = m_references.filter(sha, Reference::REF).getNames();
    if (!list.isEmpty()) {
        refsInfo.append("  Ref: " + list.join(" "));
    }

    list = m_references.filter(sha, Reference::APPLIED | Reference::UN_APPLIED).getNames();
    if (!list.isEmpty()) {
        refsInfo.append("  Patch: " + list.join(" "));
    }

    if (!tagList.isEmpty()) {
        FOREACH(ReferenceList, it, tagList) {
            Reference *ref = *it;
            TagReference *tagRef = static_cast<TagReference *>(ref);
            if (tagRef->isAnnotated()) {
                const QString &message = m_references.getTagMessage(tagRef);
                refsInfo.append("  ").append(tagRef->name()).append(" [").append(message).append("]");
            }
        }
    }
    return refsInfo.trimmed();
}



void Git::addExtraFileInfo(QString* rowName, SCRef sha, SCRef diffToSha, bool allMergeFiles)
{
    const RevFile* files = getFiles(sha, diffToSha, allMergeFiles);
    if (!files)
        return;

    int idx = findFileIndex(*files, *rowName);
    if (idx == -1)
        return;

    QString extSt(files->extendedStatus(idx));
    if (extSt.isEmpty())
        return;

    *rowName = extSt;
}

void Git::removeExtraFileInfo(QString* rowName)
{
    if (rowName->contains(" --> ")) // return destination file name
        *rowName = rowName->section(" --> ", 1, 1).section(" (", 0, 0);
}

void Git::formatPatchFileHeader(QString* rowName, SCRef sha, SCRef diffToSha,
                                bool combined, bool allMergeFiles)
{
    if (combined) {
        rowName->prepend("diff --combined ");
        return; // TODO rename/copy still not supported in this case
    }
    // let's see if it's a rename/copy...
    addExtraFileInfo(rowName, sha, diffToSha, allMergeFiles);

    if (rowName->contains(" --> ")) { // ...it is!

        SCRef destFile(rowName->section(" --> ", 1, 1).section(" (", 0, 0));
        SCRef origFile(rowName->section(" --> ", 0, 0));
        *rowName = "diff --git a/" + origFile + " b/" + destFile;
    } else
        *rowName = "diff --git a/" + *rowName + " b/" + *rowName;
}

Annotate* Git::startAnnotate(FileHistory* fh, QObject* guiObj) // non blocking
{
    Annotate* ann = new Annotate(this, guiObj);
    if (!ann->start(fh)) // non blocking call
        return NULL; // ann will delete itself when done

    return ann; // caller will delete with Git::cancelAnnotate()
}

void Git::cancelAnnotate(Annotate* ann)
{
    if (ann)
        ann->deleteWhenDone();
}

const FileAnnotation* Git::lookupAnnotation(Annotate* ann, SCRef sha)
{
    return (ann ? ann->lookupAnnotation(sha) : NULL);
}

void Git::cancelDataLoading(const FileHistory* fh)
{
// normally called when closing file viewer

    emit cancelLoading(fh); // non blocking
}

const Revision* Git::revLookup(SCRef sha, const FileHistory* fh) const
{
    return revLookup(toTempSha(sha), fh);
}

const Revision* Git::revLookup(const ShaString& sha, const FileHistory* fh) const
{
    const RevMap& r = (fh ? fh->revs : revData->revs);
    return (sha.latin1() ? r.value(sha) : NULL);
}

bool Git::run(SCRef runCmd, QString* runOutput, QObject* receiver, SCRef buf)
{
    QByteArray ba;
    bool ret = run(runOutput ? &ba : NULL, runCmd, receiver, buf);
    if (runOutput)
        *runOutput = ba;

    return ret;
}

bool Git::run(QByteArray* runOutput, SCRef runCmd, QObject* receiver, SCRef buf)
{
    MyProcess p(parent(), this, workDir, errorReportingEnabled);
    return p.runSync(runCmd, runOutput, receiver, buf);
}

MyProcess* Git::runAsync(SCRef runCmd, QObject* receiver, SCRef buf)
{
    MyProcess* p = new MyProcess(parent(), this, workDir, errorReportingEnabled);
    if (!p->runAsync(runCmd, receiver, buf)) {
        delete p;
        p = NULL;
    }
    return p; // auto-deleted when done
}

MyProcess* Git::runAsScript(SCRef runCmd, QObject* receiver, SCRef buf)
{
    const QString scriptFile(workDir + "/qgit_script" + QGit::SCRIPT_EXT);
#ifndef Q_OS_WIN32
    // without this process doesn't start under Linux
    QString cmd(runCmd.startsWith("#!") ? runCmd : "#!/bin/sh\n" + runCmd);
#else
    QString cmd(runCmd);
#endif
    if (!writeToFile(scriptFile, cmd, true))
        return NULL;

    MyProcess* p = runAsync(scriptFile, receiver, buf);
    if (p)
        connect(p, SIGNAL(eof()), this, SLOT(on_runAsScript_eof()));
    return p;
}

void Git::on_runAsScript_eof()
{
    QDir dir(workDir);
    dir.remove("qgit_script" + QGit::SCRIPT_EXT);
}

void Git::cancelProcess(MyProcess* p)
{
    if (p)
        p->on_cancel(); // non blocking call
}

int Git::findFileIndex(const RevFile& rf, SCRef name)
{
    if (name.isEmpty())
        return -1;

    int idx = name.lastIndexOf('/') + 1;
    SCRef dr = name.left(idx);
    SCRef nm = name.mid(idx);

    for (uint i = 0, cnt = rf.count(); i < cnt; ++i) {
        if (fileNamesVec[rf.nameAt(i)] == nm && dirNamesVec[rf.dirAt(i)] == dr)
            return i;
    }
    return -1;
}

const QString Git::getLaneParent(SCRef fromSHA, int laneNum)
{
    const Revision* rs = revLookup(fromSHA);
    if (!rs)
        return "";

    for (int idx = rs->orderIdx - 1; idx >= 0; idx--) {

        const Revision* r = revLookup(revData->revOrder[idx]);
        if (laneNum >= r->lanes.count())
            return "";

        if (!isFreeLane(r->lanes[laneNum])) {

            LaneType type = r->lanes[laneNum];
            int parNum = 0;
            while (!isMerge(type) && type != LANE_ACTIVE) {

                if (isHead(type))
                    parNum++;

                type = r->lanes[--laneNum];
            }
            return r->parent(parNum);
        }
    }
    return "";
}

const QStringList Git::getChilds(SCRef parent)
{
    QStringList childs;
    const Revision* r = revLookup(parent);
    if (!r)
        return childs;

    for (int i = 0; i < r->childs.count(); i++)
        childs.append(revData->revOrder[r->childs[i]]);

    // reorder childs by loading order
    QStringList::iterator itC(childs.begin());
    for ( ; itC != childs.end(); ++itC) {
        const Revision* r = revLookup(*itC);
        (*itC).prepend(QString("%1 ").arg(r->orderIdx, 6));
    }
    childs.sort();
    for (itC = childs.begin(); itC != childs.end(); ++itC)
        (*itC) = (*itC).section(' ', -1, -1);

    return childs;
}

const QString Git::getShortLog(SCRef sha)
{
    const Revision* r = revLookup(sha);
    return (r ? r->shortLog() : "");
}

MyProcess* Git::getDiff(SCRef sha, QObject* receiver, SCRef diffToSha, bool combined)
{
    if (sha.isEmpty())
        return NULL;

    QString runCmd;
    if (sha != ZERO_SHA) {
        runCmd = "git diff-tree --no-color -r --patch-with-stat ";
        runCmd.append(combined ? "-c " : "-C -m "); // TODO rename for combined
        runCmd.append(diffToSha + " " + sha); // diffToSha could be empty
    } else {
        runCmd = "git diff-index --no-color -r -m --patch-with-stat HEAD";
    }

    return runAsync(runCmd, receiver);
}

const QString Git::getWorkDirDiff(SCRef fileName)
{
    QString runCmd("git diff-index --no-color -r -z -m -p --full-index --no-commit-id HEAD");
    QString runOutput;
    if (!fileName.isEmpty())
        runCmd.append(" -- " + quote(fileName));

    if (!run(runCmd, &runOutput))
        return "";

    /* For unknown reasons file sha of index is not ZERO_SHA but
       a value of unknown origin.
       Replace that with ZERO_SHA so to not fool annotate
    */
    int idx = runOutput.indexOf("..");
    if (idx != -1)
        runOutput.replace(idx + 2, 40, ZERO_SHA);

    return runOutput;
}

const QString Git::getFileSha(SCRef file, SCRef revSha)
{
    if (revSha == ZERO_SHA) {
        QStringList files, dummy;
        getWorkDirFiles(files, dummy, RevFile::ANY);
        if (files.contains(file))
            return ZERO_SHA; // it is unknown to git
    }
    const QString sha(revSha == ZERO_SHA ? "HEAD" : revSha);
    QString runCmd("git ls-tree -r " + sha + " " + quote(file)), runOutput;
    if (!run(runCmd, &runOutput))
        return "";

    return runOutput.mid(12, 40); // could be empty, deleted file case
}

MyProcess* Git::getFile(SCRef fileSha, QObject* receiver, QByteArray* result, SCRef fileName)
{
    QString runCmd;
    /*
      symlinks in git are one line files with just the name of the target,
      not the target content. Instead 'cat' command resolves symlinks and
      returns target content. So we use 'cat' only if the file is modified
      in working dir, to let annotation work for changed files, otherwise
      we go with a safe 'git cat-file blob HEAD' instead.
      NOTE: This fails if the modified file is a new symlink, converted
      from an old plain file. In this case annotation will fail until
      change is committed.
    */
    if (fileSha == ZERO_SHA) {

#ifdef Q_OS_WIN32
        QString winPath = quote(fileName);
        winPath.replace("/", "\\");
        runCmd = "type " + winPath;
#else
        runCmd = "cat " + quote(fileName);
#endif

    } else {
        if (fileSha.isEmpty()) // deleted
            runCmd = "git diff-tree HEAD HEAD"; // fake an empty file reading
        else
            runCmd = "git cat-file blob " + fileSha;
    }
    if (!receiver) {
        run(result, runCmd);
        return NULL; // in case of sync call we ignore run() return value
    }
    return runAsync(runCmd, receiver);
}

MyProcess* Git::getHighlightedFile(SCRef fileSha, QObject* receiver, QString* result,
                                   SCRef fileName)
{
    if (!isTextHighlighter()) {
        dbs("ASSERT in getHighlightedFile: highlighter not found");
        return NULL;
    }
    QString ext(fileName.section('.', -1, -1, QString::SectionIncludeLeadingSep));
    QString inputFile(workDir + "/qgit_hlght_input" + ext);
    if (!saveFile(fileSha, fileName, inputFile))
        return NULL;

    QString runCmd("source-highlight --failsafe -f html -i " + quote(inputFile));

    if (!receiver) {
        run(runCmd, result);
        on_getHighlightedFile_eof();
        return NULL; // in case of sync call we ignore run() return value
    }
    MyProcess* p = runAsync(runCmd, receiver);
    if (p)
        connect(p, SIGNAL(eof()), this, SLOT(on_getHighlightedFile_eof()));
    return p;
}

void Git::on_getHighlightedFile_eof()
{
    QDir dir(workDir);
    const QStringList sl(dir.entryList(QStringList() << "qgit_hlght_input*"));
    FOREACH_SL (it, sl)
        dir.remove(*it);
}

bool Git::saveFile(SCRef fileSha, SCRef fileName, SCRef path)
{
    QByteArray fileData;
    getFile(fileSha, NULL, &fileData, fileName); // sync call
    if (isBinaryFile(fileName))
        return writeToFile(path, fileData);

    return writeToFile(path, QString(fileData));
}

bool Git::getTree(SCRef treeSha, TreeInfo& ti, bool isWorkingDir, SCRef path)
{
    QStringList deleted;
    if (isWorkingDir) {

        // retrieve unknown and deleted files under path
        QStringList unknowns, dummy;
        getWorkDirFiles(unknowns, dummy, RevFile::UNKNOWN);

        FOREACH_SL (it, unknowns) {

            // don't add files under other directories
            QFileInfo f(*it);
            SCRef d(f.dir().path());

            if (d == path || (path.isEmpty() && d == ".")) {
                TreeEntry te(f.fileName(), "", "?");
                ti.append(te);
            }
        }
        getWorkDirFiles(deleted, dummy, RevFile::DELETED);
    }
    // if needed fake a working directory tree starting from HEAD tree
    QString runOutput, tree(treeSha);
    if (treeSha == ZERO_SHA) {
        // HEAD could be empty for just init'ed repositories
        if (!run("git rev-parse --revs-only HEAD", &tree))
            return false;

        tree = tree.trimmed();
    }
    if (!tree.isEmpty() && !run("git ls-tree " + tree, &runOutput))
        return false;

    const QStringList sl(runOutput.split('\n', QString::SkipEmptyParts));
    FOREACH_SL (it, sl) {

        // append any not deleted file
        SCRef fn((*it).section('\t', 1, 1));
        SCRef fp(path.isEmpty() ? fn : path + '/' + fn);

        if (deleted.empty() || (deleted.indexOf(fp) == -1)) {
            TreeEntry te(fn, (*it).mid(12, 40), (*it).mid(7, 4));
            ti.append(te);
        }
    }
    qSort(ti); // list directories before files
    return true;
}

void Git::getWorkDirFiles(SList files, SList dirs, RevFile::StatusFlag status)
{
    files.clear();
    dirs.clear();
    const RevFile* f = getFiles(ZERO_SHA);
    if (!f)
        return;

    for (int i = 0; i < f->count(); i++) {

        if (f->statusCmp(i, status)) {

            SCRef fp(filePath(*f, i));
            files.append(fp);
            for (int j = 0, cnt = fp.count('/'); j < cnt; j++) {

                SCRef dir(fp.section('/', 0, j));
                if (dirs.indexOf(dir) == -1)
                    dirs.append(dir);
            }
        }
    }
}

bool Git::isNothingToCommit()
{
    if (!revsFiles.contains(ZERO_SHA_RAW))
        return true;

    const RevFile* rf = revsFiles[ZERO_SHA_RAW];
    return (rf->count() == workingDirInfo.otherFiles.count());
}

bool Git::isTreeModified(SCRef sha)
{
    const RevFile* f = getFiles(sha);
    if (!f)
        return true; // no files info, stay on the safe side

    for (int i = 0; i < f->count(); ++i)
        if (!f->statusCmp(i, RevFile::MODIFIED))
            return true;

    return false;
}

bool Git::isParentOf(SCRef par, SCRef child)
{
    const Revision* c = revLookup(child);
    return (c && c->parentsCount() == 1 && QString(c->parent(0)) == par); // no merges
}

bool Git::isSameFiles(SCRef tree1Sha, SCRef tree2Sha)
{
    // early skip common case of browsing with up and down arrows, i.e.
    // going from parent(child) to child(parent). In this case we can
    // check RevFileMap and skip a costly 'git diff-tree' call.
    if (isParentOf(tree1Sha, tree2Sha))
        return !isTreeModified(tree2Sha);

    if (isParentOf(tree2Sha, tree1Sha))
        return !isTreeModified(tree1Sha);

    const QString runCmd("git diff-tree --no-color -r " + tree1Sha + " " + tree2Sha);
    QString runOutput;
    if (!run(runCmd, &runOutput))
        return false;

    bool isChanged = (runOutput.indexOf(" A\t") != -1 || runOutput.indexOf(" D\t") != -1);
    return !isChanged;
}

const QStringList Git::getDescendantBranches(SCRef sha, bool shaOnly)
{
    QStringList tl;
    const Revision* r = revLookup(sha);
    if (!r || (r->descBrnMaster == -1))
        return tl;

    const QVector<int>& nr = revLookup(revData->revOrder[r->descBrnMaster])->descBranches;

    for (int i = 0; i < nr.count(); i++) {

        const ShaString& sha = revData->revOrder[nr[i]];
        if (shaOnly) {
            tl.append(sha);
            continue;
        }
        SCRef cap = " (" + sha + ") ";
        QString joinedBranches = m_references.filter(sha, Reference::BRANCH).getNames().join(" ");
        if (!joinedBranches.isEmpty())
            tl.append(joinedBranches.append(cap));

        QString joinedRemoteBranches = m_references.filter(sha, Reference::REMOTE_BRANCH).getNames().join(" ");
        if (!joinedRemoteBranches.isEmpty())
            tl.append(joinedRemoteBranches.append(cap));
    }
    return tl;
}

const QStringList Git::getNearTags(bool goDown, SCRef sha)
{
    QStringList tl;
    const Revision* r = revLookup(sha);
    if (!r)
        return tl;

    int nearRefsMaster = (goDown ? r->descRefsMaster : r->ancRefsMaster);
    if (nearRefsMaster == -1)
        return tl;

    const QVector<int>& nr = goDown ? revLookup(revData->revOrder[nearRefsMaster])->descRefs :
                                      revLookup(revData->revOrder[nearRefsMaster])->ancRefs;

    for (int i = 0; i < nr.count(); i++) {

        const ShaString& sha = revData->revOrder[nr[i]];
        SCRef cap = " (" + sha + ")";

        QString text = m_references.filter(sha, Reference::TAG).getNames().join(cap);
        if (!text.isEmpty())
            tl.append(text.append(cap));
    }
    return tl;
}

const QString Git::getLastCommitMsg()
{
    // FIXME: Make sure the amend action is not called when there is
    // nothing to amend. That is in empty repository or over StGit stack
    // with nothing applied.
    QString sha;
    QString top;
    if (run("git rev-parse --verify HEAD", &top))
        sha = top.trimmed();
    else {
        dbs("ASSERT: getLastCommitMsg head is not valid");
        return "";
    }

    const Revision* c = revLookup(sha);
    if (!c) {
        dbp("ASSERT: getLastCommitMsg sha <%1> not found", sha);
        return "";
    }

    return c->shortLog() + "\n\n" + c->longLog().trimmed();
}

const QString Git::getNewCommitMsg()
{
    const Revision* c = revLookup(ZERO_SHA);
    if (!c) {
        dbs("ASSERT: getNewCommitMsg zero_sha not found");
        return "";
    }

    QString status = c->longLog();
    status.prepend('\n').replace(QRegExp("\\n([^#])"), "\n#\\1"); // comment all the lines
    return status;
}

// TODO: make as static
const QString Git::colorMatch(SCRef txt, QRegExp& regExp)
{
    QString text;

    text = Qt::escape(txt);

    if (regExp.isEmpty())
        return text;

    SCRef startCol(QString::fromLatin1("<b><font color=\"red\">"));
    SCRef endCol(QString::fromLatin1("</font></b>"));
    int pos = 0;
    while ((pos = text.indexOf(regExp, pos)) != -1) {

        SCRef match(regExp.cap(0));
        const QString coloredText(startCol + match + endCol);
        text.replace(pos, match.length(), coloredText);
        pos += coloredText.length();
    }
    return text;
}

// TODO: make as static
const QString Git::formatList(SCList sl, SCRef name, bool inOneLine)
{
    if (sl.isEmpty())
        return QString();

    QString ls = "<tr><td class='h'>" + name + "</td><td>";
    const QString joinStr = inOneLine ? ", " : "</td></tr>\n" + ls;
    ls += sl.join(joinStr);
    ls += "</td></tr>\n";
    return ls;
}

// TODO: move to a view
const QString Git::getDesc(SCRef sha, QRegExp& shortLogRE, QRegExp& longLogRE, bool showHeader,
                           FileHistory* fh)
{
    if (sha.isEmpty())
        return "";

    const Revision* c = revLookup(sha, fh);
    if (!c)            // sha of a not loaded revision, as
        return ""; // example asked from file history

    QString text;
    if (c->isDiffCache)
        text = Qt::convertFromPlainText(c->longLog());
    else {
        QTextStream ts(&text);
        ts << "<html><head><style type=\"text/css\">"
                "tr.head { background-color: #a0a0e0 }\n"
                "td.h { font-weight: bold; }\n"
                "table { background-color: #e0e0f0; }\n"
                "span.h { font-weight: bold; font-size: medium; }\n"
                "div.l { white-space: pre; "
                "font-family: " << TYPE_WRITER_FONT.family() << ";"
                "font-size: " << TYPE_WRITER_FONT.pointSize() << "pt;}\n"
                "</style></head><body><div class='t'>\n"
                "<table border=0 cellspacing=0 cellpadding=2>";

        ts << "<tr class='head'> <th></th> <th><span class='h'>"
            << colorMatch(c->shortLog(), shortLogRE)
            << "</span></th></tr>";

        if (showHeader) {
            if (c->committer() != c->author())
                ts << formatList(QStringList(Qt::escape(c->committer())), "Committer");
            ts << formatList(QStringList(Qt::escape(c->author())), "Author");
            ts << formatList(QStringList(getLocalDate(c->authorDate())), " Author date");

            if (c->isUnApplied || c->isApplied) {

                QStringList patches(m_references.filter(toTempSha(sha), Reference::APPLIED).getNames().join(" "));
                patches += m_references.filter(toTempSha(sha), Reference::UN_APPLIED).getNames().join(" ");
                ts << formatList(patches, "Patch");
            } else {
                ts << formatList(c->parents(), "Parent", false);
                ts << formatList(getChilds(sha), "Child", false);
                ts << formatList(getDescendantBranches(sha), "Branch", false);
                ts << formatList(getNearTags(!optGoDown, sha), "Follows");
                ts << formatList(getNearTags(optGoDown, sha), "Precedes");
            }
        }
        QString longLog(c->longLog());
        if (showHeader) {
            longLog.prepend(QString("\n") + c->shortLog() + "\n");
        }

        QString log(colorMatch(longLog, longLogRE));
        log.replace("\n", "\n    ").prepend('\n');
        ts << "</table></div><div class='l'>" << log << "</div></body></html>";
    }
    // highlight SHA's
    //
    // added to commit logs, we avoid to call git rev-parse for a possible abbreviated
    // sha if there isn't a leading trailing space or an open parenthesis and,
    // in that case, before the space must not be a ':' character.
    // It's an ugly heuristic, but seems to work in most cases.
    QRegExp reSHA("..[0-9a-f]{21,40}|[^:][\\s(][0-9a-f]{6,20}", Qt::CaseInsensitive);
    reSHA.setMinimal(false);
    int pos = 0;
    while ((pos = text.indexOf(reSHA, pos)) != -1) {

        SCRef ref = reSHA.cap(0).mid(2);
        const Revision* r = (ref.length() == 40 ? revLookup(ref) : revLookup(m_references.getRefSha(ref)));
        if (r && r->sha() != ZERO_SHA_RAW) {
            QString slog(r->shortLog());
            if (slog.isEmpty()) // very rare but possible
                slog = r->sha();
            if (slog.length() > 60)
                slog = slog.left(57).trimmed().append("...");

            slog = Qt::escape(slog);
            const QString link("<a href=\"" + r->sha() + "\">" + slog + "</a>");
            text.replace(pos + 2, ref.length(), link);
            pos += link.length();
        } else
            pos += reSHA.cap(0).length();
    }
    return text;
}

const RevFile* Git::insertNewFiles(SCRef sha, SCRef data)
{
    /* we use an independent FileNamesLoader to avoid data
     * corruption if we are loading file names in background
     */
    FileNamesLoader fl;

    RevFile* rf = new RevFile();
    parseDiffFormat(*rf, data, fl);
    flushFileNames(fl);

    revsFiles.insert(toPersistentSha(sha, revsFilesShaBackupBuf), rf);
    return rf;
}

bool Git::runDiffTreeWithRenameDetection(SCRef runCmd, QString* runOutput)
{
/* Under some cases git could warn out:

      "too many files, skipping inexact rename detection"

   So if this occurs fallback on NO rename detection.
*/
    QString cmd(runCmd); // runCmd must be without -C option
    cmd.replace("git diff-tree", "git diff-tree -C");

    errorReportingEnabled = false;
    bool renameDetectionOk = run(cmd, runOutput);
    errorReportingEnabled = true;

    if (!renameDetectionOk) // retry without rename detection
        return run(runCmd, runOutput);

    return true;
}

const RevFile* Git::getAllMergeFiles(const Revision* r)
{
    SCRef mySha(ALL_MERGE_FILES + r->sha());
    if (revsFiles.contains(toTempSha(mySha)))
        return revsFiles[toTempSha(mySha)];

    EM_PROCESS_EVENTS; // 'git diff-tree' could be slow

    QString runCmd("git diff-tree --no-color -r -m " + r->sha());
    QString runOutput;
    if (!runDiffTreeWithRenameDetection(runCmd, &runOutput))
        return NULL;

    return insertNewFiles(mySha, runOutput);
}

const RevFile* Git::getFiles(SCRef sha, SCRef diffToSha, bool allFiles, SCRef path)
{
    const Revision* r = revLookup(sha);
    if (!r)
        return NULL;

    if (r->parentsCount() == 0) // skip initial rev
        return NULL;

    if (r->parentsCount() > 1 && diffToSha.isEmpty() && allFiles)
        return getAllMergeFiles(r);

    if (!diffToSha.isEmpty() && (sha != ZERO_SHA)) {

        QString runCmd("git diff-tree --no-color -r -m ");
        runCmd.append(diffToSha + " " + sha);
        if (!path.isEmpty())
            runCmd.append(" " + path);

        EM_PROCESS_EVENTS; // 'git diff-tree' could be slow

        QString runOutput;
        if (!runDiffTreeWithRenameDetection(runCmd, &runOutput))
            return NULL;

        // we insert a dummy revision file object. It will be
        // overwritten at each request but we don't care.
        return insertNewFiles(CUSTOM_SHA, runOutput);
    }
    if (revsFiles.contains(r->sha()))
        return revsFiles[r->sha()]; // ZERO_SHA search arrives here

    if (sha == ZERO_SHA) {
        dbs("ASSERT in Git::getFiles, ZERO_SHA not found");
        return NULL;
    }

    EM_PROCESS_EVENTS; // 'git diff-tree' could be slow

    QString runCmd("git diff-tree --no-color -r -c " + sha), runOutput;
    if (!runDiffTreeWithRenameDetection(runCmd, &runOutput))
        return NULL;

    if (revsFiles.contains(r->sha())) // has been created in the mean time?
        return revsFiles[r->sha()];

    cacheNeedsUpdate = true;
    return insertNewFiles(sha, runOutput);
}

bool Git::startFileHistory(SCRef sha, SCRef startingFileName, FileHistory* fh)
{
    QStringList args(getDescendantBranches(sha, true));
    if (args.isEmpty())
        args << "HEAD";

    QString newestFileName = getNewestFileName(args, startingFileName);
    fh->resetFileNames(newestFileName);

    args.clear(); // load history from all the branches
    args << m_references.getShas(Reference::BRANCH | Reference::REMOTE_BRANCH);

    args << "--" << newestFileName;
    return startRevList(args, fh);
}

const QString Git::getNewestFileName(SCList branches, SCRef fileName)
{
    QString curFileName(fileName), runOutput, args;
    while (true) {
        args = branches.join(" ") + " -- " + curFileName;
        if (!run("git ls-tree " + args, &runOutput))
            break;

        if (!runOutput.isEmpty())
            break;

        QString msg("Retrieving file renames, now at '" + curFileName + "'...");
        QApplication::postEvent(parent(), new MessageEvent(msg));
        EM_PROCESS_EVENTS_NO_INPUT;

        if (!run("git rev-list -n1 " + args, &runOutput))
            break;

        if (runOutput.isEmpty()) // try harder
            if (!run("git rev-list --full-history -n1 " + args, &runOutput))
                break;

        if (runOutput.isEmpty())
            break;

        SCRef sha = runOutput.trimmed();
        QStringList newCur;
        if (!populateRenamedPatches(sha, QStringList(curFileName), NULL, &newCur, true))
            break;

        curFileName = newCur.first();
    }
    return curFileName;
}

void Git::getFileFilter(SCRef path, ShaSet& shaSet) const
{
    shaSet.clear();
    QRegExp rx(path, Qt::CaseInsensitive, QRegExp::Wildcard);
    FOREACH (ShaVect, it, revData->revOrder) {

        if (!revsFiles.contains(*it))
            continue;

        // case insensitive, wildcard search
        const RevFile* rf = revsFiles[*it];
        for (int i = 0; i < rf->count(); ++i)
            if (filePath(*rf, i).contains(rx)) {
                shaSet.insert(*it);
                break;
            }
    }
}

bool Git::getPatchFilter(SCRef exp, bool isRegExp, ShaSet& shaSet)
{
    shaSet.clear();
    QString buf;
    FOREACH (ShaVect, it, revData->revOrder)
        if (*it != ZERO_SHA_RAW)
            buf.append(*it).append('\n');

    if (buf.isEmpty())
        return true;

    EM_PROCESS_EVENTS; // 'git diff-tree' could be slow

    QString runCmd("git diff-tree --no-color -r -s --stdin "), runOutput;
    if (isRegExp)
        runCmd.append("--pickaxe-regex ");

    runCmd.append(quote("-S" + exp));
    if (!run(runCmd, &runOutput, NULL, buf))
        return false;

    const QStringList sl(runOutput.split('\n', QString::SkipEmptyParts));
    FOREACH_SL (it, sl)
        shaSet.insert(*it);

    return true;
}

bool Git::resetCommits(int parentDepth)
{
    QString runCmd("git reset --soft HEAD~");
    runCmd.append(QString::number(parentDepth));
    return run(runCmd);
}

bool Git::applyPatchFile(SCRef patchPath, bool fold, bool isDragDrop)
{
    if (isStGIT) {
        if (fold) {
            bool ok = run("stg fold " + quote(patchPath)); // merge in working dir
            if (ok)
                ok = run("stg refresh"); // update top patch
            return ok;
        } else
            return run("stg import --mail " + quote(patchPath));
    }
    QString runCmd("git am --utf8 --3way ");

    QSettings settings;
    const QString APOpt(settings.value(AM_P_OPT_KEY).toString());
    if (!APOpt.isEmpty())
        runCmd.append(APOpt.trimmed() + " ");

    if (isDragDrop)
        runCmd.append("--keep ");

    if (testFlag(SIGN_PATCH_F))
        runCmd.append("--signoff ");

    return run(runCmd + quote(patchPath));
}

const QStringList Git::sortShaListByIndex(SCList shaList)
{
    QStringList orderedShaList;
    FOREACH_SL (it, shaList)
        appendNamesWithId(orderedShaList, *it, QStringList(*it), true);

    orderedShaList.sort();
    QStringList::iterator itN(orderedShaList.begin());
    for ( ; itN != orderedShaList.end(); ++itN) // strip 'idx'
        (*itN) = (*itN).section(' ', -1, -1);

        return orderedShaList;
}

bool Git::formatPatch(SCList shaList, SCRef dirPath, SCRef remoteDir)
{
    bool remote = !remoteDir.isEmpty();
    QSettings settings;
    const QString FPOpt(settings.value(FMT_P_OPT_KEY).toString());

    QString runCmd("git format-patch --no-color");
    if (testFlag(NUMBERS_F) && !remote)
        runCmd.append(" -n");

    if (remote)
        runCmd.append(" --keep-subject");

    runCmd.append(" -o " + quote(dirPath));
    if (!FPOpt.isEmpty())
        runCmd.append(" " + FPOpt.trimmed());

    const QString tmp(workDir);
    if (remote)
        workDir = remoteDir; // run() uses workDir value

    // shaList is ordered by newest to oldest
    runCmd.append(" " + shaList.last());
    runCmd.append(QString::fromLatin1("^..") + shaList.first());
    bool ret = run(runCmd);
    workDir = tmp;
    return ret;
}

const QStringList Git::getOtherFiles(SCList selFiles, bool onlyInIndex)
{
    const RevFile* files = getFiles(ZERO_SHA); // files != NULL
    QStringList notSelFiles;
    for (int i = 0; i < files->count(); ++i) {
        SCRef fp = filePath(*files, i);
        if (selFiles.indexOf(fp) == -1) { // not selected...
            if (!onlyInIndex || files->statusCmp(i, RevFile::IN_INDEX))
                notSelFiles.append(fp);
        }
    }
    return notSelFiles;
}

bool Git::updateIndex(SCList selFiles)
{
    const RevFile* files = getFiles(ZERO_SHA); // files != NULL

    QStringList toAdd, toRemove;
    FOREACH_SL (it, selFiles) {
        int idx = findFileIndex(*files, *it);
        if (files->statusCmp(idx, RevFile::DELETED))
            toRemove << *it;
        else
            toAdd << *it;
    }
    if (!toRemove.isEmpty() && !run("git rm --cached --ignore-unmatch -- " + quote(toRemove)))
        return false;

    if (!toAdd.isEmpty() && !run("git add -- " + quote(toAdd)))
        return false;

    return true;
}

bool Git::commitFiles(SCList selFiles, SCRef msg, bool amend)
{
    const QString msgFile(gitDir + "/qgit_cmt_msg.txt");
    if (!writeToFile(msgFile, msg)) // early skip
        return false;

    // add user selectable commit options
    QSettings settings;
    const QString CMArgs(settings.value(CMT_ARGS_KEY).toString());

    QString cmtOptions;
    if (!CMArgs.isEmpty())
        cmtOptions.append(" " + CMArgs);

    if (testFlag(SIGN_CMT_F))
        cmtOptions.append(" -s");

    if (testFlag(VERIFY_CMT_F))
        cmtOptions.append(" -v");

    if (amend)
        cmtOptions.append(" --amend");

    bool ret = false;

    // get not selected files but updated in index to restore at the end
    const QStringList notSel(getOtherFiles(selFiles, optOnlyInIndex));

    // call git reset to remove not selected files from index
    if (!notSel.empty() && !run("git reset -- " + quote(notSel)))
        goto fail;

    // update index with selected files
    if (!updateIndex(selFiles))
        goto fail;

    // now we can finally commit..
    if (!run("git commit" + cmtOptions + " -F " + quote(msgFile)))
        goto fail;

    // restore not selected files that were already in index
    if (!notSel.empty() && !updateIndex(notSel))
        goto fail;

    ret = true;
fail:
    QDir dir(workDir);
    dir.remove(msgFile);
    return ret;
}

bool Git::mkPatchFromWorkDir(SCRef msg, SCRef patchFile, SCList files)
{
     /* unfortunately 'git diff' sees only files already
     * known to git or already in index, so update index first
     * to be sure also unknown files are correctly found
     */
     if (!updateIndex(files))
         return false;

    QString runOutput;
//<<<<<<< HEAD
    if (!run("git diff --no-ext-diff -C HEAD -- " + quote(files), &runOutput))
//=======
//    if (!run("git diff -C HEAD -- " + quote(files), &runOutput))
//>>>>>>> antis81/ui_improvements
        return false;

    const QString patch("Subject: " + msg + "\n---\n" + runOutput);
    return writeToFile(patchFile, patch);
}

bool Git::stgCommit(SCList selFiles, SCRef msg, SCRef patchName, bool fold)
{
    /* Here the deal is to use 'stg import' and 'stg fold' to add a new
     * patch or refresh the current one respectively. Unfortunately refresh
     * does not work with partial selection of files and also does not take
     * patch message from a file that is needed to avoid artifacts with '\n'
     * and friends.
     *
     * So steps are:
     *
     * - Create a patch file with the changes you want to import/fold in StGit
     * - Stash working dir files because import/fold wants a clean directory
     * - Import/fold the patch
     * - Unstash and merge working dir modified files
     * - Restore index with not selected files
     */

    /* Step 1: Create a patch file with the changes you want to import/fold */
    bool ret = false;
    const QString patchFile(gitDir + "/qgit_tmp_patch.txt");

    // in case we don't have files to restore we can shortcut various commands
    bool partialSelection = !getOtherFiles(selFiles, !optOnlyInIndex).isEmpty();

    // get not selected files but updated in index to restore at the end
    QStringList notSel;
    if (partialSelection) // otherwise notSel is for sure empty
        notSel = getOtherFiles(selFiles, optOnlyInIndex);

    // create a patch with diffs between working dir and HEAD
    if (!mkPatchFromWorkDir(msg, patchFile, selFiles))
        goto fail;

    /* Step 2: Stash working dir modified files */
    if (partialSelection) {
        errorReportingEnabled = false;
        run("git stash"); // unfortunately 'git stash' is noisy on stderr
        errorReportingEnabled = true;
    }

    /* Step 3: Call stg import/fold */

    // setup a clean state
    if (!run("stg status --reset"))
        goto fail_and_unstash;

    if (fold) {
        // update patch message before to fold, note that
        // command 'stg edit' requires stg version 0.14 or later
        if (!msg.isEmpty() && !run("stg edit --message " + quote(msg.trimmed())))
            goto fail_and_unstash;

        if (!run("stg fold " + quote(patchFile)))
            goto fail_and_unstash;

        if (!run("stg refresh")) // refresh needed after fold
            goto fail_and_unstash;

    } else if (!run("stg import --mail --name " + quote(patchName) + " " + quote(patchFile)))
        goto fail_and_unstash;

    if (partialSelection) {

        /* Step 4: Unstash and merge working dir modified files */
        errorReportingEnabled = false;
        run("git stash pop"); // unfortunately 'git stash' is noisy on stderr
        errorReportingEnabled = true;

        /* Step 5: restore not selected files that were already in index */
        if (!notSel.empty() && !updateIndex(notSel))
            goto fail;
    }

    ret = true;
    goto exit;

fail_and_unstash:

    if (partialSelection) {
        run("git reset");
        errorReportingEnabled = false;
        run("git stash pop");
        errorReportingEnabled = true;
    }
fail:
exit:
    QDir dir(workDir);
    dir.remove(patchFile);
    return ret;
}

bool Git::makeBranch(SCRef sha, SCRef branchName)
{
    return run("git branch " + branchName + " " + sha);
}

bool Git::makeTag(SCRef sha, SCRef tagName, SCRef msg)
{
    if (msg.isEmpty())
        return run("git tag " + tagName + " " + sha);

    return run("git tag -m \"" + msg + "\" " + tagName + " " + sha);
}

bool Git::deleteTag(const QString &tagName)
{
    Reference* ref = m_references.byName(tagName, Reference::TAG);
    if (!ref) {
        return false;
    }

    return run("git tag -d " + tagName);
}

bool Git::checkout(SCRef sha)
{
    bool b = run("git checkout " + sha);
    updateCurrentBranch();
    return b;
}

bool Git::stgPush(const ShaString& sha)
{
    ReferenceList list = m_references.filter(sha, Reference::UN_APPLIED);
    if (list.count() != 1) {
        dbp("ASSERT in Git::stgPush, found %1 patches instead of 1", list.count());
        return false;
    }
    return run("stg push " + escape(list.first()->name()));
}

bool Git::stgPop(const ShaString& sha)
{
    ReferenceList list = m_references.filter(sha, Reference::APPLIED);
    if (list.count() != 1) {
        dbp("ASSERT in Git::stgPop, found %1 patches instead of 1", list.count());
        return false;
    }
    return run("stg pop " + escape(list.first()->name()));
}

bool Git::updateCurrentBranch() {
    QString curBranchName;
    if (!run("git branch", &curBranchName))
        return false;

    curBranchName = curBranchName.prepend('\n').section("\n*", 1);
    curBranchName = curBranchName.section('\n', 0, 0).trimmed();
    m_currentBranch = curBranchName;
    return true;
}

QString& Git::currentBranch() {
    return m_currentBranch;
}

// FROM git_startup.cpp ///////////////////////////////////////////////////////////////////////////

#define SHOW_MSG(x) QApplication::postEvent(parent(), new MessageEvent(x)); EM_PROCESS_EVENTS_NO_INPUT;


static bool startup = true; // it's OK to be unique among qgit windows

static QHash<QString, QString> localDates;

// TODO: move to a view
const QString Git::getLocalDate(SCRef gitDate) {
// fast path here, we use a cache to avoid the slow date calculation

    QString localDate(localDates.value(gitDate));
    if (!localDate.isEmpty())
        return localDate;

    QDateTime d;
    d.setTime_t(gitDate.toULong());
    localDate = d.toString(Qt::LocalDate);
    localDates[gitDate] = localDate;
    return localDate;
}

const QStringList Git::getArgs(bool* quit, bool repoChanged) {

    QString args;
    if (startup) {
        for (int i = 1; i < qApp->argc(); i++) {
            // in arguments with spaces double quotes
            // are stripped by Qt, so re-add them
            QString arg(qApp->argv()[i]);
            if (arg.contains(' '))
                arg.prepend('\"').append('\"');

            args.append(arg + ' ');
        }
    }
    if (testFlag(RANGE_SELECT_F) && (!startup || args.isEmpty())) {

        RangeSelectImpl rs((QWidget*)parent(), &args, repoChanged, this);
        *quit = (rs.exec() == QDialog::Rejected); // modal execution
        if (*quit)
            return QStringList();
    }
    startup = false;
    return MyProcess::splitArgList(args);
}

const QString Git::getBaseDir(bool* changed, SCRef wd, bool* ok, QString* gd) {
// we could run from a subdirectory, so we need to get correct directories

    QString runOutput, tmp(workDir);
    workDir = wd;
    errorReportingEnabled = false;
    bool ret = run("git rev-parse --git-dir", &runOutput); // run under newWorkDir
    errorReportingEnabled = true;
    workDir = tmp;
    runOutput = runOutput.trimmed();
    if (!ret || runOutput.isEmpty()) {
        *changed = true;
        if (ok)
            *ok = false;
        return wd;
    }
    // 'git rev-parse --git-dir' output could be a relative
    // to working dir (as ex .git) or an absolute path
    QDir d(runOutput.startsWith("/") ? runOutput : wd + "/" + runOutput);
    *changed = (d.absolutePath() != gitDir);
    if (gd)
        *gd = d.absolutePath();
    if (ok)
        *ok = true;
    d.cdUp();
    return d.absolutePath();
}

bool Git::getRefs() {

    // check for a StGIT stack
    QDir d(gitDir);
    QString stgCurBranch;
    if (d.exists("patches")) { // early skip
        errorReportingEnabled = false;
        isStGIT = run("stg branch", &stgCurBranch); // slow command
        errorReportingEnabled = true;
        stgCurBranch = stgCurBranch.trimmed();
    } else
        isStGIT = false;

    // check for a merge and read current branch sha
//    isMergeHead = d.exists("MERGE_HEAD");
//    QString curBranchSHA;
//    if (!run("git rev-parse --revs-only HEAD", &curBranchSHA))
//        return false;

    if (!updateCurrentBranch())
        return false;

//    curBranchSHA = curBranchSHA.trimmed();

    bool success = m_references.load(stgCurBranch);
    if (!success) return success;

    Reference* ref = m_references.byName(m_currentBranch);
    if (ref) {
        ref->setType(ref->type() | Reference::CUR_BRANCH);
    }
    return success;
}

const QStringList Git::getOthersFiles() {
// add files present in working directory but not in git archive

    QString runCmd("git ls-files --others");
    QSettings settings;
    QString exFile(settings.value(EX_KEY, EX_DEF).toString());
    if (!exFile.isEmpty()) {
        QString path = (exFile.startsWith("/")) ? exFile : workDir + "/" + exFile;
        if (QFile::exists(path))
            runCmd.append(" --exclude-from=" + quote(exFile));
    }
    QString exPerDir(settings.value(EX_PER_DIR_KEY, EX_PER_DIR_DEF).toString());
    if (!exPerDir.isEmpty())
        runCmd.append(" --exclude-per-directory=" + quote(exPerDir));

    QString runOutput;
    run(runCmd, &runOutput);
    return runOutput.split('\n', QString::SkipEmptyParts);
}

Revision* Git::fakeRevData(SCRef sha, SCList parents, SCRef author, SCRef date, SCRef log, SCRef longLog,
                      SCRef patch, int idx, FileHistory* fh) {

    QString data('>' + sha + 'X' + parents.join(" ") + " \n");
    data.append(author + '\n' + author + '\n' + date + '\n');
    data.append(log + '\n' + longLog);

    QString header("log size " + QString::number(data.size() - 1) + '\n');
    data.prepend(header);
    if (!patch.isEmpty())
        data.append('\n' + patch);

    QByteArray* ba = new QByteArray(data.toAscii());
    ba->append('\0');

    fh->rowData.append(ba);
    int dummy;
    Revision* c = new Revision(*ba, 0, idx, &dummy, !isMainHistory(fh));
    return c;
}

const Revision* Git::fakeWorkDirRev(SCRef parent, SCRef log, SCRef longLog, int idx, FileHistory* fh) {

    QString patch;
    if (!isMainHistory(fh))
        patch = getWorkDirDiff(fh->fileNames().first());

    QString date(QString::number(QDateTime::currentDateTime().toTime_t()));
    QString author("Working Dir");
    QStringList parents(parent);
    Revision* c = fakeRevData(ZERO_SHA, parents, author, date, log, longLog, patch, idx, fh);
    c->isDiffCache = true;
    c->lanes.append(LANE_EMPTY);
    return c;
}

const RevFile* Git::fakeWorkDirRevFile(const WorkingDirInfo& wd) {

    FileNamesLoader fl;
    RevFile* rf = new RevFile();
    parseDiffFormat(*rf, wd.diffIndex, fl);
    rf->onlyModified = false;

    FOREACH_SL (it, wd.otherFiles) {

        appendFileName(*rf, *it, fl);
        rf->status.append(RevFile::UNKNOWN);
        rf->mergeParent.append(1);
    }
    RevFile cachedFiles;
    parseDiffFormat(cachedFiles, wd.diffIndexCached, fl);
    flushFileNames(fl);

    for (int i = 0; i < rf->count(); i++)
        if (findFileIndex(cachedFiles, filePath(*rf, i)) != -1)
            rf->status[i] |= RevFile::IN_INDEX;
    return rf;
}

void Git::getDiffIndex() {

    QString status;
    if (!run("git status", &status)) // git status refreshes the index, run as first
        return;

    QString head;
    if (!run("git rev-parse --revs-only HEAD", &head))
        return;

    head = head.trimmed();
    if (!head.isEmpty()) { // repository initialized but still no history

        if (!run("git diff-index " + head, &workingDirInfo.diffIndex))
            return;

        // check for files already updated in cache, we will
        // save this information in status third field
        if (!run("git diff-index --cached " + head, &workingDirInfo.diffIndexCached))
            return;
    }
    // get any file not in tree
    workingDirInfo.otherFiles = getOthersFiles();

    // now mockup a RevFile
    revsFiles.insert(ZERO_SHA_RAW, fakeWorkDirRevFile(workingDirInfo));

    // then mockup the corresponding Rev
    SCRef log = (isNothingToCommit() ? "Nothing to commit" : "Working dir changes");
    const Revision* r = fakeWorkDirRev(head, log, status, revData->revOrder.count(), revData);
    revData->revs.insert(ZERO_SHA_RAW, r);
    revData->revOrder.append(ZERO_SHA_RAW);
    revData->earlyOutputCntBase = revData->revOrder.count();

    // finally send it to GUI
    emit newRevsAdded(revData, revData->revOrder);
}

void Git::parseDiffFormatLine(RevFile& rf, SCRef line, int parNum, FileNamesLoader& fl) {

    if (line[1] == ':') { // it's a combined merge

        /* For combined merges rename/copy information is useless
         * because nor the original file name, nor similarity info
         * is given, just the status tracks that in the left/right
         * branch a renamed/copy occurred (as example status could
         * be RM or MR). For visualization purposes we could consider
         * the file as modified
         */
        appendFileName(rf, line.section('\t', -1), fl);
        setStatus(rf, "M");
        rf.mergeParent.append(parNum);
    } else { // faster parsing in normal case

        if (line.at(98) == '\t') {
            appendFileName(rf, line.mid(99), fl);
            setStatus(rf, line.at(97));
            rf.mergeParent.append(parNum);
        } else
            // it's a rename or a copy, we are not in fast path now!
            setExtStatus(rf, line.mid(97), parNum, fl);
    }
}

// TODO: move into RevFile ?
void Git::setStatus(RevFile& rf, SCRef rowSt) {

    char status = rowSt.at(0).toLatin1();
    switch (status) {
    case 'M':
    case 'T':
    case 'U':
        rf.status.append(RevFile::MODIFIED);
        break;
    case 'D':
        rf.status.append(RevFile::DELETED);
        rf.onlyModified = false;
        break;
    case 'A':
        rf.status.append(RevFile::NEW);
        rf.onlyModified = false;
        break;
    case '?':
        rf.status.append(RevFile::UNKNOWN);
        rf.onlyModified = false;
        break;
    default:
        dbp("ASSERT in Git::setStatus, unknown status <%1>. "
            "'MODIFIED' will be used instead.", rowSt);
        rf.status.append(RevFile::MODIFIED);
        break;
    }
}

// TODO: move into RevFile ?
void Git::setExtStatus(RevFile& rf, SCRef rowSt, int parNum, FileNamesLoader& fl) {

    const QStringList sl(rowSt.split('\t', QString::SkipEmptyParts));
    if (sl.count() != 3) {
        dbp("ASSERT in setExtStatus, unexpected status string %1", rowSt);
        return;
    }
    // we want store extra info with format "orig --> dest (Rxx%)"
    // but git give us something like "Rxx\t<orig>\t<dest>"
    SCRef type = sl[0];
    SCRef orig = sl[1];
    SCRef dest = sl[2];
    const QString extStatusInfo(orig + " --> " + dest + " (" + type + "%)");

    /*
       NOTE: we set rf.extStatus size equal to position of latest
             copied/renamed file. So it can have size lower then
             rf.count() if after copied/renamed file there are
             others. Here we have no possibility to know final
             dimension of this RefFile. We are still in parsing.
    */

    // simulate new file
    appendFileName(rf, dest, fl);
    rf.mergeParent.append(parNum);
    rf.status.append(RevFile::NEW);
    rf.extStatus.resize(rf.status.size());
    rf.extStatus[rf.status.size() - 1] = extStatusInfo;

    // simulate deleted orig file only in case of rename
    if (type.at(0) == 'R') { // renamed file
        appendFileName(rf, orig, fl);
        rf.mergeParent.append(parNum);
        rf.status.append(RevFile::DELETED);
        rf.extStatus.resize(rf.status.size());
        rf.extStatus[rf.status.size() - 1] = extStatusInfo;
    }
    rf.onlyModified = false;
}

void Git::parseDiffFormat(RevFile& rf, SCRef buf, FileNamesLoader& fl) {

    int parNum = 1, startPos = 0, endPos = buf.indexOf('\n');
    while (endPos != -1) {

        SCRef line = buf.mid(startPos, endPos - startPos);
        if (line[0] == ':') // avoid sha's in merges output
            parseDiffFormatLine(rf, line, parNum, fl);
        else
            parNum++;

        startPos = endPos + 1;
        endPos = buf.indexOf('\n', endPos + 99);
    }
}

bool Git::startParseProc(SCList initCmd, FileHistory* fh, SCRef buf) {

    DataLoader* dl = new DataLoader(this, fh); // auto-deleted when done

    connect(this, SIGNAL(cancelLoading(const FileHistory*)),
            dl, SLOT(on_cancel(const FileHistory*)));

    connect(dl, SIGNAL(newDataReady(const FileHistory*)),
            this, SLOT(on_newDataReady(const FileHistory*)));

    connect(dl, SIGNAL(loaded(FileHistory*, ulong, int,
            bool, const QString&, const QString&)), this,
            SLOT(on_loaded(FileHistory*, ulong, int,
            bool, const QString&, const QString&)));

    return dl->start(initCmd, workDir, buf);
}

bool Git::startRevList(SCList args, FileHistory* fh) {

    QString baseCmd("git log --topo-order --no-color "

#ifndef Q_OS_WIN32
                    "--log-size " // FIXME broken on Windows
#endif
                    "--parents --boundary -z "
                    "--pretty=format:%m%HX%PX%n%cn<%ce>%n%an<%ae>%n%at%n%s%n");

    // we don't need log message body for file history
    if (isMainHistory(fh))
        baseCmd.append("%b");

    QStringList initCmd(baseCmd.split(' '));
    if (!isMainHistory(fh)) {
    /*
       NOTE: we don't use '--remove-empty' option because
       in case a file is deleted and then a new file with
       the same name is created again in the same directory
       then, with this option, file history is truncated to
       the file deletion revision.
    */
        initCmd << QString("-r -m -p --full-index").split(' ');
    } else
        {} // initCmd << QString("--early-output"); currently disabled

    return startParseProc(initCmd + args, fh, QString());
}

bool Git::startUnappliedList() {

    QStringList unAppliedShaList(m_references.getShas(Reference::UN_APPLIED));
    if (unAppliedShaList.isEmpty())
        return false;

    // WARNING: with this command 'git log' could send spurious
    // revs so we need some filter out logic during loading
    QString cmd("git log --no-color --parents -z "

#ifndef Q_OS_WIN32
                "--log-size " // FIXME broken on Windows
#endif
                "--pretty=format:%m%HX%PX%n%an<%ae>%n%at%n%s%n%b ^HEAD");

    QStringList sl(cmd.split(' '));
    sl << unAppliedShaList;
    return startParseProc(sl, revData, QString());
}

void Git::stop(bool saveCache) {
// normally called when changing directory or closing

    EM_RAISE(exGitStopped);

    // stop all data sending from process and asks them
    // to terminate. Note that process could still keep
    // running for a while although silently
    emit cancelAllProcesses(); // non blocking

    // after cancelAllProcesses() procFinished() is not called anymore
    // TODO perhaps is better to call procFinished() also if process terminated
    // incorrectly as QProcess does. BUt first we need to fix FileView::on_loadCompleted()
    emit fileNamesLoad(1, revsFiles.count() - filesLoadingStartOfs);

    if (cacheNeedsUpdate && saveCache) {

        cacheNeedsUpdate = false;
        if (!filesLoadingCurSha.isEmpty()) // we are in the middle of a loading
            revsFiles.remove(toTempSha(filesLoadingCurSha)); // remove partial data

        if (!revsFiles.isEmpty()) {
            SHOW_MSG("Saving cache. Please wait...");
            if (!Cache::save(gitDir, revsFiles, dirNamesVec, fileNamesVec))
                dbs("ERROR unable to save file names cache");
        }
    }
}

void Git::clearRevs() {

    revData->clear();
    firstNonStGitPatch = "";
    workingDirInfo.clear();
    revsFiles.remove(ZERO_SHA_RAW);
}

void Git::clearFileNames() {

    qDeleteAll(revsFiles);
    revsFiles.clear();
    fileNamesMap.clear();
    dirNamesMap.clear();
    dirNamesVec.clear();
    fileNamesVec.clear();
    revsFilesShaBackupBuf.clear();
    cacheNeedsUpdate = false;
}

bool Git::init(SCRef wd, bool askForRange, const QStringList* passedArgs, bool overwriteArgs, bool* quit) {
// normally called when changing git directory. Must be called after stop()

    *quit = false;
    clearRevs();

    /* we only update filtering info here, original arguments
     * are not overwritten. Only getArgs() can update arguments,
     * an exception is if flag overwriteArgs is set
     */
    loadArguments.filteredLoading = (!overwriteArgs && passedArgs != NULL);
    if (loadArguments.filteredLoading)
        loadArguments.filterList = *passedArgs;

    if (overwriteArgs) // in this case must be passedArgs != NULL
        loadArguments.args = *passedArgs;

    try {
        setThrowOnStop(true);

        const QString msg1("Path is '" + workDir + "'    Loading ");

        // check if repository is valid
        bool repoChanged;
        workDir = getBaseDir(&repoChanged, wd, &isGIT, &gitDir);

        if (repoChanged) {
            localDates.clear();
            clearFileNames();
            fileCacheAccessed = false;

            SHOW_MSG(msg1 + "file names cache...");
            loadFileCache();
            SHOW_MSG("");
        }
        if (!isGIT) {
            setThrowOnStop(false);
            return false;
        }
        if (!passedArgs) {

            // update text codec according to repo settings
            bool dummy;
            QTextCodec::setCodecForCStrings(getTextCodec(&dummy));

            // load references
            SHOW_MSG(msg1 + "refs...");
            if (!getRefs())
                dbs("WARNING: no tags or heads found");

            // startup input range dialog
            SHOW_MSG("");
            if (startup || askForRange) {
                loadArguments.args = getArgs(quit, repoChanged); // must be called with refs loaded
                if (*quit) {
                    setThrowOnStop(false);
                    return false;
                }
            }
            // load StGit unapplied patches, must be after getRefs()
            if (isStGIT) {
                loadingUnAppliedPatches = startUnappliedList();
                if (loadingUnAppliedPatches) {

                    SHOW_MSG(msg1 + "StGIT unapplied patches...");
                    setThrowOnStop(false);

                    // we will continue with init2() at
                    // the end of loading...
                    return true;
                }
            }
        }
        init2();
        setThrowOnStop(false);
        return true;

    } catch (int i) {

        setThrowOnStop(false);

        if (isThrowOnStopRaised(i, "initializing 1")) {
            EM_THROW_PENDING;
            return false;
        }
        const QString info("Exception \'" + EM_DESC(i) + "\' "
                           "not handled in init...re-throw");
        dbs(info);
        throw;
    }
}

void Git::init2() {

    const QString msg1("Path is '" + workDir + "'    Loading ");

    // after loading unapplied patch update base early output offset to
    // avoid losing unapplied patches at first early output event
    if (isStGIT)
        revData->earlyOutputCntBase = revData->revOrder.count();

    try {
        setThrowOnStop(true);

        // load working dir files
        if (!loadArguments.filteredLoading && testFlag(DIFF_INDEX_F)) {
            SHOW_MSG(msg1 + "working directory changed files...");
            getDiffIndex(); // blocking, we could be in setRepository() now
        }
        SHOW_MSG(msg1 + "revisions...");

        // build up command line arguments
        QStringList args(loadArguments.args);
        if (loadArguments.filteredLoading) {
            if (!args.contains("--"))
                args << "--";

            args << loadArguments.filterList;
        }
        if (!startRevList(args, revData))
            SHOW_MSG("ERROR: unable to start 'git log'");

        setThrowOnStop(false);

    } catch (int i) {

        setThrowOnStop(false);

        if (isThrowOnStopRaised(i, "initializing 2")) {
            EM_THROW_PENDING;
            return;
        }
        const QString info("Exception \'" + EM_DESC(i) + "\' "
                           "not handled in init2...re-throw");
        dbs(info);
        throw;
    }
}

void Git::on_newDataReady(const FileHistory* fh) {

    emit newRevsAdded(fh , fh->revOrder);
}

void Git::on_loaded(FileHistory* fh, ulong byteSize, int loadTime,
                    bool normalExit, SCRef cmd, SCRef errorDesc) {

    if (!errorDesc.isEmpty()) {
        MainExecErrorEvent* e = new MainExecErrorEvent(cmd, errorDesc);
        QApplication::postEvent(parent(), e);
    }
    if (normalExit) { // do not send anything if killed

        on_newDataReady(fh);

        if (!loadingUnAppliedPatches) {

            fh->loadTime += loadTime;

            uint kb = byteSize / 1024;
            float mbs = (float)byteSize / fh->loadTime / 1000;
            QString tmp;
            tmp.sprintf("Loaded %i revisions  (%i KB),   "
                        "time elapsed: %i ms  (%.2f MB/s)",
                        fh->revs.count(), kb, fh->loadTime, mbs);

            if (!tryFollowRenames(fh))
                emit loadCompleted(fh, tmp);

            if (isMainHistory(fh))
                // wait the dust to settle down before to start
                // background file names loading for new revisions
                QTimer::singleShot(500, this, SLOT(loadFileNames()));
        }
    }
    if (loadingUnAppliedPatches) {
        loadingUnAppliedPatches = false;
        revData->lns->clear(); // again to reset lanes
        init2(); // continue with loading of remaining revisions
    }
}

bool Git::tryFollowRenames(FileHistory* fh) {

    if (isMainHistory(fh))
        return false;

    QStringList oldNames;
    QMutableStringListIterator it(fh->renamedRevs);
    while (it.hasNext())
        if (!populateRenamedPatches(it.next(), fh->curFNames, fh, &oldNames, false))
            it.remove();

    if (fh->renamedRevs.isEmpty())
        return false;

    QStringList args;
    args << fh->renamedRevs << "--" << oldNames;
    fh->fNames << oldNames;
    fh->curFNames = oldNames;
    fh->renamedRevs.clear();
    return startRevList(args, fh);
}

bool Git::populateRenamedPatches(SCRef renamedSha, SCList newNames, FileHistory* fh,
                                 QStringList* oldNames, bool backTrack) {

    QString runOutput;
    if (!run("git diff-tree -r -M " + renamedSha, &runOutput))
        return false;

    // find the first renamed file with the new file name in renamedFiles list
    QString line;
    FOREACH_SL (it, newNames) {
        if (backTrack) {
            line = runOutput.section('\t' + *it + '\t', 0, 0,
                                     QString::SectionIncludeTrailingSep);
            line.chop(1);
        } else
            line = runOutput.section('\t' + *it + '\n', 0, 0);

        if (!line.isEmpty())
            break;
    }
    if (line.contains('\n'))
        line = line.section('\n', -1, -1);

    SCRef status = line.section('\t', -2, -2).section(' ', -1, -1);
    if (!status.startsWith('R'))
        return false;

    if (backTrack) {
        SCRef nextFile = runOutput.section(line, 1, 1).section('\t', 1, 1);
        oldNames->append(nextFile.section('\n', 0, 0));
        return true;
    }
    // get the diff betwen two files
    SCRef prevFileSha = line.section(' ', 2, 2);
    SCRef lastFileSha = line.section(' ', 3, 3);
    if (prevFileSha == lastFileSha) // just renamed
        runOutput.clear();
    else if (!run("git diff --no-ext-diff -r --full-index " + prevFileSha + " " + lastFileSha, &runOutput))
        return false;

    SCRef prevFile = line.section('\t', -1, -1);
    if (!oldNames->contains(prevFile))
        oldNames->append(prevFile);

    // save the patch, will be used later to create a
    // proper graft sha with correct parent info
    if (fh) {
        QString tmp(!runOutput.isEmpty() ? runOutput : "diff --no-ext-diff --\nsimilarity index 100%\n");
        fh->renamedPatches.insert(renamedSha, tmp);
    }
    return true;
}

void Git::populateFileNamesMap() {

    for (int i = 0; i < dirNamesVec.count(); ++i)
        dirNamesMap.insert(dirNamesVec[i], i);

    for (int i = 0; i < fileNamesVec.count(); ++i)
        fileNamesMap.insert(fileNamesVec[i], i);
}

void Git::loadFileCache() {

    if (!fileCacheAccessed) {

        fileCacheAccessed = true;
        QByteArray shaBuf;
        if (Cache::load(gitDir, revsFiles, dirNamesVec, fileNamesVec, shaBuf)) {
            revsFilesShaBackupBuf.append(shaBuf);
            populateFileNamesMap();
        } else
            dbs("ERROR: unable to load file names cache");
    }
}

void Git::loadFileNames() {

    indexTree(); // we are sure data loading is finished at this point

    int revCnt = 0;
    QString diffTreeBuf;
    FOREACH (ShaVect, it, revData->revOrder) {

        if (!revsFiles.contains(*it)) {
            const Revision* c = revLookup(*it);
            if (c->parentsCount() == 1) { // skip initials and merges
                diffTreeBuf.append(*it).append('\n');
                revCnt++;
            }
        }
    }
    if (!diffTreeBuf.isEmpty()) {
        filesLoadingPending = filesLoadingCurSha = "";
        filesLoadingStartOfs = revsFiles.count();
        emit fileNamesLoad(3, revCnt);

        const QString runCmd("git diff-tree --no-color -r -C --stdin");
        runAsync(runCmd, this, diffTreeBuf);
    }
}

bool Git::filterEarlyOutputRev(FileHistory* fh, Revision* rev) {

    if (fh->earlyOutputCnt < fh->revOrder.count()) {

        const ShaString& sha = fh->revOrder[fh->earlyOutputCnt++];
        const Revision* c = revLookup(sha, fh);
        if (c) {
            if (rev->sha() != sha || rev->parents() != c->parents()) {
                // mismatch found! set correct value, 'rev' will
                // overwrite 'c' upon returning
                rev->orderIdx = c->orderIdx;
                revData->clear(false); // flush the tail
            } else
                return true; // filter out 'rev'
        }
    }
    // we have new revisions, exit from early output state
    fh->setEarlyOutputState(false);
    return false;
}

int Git::addChunk(FileHistory* fh, const QByteArray& ba, int start) {

    RevMap& r = fh->revs;
    int nextStart;
    Revision* rev;

    do {
        // only here we create a new rev
        rev = new Revision(ba, start, fh->revOrder.count(), &nextStart, !isMainHistory(fh));

        if (nextStart == -2) {
            delete rev;
            fh->setEarlyOutputState(true);
            start = ba.indexOf('\n', start) + 1;
        }

    } while (nextStart == -2);

    if (nextStart == -1) { // half chunk detected
        delete rev;
        return -1;
    }

    const ShaString& sha = rev->sha();

    if (fh->earlyOutputCnt != -1 && filterEarlyOutputRev(fh, rev)) {
        delete rev;
        return nextStart;
    }

    if (isStGIT) {
        if (loadingUnAppliedPatches) { // filter out possible spurious revs

            uint type = m_references.containsType(sha, Reference::UN_APPLIED);
            if (!type) {
                delete rev;
                return nextStart;
            }
        }
        // remove StGIT spurious revs filter
        if (!firstNonStGitPatch.isEmpty() && firstNonStGitPatch == sha)
            firstNonStGitPatch = "";

        // StGIT called with --all option creates spurious revs so filter
        // out unknown revs until no more StGIT patches are waited and
        // firstNonStGitPatch is reached
        if (!(firstNonStGitPatch.isEmpty() && m_references.patchesStillToFind == 0) &&
            !loadingUnAppliedPatches && isMainHistory(fh)) {

            uint type = m_references.containsType(sha, Reference::APPLIED);
            if (!type) {
                delete rev;
                return nextStart;
            }
        }
        if (r.contains(sha)) {
            // StGIT unapplied patches could be sent again by
            // 'git log' as example if called with --all option.
            if (r[sha]->isUnApplied) {
                delete rev;
                return nextStart;
            }
            // could be a side effect of 'git log -m', see below
            if (isMainHistory(fh) || rev->parentsCount() < 2)
                dbp("ASSERT: addChunk sha <%1> already received", sha);
        }
    }
    if (r.isEmpty() && !isMainHistory(fh)) {
        bool added = copyDiffIndex(fh, sha);
        rev->orderIdx = added ? 1 : 0;
    }
    if (   !isMainHistory(fh)
        && !fh->renamedPatches.isEmpty()
        &&  fh->renamedPatches.contains(sha)) {

        // this is the new rev with renamed file, the rev is correct but
        // the patch, create a new rev with proper patch and use that instead
        const Revision* prevSha = revLookup(sha, fh);
        Revision* c = fakeRevData(sha, rev->parents(), rev->author(),
                             rev->authorDate(), rev->shortLog(), rev->longLog(),
                             fh->renamedPatches[sha], prevSha->orderIdx, fh);

        r.insert(sha, c); // overwrite old content
        fh->renamedPatches.remove(sha);
        return nextStart;
    }
    if (!isMainHistory(fh) && rev->parentsCount() > 1 && r.contains(sha)) {
    /* In this case git log is called with -m option and merges are splitted
       in one commit per parent but all them have the same sha.
       So we add only the first to fh->revOrder to display history correctly,
       but we nevertheless add all the commits to 'r' so that annotation code
       can get the patches.
    */
        QString mergeSha;
        int i = 0;
        do
            mergeSha = QString::number(++i) + " m " + sha;
        while (r.contains(toTempSha(mergeSha)));

        const ShaString& ss = toPersistentSha(mergeSha, shaBackupBuf);
        r.insert(ss, rev);
    } else {
        r.insert(sha, rev);
        fh->revOrder.append(sha);

        if (rev->parentsCount() == 0 && !isMainHistory(fh))
            fh->renamedRevs.append(sha);
    }
    if (isStGIT) {
        // updateLanes() is called too late, after loadingUnAppliedPatches
        // has been reset so update the lanes now.
        if (loadingUnAppliedPatches) {

            Revision* c = const_cast<Revision*>(revLookup(sha, fh));
            c->isUnApplied = true;
            c->lanes.append(LANE_UNAPPLIED);

        } else if (m_references.patchesStillToFind > 0 || !isMainHistory(fh)) { // try to avoid costly lookup

            uint type = m_references.containsType(sha, Reference::APPLIED);
            if (type) {
                Revision* c = const_cast<Revision*>(revLookup(sha, fh));
                c->isApplied = true;
                if (isMainHistory(fh)) {
                    m_references.patchesStillToFind--;
                    if (m_references.patchesStillToFind == 0)
                        // any rev will be discarded until
                        // firstNonStGitPatch arrives
                        firstNonStGitPatch = c->parent(0);
                }
            }
        }
    }
    return nextStart;
}

bool Git::copyDiffIndex(FileHistory* fh, SCRef parent) {
// must be called with empty revs and empty revOrder

    if (!fh->revOrder.isEmpty() || !fh->revs.isEmpty()) {
        dbs("ASSERT in copyDiffIndex: called with wrong context");
        return false;
    }
    const Revision* r = revLookup(ZERO_SHA);
    if (!r)
        return false;

    const RevFile* files = getFiles(ZERO_SHA);
    if (!files || findFileIndex(*files, fh->fileNames().first()) == -1)
        return false;

    // insert a custom ZERO_SHA rev with proper parent
    const Revision* rf = fakeWorkDirRev(parent, "Working dir changes", "long log\n", 0, fh);
    fh->revs.insert(ZERO_SHA_RAW, rf);
    fh->revOrder.append(ZERO_SHA_RAW);
    return true;
}

void Git::setLane(SCRef sha, FileHistory* fh) {

    Lanes* l = fh->lns;
    uint i = fh->firstFreeLane;
    QVector<QByteArray> ba;
    const ShaString& ss = toPersistentSha(sha, ba);
    const ShaVect& shaVec(fh->revOrder);

    for (uint cnt = shaVec.count(); i < cnt; ++i) {

        const ShaString& curSha = shaVec[i];
        Revision* r = const_cast<Revision*>(revLookup(curSha, fh));
        if (r->lanes.count() == 0)
            updateLanes(*r, *l, curSha);

        if (curSha == ss)
            break;
    }
    fh->firstFreeLane = ++i;
}

void Git::updateLanes(Revision& c, Lanes& lns, SCRef sha) {
// we could get third argument from c.sha(), but we are in fast path here
// and c.sha() involves a deep copy, so we accept a little redundancy

    if (lns.isEmpty())
        lns.init(sha);

    bool isDiscontinuity;
    bool isFork = lns.isFork(sha, isDiscontinuity);
    bool isMerge = (c.parentsCount() > 1);
    bool isInitial = (c.parentsCount() == 0);

    if (isDiscontinuity)
        lns.changeActiveLane(sha); // uses previous isBoundary state

    lns.setBoundary(c.isBoundary()); // update must be here

    if (isFork)
        lns.setFork(sha);
    if (isMerge)
        lns.setMerge(c.parents());
    if (c.isApplied)
        lns.setApplied();
    if (isInitial)
        lns.setInitial();

    lns.getLanes(c.lanes); // here lanes are snapshotted

    SCRef nextSha = (isInitial) ? "" : QString(c.parent(0));

    lns.nextParent(nextSha);

    if (c.isApplied)
        lns.afterApplied();
    if (isMerge)
        lns.afterMerge();
    if (isFork)
        lns.afterFork();
    if (lns.isBranch())
        lns.afterBranch();

//    QString tmp = "", tmp2;
//    for (uint i = 0; i < c.lanes.count(); i++) {
//    	tmp2.setNum(c.lanes[i]);
//    	tmp.append(tmp2 + "-");
//    }
//    qDebug("%s %s", tmp.toUtf8().data(), sha.toUtf8().data());
}

void Git::procFinished() {

    flushFileNames(fileLoader);
    filesLoadingPending = filesLoadingCurSha = "";
    emit fileNamesLoad(1, revsFiles.count() - filesLoadingStartOfs);
}

void Git::procReadyRead(const QByteArray& fileChunk) {

    if (filesLoadingPending.isEmpty())
        filesLoadingPending = fileChunk;
    else
        filesLoadingPending.append(fileChunk); // add to previous half lines

    RevFile* rf = NULL;
    if (!filesLoadingCurSha.isEmpty() && revsFiles.contains(toTempSha(filesLoadingCurSha)))
        rf = const_cast<RevFile*>(revsFiles[toTempSha(filesLoadingCurSha)]);

    int nextEOL = filesLoadingPending.indexOf('\n');
    int lastEOL = -1;
    while (nextEOL != -1) {

        SCRef line(filesLoadingPending.mid(lastEOL + 1, nextEOL - lastEOL - 1));
        if (line.at(0) != ':') {
            SCRef sha = line.left(40);
            if (!rf || sha != filesLoadingCurSha) { // new commit
                rf = new RevFile();
                revsFiles.insert(toPersistentSha(sha, revsFilesShaBackupBuf), rf);
                filesLoadingCurSha = sha;
                cacheNeedsUpdate = true;
            } else
                dbp("ASSERT: repeated sha %1 in file names loading", sha);
        } else // line.constref(0) == ':'
            parseDiffFormatLine(*rf, line, 1, fileLoader);

        lastEOL = nextEOL;
        nextEOL = filesLoadingPending.indexOf('\n', lastEOL + 1);
    }
    if (lastEOL != -1)
        filesLoadingPending.remove(0, lastEOL + 1);

    emit fileNamesLoad(2, revsFiles.count() - filesLoadingStartOfs);
}

void Git::flushFileNames(FileNamesLoader& fl) {

    if (!fl.rf)
        return;

    QByteArray& b = fl.rf->pathsIdx;
    QVector<int>& dirs = fl.rfDirs;

    b.clear();
    b.resize(2 * dirs.size() * sizeof(int));

    int* d = (int*)(b.data());

    for (int i = 0; i < dirs.size(); i++) {

        d[i] = dirs.at(i);
        d[dirs.size() + i] = fl.rfNames.at(i);
    }
    dirs.clear();
    fl.rfNames.clear();
    fl.rf = NULL;
}

void Git::appendFileName(RevFile& rf, SCRef name, FileNamesLoader& fl) {

    if (fl.rf != &rf) {
        flushFileNames(fl);
        fl.rf = &rf;
    }
    int idx = name.lastIndexOf('/') + 1;
    SCRef dr = name.left(idx);
    SCRef nm = name.mid(idx);

    QHash<QString, int>::const_iterator it(dirNamesMap.constFind(dr));
    if (it == dirNamesMap.constEnd()) {
        int idx = dirNamesVec.count();
        dirNamesMap.insert(dr, idx);
        dirNamesVec.append(dr);
        fl.rfDirs.append(idx);
    } else
        fl.rfDirs.append(*it);

    it = fileNamesMap.constFind(nm);
    if (it == fileNamesMap.constEnd()) {
        int idx = fileNamesVec.count();
        fileNamesMap.insert(nm, idx);
        fileNamesVec.append(nm);
        fl.rfNames.append(idx);
    } else
        fl.rfNames.append(*it);
}

void Git::updateDescMap(const Revision* r,uint idx, QHash<QPair<uint, uint>, bool>& dm,
                        QHash<uint, QVector<int> >& dv) {

    QVector<int> descVec;
    if (r->descRefsMaster != -1) {

        const Revision* tmp = revLookup(revData->revOrder[r->descRefsMaster]);
        const QVector<int>& nr = tmp->descRefs;

        for (int i = 0; i < nr.count(); i++) {

            if (!dv.contains(nr[i])) {
                dbp("ASSERT descendant for %1 not found", r->sha());
                return;
            }
            const QVector<int>& dvv = dv[nr[i]];

            // copy the whole vector instead of each element
            // in the first iteration of the loop below
            descVec = dvv; // quick (shared) copy

            for (int y = 0; y < dvv.count(); y++) {

                uint v = (uint)dvv[y];
                QPair<uint, uint> key = qMakePair(idx, v);
                QPair<uint, uint> keyN = qMakePair(v, idx);
                dm.insert(key, true);
                dm.insert(keyN, false);

                // we don't want duplicated entry, otherwise 'dvv' grows
                // greatly in repos with many tagged development branches
                if (i > 0 && !descVec.contains(v)) // i > 0 is rare, no
                    descVec.append(v);         // need to optimize
            }
        }
    }
    descVec.append(idx);
    dv.insert(idx, descVec);
}

void Git::mergeBranches(Revision* p, const Revision* r) {

    int r_descBrnMaster = (m_references.containsType(r->sha(), Reference::BRANCH | Reference::REMOTE_BRANCH) ? r->orderIdx : r->descBrnMaster);

    if (p->descBrnMaster == r_descBrnMaster || r_descBrnMaster == -1)
        return;

    // we want all the descendant branches, so just avoid duplicates
    const QVector<int>& src1 = revLookup(revData->revOrder[p->descBrnMaster])->descBranches;
    const QVector<int>& src2 = revLookup(revData->revOrder[r_descBrnMaster])->descBranches;
    QVector<int> dst(src1);
    for (int i = 0; i < src2.count(); i++)
        if (qFind(src1.constBegin(), src1.constEnd(), src2[i]) == src1.constEnd())
            dst.append(src2[i]);

    p->descBranches = dst;
    p->descBrnMaster = p->orderIdx;
}

void Git::mergeNearTags(bool down, Revision* p, const Revision* r, const QHash<QPair<uint, uint>, bool>& dm) {

    bool isTag = m_references.containsType(r->sha(), Reference::TAG);
    int r_descRefsMaster = isTag ? r->orderIdx : r->descRefsMaster;
    int r_ancRefsMaster = isTag ? r->orderIdx : r->ancRefsMaster;

    if (down && (p->descRefsMaster == r_descRefsMaster || r_descRefsMaster == -1))
        return;

    if (!down && (p->ancRefsMaster == r_ancRefsMaster || r_ancRefsMaster == -1))
        return;

    // we want the nearest tag only, so remove any tag
    // that is ancestor of any other tag in p U r
    const ShaVect& ro = revData->revOrder;
    const ShaString& sha1 = down ? ro[p->descRefsMaster] : ro[p->ancRefsMaster];
    const ShaString& sha2 = down ? ro[r_descRefsMaster] : ro[r_ancRefsMaster];
    const QVector<int>& src1 = down ? revLookup(sha1)->descRefs : revLookup(sha1)->ancRefs;
    const QVector<int>& src2 = down ? revLookup(sha2)->descRefs : revLookup(sha2)->ancRefs;
    QVector<int> dst(src1);

    for (int s2 = 0; s2 < src2.count(); s2++) {

        bool add = false;
        for (int s1 = 0; s1 < src1.count(); s1++) {

            if (src2[s2] == src1[s1]) {
                add = false;
                break;
            }
            QPair<uint, uint> key = qMakePair((uint)src2[s2], (uint)src1[s1]);

            if (!dm.contains(key)) { // could be empty if all tags are independent
                add = true; // could be an independent path
                continue;
            }
            add = (down && dm[key]) || (!down && !dm[key]);
            if (add)
                dst[s1] = -1; // mark for removing
            else
                break;
        }
        if (add)
            dst.append(src2[s2]);
    }
    QVector<int>& nearRefs = (down ? p->descRefs : p->ancRefs);
    int& nearRefsMaster = (down ? p->descRefsMaster : p->ancRefsMaster);

    nearRefs.clear();
    for (int s2 = 0; s2 < dst.count(); s2++)
        if (dst[s2] != -1)
            nearRefs.append(dst[s2]);

    nearRefsMaster = p->orderIdx;
}

void Git::indexTree() {

    const ShaVect& ro = revData->revOrder;
    if (ro.count() == 0)
        return;

    // we keep the pairs(x, y). Value is true if x is
    // ancestor of y or false if y is ancestor of x
    QHash<QPair<uint, uint>, bool> descMap;
    QHash<uint, QVector<int> > descVect;

    // walk down the tree from latest to oldest,
    // compute children and nearest descendants
    for (uint i = 0, cnt = ro.count(); i < cnt; i++) {

        uint type = m_references.containsType(ro[i]);
        bool isB = (type & (Reference::BRANCH | Reference::REMOTE_BRANCH));
        bool isT = (type & Reference::TAG);

        const Revision* r = revLookup(ro[i]);

        if (isB) {
            Revision* rr = const_cast<Revision*>(r);
            if (r->descBrnMaster != -1) {
                const ShaString& sha = ro[r->descBrnMaster];
                rr->descBranches = revLookup(sha)->descBranches;
            }
            rr->descBranches.append(i);
        }
        if (isT) {
            updateDescMap(r, i, descMap, descVect);
            Revision* rr = const_cast<Revision*>(r);
            rr->descRefs.clear();
            rr->descRefs.append(i);
        }
        for (uint y = 0; y < r->parentsCount(); y++) {

            Revision* p = const_cast<Revision*>(revLookup(r->parent(y)));
            if (p) {
                p->childs.append(i);

                if (p->descBrnMaster == -1)
                    p->descBrnMaster = isB ? r->orderIdx : r->descBrnMaster;
                else
                    mergeBranches(p, r);

                if (p->descRefsMaster == -1)
                    p->descRefsMaster = isT ? r->orderIdx : r->descRefsMaster;
                else
                    mergeNearTags(optGoDown, p, r, descMap);
            }
        }
    }
    // walk backward through the tree and compute nearest tagged ancestors
    for (int i = ro.count() - 1; i >= 0; i--) {

        const Revision* r = revLookup(ro[i]);
        bool isTag = m_references.containsType(ro[i], Reference::TAG);

        if (isTag) {
            Revision* rr = const_cast<Revision*>(r);
            rr->ancRefs.clear();
            rr->ancRefs.append(i);
        }
        for (int y = 0; y < r->childs.count(); y++) {

            Revision* c = const_cast<Revision*>(revLookup(ro[r->childs[y]]));
            if (c) {
                if (c->ancRefsMaster == -1)
                    c->ancRefsMaster = isTag ? r->orderIdx:r->ancRefsMaster;
                else
                    mergeNearTags(!optGoDown, c, r, descMap);
            }
        }
    }
}



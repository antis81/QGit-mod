/*
    Author: Marco Costalba (C) 2005-2007

    Copyright: See COPYING file that comes with this distribution

*/
#ifndef GIT_H
#define GIT_H

#include <QAbstractItemModel>
#include "exceptionmanager.h"
#include "common.h"
#include "domain.h"
#include "model/revision.h"
#include "model/reference.h"
#include "model/tagreference.h"

class Git;
#include "git/references.h"
#include "git/rungit_interface.h"
//#include "filehistory.h"

template <class, class> struct QPair;
class QRegExp;
class QTextCodec;
class Annotate;
class Cache;
class DataLoader;
class Domain;
class Git;
class Lanes;
class MyProcess;
class FileHistory;

// Need to add in class (conflict in git_startup.cpp)


class Git : public QObject, RunGitInterface
{
    Q_OBJECT
public:
    References m_references;

    explicit Git(QObject *parent);
    virtual ~Git();
    // used as self-documenting boolean parameters
    static const bool optFalse       = false;
    static const bool optSaveCache   = true;
    static const bool optGoDown      = true;
    static const bool optOnlyLoaded  = true;
    static const bool optDragDrop    = true;
    static const bool optFold        = true;
    static const bool optAmend       = true;
    static const bool optOnlyInIndex = true;
    static const bool optCreate      = true;

    struct TreeEntry
    {
        TreeEntry(SCRef n, SCRef s, SCRef t) : name(n), sha(s), type(t) {}
        bool operator<(const TreeEntry&) const;
        QString name;
        QString sha;
        QString type;
    };

    typedef QList<TreeEntry> TreeInfo;

    void setDefaultModel(FileHistory* fh) { revData = fh; }
    void checkEnvironment();
    void userInfo(SList info);
    const QStringList getGitConfigList(bool global);
    const QString getBaseDir(bool* c, SCRef wd, bool* ok = NULL, QString* gd = NULL);
    bool init(SCRef wd, bool range, const QStringList* args, bool overwrite, bool* quit);
    void stop(bool saveCache);
    void setThrowOnStop(bool b);
    bool isThrowOnStopRaised(int excpId, SCRef curContext);
    void setLane(SCRef sha, FileHistory* fh);
    Annotate* startAnnotate(FileHistory* fh, QObject* guiObj);
    const FileAnnotation* lookupAnnotation(Annotate* ann, SCRef sha);
    void cancelAnnotate(Annotate* ann);
    bool startFileHistory(SCRef sha, SCRef startingFileName, FileHistory* fh);
    void cancelDataLoading(const FileHistory* fh);
    void cancelProcess(MyProcess* p);
    bool isCommittingMerge() const { return isMergeHead; }
    bool isStGITStack() const { return isStGIT; }
    bool isSameFiles(SCRef tree1Sha, SCRef tree2Sha);
    static bool isImageFile(SCRef file);
    static bool isBinaryFile(SCRef file);
    bool isNothingToCommit();
    bool isUnknownFiles() const { return (workingDirInfo.otherFiles.count() > 0); }
    bool isTextHighlighter() const { return isTextHighlighterFound; }
    bool isMainHistory(const FileHistory* fh) { return (fh == revData); }
    MyProcess* getDiff(SCRef sha, QObject* receiver, SCRef diffToSha, bool combined);
    const QString getWorkDirDiff(SCRef fileName = "");
    MyProcess* getFile(SCRef fileSha, QObject* receiver, QByteArray* result, SCRef fileName);
    MyProcess* getHighlightedFile(SCRef fileSha, QObject* receiver, QString* result, SCRef fileName);
    const QString getFileSha(SCRef file, SCRef revSha);
    bool saveFile(SCRef fileSha, SCRef fileName, SCRef path);
    void getFileFilter(SCRef path, ShaSet& shaSet) const;
    bool getPatchFilter(SCRef exp, bool isRegExp, ShaSet& shaSet);
    const RevFile* getFiles(SCRef sha, SCRef sha2 = "", bool all = false, SCRef path = "");
    bool getTree(SCRef ts, TreeInfo& ti, bool wd, SCRef treePath);
    static const QString getLocalDate(SCRef gitDate);
    const QString getDesc(SCRef sha, QRegExp& slogRE, QRegExp& lLogRE, bool showH, FileHistory* fh);
    const QString getLastCommitMsg();
    const QString getNewCommitMsg();
    const QString getLaneParent(SCRef fromSHA, int laneNum);
    const QStringList getChilds(SCRef parent);
    const QStringList getNearTags(bool goDown, SCRef sha);
    const QStringList getDescendantBranches(SCRef sha, bool shaOnly = false);
    const QString getShortLog(SCRef sha);
    const Revision* revLookup(const ShaString& sha, const FileHistory* fh = NULL) const;
    const Revision* revLookup(SCRef sha, const FileHistory* fh = NULL) const;
    const QString getRevInfo(const ShaString &sha);
    const QStringList getAllRefNames(uint mask, bool onlyLoaded);
    const QStringList sortShaListByIndex(SCList shaList);
    void getWorkDirFiles(SList files, SList dirs, RevFile::StatusFlag status);
    QTextCodec* getTextCodec(bool* isGitArchive);
    bool formatPatch(SCList shaList, SCRef dirPath, SCRef remoteDir = "");
    bool updateIndex(SCList selFiles);
    bool commitFiles(SCList files, SCRef msg, bool amend);
    bool makeBranch(SCRef sha, SCRef branchName);
    bool makeTag(SCRef sha, SCRef tag, SCRef msg);
    bool deleteTag(const QString &tagName);
    bool checkout(SCRef sha);
    bool applyPatchFile(SCRef patchPath, bool fold, bool sign);
    bool resetCommits(int parentDepth);
    bool stgCommit(SCList selFiles, SCRef msg, SCRef patchName, bool fold);
    bool stgPush(const ShaString& sha);
    bool stgPop(const ShaString& sha);
    void setTextCodec(QTextCodec* tc);
    void addExtraFileInfo(QString* rowName, SCRef sha, SCRef diffToSha, bool allMergeFiles);
    void removeExtraFileInfo(QString* rowName);
    void formatPatchFileHeader(QString* rowName, SCRef sha, SCRef dts, bool cmb, bool all);
    int findFileIndex(const RevFile& rf, SCRef name);

    const QString filePath(const RevFile& rf, uint i) const
    {
        return dirNamesVec[rf.dirAt(i)] + fileNamesVec[rf.nameAt(i)];
    }

    void setCurContext(Domain* d) { curDomain = d; }
    Domain* curContext() const { return curDomain; }
    bool updateCurrentBranch();
    QString& currentBranch();
    static const QString escape(QString s);
    static const QString unescape(QString s);

    // FIXME: this is the public delegate for the private function. This will be removed in the future.
    bool runGit(const QString& cmd, QString* out = NULL, QObject* rcv = NULL, const QString& buf = "")
    {
        return run(cmd, out, rcv, buf);
    }

signals:
    void newRevsAdded(const FileHistory*, const QVector<ShaString>&);
    void loadCompleted(const FileHistory*, const QString&);
    void cancelLoading(const FileHistory*);
    void cancelAllProcesses();
    void annotateReady(Annotate*, bool, const QString&);
    void fileNamesLoad(int, int);
    void changeFont(const QFont&);

public slots:
    void procReadyRead(const QByteArray&);
    void procFinished();

private slots:
    void loadFileCache();
    void loadFileNames();
    void on_runAsScript_eof();
    void on_getHighlightedFile_eof();
    void on_newDataReady(const FileHistory*);
    void on_loaded(FileHistory*, ulong,int,bool,const QString&,const QString&);

private:
    friend class MainImpl;
    friend class DataLoader;
    friend class ConsoleImpl;
    friend class RevsView;

    struct WorkingDirInfo
    {
        void clear() { diffIndex = diffIndexCached = ""; otherFiles.clear(); }
        QString diffIndex;
        QString diffIndexCached;
        QStringList otherFiles;
    };

    WorkingDirInfo workingDirInfo;

    struct LoadArguments
    { // used to pass arguments to init2()
        QStringList args;
        bool filteredLoading;
        QStringList filterList;
    };

    LoadArguments loadArguments;

    struct FileNamesLoader
    {
        FileNamesLoader() : rf(NULL) {}

        RevFile* rf;
        QVector<int> rfDirs;
        QVector<int> rfNames;
    };

    FileNamesLoader fileLoader;

    void init2();
    bool run(SCRef cmd, QString* out = NULL, QObject* rcv = NULL, SCRef buf = "");
    bool run(QByteArray* runOutput, SCRef cmd, QObject* rcv = NULL, SCRef buf = "");
    MyProcess* runAsync(SCRef cmd, QObject* rcv, SCRef buf = "");
    MyProcess* runAsScript(SCRef cmd, QObject* rcv = NULL, SCRef buf = "");
    const QStringList getArgs(bool* quit, bool repoChanged);
    bool getRefs();
    void clearRevs();
    void clearFileNames();
    bool startRevList(SCList args, FileHistory* fh);
    bool startUnappliedList();
    bool startParseProc(SCList initCmd, FileHistory* fh, SCRef buf);
    bool tryFollowRenames(FileHistory* fh);
    bool populateRenamedPatches(SCRef sha, SCList nn, FileHistory* fh, QStringList* on, bool bt);
    bool filterEarlyOutputRev(FileHistory* fh, Revision* rev);
    int addChunk(FileHistory* fh, const QByteArray& ba, int ofs);
    void parseDiffFormat(RevFile& rf, SCRef buf, FileNamesLoader& fl);
    void parseDiffFormatLine(RevFile& rf, SCRef line, int parNum, FileNamesLoader& fl);
    void getDiffIndex();
    Revision* fakeRevData(SCRef sha, SCList parents, SCRef author, SCRef date, SCRef log,
                         SCRef longLog, SCRef patch, int idx, FileHistory* fh);
    const Revision* fakeWorkDirRev(SCRef parent, SCRef log, SCRef longLog, int idx, FileHistory* fh);
    const RevFile* fakeWorkDirRevFile(const WorkingDirInfo& wd);
    bool copyDiffIndex(FileHistory* fh, SCRef parent);
    const RevFile* insertNewFiles(SCRef sha, SCRef data);
    const RevFile* getAllMergeFiles(const Revision* r);
    bool runDiffTreeWithRenameDetection(SCRef runCmd, QString* runOutput);
    bool isParentOf(SCRef par, SCRef child);
    bool isTreeModified(SCRef sha);
    void indexTree();
    void updateDescMap(const Revision* r, uint i, QHash<QPair<uint, uint>,bool>& dm,
                       QHash<uint, QVector<int> >& dv);
    void mergeNearTags(bool down, Revision* p, const Revision* r, const QHash<QPair<uint, uint>, bool>&dm);
    void mergeBranches(Revision* p, const Revision* r);
    void updateLanes(Revision& c, Lanes& lns, SCRef sha);
    bool mkPatchFromWorkDir(SCRef msg, SCRef patchFile, SCList files);
    const QStringList getOthersFiles();
    const QStringList getOtherFiles(SCList selFiles, bool onlyInIndex);
    const QString getNewestFileName(SCList args, SCRef fileName);
    static const QString colorMatch(SCRef txt, QRegExp& regExp);
    void appendFileName(RevFile& rf, SCRef name, FileNamesLoader& fl);
    void flushFileNames(FileNamesLoader& fl);
    void populateFileNamesMap();
    const QString formatList(SCList sl, SCRef name, bool inOneLine = true);
    static const QString quote(SCRef nm);
    static const QString quote(SCList sl);
    static const QStringList noSpaceSepHack(SCRef cmd);
    void removeDeleted(SCList selFiles);
    void setStatus(RevFile& rf, SCRef rowSt);
    void setExtStatus(RevFile& rf, SCRef rowSt, int parNum, FileNamesLoader& fl);
    void appendNamesWithId(QStringList& names, SCRef sha, SCList data, bool onlyLoaded);
    EM_DECLARE(exGitStopped);

    Domain* curDomain;
    QString workDir; // workDir is always without trailing '/'
    QString gitDir;
    QString filesLoadingPending;
    QString filesLoadingCurSha;
    int filesLoadingStartOfs;
    bool cacheNeedsUpdate;
    bool errorReportingEnabled;
    bool isMergeHead;
    bool isStGIT;
    bool isGIT;
    bool isTextHighlighterFound;
    bool loadingUnAppliedPatches;
    bool fileCacheAccessed;
    QString firstNonStGitPatch;
    RevFileMap revsFiles;
    QVector<QByteArray> revsFilesShaBackupBuf;
    // TODO: move to References
    QVector<QByteArray> shaBackupBuf;
    StrVect fileNamesVec;
    StrVect dirNamesVec;
    QHash<QString, int> fileNamesMap; // quick lookup file name
    QHash<QString, int> dirNamesMap;  // quick lookup directory name
    FileHistory* revData;
    QString m_currentBranch;
};

#endif

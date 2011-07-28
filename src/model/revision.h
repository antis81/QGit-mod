#ifndef REVISION_H
#define REVISION_H

#include <QByteArray>
#include <QString>
#include <QHash>
#include <QVector>
#include <QStringList>
#include "shastring.h"
#include "lanes.h" // FIXME: model or view?

class Revision
{
    // prevent implicit C++ compiler defaults
    Revision();
    Revision(const Revision&);
    Revision& operator=(const Revision&);
public:
    Revision(const QByteArray& b, uint s, int idx, int* next, bool withDiff)
        : orderIdx(idx), ba(b), start(s) {

        indexed = isDiffCache = isApplied = isUnApplied = false;
        descRefsMaster = ancRefsMaster = descBrnMaster = -1;
        *next = indexData(true, withDiff);
    }
    bool isDiffCache; //
    bool isApplied;   //
    bool isUnApplied; // put here to optimize padding
    bool isBoundary() const { return (ba.at(shaStart - 1) == '-'); }
    uint parentsCount() const { return parentsCnt; }
    const ShaString parent(int idx) const;
    const QStringList parents() const;
    const ShaString sha() const { return ShaString(ba.constData() + shaStart); }
    const QString committer() const { setup(); return mid(comStart, autStart - comStart - 1); }
    const QString author() const { setup(); return mid(autStart, autDateStart - autStart - 1); }
    const QString authorDate() const { setup(); return mid(autDateStart, 10); }
    const QString shortLog() const { setup(); return mid(sLogStart, sLogLen); }
    const QString longLog() const { setup(); return mid(lLogStart, lLogLen); }
    const QString diff() const { setup(); return mid(diffStart, diffLen); }

    QVector<LaneType> lanes;
    QVector<int> childs;
    QVector<int> descRefs;     // list of descendant refs index, normally tags
    QVector<int> ancRefs;      // list of ancestor refs index, normally tags
    QVector<int> descBranches; // list of descendant branches index
    int descRefsMaster; // in case of many Rev have the same descRefs, ancRefs or
    int ancRefsMaster;  // descBranches these are stored only once in a Rev pointed
    int descBrnMaster;  // by corresponding index xxxMaster
    int orderIdx;
private:
    inline void setup() const { if (!indexed) indexData(false, false); }
    int indexData(bool quick, bool withDiff) const;
    const QString mid(int start, int len) const;
    const QString midSha(int start, int len) const;

    const QByteArray& ba; // reference here!
    const int start;
    // FIXME: Soo many operators in one line
    // {
    mutable int parentsCnt, shaStart, comStart, autStart, autDateStart;
    mutable int sLogStart, sLogLen, lLogStart, lLogLen, diffStart, diffLen;
    // }
    mutable bool indexed;
};

// FIXME: include in class
typedef QHash<ShaString, const Revision*> RevMap;  // faster then a map

#endif // REVISION_H

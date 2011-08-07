/*
    Description: history graph computation

    Author: Marco Costalba (C) 2005-2007

    Copyright: See COPYING file that comes with this distribution

*/
#include <QStringList>
#include "common.h"
#include "lanes.h"

#define IS_NODE(x) (x == NODE || x == NODE_R || x == NODE_L)

using namespace QGit;

void Lanes::init(const QString& expectedSha)
{
    clear();
    activeLane = 0;
    setBoundary(false);
    add(LANE_BRANCH, expectedSha, activeLane);
}

void Lanes::clear()
{
    typeVec.clear();
    nextShaVec.clear();
}

void Lanes::setBoundary(bool b)
{
// changes the state so must be called as first one

    NODE   = b ? LANE_BOUNDARY_C : LANE_MERGE_FORK;
    NODE_R = b ? LANE_BOUNDARY_R : LANE_MERGE_FORK_R;
    NODE_L = b ? LANE_BOUNDARY_L : LANE_MERGE_FORK_L;
    boundary = b;

    if (boundary)
        typeVec[activeLane] = LANE_BOUNDARY;
}

bool Lanes::isFork(const QString& sha, bool& isDiscontinuity)
{
    int pos = findNextSha(sha, 0);
    isDiscontinuity = (activeLane != pos);
    if (pos == -1) // new branch case
        return false;

    return (findNextSha(sha, pos + 1) != -1);
/*
    int cnt = 0;
    while (pos != -1) {
        cnt++;
        pos = findNextSha(sha, pos + 1);
//        if (isDiscontinuity)
//            isDiscontinuity = (activeLane != pos);
    }
    return (cnt > 1);
*/
}

void Lanes::setFork(const QString& sha)
{
    int rangeStart, rangeEnd, idx;
    rangeStart = rangeEnd = idx = findNextSha(sha, 0);

    while (idx != -1) {
        rangeEnd = idx;
        typeVec[idx] = LANE_TAIL;
        idx = findNextSha(sha, idx + 1);
    }
    typeVec[activeLane] = NODE;

    LaneType& startT = typeVec[rangeStart];
    LaneType& endT = typeVec[rangeEnd];

    if (startT == NODE)
        startT = NODE_L;

    if (endT == NODE)
        endT = NODE_R;

    if (startT == LANE_TAIL)
        startT = LANE_TAIL_L;

    if (endT == LANE_TAIL)
        endT = LANE_TAIL_R;

    for (int i = rangeStart + 1; i < rangeEnd; i++) {

        LaneType& t = typeVec[i];

        if (t == LANE_NOT_ACTIVE)
            t = LANE_CROSS;

        else if (t == LANE_EMPTY)
            t = LANE_CROSS_EMPTY;
    }
}

void Lanes::setMerge(const QStringList& parents)
{
// setFork() must be called before setMerge()

    if (boundary)
        return; // handle as a simple active line

    LaneType& t = typeVec[activeLane];
    bool wasFork   = (t == NODE);
    bool wasFork_L = (t == NODE_L);
    bool wasFork_R = (t == NODE_R);
    bool startJoinWasACross = false, endJoinWasACross = false;

    t = NODE;

    int rangeStart = activeLane, rangeEnd = activeLane;
    QStringList::const_iterator it(parents.constBegin());
    for (++it; it != parents.constEnd(); ++it) { // skip first parent

        int idx = findNextSha(*it, 0);
        if (idx != -1) {

            if (idx > rangeEnd) {

                rangeEnd = idx;
                endJoinWasACross = typeVec[idx] == LANE_CROSS;
            }

            if (idx < rangeStart) {

                rangeStart = idx;
                startJoinWasACross = typeVec[idx] == LANE_CROSS;
            }

            typeVec[idx] = LANE_JOIN;
        } else
            rangeEnd = add(LANE_HEAD, *it, rangeEnd + 1);
    }
    LaneType& startT = typeVec[rangeStart];
    LaneType& endT = typeVec[rangeEnd];

    if (startT == NODE && !wasFork && !wasFork_R)
        startT = NODE_L;

    if (endT == NODE && !wasFork && !wasFork_L)
        endT = NODE_R;

    if (startT == LANE_JOIN && !startJoinWasACross)
        startT = LANE_JOIN_L;

    if (endT == LANE_JOIN && !endJoinWasACross)
        endT = LANE_JOIN_R;

    if (startT == LANE_HEAD)
        startT = LANE_HEAD_L;

    if (endT == LANE_HEAD)
        endT = LANE_HEAD_R;

    for (int i = rangeStart + 1; i < rangeEnd; i++) {

        LaneType& t = typeVec[i];

        if (t == LANE_NOT_ACTIVE)
            t = LANE_CROSS;

        else if (t == LANE_EMPTY)
            t = LANE_CROSS_EMPTY;

        else if (t == LANE_TAIL_R || t == LANE_TAIL_L)
            t = LANE_TAIL;
    }
}

void Lanes::setInitial()
{
    LaneType& t = typeVec[activeLane];
    if (!IS_NODE(t) && t != LANE_APPLIED)
        t = (boundary ? LANE_BOUNDARY : LANE_INITIAL);
}

void Lanes::setApplied()
{
    // applied patches are not merges, nor forks
    typeVec[activeLane] = LANE_APPLIED; // TODO test with boundaries
}

void Lanes::changeActiveLane(const QString& sha)
{
    LaneType& t = typeVec[activeLane];
    if (t == LANE_INITIAL || isBoundary(t))
        t = LANE_EMPTY;
    else
        t = LANE_NOT_ACTIVE;

    int idx = findNextSha(sha, 0); // find first sha
    if (idx != -1)
        typeVec[idx] = LANE_ACTIVE; // called before setBoundary()
    else
        idx = add(LANE_BRANCH, sha, activeLane); // new branch

    activeLane = idx;
}

void Lanes::afterMerge()
{
    if (boundary)
        return; // will be reset by changeActiveLane()

    for (int i = 0; i < typeVec.count(); i++) {

        LaneType& t = typeVec[i];

        if (isHead(t) || isJoin(t) || t == LANE_CROSS)
            t = LANE_NOT_ACTIVE;

        else if (t == LANE_CROSS_EMPTY)
            t = LANE_EMPTY;

        else if (IS_NODE(t))
            t = LANE_ACTIVE;
    }
}

void Lanes::afterFork()
{
    for (int i = 0; i < typeVec.count(); i++) {

        LaneType& t = typeVec[i];

        if (t == LANE_CROSS)
            t = LANE_NOT_ACTIVE;

        else if (isTail(t) || t == LANE_CROSS_EMPTY)
            t = LANE_EMPTY;

        if (!boundary && IS_NODE(t))
            t = LANE_ACTIVE; // boundary will be reset by changeActiveLane()
    }
    while (typeVec.last() == LANE_EMPTY) {
        typeVec.pop_back();
        nextShaVec.pop_back();
    }
}

bool Lanes::isBranch()
{
    return (typeVec[activeLane] == LANE_BRANCH);
}

void Lanes::afterBranch()
{
    typeVec[activeLane] = LANE_ACTIVE; // TODO test with boundaries
}

void Lanes::afterApplied()
{
    typeVec[activeLane] = LANE_ACTIVE; // TODO test with boundaries
}

void Lanes::nextParent(const QString& sha)
{
    nextShaVec[activeLane] = (boundary ? "" : sha);
}

int Lanes::findNextSha(const QString& next, int pos)
{
    for (int i = pos; i < nextShaVec.count(); i++)
        if (nextShaVec[i] == next)
            return i;
    return -1;
}

int Lanes::findType(LaneType type, int pos)
{
    for (int i = pos; i < typeVec.count(); i++)
        if (typeVec[i] == type)
            return i;
    return -1;
}

int Lanes::add(LaneType type, const QString& next, int pos)
{
    // first check empty lanes starting from pos
    if (pos < (int)typeVec.count()) {
        pos = findType(LANE_EMPTY, pos);
        if (pos != -1) {
            typeVec[pos] = type;
            nextShaVec[pos] = next;
            return pos;
        }
    }
    // if all lanes are occupied add a new lane
    typeVec.append(type);
    nextShaVec.append(next);
    return typeVec.count() - 1;
}

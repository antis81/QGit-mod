/*
    Author: Marco Costalba (C) 2005-2007

    Copyright: See COPYING file that comes with this distribution

*/
#ifndef LANES_H
#define LANES_H

#include <QString>
#include <QVector>
#include <QStringList>

// graph elements
enum LaneType {
    LANE_UNDEFINED = -1,
    LANE_EMPTY = 0,
    LANE_ACTIVE,
    LANE_NOT_ACTIVE,
    LANE_MERGE_FORK,
    LANE_MERGE_FORK_R,
    LANE_MERGE_FORK_L,
    LANE_JOIN,
    LANE_JOIN_R,
    LANE_JOIN_L,
    LANE_HEAD,
    LANE_HEAD_R,
    LANE_HEAD_L,
    LANE_TAIL,
    LANE_TAIL_R,
    LANE_TAIL_L,
    LANE_CROSS,
    LANE_CROSS_EMPTY,
    LANE_INITIAL,
    LANE_BRANCH,
    LANE_UNAPPLIED,
    LANE_APPLIED,
    LANE_BOUNDARY,
    LANE_BOUNDARY_C, // corresponds to MERGE_FORK
    LANE_BOUNDARY_R, // corresponds to MERGE_FORK_R
    LANE_BOUNDARY_L, // corresponds to MERGE_FORK_L

    LANE_TYPES_NUM
};
//
//  At any given time, the Lanes class represents a single revision (row) of the history graph.
//  The Lanes class contains a vector of the sha1 hashes of the next commit to appear in each lane (column).
//  The Lanes class also contains a vector used to decide which glyph to draw on the history graph.
//
//  For each revision (row) (from recent (top) to ancient past (bottom)), the Lanes class is updated, and the
//  current revision (row) of glyphs is saved elsewhere (via getLanes()).
//
//  The ListView class is responsible for rendering the glyphs.
//

namespace QGit {

// graph helpers
inline bool isHead(LaneType x) { return (x == LANE_HEAD || x == LANE_HEAD_R || x == LANE_HEAD_L); }
inline bool isTail(LaneType x) { return (x == LANE_TAIL || x == LANE_TAIL_R || x == LANE_TAIL_L); }
inline bool isJoin(LaneType x) { return (x == LANE_JOIN || x == LANE_JOIN_R || x == LANE_JOIN_L); }
inline bool isFreeLane(LaneType x) { return (x == LANE_NOT_ACTIVE || x == LANE_CROSS || isJoin(x)); }
inline bool isBoundary(LaneType x) { return (x == LANE_BOUNDARY || x == LANE_BOUNDARY_C ||
                                        x == LANE_BOUNDARY_R || x == LANE_BOUNDARY_L); }
inline bool isMerge(LaneType x) { return (x == LANE_MERGE_FORK || x == LANE_MERGE_FORK_R ||
                                     x == LANE_MERGE_FORK_L || isBoundary(x)); }
inline bool isActive(LaneType x) { return (x == LANE_ACTIVE || x == LANE_INITIAL || x == LANE_BRANCH ||                                      isMerge(x)); }

}

class Lanes
{
public:
    Lanes() {} // init() will setup us later, when data is available
    bool isEmpty() { return typeVec.empty(); }
    void init(const QString& expectedSha);
    void clear();
    bool isFork(const QString& sha, bool& isDiscontinuity);
    void setBoundary(bool isBoundary);
    void setFork(const QString& sha);
    void setMerge(const QStringList& parents);
    void setInitial();
    void setApplied();
    void changeActiveLane(const QString& sha);
    void afterMerge();
    void afterFork();
    bool isBranch();
    void afterBranch();
    void afterApplied();
    void nextParent(const QString &sha);
    void getLanes(QVector<LaneType> &ln) { ln = typeVec; } // O(1) vector is implicitly shared

private:
    int findNextSha(const QString &next, int pos);
    int findType(LaneType type, int pos);
    int add(LaneType type, const QString &next, int pos);

    int activeLane;
    QVector<LaneType> typeVec;  // Describes which glyphs should be drawn.
    QVector<QString> nextShaVec;  // The sha1 hashes of the next commit to appear in each lane (column).
    bool boundary;
    LaneType NODE, NODE_L, NODE_R;
};

#endif

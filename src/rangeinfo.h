#ifndef RANGEINFO_H
#define RANGEINFO_H

class RangeInfo
{
public:
    RangeInfo() { clear(); }
    RangeInfo(int s, int e, bool m) : start(s), end(e), modified(m) {}
    void clear() { start = end = 0; modified = false; }
    int start, end; // ranges count file lines from 1 like patches diffs
    bool modified;
};

#endif // RANGEINFO_H

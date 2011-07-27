#include "revision.h"
#include "common.h"

const QString Revision::mid(int start, int len) const {

    // warning no sanity check is done on arguments
    const char* data = ba.constData();
    return QString::fromAscii(data + start, len);
}

const QString Revision::midSha(int start, int len) const {

    // warning no sanity check is done on arguments
    const char* data = ba.constData();
    return QString::fromLatin1(data + start, len); // faster then formAscii
}

const ShaString Revision::parent(int idx) const {

    return ShaString(ba.constData() + shaStart + 41 + 41 * idx);
}

const QStringList Revision::parents() const {

    QStringList p;
    int idx = shaStart + 41;

    for (int i = 0; i < parentsCnt; i++) {
        p.append(midSha(idx, 40));
        idx += 41;
    }
    return p;
}

int Revision::indexData(bool quick, bool withDiff) const {
/*
  This is what 'git log' produces:

    - a possible one line with "Final output:\n" in case of --early-output option
    - one line with "log size" + len of this record
    - one line with boundary info + sha + an arbitrary amount of parent's sha
    - one line with committer name + e-mail
    - one line with author name + e-mail
    - one line with author date as unix timestamp
    - zero or more non blank lines with other info, as the encoding FIXME
    - one blank line
    - zero or one line with log title
    - zero or more lines with log message
    - zero or more lines with diff content (only for file history)
    - a terminating '\0'
*/
    const int last = ba.size() - 1;
    int logSize = 0, idx = start;
    int logEnd, revEnd;

    // direct access is faster then QByteArray.at()
    const char* data = ba.constData();
    char* fixup = const_cast<char*>(data); // to build '\0' terminating strings

    if (start + 42 > last) // at least sha + 'X' + 'X' + '\n' + must be present
        return -1;

    if (data[start] == 'F') // "Final output", let caller handle this
        return (ba.indexOf('\n', start) != -1 ? -2 : -1);

    // parse log size if present
    if (data[idx] == 'l') { // 'log size xxx\n'

        idx += 9; // move idx to beginning of log size
        int tmp;
        while ((tmp = data[idx++]) != '\n')
            logSize = logSize * 10 + tmp - 48;
    }
    // idx points to the boundary information
    if (++idx + 42 > last)
        return -1;

    shaStart = idx;

    // ok, now shaStart is valid but msgSize
    // could be still 0 if not available
    logEnd = shaStart - 1 + logSize;
    if (logEnd > last)
        return -1;

    idx += 40; // now points to 'X' place holder

    fixup[idx] = '\0'; // we want sha to be a '\0' terminated ascii string

    parentsCnt = 0;

    if (data[idx + 2] == '\n') // initial revision
        ++idx;
    else do {
        parentsCnt++;
        idx += 41;

        if (idx + 1 >= last)
            break;

        fixup[idx] = '\0'; // we want parents '\0' terminated

    } while (data[idx + 1] != '\n');

    ++idx; // now points to the trailing '\n' of sha line

    // check for !msgSize
    if (withDiff || !logSize) {

        revEnd = (logEnd > idx) ? logEnd - 1: idx;
        do { // search for "\n\0" to handle (rare) cases of '\0'
             // in content, see c42012 and bb8d8a6 in Linux tree
            revEnd = ba.indexOf('\0', revEnd + 1);
            if (revEnd == -1)
                return -1;

        } while (data[revEnd - 1] != '\n');

    } else
        revEnd = logEnd;

    if (revEnd > last) // after this point we know to have the whole record
        return -1;

    // ok, now revEnd is valid but logEnd could be not if !logSize
    // in case of diff we are sure content will be consumed so
    // we go all the way
    if (quick && !withDiff)
        return ++revEnd;

    comStart = ++idx;
    idx = ba.indexOf('\n', idx); // committer line end
    if (idx == -1) {
        dbs("ASSERT in indexData: unexpected end of data");
        return -1;
    }

    autStart = ++idx;
    idx = ba.indexOf('\n', idx); // author line end
    if (idx == -1) {
        dbs("ASSERT in indexData: unexpected end of data");
        return -1;
    }
    autDateStart = ++idx;
    idx += 11; // date length + trailing '\n'

    diffStart = diffLen = 0;
    if (withDiff) {
        diffStart = logSize ? logEnd : ba.indexOf("\ndiff ", idx);

        if (diffStart != -1 && diffStart < revEnd)
            diffLen = revEnd - ++diffStart;
        else
            diffStart = 0;
    }
    if (!logSize)
        logEnd = diffStart ? diffStart : revEnd;

    // ok, now logEnd is valid and we can handle the log
    sLogStart = idx;

    if (logEnd < sLogStart) { // no shortlog no longLog

        sLogStart = sLogLen = 0;
        lLogStart = lLogLen = 0;
    } else {
        lLogStart = ba.indexOf('\n', sLogStart);
        if (lLogStart != -1 && lLogStart < logEnd - 1) {

            sLogLen = lLogStart - sLogStart; // skip sLog trailing '\n'
            lLogLen = logEnd - lLogStart; // include heading '\n' in long log

        } else { // no longLog
            sLogLen = logEnd - sLogStart;
            if (data[sLogStart + sLogLen - 1] == '\n')
                sLogLen--; // skip trailing '\n' if any

            lLogStart = lLogLen = 0;
        }
    }
    indexed = true;
    return ++revEnd;
}

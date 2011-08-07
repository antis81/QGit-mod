#ifndef PATCHCONTENTFINDSUPPORT_H
#define PATCHCONTENTFINDSUPPORT_H

#include "findsupport.h"
#include <QPlainTextEdit>
#include "patchcontent.h"
//class PatchContent;

class PatchContentFindSupport : public FindSupport
{
public:
    PatchContentFindSupport(PatchContent* patchContent);

    bool find();
    bool findNext();

    void setText(QString text, bool re);
    void setText(QString text);
private:
    friend class PatchContent;

    PatchContent* m_patchContent;
    bool isRegExp;
    QRegExp pickAxeRE;

    struct MatchSelection
    {
        int from;
        int to;
    };

    typedef QVector<MatchSelection> Matches;

    Matches matches;

    QTextCursor findNextMatch(const QTextDocument* document, QTextCursor& cursor);
    bool computeMatches();
};

#endif // PATCHCONTENTFINDSUPPORT_H

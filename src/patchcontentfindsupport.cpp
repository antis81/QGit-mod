#include "patchcontentfindsupport.h"

PatchContentFindSupport::PatchContentFindSupport(PatchContent* patchContent)
        : m_patchContent(patchContent)
{
    pickAxeRE.setMinimal(true);
    pickAxeRE.setCaseSensitivity(Qt::CaseInsensitive);
}

void PatchContentFindSupport::setText(QString text)
{
    FindSupport::setText(text);
    this->find();
}

void PatchContentFindSupport::setText(QString text, bool re)
{
    isRegExp = re;
    setText(text);
}


bool PatchContentFindSupport::find()
{
    return computeMatches();
}

bool PatchContentFindSupport::findNext()
{
    return false;
}

bool PatchContentFindSupport::computeMatches()
{
    matches.clear();
    if (!m_text || m_text->isEmpty()) {
        return false;
    }

    if (isRegExp) {
        pickAxeRE.setPattern(*m_text);
    }

    QTextDocument* document = m_patchContent->document();

    QTextCursor cursor;
    while (!(cursor = findNextMatch(document, cursor)).isNull()) {

        matches.append(MatchSelection());
        MatchSelection& s = matches.last();

        s.from = cursor.selectionStart();
        s.to = cursor.selectionEnd();
    }
    return !matches.isEmpty();
}

QTextCursor PatchContentFindSupport::findNextMatch(const QTextDocument* document, QTextCursor& cursor)
{
    return isRegExp ? document->find(pickAxeRE, cursor) : document->find(*m_text, cursor);
}


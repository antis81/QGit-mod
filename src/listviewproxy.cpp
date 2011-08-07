#include "listviewproxy.h"

using   namespace QGit;

ListViewProxy::ListViewProxy(QObject* p, Domain * dm, Git * g) : QSortFilterProxyModel(p) {

    d = dm;
    git = g;
    colNum = 0;
    isHighLight = false;
    setDynamicSortFilter(false);
}

bool ListViewProxy::isMatch(SCRef sha) const {

    if (colNum == SHA_MAP_COL)
        // in this case shaMap contains all good sha to search for
        return shaSet.contains(sha);

    const Revision* r = git->revLookup(sha);
    if (!r) {
        dbp("ASSERT in ListViewFilter::isMatch, sha <%1> not found", sha);
        return false;
    }
    QString target;
    if (colNum == LOG_COL)
        target = r->shortLog();
    else if (colNum == AUTH_COL)
        target = r->author();
    else if (colNum == LOG_MSG_COL)
        target = r->longLog();
    else if (colNum == COMMIT_COL)
        target = sha;

    // wildcard search, case insensitive
    return (target.contains(filter));
}

bool ListViewProxy::isMatch(int source_row) const {

    FileHistory* fh = d->model();
    if (fh->rowCount() <= source_row) // FIXME required to avoid an ASSERT in d->isMatch()
        return false;

    bool extFilter = (colNum == -1);
    return ((!extFilter && isMatch(fh->sha(source_row)))
          ||( extFilter && d->isMatch(fh->sha(source_row))));
}

bool ListViewProxy::isHighlighted(int row) const {

    // FIXME row == source_row only because when
    // higlights the rows are not hidden
    return (isHighLight && isMatch(row));
}

bool ListViewProxy::filterAcceptsRow(int source_row, const QModelIndex&) const {

    return (isHighLight || isMatch(source_row));
}

int ListViewProxy::setFilter(bool isOn, bool h, SCRef fl, int cn, ShaSet* s) {

    filter = QRegExp(fl, Qt::CaseInsensitive, QRegExp::Wildcard);
    colNum = cn;
    if (s)
        shaSet = *s;

    // isHighlighted() is called also when filter is off,
    // so reset 'isHighLight' flag in that case
    isHighLight = h && isOn;

    ListView* lv = static_cast<ListView*>(parent());
    FileHistory* fh = d->model();

    if (!isOn && sourceModel()){
        lv->setModel(fh);
        setSourceModel(NULL);

    } else if (isOn && !isHighLight) {
        setSourceModel(fh); // trigger a rows scanning
        lv->setModel(this);
    }
    return (sourceModel() ? rowCount() : 0);
}

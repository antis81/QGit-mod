/*
    Description: qgit revision list view

    Author: Marco Costalba (C) 2005-2007

    Copyright: See COPYING file that comes with this distribution

*/
#include <QApplication>
#include <QHeaderView>
#include <QMouseEvent>
#include <QPainter>
#include <QPixmap>
#include <QShortcut>
#include "domain.h"
#include "git.h"
#include "listview.h"
#include "filehistory.h"
#include "listviewdelegate.h"


using namespace QGit;

ListView::ListView(QWidget* parent) : QTreeView(parent), d(NULL), git(NULL), fh(NULL), lp(NULL)
{
}

void ListView::setup(Domain* dm, Git* g)
{
    d = dm;
    git = g;
    fh = d->model();
    st = &(d->st);
    filterNextContextMenuRequest = false;

    setFont(QGit::STD_FONT);

    // create ListViewProxy unplugged, will be plug
    // to the model only when filtering is needed
    lp = new ListViewProxy(this, d, git);
    setModel(fh);

    ListViewDelegate* lvd = new ListViewDelegate(git, lp, this);
    lvd->setLaneHeight(fontMetrics().height());
    setItemDelegate(lvd);

    setupGeometry(); // after setting delegate

    // shortcuts are activated only if widget is visible, this is good
    new QShortcut(Qt::Key_Up,   this, SLOT(on_keyUp()));
    new QShortcut(Qt::Key_Down, this, SLOT(on_keyDown()));

    connect(lvd, SIGNAL(updateView()), viewport(), SLOT(update()));

    connect(this, SIGNAL(diffTargetChanged(int)), lvd, SLOT(diffTargetChanged(int)));

    connect(this, SIGNAL(customContextMenuRequested(const QPoint&)),
            this, SLOT(on_customContextMenuRequested(const QPoint&)));
}

ListView::~ListView()
{
    git->cancelDataLoading(fh); // non blocking
}

const QString ListView::sha(int row) const
{
    if (!lp->sourceModel()) // unplugged
        return fh->sha(row);

    QModelIndex idx = lp->mapToSource(lp->index(row, 0));
    return fh->sha(idx.row());
}

int ListView::row(SCRef sha) const
{
    if (!lp->sourceModel()) // unplugged
        return fh->row(sha);

    int row = fh->row(sha);
    QModelIndex idx = fh->index(row, 0);
    return lp->mapFromSource(idx).row();
}

void ListView::setupGeometry()
{
    QPalette pl = palette();
    pl.setColor(QPalette::Base, ODD_LINE_COL);
    pl.setColor(QPalette::AlternateBase, EVEN_LINE_COL);
    setPalette(pl); // does not seem to inherit application paletteAnnotate

    QHeaderView* hv = header();
    hv->setStretchLastSection(true);
    hv->setResizeMode(LOG_COL, QHeaderView::Interactive);
    hv->setResizeMode(TIME_COL, QHeaderView::Interactive);
    hv->setResizeMode(ANN_ID_COL, QHeaderView::ResizeToContents);
    hv->resizeSection(GRAPH_COL, DEF_GRAPH_COL_WIDTH);
    hv->resizeSection(LOG_COL, DEF_LOG_COL_WIDTH);
    hv->resizeSection(AUTH_COL, DEF_AUTH_COL_WIDTH);
    hv->resizeSection(TIME_COL, DEF_TIME_COL_WIDTH);

    if (git->isMainHistory(fh))
        hideColumn(ANN_ID_COL);
}

void ListView::scrollToNextHighlighted(int direction)
{
    // Depending on the value of direction, scroll to:
    // -1 = the next highlighted item above the current one (i.e. newer in history)
    //  1 = the next highlighted item below the current one (i.e. older in history)
    //  0 = the first highlighted item from the top of the list

    QModelIndex idx = currentIndex();

    if (!direction) {
        idx = idx.sibling(0,0);
        if (lp->isHighlighted(idx.row())) {
            setCurrentIndex(idx);
            return;
        }
    }

    do {
        idx = (direction >= 0 ? indexBelow(idx) : indexAbove(idx));
        if (!idx.isValid())
            return;

    } while (!lp->isHighlighted(idx.row()));

    setCurrentIndex(idx);
}

void ListView::scrollToCurrent(ScrollHint hint)
{
    if (currentIndex().isValid())
        scrollTo(currentIndex(), hint);
}

void ListView::on_keyUp()
{
    QModelIndex idx = indexAbove(currentIndex());
    if (idx.isValid())
        setCurrentIndex(idx);
}

void ListView::on_keyDown()
{
    QModelIndex idx = indexBelow(currentIndex());
    if (idx.isValid())
        setCurrentIndex(idx);
}

void ListView::on_changeFont(const QFont& f)
{
    setFont(f);
    ListViewDelegate* lvd = static_cast<ListViewDelegate*>(itemDelegate());
    lvd->setLaneHeight(fontMetrics().height());
    scrollToCurrent();
}

const QString ListView::currentText(int column)
{
    QModelIndex idx = model()->index(currentIndex().row(), column);
    return (idx.isValid() ? idx.data().toString() : "");
}

LaneType ListView::getLaneType(SCRef sha, int pos) const
{
    const Revision* r = git->revLookup(sha, fh);
    return (r && pos < r->lanes.count() && pos >= 0 ? r->lanes.at(pos) : LANE_UNDEFINED);
}

void ListView::showIdValues()
{
    fh->setAnnIdValid();
    viewport()->update();
}

void ListView::getSelectedItems(QStringList& selectedItems)
{
    selectedItems.clear();
    QModelIndexList ml = selectionModel()->selectedRows();
    FOREACH (QModelIndexList, it, ml)
        selectedItems.append(sha((*it).row()));

    // selectedRows() returns the items in an unspecified order,
    // so be sure rows are ordered from newest to oldest.
    selectedItems = git->sortShaListByIndex(selectedItems);
}

const QString ListView::shaFromAnnId(uint id)
{
    if (git->isMainHistory(fh))
        return "";

    return sha(model()->rowCount() - id);
}

int ListView::filterRows(bool isOn, bool highlight, SCRef filter, int colNum, ShaSet* set)
{
    setUpdatesEnabled(false);
    int matchedNum = lp->setFilter(isOn, highlight, filter, colNum, set);
    viewport()->update();
    setUpdatesEnabled(true);
    UPDATE_DOMAIN(d);
    return matchedNum;
}

bool ListView::update()
{
    int stRow = row(st->sha());
    if (stRow == -1)
        return false; // main/tree view asked us a sha not in history

    QModelIndex index = currentIndex();
    QItemSelectionModel* sel = selectionModel();

    if (index.isValid() && (index.row() == stRow)) {

        if (sel->isSelected(index) != st->selectItem())
            sel->select(index, QItemSelectionModel::Toggle);

        scrollTo(index);
    } else {
        // setCurrentIndex() does not clear previous
        // selections in a multi selection QListView
        clearSelection();

        QModelIndex newIndex = model()->index(stRow, 0);
        if (newIndex.isValid()) {

            // emits QItemSelectionModel::currentChanged()
            setCurrentIndex(newIndex);
            scrollTo(newIndex);
            if (!st->selectItem())
                sel->select(newIndex, QItemSelectionModel::Deselect);
        }
    }
    if (git->isMainHistory(fh))
        emit diffTargetChanged(row(st->diffToSha()));

    return currentIndex().isValid();
}

void ListView::currentChanged(const QModelIndex& index, const QModelIndex&)
{
    SCRef selRev = sha(index.row());
    if (st->sha() != selRev) { // to avoid looping
        st->setSha(selRev);
        st->setSelectItem(true);
        UPDATE_DOMAIN(d);
    }
}

bool ListView::filterRightButtonPressed(QMouseEvent* e)
{
    QModelIndex index = indexAt(e->pos());
    SCRef selSha = sha(index.row());
    if (selSha.isEmpty())
        return false;

    if (e->modifiers() == Qt::ControlModifier) { // check for 'diff to' function

        if (selSha != ZERO_SHA && st->sha() != ZERO_SHA) {

            if (selSha != st->diffToSha())
                st->setDiffToSha(selSha);
            else
                st->setDiffToSha(""); // restore std view

            filterNextContextMenuRequest = true;
            UPDATE_DOMAIN(d);
            return true; // filter event out
        }
    }
    // check for 'children & parents' function, i.e. if mouse is on the graph
    if (index.column() == GRAPH_COL) {

        filterNextContextMenuRequest = true;
        QStringList parents, children;
        if (getLaneParentsChilds(selSha, e->pos().x(), parents, children))
            emit lanesContextMenuRequested(parents, children);

        return true; // filter event out
    }
    return false;
}

void ListView::mousePressEvent(QMouseEvent* e)
{
    if (currentIndex().isValid() && e->button() == Qt::LeftButton)
        d->setReadyToDrag(true);

    if (e->button() == Qt::RightButton && filterRightButtonPressed(e))
        return; // filtered out

    QTreeView::mousePressEvent(e);
}

void ListView::mouseReleaseEvent(QMouseEvent* e)
{
    d->setReadyToDrag(false); // in case of just click without moving
    QTreeView::mouseReleaseEvent(e);
}

void ListView::mouseMoveEvent(QMouseEvent* e)
{
    if (d->isReadyToDrag()) {

        if (indexAt(e->pos()).row() == currentIndex().row())
            return; // move at least by one line to activate drag

        if (!d->setDragging(true))
            return;

        QStringList selRevs;
        getSelectedItems(selRevs);
        selRevs.removeAll(ZERO_SHA);
        if (!selRevs.empty())
            emit revisionsDragged(selRevs); // blocking until drop event

        d->setDragging(false);
    }
    QTreeView::mouseMoveEvent(e);
}

void ListView::dragEnterEvent(QDragEnterEvent* e)
{
    if (e->mimeData()->hasFormat("text/plain"))
        e->accept();
}

void ListView::dragMoveEvent(QDragMoveEvent* e)
{
    // already checked by dragEnterEvent()
    e->accept();
}

void ListView::dropEvent(QDropEvent *e)
{
    SCList remoteRevs(e->mimeData()->text().split('\n', QString::SkipEmptyParts));
    if (!remoteRevs.isEmpty()) {
        // some sanity check on dropped data
        SCRef sha(remoteRevs.first().section('@', 0, 0));
        SCRef remoteRepo(remoteRevs.first().section('@', 1));
        if (sha.length() == 40 && !remoteRepo.isEmpty())
            emit revisionsDropped(remoteRevs);
    }
}

void ListView::on_customContextMenuRequested(const QPoint& pos)
{
    QModelIndex index = indexAt(pos);
    if (!index.isValid())
        return;

    if (filterNextContextMenuRequest) {
        // event filter does not work on them
        filterNextContextMenuRequest = false;
        return;
    }
    emit contextMenu(sha(index.row()), POPUP_LIST_EV);
}

bool ListView::getLaneParentsChilds(SCRef sha, int x, SList p, SList c)
{
    ListViewDelegate* lvd = static_cast<ListViewDelegate*>(itemDelegate());
    uint lane = x / lvd->laneWidth();
    LaneType t = getLaneType(sha, lane);
    if (t == LANE_EMPTY || t == -1)
        return false;

    // first find the parents
    p.clear();
    QString root;
    if (!isFreeLane(t)) {
        p = git->revLookup(sha, fh)->parents(); // pointer cannot be NULL
        root = sha;
    } else {
        SCRef par(git->getLaneParent(sha, lane));
        if (par.isEmpty()) {
            dbs("ASSERT getLaneParentsChilds: parent not found");
            return false;
        }
        p.append(par);
        root = p.first();
    }
    // then find children
    c = git->getChilds(root);
    return true;
}

// *****************************************************************************



// *****************************************************************************

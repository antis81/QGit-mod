#include "listviewdelegate.h"

ListViewDelegate::ListViewDelegate(Git* g, ListViewProxy* px, QObject* p) : QItemDelegate(p)
{
    git = g;
    lp = px;
    laneHeight = 0;
    diffTargetRow = -1;
}

QSize ListViewDelegate::sizeHint(const QStyleOptionViewItem&, const QModelIndex&) const
{
    return QSize(laneWidth(), laneHeight);
}

void ListViewDelegate::diffTargetChanged(int row) {

    if (diffTargetRow != row) {
        diffTargetRow = row;
        emit updateView();
    }
}

const Revision* ListViewDelegate::revLookup(int row, FileHistory** fhPtr) const {

    ListView* lv = static_cast<ListView*>(parent());
    FileHistory* fh = static_cast<FileHistory*>(lv->model());

    if (lp->sourceModel())
        fh = static_cast<FileHistory*>(lp->sourceModel());

    if (fhPtr)
        *fhPtr = fh;

    return git->revLookup(lv->sha(row), fh);
}

static QColor blend(const QColor& col1, const QColor& col2, int amount = 128) {

    // Returns ((256 - amount)*col1 + amount*col2) / 256;
    return QColor(((256 - amount)*col1.red()   + amount*col2.red()  ) / 256,
                  ((256 - amount)*col1.green() + amount*col2.green()) / 256,
                  ((256 - amount)*col1.blue()  + amount*col2.blue() ) / 256);
}

void ListViewDelegate::paintGraphLane(QPainter* p, int type, int x1, int x2,
                                      const QColor& col, const QColor& activeCol, const QBrush& back) const {

    int h = laneHeight / 2;
    int m = (x1 + x2) / 2;
    int r = (x2 - x1) * 5 / 12;
    int d =  2 * r;

    #define P_CENTER m , h
    #define P_0      x2, h      // >
    #define P_90     m , 0      // ^
    #define P_180    x1, h      // <
    #define P_270    m , 2 * h  // v
    #define DELTA_UR 2*(x1 - m), 2*h ,   0*16, 90*16  // -,
    #define DELTA_DR 2*(x1 - m), 2*-h, 270*16, 90*16  // -'
    #define DELTA_UL 2*(x2 - m), 2*h ,  90*16, 90*16  //  ,-
    #define DELTA_DL 2*(x2 - m), 2*-h, 180*16, 90*16  //  '-
    #define CENTER_UR x1, 2*h, 225
    #define CENTER_DR x1, 0  , 135
    #define CENTER_UL x2, 2*h, 315
    #define CENTER_DL x2, 0  ,  45
    #define R_CENTER m - r, h - r, d, d

    static QPen myPen(Qt::black, 2); // fast path here

    // arc
    switch (type) {
    case LANE_JOIN:
    case LANE_JOIN_R:
    case LANE_HEAD:
    case LANE_HEAD_R: {
        QConicalGradient gradient(CENTER_UR);
        gradient.setColorAt(0.375, col);
        gradient.setColorAt(0.625, activeCol);
        myPen.setBrush(gradient);
        p->setPen(myPen);
        p->drawArc(P_CENTER, DELTA_UR);
        break;
    }
    case LANE_JOIN_L: {
        QConicalGradient gradient(CENTER_UL);
        gradient.setColorAt(0.375, activeCol);
        gradient.setColorAt(0.625, col);
        myPen.setBrush(gradient);
        p->setPen(myPen);
        p->drawArc(P_CENTER, DELTA_UL);
        break;
    }
    case LANE_TAIL:
    case LANE_TAIL_R: {
        QConicalGradient gradient(CENTER_DR);
        gradient.setColorAt(0.375, activeCol);
        gradient.setColorAt(0.625, col);
        myPen.setBrush(gradient);
        p->setPen(myPen);
        p->drawArc(P_CENTER, DELTA_DR);
        break;
    }
    default:
        break;
    }

    myPen.setColor(col);
    p->setPen(myPen);

    // vertical line
    switch (type) {
    case LANE_ACTIVE:
    case LANE_NOT_ACTIVE:
    case LANE_MERGE_FORK:
    case LANE_MERGE_FORK_R:
    case LANE_MERGE_FORK_L:
    case LANE_JOIN:
    case LANE_JOIN_R:
    case LANE_JOIN_L:
    case LANE_CROSS:
        p->drawLine(P_90, P_270);
        break;
    case LANE_HEAD_L:
    case LANE_BRANCH:
        p->drawLine(P_CENTER, P_270);
        break;
    case LANE_TAIL_L:
    case LANE_INITIAL:
    case LANE_BOUNDARY:
    case LANE_BOUNDARY_C:
    case LANE_BOUNDARY_R:
    case LANE_BOUNDARY_L:
        p->drawLine(P_90, P_CENTER);
        break;
    default:
        break;
    }

    myPen.setColor(activeCol);
    p->setPen(myPen);

    // horizontal line
    switch (type) {
    case LANE_MERGE_FORK:
    case LANE_JOIN:
    case LANE_HEAD:
    case LANE_TAIL:
    case LANE_CROSS:
    case LANE_CROSS_EMPTY:
    case LANE_BOUNDARY_C:
        p->drawLine(P_180, P_0);
        break;
    case LANE_MERGE_FORK_R:
    case LANE_BOUNDARY_R:
        p->drawLine(P_180, P_CENTER);
        break;
    case LANE_MERGE_FORK_L:
    case LANE_HEAD_L:
    case LANE_TAIL_L:
    case LANE_BOUNDARY_L:
        p->drawLine(P_CENTER, P_0);
        break;
    default:
        break;
    }

    // center symbol, e.g. rect or ellipse
    switch (type) {
    case LANE_ACTIVE:
    case LANE_INITIAL:
    case LANE_BRANCH:
        p->setPen(Qt::black);
        p->setBrush(col);
        p->drawEllipse(R_CENTER);
        break;
    case LANE_MERGE_FORK:
    case LANE_MERGE_FORK_R:
    case LANE_MERGE_FORK_L:
        p->setPen(Qt::black);
        p->setBrush(col);
        p->drawRect(R_CENTER);
        break;
    case LANE_UNAPPLIED:
        // Red minus sign
        p->setPen(Qt::NoPen);
        p->setBrush(Qt::red);
        p->drawRect(m - r, h - 1, d, 2);
        break;
    case LANE_APPLIED:
        // Green plus sign
        p->setPen(Qt::NoPen);
        p->setBrush(DARK_GREEN);
        p->drawRect(m - r, h - 1, d, 2);
        p->drawRect(m - 1, h - r, 2, d);
        break;
    case LANE_BOUNDARY:
        p->setPen(Qt::black);
        p->setBrush(back);
        p->drawEllipse(R_CENTER);
        break;
    case LANE_BOUNDARY_C:
    case LANE_BOUNDARY_R:
    case LANE_BOUNDARY_L:
        p->setPen(Qt::black);
        p->setBrush(back);
        p->drawRect(R_CENTER);
        break;
    default:
        break;
    }
    #undef P_CENTER
    #undef P_0
    #undef P_90
    #undef P_180
    #undef P_270
    #undef DELTA_UR
    #undef DELTA_DR
    #undef DELTA_UL
    #undef DELTA_DL
    #undef CENTER_UR
    #undef CENTER_DR
    #undef CENTER_UL
    #undef CENTER_DL
    #undef R_CENTER
}

void ListViewDelegate::paintGraph(QPainter* p, const QStyleOptionViewItem& opt,
                                  const QModelIndex& i) const {

    static const QColor colors[COLORS_NUM] = { Qt::red, DARK_GREEN,
                                               Qt::blue, Qt::darkGray, BROWN,
                                               Qt::magenta, ORANGE };
    if (opt.state & QStyle::State_Selected)
        p->fillRect(opt.rect, opt.palette.highlight());
    else if (i.row() & 1)
        p->fillRect(opt.rect, opt.palette.alternateBase());
    else
        p->fillRect(opt.rect, opt.palette.base());

    FileHistory* fh;
    const Revision* r = revLookup(i.row(), &fh);
    if (!r)
        return;

    p->save();
    p->setClipRect(opt.rect, Qt::IntersectClip);
    p->translate(opt.rect.topLeft());

    // calculate lanes
    if (r->lanes.count() == 0)
        git->setLane(r->sha(), fh);

    QBrush back = opt.palette.base();
    const QVector<LaneType>& lanes(r->lanes);
    uint laneNum = lanes.count();
    uint activeLane = 0;
    for (uint i = 0; i < laneNum; i++)
        if (isActive(lanes[i])) {
            activeLane = i;
            break;
        }

    int x1 = 0, x2 = 0;
    int maxWidth = opt.rect.width();
    int lw = laneWidth();
    QColor activeColor = colors[activeLane % COLORS_NUM];
    if (opt.state & QStyle::State_Selected)
        activeColor = blend(activeColor, opt.palette.highlightedText().color(), 208);
    for (uint i = 0; i < laneNum && x2 < maxWidth; i++) {

        x1 = x2;
        x2 += lw;

        int ln = lanes[i];
        if (ln == LANE_EMPTY)
            continue;

        QColor color = i == activeLane ? activeColor : colors[i % COLORS_NUM];
        paintGraphLane(p, ln, x1, x2, color, activeColor, back);
    }
    p->restore();
}

void ListViewDelegate::paintLog(QPainter* p, const QStyleOptionViewItem& opt,
                                const QModelIndex& index) const {

    int row = index.row();
    const Revision* r = revLookup(row);
    if (!r)
        return;

    if (r->isDiffCache)
        p->fillRect(opt.rect, changedFiles(ZERO_SHA) ? ORANGE : DARK_ORANGE);

    if (diffTargetRow == row)
        p->fillRect(opt.rect, LIGHT_BLUE);

    bool isHighlighted = lp->isHighlighted(row);
    QPixmap* pm = getTagMarks(r->sha(), opt);

    if (!pm && !isHighlighted) { // fast path in common case
        QItemDelegate::paint(p, opt, index);
        return;
    }
    QStyleOptionViewItem newOpt(opt); // we need a copy
    if (pm) {
        p->drawPixmap(newOpt.rect.x(), newOpt.rect.y(), *pm);
        newOpt.rect.adjust(pm->width(), 0, 0, 0);
        delete pm;
    }
    if (isHighlighted)
        newOpt.font.setBold(true);

    QItemDelegate::paint(p, newOpt, index);
}

void ListViewDelegate::paint(QPainter* p, const QStyleOptionViewItem& opt,
                             const QModelIndex& index) const {

  p->setRenderHints(QPainter::Antialiasing);

    if (index.column() == GRAPH_COL)
        return paintGraph(p, opt, index);

    if (index.column() == LOG_COL)
        return paintLog(p, opt, index);

    return QItemDelegate::paint(p, opt, index);
}

bool ListViewDelegate::changedFiles(SCRef sha) const {

    const RevFile* f = git->getFiles(sha);
    if (f)
        for (int i = 0; i < f->count(); i++)
            if (!f->statusCmp(i, RevFile::UNKNOWN))
                return true;
    return false;
}

QPixmap* ListViewDelegate::getTagMarks(SCRef sha, const QStyleOptionViewItem& opt) const {

    uint rt = git->shaMap.checkRef(sha);
    if (rt == 0)
        return NULL; // common case

    QPixmap* pm = new QPixmap(); // must be deleted by caller

    if (rt & Reference::BRANCH)
        addRefPixmap(&pm, sha, Reference::BRANCH, opt);

    if (rt & Reference::REMOTE_BRANCH)
        addRefPixmap(&pm, sha, Reference::REMOTE_BRANCH, opt);

    if (rt & Reference::TAG)
        addRefPixmap(&pm, sha, Reference::TAG, opt);

    if (rt & Reference::REF)
        addRefPixmap(&pm, sha, Reference::REF, opt);

    return pm;
}

void ListViewDelegate::addRefPixmap(QPixmap** pp, SCRef sha, int type, QStyleOptionViewItem opt) const {

    SCList refs = git->shaMap.getRefName(sha, (Reference::Type)type);
    FOREACH_SL (it, refs) {

        bool isCur = git->currentBranch().compare(*it) == 0;
        opt.font.setBold(isCur);

        QColor clr;
        if (type == Reference::BRANCH)
            clr = (isCur ? Qt::green : DARK_GREEN);

        else if (type == Reference::REMOTE_BRANCH)
            clr = LIGHT_ORANGE;

        else if (type == Reference::TAG)
            clr = Qt::yellow;

        else if (type == Reference::REF)
            clr = PURPLE;

        opt.palette.setColor(QPalette::Window, clr);
        addTextPixmap(pp, *it, opt);
    }
}

void ListViewDelegate::addTextPixmap(QPixmap** pp, SCRef txt, const QStyleOptionViewItem& opt) const {

    QPixmap* pm = *pp;
    int ofs = pm->isNull() ? 0 : pm->width() + 2;
    int spacing = 2;
    QFontMetrics fm(opt.font);
    int pw = fm.boundingRect(txt).width() + 2 * (spacing + int(opt.font.bold()));
    int ph = fm.height() - 1; // leave vertical space between two consecutive tags

    QPixmap* newPm = new QPixmap(ofs + pw, ph);
    QPainter p;
    p.begin(newPm);
    if (!pm->isNull()) {
        newPm->fill(opt.palette.base().color());
        p.drawPixmap(0, 0, *pm);
    }
    p.setPen(opt.palette.color(QPalette::WindowText));
    p.setBrush(opt.palette.color(QPalette::Window));
    p.setFont(opt.font);
    p.drawRect(ofs, 0, pw - 1, ph - 1);
    p.drawText(ofs + spacing, fm.ascent(), txt);
    p.end();

    delete pm;
    *pp = newPm;
}

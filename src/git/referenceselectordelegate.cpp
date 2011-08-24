#include "referenceselectordelegate.h"

#include <QObject>
#include <QMenu>
#include <QAction>

ReferenceSelectorDelegate::ReferenceSelectorDelegate()
    : ReferenceTreeViewDelegate()
{
}

void ReferenceSelectorDelegate::setup(Domain* domain)
{
    m_domain = domain;
}

void ReferenceSelectorDelegate::processDblClick(const QString& referenceName)
{
    m_domain->m()->changeBranch(referenceName);
}

void ReferenceSelectorDelegate::processContextMenu(const QPoint& pos, const QString& referenceName)
{
    Reference* reference = m_domain->git->m_references.byName(referenceName);
    if (!reference) {
        return;
    }

    QMenu contextMenu(QObject::tr("Context menu"));

    //! \attention FIXME: MEMORY LEAKS
    QAction* checkoutAction = new QAction(QObject::tr("Checkout"), this);
    QObject::connect(checkoutAction, SIGNAL(triggered()),
                     this, SLOT(checkout()));

    //! \attention FIXME: MEMORY LEAKS
    QAction* removeTagAction = new QAction(QObject::tr("Remove"), this);
    QObject::connect(removeTagAction, SIGNAL(triggered()),
                     this, SLOT(removeTag()));

    //! \attention FIXME: MEMORY LEAKS
//    QAction* expandNodeAction = new QAction(QObject::tr("Expand"), this);
//    QObject::connect(expandNodeAction, SIGNAL(triggered()),
//                     this, SLOT(expandNode()));

    //! \attention FIXME: MEMORY LEAKS
//    QAction* collapseNodeAction = new QAction(QObject::tr("Collapse"), this);
//    QObject::connect(expandNodeAction, SIGNAL(triggered()),
//                     this, SLOT(collapseNode()));

    if (reference->type() & Reference::BRANCH) {
        checkoutAction->setData(referenceName);
        contextMenu.addAction(checkoutAction);
    } else if (reference->type() & Reference::TAG) {
        checkoutAction->setData(referenceName);
        contextMenu.addAction(checkoutAction);
        contextMenu.addAction(removeTagAction);
    }

    if (!contextMenu.isEmpty()) {
        contextMenu.exec(pos);
    }
}

void ReferenceSelectorDelegate::checkout()
{
    QAction* action = static_cast<QAction* >(sender());
    if (!action) {
        return;
    }

    const QString& branch = action->data().toString();
    m_domain->git->checkout(branch);
}

void ReferenceSelectorDelegate::removeTag()
{
}

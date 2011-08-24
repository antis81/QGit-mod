/*
    Author: Marco Costalba (C) 2005-2007

    Copyright: See COPYING file that comes with this distribution

*/
#ifndef DOMAIN_H
#define DOMAIN_H

#include <QObject>
#include <QEvent>
#include "exceptionmanager.h"
#include "common.h"
#include "updatedomainevent.h"
#include "stateinfo.h"

#define UPDATE_DOMAIN(x)       QApplication::postEvent(x, new UpdateDomainEvent(false))
#define UPDATE()               QApplication::postEvent(this, new UpdateDomainEvent(false))
#define UPDATE_DM_MASTER(x, f) QApplication::postEvent(x, new UpdateDomainEvent(true, f))

class Domain;
class FileHistory;
class Git;
class MainImpl;

class Domain: public QObject
{
    Q_OBJECT
public:
    Domain();
    Domain(MainImpl *m, Git *git, bool isMain);
    virtual ~Domain();
    void deleteWhenDone(); // will delete when no more run() are pending
    void showStatusBarMessage(const QString& msg, int timeout = 0);
    void setThrowOnDelete(bool b);
    bool isThrowOnDeleteRaised(int excpId, SCRef curContext);
    MainImpl* m() const ;
    FileHistory* model() const;
    bool isReadyToDrag() const;
    bool setReadyToDrag(bool b);
    bool isDragging() const;
    bool setDragging(bool b);
    bool isDropping() const;
    void setDropping(bool b);
    bool isLinked() const;
    QWidget* tabPage() const;
    virtual bool isMatch(SCRef);

    StateInfo st;
    Git *git;
signals:
    void updateRequested(StateInfo newSt);
    void cancelDomainProcesses();

public slots:
    void on_closeAllTabs();

protected slots:
    virtual void on_contextMenu(const QString&, int);
    void on_updateRequested(StateInfo newSt);
    void on_deleteWhenDone();

protected:
    virtual void clear(bool complete = true);
    virtual bool event(QEvent *e);
    virtual bool doUpdate(bool force) = 0;
    void linkDomain(Domain *d);
    void unlinkDomain(Domain *d);
    void setTabCaption(const QString &caption);


    QWidget *container;
    bool busy;

private:
    void populateState();
    void update(bool fromMaster, bool force);
    bool flushQueue();
    void sendPopupEvent();

    EM_DECLARE(exDeleteRequest);
    EM_DECLARE(exCancelRequest);

    FileHistory *fileHistory;
    bool readyToDrag;
    bool dragging;
    bool dropping;
    bool linked;
    int popupType;
    QString popupData;
    QString statusBarRequest;
};

#endif

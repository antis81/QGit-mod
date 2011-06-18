#ifndef BRANCHESLIST_H
#define BRANCHESLIST_H

#include <QListWidget>
#include "git.h"
#include "domain.h"

class BranchesList : public QListWidget
{
    Q_OBJECT
public:
    explicit BranchesList(QWidget *parent = 0);

    void setup(Domain* d, Git* git);
    void update();

private:
    Git* git;
    Domain* d;
    void addList(QString header, Git::RefType type);
signals:

public slots:
    void itemDoubleClicked(QListWidgetItem *item);
};

class BranchListItem : public QListWidgetItem
{
public:
    BranchListItem(QString branchName);
    QString getBranchName() { return branchName; }
private:
    QString branchName;
};

#endif // BRANCHESLIST_H

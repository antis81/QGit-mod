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

    void setup(Domain* domain, Git* git);
    void update();

private:
    // я тут поменял, хотя в коде возможно есть конфликт имён (ибо не знаю с какого перепуга
    // гражданин Скальм использовал переменные g и git наравне. Она что (g),
    // глобальная чтоль какая? Однако работает, но это стоит учесть
    Git *g;
    Domain *d;
    void addList(QString header, Git::RefType type);

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

#ifndef REACHINFO_H
#define REACHINFO_H

// инклюды стырил из Annotate (первоначально куда были слиты все эти классы)

#include <QObject>
#include <QTime>
#include "exceptionmanager.h"
#include "common.h"

class ReachInfo
{
public:
    ReachInfo() {}
    ReachInfo(SCRef s, int i, int t) : sha(s), id(i), type(t) {}
    const QString sha;
    int id, type;
    QStringList roots;
};

#endif // REACHINFO_H

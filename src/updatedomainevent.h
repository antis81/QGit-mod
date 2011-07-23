#ifndef UPDATEDOMAINEVENT_H
#define UPDATEDOMAINEVENT_H

#include <QEvent>
#include "common.h"

class UpdateDomainEvent : public QEvent
{
public:
    UpdateDomainEvent(bool fromMaster, bool force = false);
    bool isForced() const;
private:
    bool f;
};

#endif // UPDATEDOMAINEVENT_H

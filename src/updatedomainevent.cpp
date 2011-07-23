#include "updatedomainevent.h"

UpdateDomainEvent::UpdateDomainEvent(bool fromMaster, bool force)
    : QEvent(fromMaster ? (QEvent::Type)QGit::UPD_DM_MST_EV
                        : (QEvent::Type)QGit::UPD_DM_EV), f(force)
{
}

bool UpdateDomainEvent::isForced() const
{
    return f;
}

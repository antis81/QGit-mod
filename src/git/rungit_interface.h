#ifndef RUNGIT_INTERFACE_H
#define RUNGIT_INTERFACE_H

#include <QString>
#include <QObject>

// FIXME: temporary class
class RunGitInterface {
public:
    virtual bool runGit(const QString& cmd, QString* out = NULL, QObject* rcv = NULL, const QString& buf = "") = 0;

    // FIXME: temporary
    virtual bool isStGITStack() const = 0;
};

#endif // RUNGIT_INTERFACE_H

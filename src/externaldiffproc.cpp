#include "externaldiffproc.h"

ExternalDiffProc::ExternalDiffProc(const QStringList& f, QObject* p)
        : QProcess(p), filenames(f)
{
        connect(this, SIGNAL(finished(int, QProcess::ExitStatus)),
                this, SLOT(on_finished(int, QProcess::ExitStatus)));
}

ExternalDiffProc::~ExternalDiffProc()
{
    terminate();
    removeFiles();
}

void ExternalDiffProc::on_finished(int, QProcess::ExitStatus)
{
    deleteLater();
}

void ExternalDiffProc::removeFiles()
{
    if (!filenames.empty()) {
        QDir d; // remove temporary files to diff on
        d.remove(filenames[0]);
        d.remove(filenames[1]);
    }
}

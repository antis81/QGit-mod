#ifndef EXTERNALDIFFPROC_H
#define EXTERNALDIFFPROC_H

#include <QProcess>
#include <QDir>

class ExternalDiffProc : public QProcess
{
    Q_OBJECT
public:
    ExternalDiffProc(const QStringList& f, QObject* p);
    ~ExternalDiffProc();
    QStringList filenames;

private slots:
    void on_finished(int, QProcess::ExitStatus);

private:
    void removeFiles();
};

#endif // EXTERNALDIFFPROC_H

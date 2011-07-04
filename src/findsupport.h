#ifndef FINDSUPPORT_H
#define FINDSUPPORT_H

#include <QString>

 class FindSupport
{
protected:
    QString* m_text;
    bool m_rememberText;

public:
    FindSupport();
    ~FindSupport();
    virtual void cancel();

    virtual void setText(QString text);
    QString text();

    virtual bool find() { return false; };
    virtual bool findNext() { return false; };

    bool find(QString text);
};

#endif // FINDSUPPORT_H

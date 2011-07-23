#ifndef CUSTOMTAB_H
#define CUSTOMTAB_H

#include <QString>
#include <QWidget>
#include <QVariant>

class CustomTab
{
public:
    virtual bool canCloseTab() = 0;
    virtual bool closeTab() = 0;
    virtual QString tabLabel() = 0;
    virtual QWidget* tabWidget() = 0;
};
Q_DECLARE_METATYPE(CustomTab* )

#endif // CUSTOMTAB_H

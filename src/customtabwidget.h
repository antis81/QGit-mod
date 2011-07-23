#ifndef CUSTOMTABWIDGET_H
#define CUSTOMTABWIDGET_H

#include <QTabWidget>
#include <QVariant>
#include <QToolButton>

#include "customtab.h"

class CustomTabWidget : public QTabWidget
{
    Q_OBJECT
public:
    explicit CustomTabWidget(QWidget *parent = 0);
    void addTab(CustomTab *tab);
    int addTab(QWidget *widget, const QString &label);

private:
    int findTabByCloseButton(QToolButton *closeButton);
signals:

private slots:
    void onCloseTabButtonClicked();
};

#endif // CUSTOMTABWIDGET_H

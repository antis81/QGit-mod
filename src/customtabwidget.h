#ifndef CUSTOMTABWIDGET_H
#define CUSTOMTABWIDGET_H

#include <QTabWidget>
#include <QVariant>
#include <QToolButton>

class CustomTab;

class CustomTabWidget : public QTabWidget
{
    Q_OBJECT
public:
    explicit CustomTabWidget(QWidget *parent = 0);
    void addTab(CustomTab* tab);
    int addTab(QWidget *widget, const QString &label) { return QTabWidget::addTab(widget, label); };

private:
    int findTabByCloseButton(QToolButton* closeButton);
signals:

private slots:
    void onCloseTabButtonClicked();
};

class CustomTab {
public:
    virtual bool canCloseTab() = 0;
    virtual bool closeTab() = 0;
    virtual QString tabLabel() = 0;
    virtual QWidget* tabWidget() = 0;

};
Q_DECLARE_METATYPE(CustomTab* );

#endif // CUSTOMTABWIDGET_H

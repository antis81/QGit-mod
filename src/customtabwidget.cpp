#include "customtabwidget.h"
#include <QTabBar>
#include <QToolButton>
#include <QVariant>

CustomTabWidget::CustomTabWidget(QWidget *parent) :
    QTabWidget(parent)
{

}

void CustomTabWidget::addTab(CustomTab* tab)
{
    int index = QTabWidget::addTab(tab->tabWidget(), tab->tabLabel());

    QVariant v;
    v.setValue(tab);
    tabBar()->setTabData(index, v);
    if (tab->canCloseTab()) {
        QToolButton* closeButton = new QToolButton();
        closeButton->setIcon(QIcon(":/icons/resources/close.png"));
        closeButton->setAutoRaise(true);
        tabBar()->setTabButton(index, QTabBar::RightSide, closeButton);
        connect(closeButton, SIGNAL(pressed()), this, SLOT(onCloseTabButtonClicked()));
    }
}

void CustomTabWidget::onCloseTabButtonClicked()
{
    QObject* sender = this->sender();
    if (!sender) {
        return;
    }
    QToolButton* closeButton = (QToolButton*)sender;
    int tabIndex = findTabByCloseButton(closeButton);
    if (tabIndex < 0) return;

    QVariant v = tabBar()->tabData(tabIndex);
    CustomTab* tab = v.value<CustomTab*>();

    if (!tab || !tab->canCloseTab() || !tab->closeTab()) {
        return;
    }

    removeTab(tabIndex);

}

int CustomTabWidget::findTabByCloseButton(QToolButton* closeButton)
{
    int count = tabBar()->count();
    for (int tabIndex = 0; tabIndex < count; tabIndex++) {
        if (tabBar()->tabButton(tabIndex, QTabBar::RightSide) == closeButton) {
            return tabIndex;
        }
    }
    return -1;
}

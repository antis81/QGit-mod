/*
    Author: Marco Costalba (C) 2005-2007

    Copyright: See COPYING file that comes with this distribution
*/
#ifndef REVDESC_H
#define REVDESC_H

#include <QTextBrowser>

class Domain;

class RevDesc: public QTextBrowser
{
    Q_OBJECT
public:
    RevDesc(QWidget* parent);
    void setup(Domain* dm) { d = dm; }
        QSize sizeHint() const;

protected:
    virtual void contextMenuEvent(QContextMenuEvent* e);

private slots:
    void on_anchorClicked(const QUrl& link);
    void on_highlighted(const QUrl& link);
    void on_linkCopy();
    void onTextChanged();

private:
    Domain* d;
    QString highlightedLink;
        int fitted_height;
        void fitHeightToDocument();

};

#endif

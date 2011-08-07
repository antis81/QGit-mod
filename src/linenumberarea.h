#ifndef LINENUMBERAREA_H
#define LINENUMBERAREA_H

#include <QWidget>
#include "patchcontent.h"

class LineNumberArea : public QWidget
{
public:
    LineNumberArea(PatchContent *editor);
    QSize sizeHint() const;

protected:
    void paintEvent(QPaintEvent *event);

private:
    PatchContent *codeEditor;
};

#endif // LINENUMBERAREA_H

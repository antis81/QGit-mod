#ifndef PATCHTEXTBLOCKUSERDATA_H
#define PATCHTEXTBLOCKUSERDATA_H

#include <QTextBlockUserData>
#include "patchcontent.h"

class PatchTextBlockUserData : public QTextBlockUserData
{
public:
    PatchContent::RowType rowType;

    long* rowNumbers;
    int partCount;

    PatchTextBlockUserData() : rowNumbers(NULL), partCount(0), rowType(PatchContent::ROW_OTHER) {}
    ~PatchTextBlockUserData() {
        if (rowNumbers) {
            delete[] rowNumbers;
            rowNumbers = NULL;
        }
    }
};
#endif // PATCHTEXTBLOCKUSERDATA_H

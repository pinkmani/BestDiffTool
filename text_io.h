#pragma once

#include "diff_types.h"

#include <QString>

struct ReadTextFileResult {
    bool ok{false};
    QString error;
    TextLines lines;
};

ReadTextFileResult readTextFile(const QString &path);

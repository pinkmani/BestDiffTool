#pragma once

#include "diff_types.h"

#include <string>

// Результат computeLineDiff: лента или сообщение об ошибке (например, слишком большие файлы).
struct LineDiffResult {
    bool ok{true};
    std::string error;
    DiffResultLines lines;
};

LineDiffResult computeLineDiff(const TextLines &left, const TextLines &right);

#pragma once


#include <string>
#include <vector>

using TextLines = std::vector<std::string>;

// Вариант одной визуальной строки (по номеру строки i слева и справа).
enum DiffKind { Same, Changed, Removed, Added };

// Тексты A и B для этой строки; при Same — оба одинаковые; при Removed/Added — одна сторона пустая.
struct DiffLine {
    DiffKind kind{};
    std::string textA{};
    std::string textB{};
    int lineA{-1};
    int lineB{-1};
};

using DiffResultLines = std::vector<DiffLine>;

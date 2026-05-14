#include "diff_engine.h"

#include <algorithm>

// Ограничение числа строк результата (построчное сравнение по индексу, без DP).
static constexpr std::size_t kMaxLines = 10'000'000;

LineDiffResult computeLineDiff(const TextLines &left, const TextLines &right)
{
    LineDiffResult result;
    const std::size_t n = left.size();
    const std::size_t m = right.size();
    const std::size_t rows = std::max(n, m);

    if (rows > kMaxLines) {
        result.ok = false;
        result.error = "Слишком много строк для сравнения.";
        return result;
    }

    result.lines.reserve(rows);

    for (std::size_t i = 0; i < rows; ++i) {
        const bool ha = i < n;
        const bool hb = i < m;
        DiffLine row;

        if (ha) {
            row.lineA = static_cast<int>(i + 1);
        }
        if (hb) {
            row.lineB = static_cast<int>(i + 1);
        }

        if (ha && hb) {
            if (left[i] == right[i]) {
                row.kind = Same;
                row.textA = left[i];
                row.textB = right[i];
            } else {
                row.kind = Changed;
                row.textA = left[i];
                row.textB = right[i];
            }
        } else if (ha) {
            row.kind = Removed;
            row.textA = left[i];
        } else {
            row.kind = Added;
            row.textB = right[i];
        }

        result.lines.push_back(std::move(row));
    }

    return result;
}

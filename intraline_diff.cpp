#include "intraline_diff.h"

#include <algorithm>

// Ограничение O(n*m) для DP по символам внутри одной строки.
static constexpr qsizetype kMaxIntraProduct = 1'500'000;

bool intraLineLcsEmphasis(const QString &a, const QString &b, QVector<bool> &emphA, QVector<bool> &emphB)
{
    const int n = a.size();
    const int m = b.size();
    emphA.clear();
    emphB.clear();
    if (n == 0 && m == 0) {
        return true;
    }
    if (static_cast<qsizetype>(n) * static_cast<qsizetype>(m) > kMaxIntraProduct) {
        return false;
    }

    QVector<QVector<int>> dp(n + 1, QVector<int>(m + 1, 0));
    for (int i = 1; i <= n; ++i) {
        for (int j = 1; j <= m; ++j) {
            if (a[i - 1] == b[j - 1]) {
                dp[i][j] = dp[i - 1][j - 1] + 1;
            } else {
                dp[i][j] = std::max(dp[i - 1][j], dp[i][j - 1]);
            }
        }
    }

    const int lcsLen = dp[n][m];
    // Коэффициент Sørensen–Dice: 2*|LCS|/(|a|+|b|). Условие «ниже порога»:
    // 200*lcs < 41*(n+m)  ⟺  dice < 0.41  (раньше ошибочно стояло 82 — срабатывало почти всегда.)
    if (n + m > 0 && 200 * lcsLen < 41 * (n + m)) {
        emphA.fill(true, n);
        emphB.fill(true, m);
        return true;
    }

    emphA.fill(false, n);
    emphB.fill(false, m);
    int i = n;
    int j = m;
    while (i > 0 || j > 0) {
        if (i > 0 && j > 0 && a[i - 1] == b[j - 1]) {
            --i;
            --j;
        } else if (j == 0 || (i > 0 && dp[i - 1][j] >= dp[i][j - 1])) {
            emphA[i - 1] = true;
            --i;
        } else {
            emphB[j - 1] = true;
            --j;
        }
    }
    return true;
}

QString intraLineToHtml(const QString &text, const QVector<bool> &emph, const QColor &emphColor)
{
    if (text.isEmpty()) {
        return QString();
    }
    if (emph.size() != text.size()) {
        return text.toHtmlEscaped();
    }

    const QString colorName = emphColor.name();
    QString out;
    out.reserve(static_cast<int>(text.size() * 2));

    int runStart = 0;
    const auto flush = [&](int runEnd, bool useEmph) {
        if (runStart >= runEnd) {
            return;
        }
        const QString chunk = text.mid(runStart, runEnd - runStart);
        const QString esc = chunk.toHtmlEscaped();
        if (useEmph) {
            out += QStringLiteral("<span style=\"background-color:%1;border-radius:3px;padding:0 1px;\">%2</span>")
                       .arg(colorName, esc);
        } else {
            out += esc;
        }
        runStart = runEnd;
    };

    bool cur = emph[0];
    for (int k = 1; k < text.size(); ++k) {
        if (emph[k] != cur) {
            flush(k, cur);
            cur = emph[k];
        }
    }
    flush(static_cast<int>(text.size()), cur);
    return out;
}

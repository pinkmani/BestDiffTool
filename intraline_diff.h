#pragma once

#include <QAbstractItemModel>
#include <QColor>
#include <QString>
#include <QVector>

// Роль для фрагмента HTML во второй колонке таблицы diff (поверх светлого фона ячейки).
inline constexpr int kDiffLineHtmlRole = static_cast<int>(Qt::UserRole) + 1;

// Помечает символы, не вошедшие в выбранную НВП (подсветка «отличие»). false — совпало в LCS.
// Если строки «совсем разные» (низкий коэффициент схожести по LCS), все символы помечаются как отличие — одна сплошная подсветка.
// Возвращает false, если строки слишком длинные (тогда показывать plain).
bool intraLineLcsEmphasis(const QString &a, const QString &b, QVector<bool> &emphA, QVector<bool> &emphB);

// Собирает HTML с <span> для участков emph[i]==true (тёмнее фона ячейки).
QString intraLineToHtml(const QString &text, const QVector<bool> &emph, const QColor &emphColor);

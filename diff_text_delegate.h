#pragma once

#include <QStyledItemDelegate>

// Рисует вторую колонку: при наличии HTML в kDiffLineHtmlRole — через QTextDocument, иначе как обычно.
class DiffTextDelegate final : public QStyledItemDelegate {
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;
};

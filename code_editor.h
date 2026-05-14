#pragma once

#include <QPaintEvent>
#include <QPlainTextEdit>
#include <QResizeEvent>

// Поле ввода с полосой номеров строк слева (рисуется отдельным виджетом).
class CodeEditor final : public QPlainTextEdit {
    Q_OBJECT

public:
    explicit CodeEditor(QWidget *parent = nullptr);

    void lineNumberAreaPaintEvent(QPaintEvent *event);
    int lineNumberAreaWidth() const;

protected:
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void updateLineNumberAreaWidth();
    void updateLineNumberArea(const QRect &rect, int dy);

private:
    class LineNumberArea;
    LineNumberArea *lineNumberArea{};
};

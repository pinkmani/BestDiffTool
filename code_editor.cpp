#include "code_editor.h"

#include <QPainter>
#include <QScrollBar>
#include <QTextBlock>

class CodeEditor::LineNumberArea final : public QWidget {
public:
    explicit LineNumberArea(CodeEditor *editor) : QWidget(editor), codeEditor(editor) {}

    QSize sizeHint() const override { return QSize(codeEditor->lineNumberAreaWidth(), 0); }

protected:
    void paintEvent(QPaintEvent *event) override { codeEditor->lineNumberAreaPaintEvent(event); }

private:
    CodeEditor *codeEditor{};
};

CodeEditor::CodeEditor(QWidget *parent) : QPlainTextEdit(parent)
{
    lineNumberArea = new LineNumberArea(this);

    connect(this, &CodeEditor::blockCountChanged, this, [this](int) { updateLineNumberAreaWidth(); });
    connect(this, &CodeEditor::updateRequest, this, &CodeEditor::updateLineNumberArea);
    connect(this, &CodeEditor::cursorPositionChanged, this, [this]() { updateLineNumberAreaWidth(); });

    updateLineNumberAreaWidth();
}

int CodeEditor::lineNumberAreaWidth() const
{
    int digits = 1;
    int mx = qMax(1, blockCount());
    while (mx >= 10) {
        mx /= 10;
        ++digits;
    }
    const int space = 8 + fontMetrics().horizontalAdvance(QLatin1Char('9')) * digits;
    return space;
}

void CodeEditor::updateLineNumberAreaWidth()
{
    setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
}

void CodeEditor::updateLineNumberArea(const QRect &rect, int dy)
{
    if (dy != 0) {
        lineNumberArea->scroll(0, dy);
    } else {
        lineNumberArea->update(0, rect.y(), lineNumberArea->width(), rect.height());
    }

    if (rect.contains(viewport()->rect())) {
        updateLineNumberAreaWidth();
    }
}

void CodeEditor::resizeEvent(QResizeEvent *event)
{
    QPlainTextEdit::resizeEvent(event);

    const QRect cr = contentsRect();
    lineNumberArea->setGeometry(QRect(cr.left(), cr.top(), lineNumberAreaWidth(), cr.height()));
}

void CodeEditor::lineNumberAreaPaintEvent(QPaintEvent *event)
{
    QPainter painter(lineNumberArea);
    painter.fillRect(event->rect(), QColor(0xec, 0xec, 0xf2));

    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    qreal top = blockBoundingGeometry(block).translated(contentOffset()).top();
    qreal bottom = top + blockBoundingGeometry(block).height();

    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) {
            const QString number = QString::number(blockNumber + 1);
            painter.setPen(QColor(0x55, 0x55, 0x60));
            painter.drawText(0,
                             static_cast<int>(top),
                             lineNumberArea->width() - 4,
                             fontMetrics().height(),
                             Qt::AlignRight | Qt::AlignVCenter,
                             number);
        }

        block = block.next();
        top = bottom;
        bottom = top + blockBoundingGeometry(block).height();
        ++blockNumber;
    }
}

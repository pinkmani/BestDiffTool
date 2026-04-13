#include "mainwindow.h"

#include "diff_engine.h"
#include "text_io.h"

#include <QFileDialog>
#include <QFont>
#include <QFontDatabase>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSplitter>
#include <QVBoxLayout>
#include <QWidget>

#include <string>
#include <vector>

// Вспомогательные функции только для этого .cpp: внутренняя связь (static), без анонимного namespace.

/** QString из UI → байты UTF-8 в std::string, по одной строке списка. */
static std::vector<std::string> QStringListToUtf8Lines(const QStringList &ql)
{
    std::vector<std::string> lines;
    lines.reserve(static_cast<std::size_t>(ql.size()));
    for (const QString &q : ql) {
        const QByteArray utf8 = q.toUtf8();
        lines.emplace_back(utf8.constData(), static_cast<std::size_t>(utf8.size()));
    }
    return lines;
}

/**
 * Весь текст из QPlainTextEdit → вектор строк для diff_engine.
 * Сначала приводим переводы строк к \n, чтобы Windows (\r\n) и старый Mac (\r) не ломали разбиение.
 */
static std::vector<std::string> PlainTextToUtf8Lines(const QString &plain)
{
    QString normalized = plain;
    normalized.replace(QStringLiteral("\r\n"), QStringLiteral("\n"));
    normalized.replace(QLatin1Char('\r'), QLatin1Char('\n'));
    return QStringListToUtf8Lines(normalized.split(QLatin1Char('\n')));
}

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    setWindowTitle(QStringLiteral("Сравнение текстов"));
    resize(1000, 700);

    // Корневой виджет и вертикальный layout на всё окно.
    auto *centralWidget = new QWidget(this);
    auto *mainLay = new QVBoxLayout(centralWidget);

    // Два независимых блока ввода рядом; пользователь может менять ширину колонок.
    auto *split = new QSplitter(Qt::Horizontal, centralWidget);

    auto *groupA = new QGroupBox(QStringLiteral("Текст A"), split);
    auto *layA = new QVBoxLayout(groupA);
    textEditA = new QPlainTextEdit(groupA);
    textEditA->setPlaceholderText(QStringLiteral("Введите текст или загрузите файл…"));
    btnLoadA = new QPushButton(QStringLiteral("Загрузить из файла…"), groupA);
    layA->addWidget(textEditA);
    layA->addWidget(btnLoadA);

    auto *groupB = new QGroupBox(QStringLiteral("Текст B"), split);
    auto *layB = new QVBoxLayout(groupB);
    textEditB = new QPlainTextEdit(groupB);
    textEditB->setPlaceholderText(QStringLiteral("Введите текст или загрузите файл…"));
    btnLoadB = new QPushButton(QStringLiteral("Загрузить из файла…"), groupB);
    layB->addWidget(textEditB);
    layB->addWidget(btnLoadB);

    split->addWidget(groupA);
    split->addWidget(groupB);
    split->setStretchFactor(0, 1);
    split->setStretchFactor(1, 1);

    // Моноширинный шрифт удобнее для сравнения колонок с номерами строк.
    const QFont monoFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    textEditA->setFont(monoFont);
    textEditB->setFont(monoFont);

    btnAnalyze = new QPushButton(QStringLiteral("Анализ"), centralWidget);
    btnAnalyze->setMinimumHeight(36);

    sourcesLabel = new QLabel(centralWidget);
    sourcesLabel->setWordWrap(true);

    resultEdit = new QPlainTextEdit(centralWidget);
    resultEdit->setReadOnly(true);
    resultEdit->setFont(monoFont);
    resultEdit->setPlaceholderText(QStringLiteral("Здесь появится результат сравнения…"));

    // stretch: верх (ввод) получает больше места по вертикали, чем подпись; низ — результат.
    mainLay->addWidget(split, 2);
    mainLay->addWidget(btnAnalyze);
    mainLay->addWidget(sourcesLabel);
    mainLay->addWidget(resultEdit, 1);

    setCentralWidget(centralWidget);

    connect(btnLoadA, &QPushButton::clicked, this, &MainWindow::pickFileA);
    connect(btnLoadB, &QPushButton::clicked, this, &MainWindow::pickFileB);
    connect(btnAnalyze, &QPushButton::clicked, this, &MainWindow::runAnalysis);

    updateSourceLabels();
}

void MainWindow::pickFileA()
{
    const QString chosenPath = QFileDialog::getOpenFileName(
        this,
        QStringLiteral("Выберите файл для текста A"),
        QString(),
        QStringLiteral("Текстовые файлы (*.txt);;Все файлы (*)"));
    if (chosenPath.isEmpty()) {
        return;
    }
    const ReadFileResult loaded = readTextFileLines(chosenPath);
    if (!loaded.ok) {
        QMessageBox::warning(this, QStringLiteral("Ошибка"), loaded.error);
        return;
    }
    pathA = chosenPath;
    textEditA->setPlainText(loaded.lines.join(QLatin1Char('\n')));
    updateSourceLabels();
}

void MainWindow::pickFileB()
{
    const QString chosenPath = QFileDialog::getOpenFileName(
        this,
        QStringLiteral("Выберите файл для текста B"),
        QString(),
        QStringLiteral("Текстовые файлы (*.txt);;Все файлы (*)"));
    if (chosenPath.isEmpty()) {
        return;
    }
    const ReadFileResult loaded = readTextFileLines(chosenPath);
    if (!loaded.ok) {
        QMessageBox::warning(this, QStringLiteral("Ошибка"), loaded.error);
        return;
    }
    pathB = chosenPath;
    textEditB->setPlainText(loaded.lines.join(QLatin1Char('\n')));
    updateSourceLabels();
}

void MainWindow::updateSourceLabels()
{
    const QString lineA = pathA.isEmpty()
        ? QStringLiteral("A: ввод вручную")
        : QStringLiteral("A: загружен из %1").arg(pathA);
    const QString lineB = pathB.isEmpty()
        ? QStringLiteral("B: ввод вручную")
        : QStringLiteral("B: загружен из %1").arg(pathB);
    sourcesLabel->setText(lineA + QLatin1Char('\n') + lineB);
}

void MainWindow::runAnalysis()
{
    resultEdit->clear();

    const std::vector<std::string> leftLines = PlainTextToUtf8Lines(textEditA->toPlainText());
    const std::vector<std::string> rightLines = PlainTextToUtf8Lines(textEditB->toPlainText());

    const DiffResult diff = computeLineDiff(leftLines, rightLines);
    if (!diff.ok) {
        QMessageBox::warning(this, QStringLiteral("Анализ"), QString::fromStdString(diff.error));
        return;
    }

    resultEdit->setPlainText(formatDiffOutput(diff));
}

QString MainWindow::formatDiffOutput(const DiffResult &dr)
{
    QString out;
    for (const DiffLine &row : dr.lines) {
        const QString numA = row.lineA >= 0 ? QString::number(row.lineA) : QStringLiteral("");
        const QString numB = row.lineB >= 0 ? QString::number(row.lineB) : QStringLiteral("");
        const QString lineText = QString::fromUtf8(row.text.data(),
                                                   static_cast<int>(row.text.size()));

        switch (row.kind) {
        case DiffKind::Same:
            out += QStringLiteral("   %1\t%2\t%3\n")
                       .arg(numA, 4)
                       .arg(numB, 4)
                       .arg(lineText);
            break;
        case DiffKind::Removed:
            out += QStringLiteral("-  %1\t%2\t%3\n")
                       .arg(numA, 4)
                       .arg(QStringLiteral("—"), 4)
                       .arg(lineText);
            break;
        case DiffKind::Added:
            out += QStringLiteral("+  %1\t%2\t%3\n")
                       .arg(QStringLiteral("—"), 4)
                       .arg(numB, 4)
                       .arg(lineText);
            break;
        }
    }
    return out;
}

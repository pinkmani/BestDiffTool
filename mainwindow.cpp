#include "mainwindow.h"

#include "code_editor.h"
#include "diff_engine.h"
#include "diff_text_delegate.h"
#include "intraline_diff.h"
#include "text_io.h"

#include <QBrush>
#include <QColor>
#include <QFileDialog>
#include <QFontDatabase>
#include <QFontMetrics>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollBar>
#include <QSignalBlocker>
#include <QStackedWidget>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>
#include <QVector>
#include <QWidget>
#include <QAbstractItemView>
#include <QPlainTextEdit>

#include <string>

// Максимальная длина логической строки перед разбиением при анализе (только для расчёта diff).
static constexpr int kReflowMaxColumnChars = 80;

static TextLines qStringListToTextLines(const QStringList &ql)
{
    TextLines out;
    out.reserve(static_cast<std::size_t>(ql.size()));
    for (const QString &q : ql) {
        const QByteArray utf8 = q.toUtf8();
        out.emplace_back(utf8.constData(), static_cast<std::size_t>(utf8.size()));
    }
    return out;
}

static TextLines plainTextToTextLines(const QString &plain)
{
    QString n = plain;
    n.replace(QStringLiteral("\r\n"), QStringLiteral("\n"));
    n.replace(QLatin1Char('\r'), QLatin1Char('\n'));
    QStringList parts = n.split(QLatin1Char('\n'));
    while (!parts.isEmpty() && parts.last().isEmpty()) {
        parts.removeLast();
    }
    return qStringListToTextLines(parts);
}

// Делит слишком длинные строки по пробелам (или жёстко), чтобы анализ шёл по нескольким логическим строкам.
static QString reflowLongLines(const QString &plain, int maxCol)
{
    maxCol = qBound(8, maxCol, 500);
    QString n = plain;
    n.replace(QStringLiteral("\r\n"), QStringLiteral("\n"));
    n.replace(QLatin1Char('\r'), QLatin1Char('\n'));
    QStringList parts = n.split(QLatin1Char('\n'));
    while (!parts.isEmpty() && parts.last().isEmpty()) {
        parts.removeLast();
    }

    QStringList out;
    out.reserve(parts.size() * 2);
    for (QString line : parts) {
        if (line.isEmpty()) {
            out.append(QString());
            continue;
        }
        while (line.size() > maxCol) {
            int cut = maxCol;
            int sp = line.lastIndexOf(QLatin1Char(' '), cut);
            if (sp <= 0) {
                sp = line.lastIndexOf(QLatin1Char('\t'), cut);
            }
            if (sp > 0 && sp >= cut / 4) {
                out.append(line.left(sp));
                line = line.mid(sp + 1);
            } else {
                out.append(line.left(maxCol));
                line = line.mid(maxCol);
            }
        }
        out.append(line);
    }
    return out.join(QLatin1Char('\n'));
}

static void setCell(QTableWidget *table, int row, int col, const QString &text, const QColor &bg)
{
    auto *item = new QTableWidgetItem(text);
    item->setFlags(item->flags() & ~Qt::ItemIsEditable & ~Qt::ItemIsSelectable);
    item->setBackground(QBrush(bg));
    table->setItem(row, col, item);
}

// Тёмнее светлого фона ячейки — «внутристрочные» отличия.
static const QColor kEmphRemoved(0xf0, 0xa8, 0xa8);
static const QColor kEmphAdded(0x8e, 0xd4, 0x98);

static void setTextLineCell(QTableWidget *table, int row, const QString &plain, const QColor &bg,
                            const QString &htmlFragment)
{
    auto *item = new QTableWidgetItem(plain);
    item->setFlags(item->flags() & ~Qt::ItemIsEditable & ~Qt::ItemIsSelectable);
    item->setBackground(QBrush(bg));
    if (!htmlFragment.isEmpty()) {
        item->setData(kDiffLineHtmlRole, htmlFragment);
    }
    table->setItem(row, 1, item);
}

static const QColor kNumBg(0xcf, 0xd3, 0xdc);
static const QColor kColSame(0xe6, 0xe9, 0xef);
static const QColor kColRemoved(0xff, 0xd8, 0xd8);
static const QColor kColAdded(0xc8, 0xf5, 0xc8);
static const QColor kColGap(0xf4, 0xf4, 0xf6);

static void setupDiffTable(QTableWidget *t)
{
    t->setColumnCount(2);
    t->setHorizontalHeaderLabels({QStringLiteral("#"), QStringLiteral("Строка")});
    t->horizontalHeader()->setVisible(false);
    t->verticalHeader()->setVisible(false);
    t->setShowGrid(false);
    t->setEditTriggers(QAbstractItemView::NoEditTriggers);
    t->setSelectionMode(QAbstractItemView::NoSelection);
    t->setFocusPolicy(Qt::ClickFocus);
    t->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    t->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    t->setWordWrap(false);
    t->setTextElideMode(Qt::ElideRight);
}

static void fillIntraLineChanged(const QString &qa, const QString &qb, QString *htmlA, QString *htmlB)
{
    QVector<bool> emphA;
    QVector<bool> emphB;
    if (intraLineLcsEmphasis(qa, qb, emphA, emphB)) {
        *htmlA = intraLineToHtml(qa, emphA, kEmphRemoved);
        *htmlB = intraLineToHtml(qb, emphB, kEmphAdded);
    } else {
        htmlA->clear();
        htmlB->clear();
    }
}

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    setWindowTitle(QStringLiteral("BestDiffTool"));
    resize(1100, 780);
    setMinimumSize(900, 560);

    auto *central = new QWidget(this);
    auto *mainLay = new QVBoxLayout(central);
    central->setStyleSheet(
        QStringLiteral("QWidget#centralRoot { background: #ececf0; }"
                       "QGroupBox { font-weight: 600; border: 1px solid #b8bcc8; border-radius: 8px; "
                       "margin-top: 10px; padding-top: 12px; background: #f6f6f8; }"
                       "QGroupBox::title { subcontrol-origin: margin; left: 12px; padding: 0 6px; color: #2a2a2e; }"
                       "QPushButton { padding: 8px 16px; border-radius: 6px; background: #fff; "
                       "border: 1px solid #b0b4c0; }"
                       "QPushButton:hover { background: #f0f4ff; border-color: #6b8cce; }"
                       "QPushButton:pressed { background: #e0e8ff; }"
                       "QPlainTextEdit, QTableWidget { border: 1px solid #c5c9d4; border-radius: 6px; "
                       "background: #fff; }"));
    central->setObjectName(QStringLiteral("centralRoot"));

    auto *twoCols = new QWidget(central);
    auto *twoColsLay = new QHBoxLayout(twoCols);
    twoColsLay->setContentsMargins(0, 0, 0, 0);

    auto *boxA = new QGroupBox(QStringLiteral("Текст 1"), twoCols);
    auto *layA = new QVBoxLayout(boxA);
    stackA = new QStackedWidget(boxA);
    codeEditA = new CodeEditor(boxA);
    diffTableA = new QTableWidget(boxA);
    setupDiffTable(diffTableA);
    stackA->addWidget(codeEditA);
    stackA->addWidget(diffTableA);
    btnLoadA = new QPushButton(QStringLiteral("Загрузить из файла…"), boxA);
    layA->addWidget(stackA);
    layA->addWidget(btnLoadA);

    auto *boxB = new QGroupBox(QStringLiteral("Текст 2"), twoCols);
    auto *layB = new QVBoxLayout(boxB);
    stackB = new QStackedWidget(boxB);
    codeEditB = new CodeEditor(boxB);
    diffTableB = new QTableWidget(boxB);
    setupDiffTable(diffTableB);
    stackB->addWidget(codeEditB);
    stackB->addWidget(diffTableB);
    btnLoadB = new QPushButton(QStringLiteral("Загрузить из файла…"), boxB);
    layB->addWidget(stackB);
    layB->addWidget(btnLoadB);

    twoColsLay->addWidget(boxA, 1);
    twoColsLay->addWidget(boxB, 1);
    boxA->setMinimumWidth(280);
    boxB->setMinimumWidth(280);

    QFont mono = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    codeEditA->setFont(mono);
    codeEditB->setFont(mono);
    diffTableA->setFont(mono);
    diffTableB->setFont(mono);
    codeEditA->setPlaceholderText(QStringLiteral("Ввод или загрузка файла…"));
    codeEditB->setPlaceholderText(QStringLiteral("Ввод или загрузка файла…"));
    codeEditA->setLineWrapMode(QPlainTextEdit::WidgetWidth);
    codeEditB->setLineWrapMode(QPlainTextEdit::WidgetWidth);

    auto *diffLineDelegate = new DiffTextDelegate(central);
    diffTableA->setItemDelegateForColumn(1, diffLineDelegate);
    diffTableB->setItemDelegateForColumn(1, diffLineDelegate);
    const int compactRowH = QFontMetrics(mono).height() + 6;
    diffTableA->verticalHeader()->setDefaultSectionSize(compactRowH);
    diffTableB->verticalHeader()->setDefaultSectionSize(compactRowH);

    auto *btnRow = new QHBoxLayout();
    btnAnalyze = new QPushButton(QStringLiteral("Анализ"), central);
    btnAnalyze->setMinimumHeight(40);
    btnAnalyze->setStyleSheet(QStringLiteral(
        "QPushButton { font-weight: 600; background: #2d6cdf; color: white; border: none; padding: 10px; }"
        "QPushButton:hover { background: #1f5bc4; }"
        "QPushButton:pressed { background: #184a9e; }"));
    btnBackToEdit = new QPushButton(QStringLiteral("Редактировать тексты"), central);
    btnBackToEdit->setEnabled(false);
    btnRow->addWidget(btnAnalyze);
    btnRow->addWidget(btnBackToEdit);
    btnRow->addStretch();

    sourcesLabel = new QLabel(central);
    sourcesLabel->setWordWrap(true);
    sourcesLabel->setStyleSheet(QStringLiteral("color: #444; font-size: 11pt; padding: 4px 0;"));

    diffLegend = new QLabel(QStringLiteral("Сравнение по номеру строки: серый — совпадение, розовый/зелёный — различие."),
                            central);
    diffLegend->setWordWrap(true);
    diffLegend->setStyleSheet(QStringLiteral("color: #555; font-size: 10pt; padding: 2px 0 6px 0;"));

    mainLay->addWidget(twoCols, 1);
    mainLay->addLayout(btnRow);
    mainLay->addWidget(sourcesLabel);
    mainLay->addWidget(diffLegend);

    setCentralWidget(central);

    connect(btnLoadA, &QPushButton::clicked, this, &MainWindow::loadFileA);
    connect(btnLoadB, &QPushButton::clicked, this, &MainWindow::loadFileB);
    connect(btnAnalyze, &QPushButton::clicked, this, &MainWindow::runAnalysis);
    connect(btnBackToEdit, &QPushButton::clicked, this, &MainWindow::backToEditing);

    setupScrollSync();
    updateSourceLabels();
}

void MainWindow::setupScrollSync()
{
    auto *ba = codeEditA->verticalScrollBar();
    auto *bb = codeEditB->verticalScrollBar();
    connect(ba, &QScrollBar::valueChanged, this, [ba, bb](int v) {
        if (bb->value() != v) {
            QSignalBlocker blocker(bb);
            bb->setValue(v);
        }
    });
    connect(bb, &QScrollBar::valueChanged, this, [ba, bb](int v) {
        if (ba->value() != v) {
            QSignalBlocker blocker(ba);
            ba->setValue(v);
        }
    });

    auto *da = diffTableA->verticalScrollBar();
    auto *db = diffTableB->verticalScrollBar();
    connect(da, &QScrollBar::valueChanged, this, [da, db](int v) {
        if (db->value() != v) {
            QSignalBlocker blocker(db);
            db->setValue(v);
        }
    });
    connect(db, &QScrollBar::valueChanged, this, [da, db](int v) {
        if (da->value() != v) {
            QSignalBlocker blocker(da);
            da->setValue(v);
        }
    });
}

void MainWindow::applySideBySideDiff(const DiffResultLines &lines)
{
    const int n = static_cast<int>(lines.size());
    diffTableA->clearContents();
    diffTableB->clearContents();
    diffTableA->setRowCount(n);
    diffTableB->setRowCount(n);

    for (int r = 0; r < n; ++r) {
        const DiffLine &row = lines[static_cast<std::size_t>(r)];
        const QString qa = QString::fromUtf8(row.textA.data(), static_cast<int>(row.textA.size()));
        const QString qb = QString::fromUtf8(row.textB.data(), static_cast<int>(row.textB.size()));

        switch (row.kind) {
        case Same:
            setCell(diffTableA, r, 0, QString::number(row.lineA), kNumBg);
            setCell(diffTableA, r, 1, qa, kColSame);
            setCell(diffTableB, r, 0, QString::number(row.lineB), kNumBg);
            setCell(diffTableB, r, 1, qb, kColSame);
            break;
        case Changed: {
            QString htmlA;
            QString htmlB;
            fillIntraLineChanged(qa, qb, &htmlA, &htmlB);
            setCell(diffTableA, r, 0, QString::number(row.lineA), kNumBg);
            setCell(diffTableB, r, 0, QString::number(row.lineB), kNumBg);
            if (htmlA.isEmpty()) {
                setCell(diffTableA, r, 1, qa, kColRemoved);
            } else {
                setTextLineCell(diffTableA, r, qa, kColRemoved, htmlA);
            }
            if (htmlB.isEmpty()) {
                setCell(diffTableB, r, 1, qb, kColAdded);
            } else {
                setTextLineCell(diffTableB, r, qb, kColAdded, htmlB);
            }
            break;
        }
        case Removed: {
            setCell(diffTableA, r, 0, QString::number(row.lineA), kNumBg);
            QVector<bool> allEmph(qa.size(), true);
            const QString htmlRm = qa.isEmpty() ? QString() : intraLineToHtml(qa, allEmph, kEmphRemoved);
            if (htmlRm.isEmpty()) {
                setCell(diffTableA, r, 1, qa, kColRemoved);
            } else {
                setTextLineCell(diffTableA, r, qa, kColRemoved, htmlRm);
            }
            setCell(diffTableB, r, 0, QString(), kColGap);
            setCell(diffTableB, r, 1, QString(), kColGap);
            break;
        }
        case Added: {
            setCell(diffTableA, r, 0, QString(), kColGap);
            setCell(diffTableA, r, 1, QString(), kColGap);
            setCell(diffTableB, r, 0, QString::number(row.lineB), kNumBg);
            QVector<bool> allEmphB(qb.size(), true);
            const QString htmlAd = qb.isEmpty() ? QString() : intraLineToHtml(qb, allEmphB, kEmphAdded);
            if (htmlAd.isEmpty()) {
                setCell(diffTableB, r, 1, qb, kColAdded);
            } else {
                setTextLineCell(diffTableB, r, qb, kColAdded, htmlAd);
            }
            break;
        }
        }
    }

    diffTableA->verticalScrollBar()->setValue(0);
    diffTableB->verticalScrollBar()->setValue(0);
}

void MainWindow::loadFileA()
{
    const QString p = QFileDialog::getOpenFileName(
        this,
        QStringLiteral("Файл для текста 1"),
        QString(),
        QStringLiteral("Текст (*.txt);;Все файлы (*)"));
    if (p.isEmpty()) {
        return;
    }
    const ReadTextFileResult r = readTextFile(p);
    if (!r.ok) {
        QMessageBox::warning(this, QStringLiteral("Ошибка"), r.error);
        return;
    }
    pathA = p;
    QString joined;
    for (std::size_t i = 0; i < r.lines.size(); ++i) {
        if (i > 0) {
            joined += QLatin1Char('\n');
        }
        const std::string &line = r.lines[i];
        joined += QString::fromUtf8(line.data(), static_cast<int>(line.size()));
    }
    codeEditA->setPlainText(joined);
    updateSourceLabels();
}

void MainWindow::loadFileB()
{
    const QString p = QFileDialog::getOpenFileName(
        this,
        QStringLiteral("Файл для текста 2"),
        QString(),
        QStringLiteral("Текст (*.txt);;Все файлы (*)"));
    if (p.isEmpty()) {
        return;
    }
    const ReadTextFileResult r = readTextFile(p);
    if (!r.ok) {
        QMessageBox::warning(this, QStringLiteral("Ошибка"), r.error);
        return;
    }
    pathB = p;
    QString joined;
    for (std::size_t i = 0; i < r.lines.size(); ++i) {
        if (i > 0) {
            joined += QLatin1Char('\n');
        }
        const std::string &line = r.lines[i];
        joined += QString::fromUtf8(line.data(), static_cast<int>(line.size()));
    }
    codeEditB->setPlainText(joined);
    updateSourceLabels();
}

void MainWindow::updateSourceLabels()
{
    const QString a = pathA.isEmpty() ? QStringLiteral("Текст 1: вручную") : QStringLiteral("Текст 1: %1").arg(pathA);
    const QString b = pathB.isEmpty() ? QStringLiteral("Текст 2: вручную") : QStringLiteral("Текст 2: %1").arg(pathB);
    sourcesLabel->setText(a + QLatin1Char('\n') + b);
}

void MainWindow::runAnalysis()
{
    QString pa;
    QString pb;
    if (stackA->currentIndex() == 1) {
        pa = savedPlainA;
        pb = savedPlainB;
    } else {
        pa = codeEditA->toPlainText();
        pb = codeEditB->toPlainText();
    }

    savedPlainA = pa;
    savedPlainB = pb;

    const QString qa = reflowLongLines(pa, kReflowMaxColumnChars);
    const QString qb = reflowLongLines(pb, kReflowMaxColumnChars);

    const TextLines left = plainTextToTextLines(qa);
    const TextLines right = plainTextToTextLines(qb);

    const LineDiffResult diff = computeLineDiff(left, right);
    if (!diff.ok) {
        QMessageBox::warning(this, QStringLiteral("Анализ"), QString::fromStdString(diff.error));
        return;
    }

    applySideBySideDiff(diff.lines);

    stackA->setCurrentIndex(1);
    stackB->setCurrentIndex(1);
    btnLoadA->setEnabled(false);
    btnLoadB->setEnabled(false);
    btnBackToEdit->setEnabled(true);
}

void MainWindow::backToEditing()
{
    codeEditA->setPlainText(savedPlainA);
    codeEditB->setPlainText(savedPlainB);
    diffTableA->setRowCount(0);
    diffTableB->setRowCount(0);
    stackA->setCurrentIndex(0);
    stackB->setCurrentIndex(0);
    btnLoadA->setEnabled(true);
    btnLoadB->setEnabled(true);
    btnBackToEdit->setEnabled(false);
}

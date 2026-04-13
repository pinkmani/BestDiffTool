#pragma once

// Главное окно: два поля ввода, загрузка из файлов, вывод различий.

#include "diff_engine.h"

#include <QMainWindow>
#include <QString>

class QLabel;
class QPlainTextEdit;
class QPushButton;

class MainWindow final : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    void pickFileA();
    void pickFileB();
    void runAnalysis();

private:
    void updateSourceLabels();
    // Собирает многострочный текст для нижнего поля: префиксы и номера строк.
    static QString formatDiffOutput(const DiffResult &dr);

    // Виджеты (имена без префикса m_: стиль textEditA, btnAnalyze, resultEdit …)

    QPlainTextEdit *textEditA{};
    QPlainTextEdit *textEditB{};
    QPushButton *btnLoadA{};
    QPushButton *btnLoadB{};
    QPushButton *btnAnalyze{};
    QLabel *sourcesLabel{};
    // Только чтение: сюда выводится результат сравнения после «Анализ».
    QPlainTextEdit *resultEdit{};

    // Путь последнего успешного «Загрузить из файла» (для подписи); пусто = считаем ручной ввод.
    QString pathA;
    QString pathB;
};

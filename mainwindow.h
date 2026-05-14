#pragma once

#include "diff_types.h"

#include <QMainWindow>
#include <QString>

class CodeEditor;
class QLabel;
class QPushButton;
class QStackedWidget;
class QTableWidget;

class MainWindow final : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    void loadFileA();
    void loadFileB();
    void runAnalysis();
    void backToEditing();

private:
    void updateSourceLabels();
    void setupScrollSync();
    void applySideBySideDiff(const DiffResultLines &lines);

    QStackedWidget *stackA{};
    QStackedWidget *stackB{};
    CodeEditor *codeEditA{};
    CodeEditor *codeEditB{};
    QTableWidget *diffTableA{};
    QTableWidget *diffTableB{};

    QPushButton *btnLoadA{};
    QPushButton *btnLoadB{};
    QPushButton *btnAnalyze{};
    QPushButton *btnBackToEdit{};
    QLabel *sourcesLabel{};
    QLabel *diffLegend{};

    QString savedPlainA;
    QString savedPlainB;

    QString pathA;
    QString pathB;
};

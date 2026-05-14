#include "mainwindow.h"

#include <QApplication>
#include <QPalette>
#include <QStyleFactory>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    app.setStyle(QStyleFactory::create(QStringLiteral("Fusion")));

    QPalette pal;
    pal.setColor(QPalette::Window, QColor(0xec, 0xec, 0xf0));
    pal.setColor(QPalette::WindowText, QColor(0x22, 0x22, 0x28));
    pal.setColor(QPalette::Base, QColor(0xff, 0xff, 0xff));
    pal.setColor(QPalette::AlternateBase, QColor(0xf5, 0xf5, 0xf8));
    pal.setColor(QPalette::Text, QColor(0x22, 0x22, 0x28));
    pal.setColor(QPalette::Button, QColor(0xf8, 0xf8, 0xfa));
    pal.setColor(QPalette::ButtonText, QColor(0x22, 0x22, 0x28));
    app.setPalette(pal);

    MainWindow w;
    w.show();
    return app.exec();
}

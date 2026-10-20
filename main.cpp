#include <QApplication>

#include "mainwindow.h"

int main(int argc, char *argv[])
{
    // Объект приложения: один на всю программу, запускает цикл событий
    QApplication app(argc, argv);

    MainWindow window;
    window.resize(800, 600);
    window.show();

    // Цикл событий: программа ждёт кликов, нажатий клавиш, ответов сети
    return app.exec();
}

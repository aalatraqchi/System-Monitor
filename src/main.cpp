#include <QApplication>
#include <QTimer>
#include "MainWindow.h"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName("System Monitor");
    QApplication::setApplicationVersion("0.1.0");

    MainWindow window;
    window.show();

    // Allows a quick startup check without leaving a window running.
    if (app.arguments().contains("--smoke-test")) {
        QTimer::singleShot(200, &app, &QApplication::quit);
    }
    return app.exec();
}

#include <QApplication>
#include <QTimer>
#include <QIcon>
#include "MainWindow.h"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    QApplication::setStyle("Fusion");
    QApplication::setOrganizationName("SystemMonitor");
    QApplication::setApplicationName("System Monitor");
    QApplication::setApplicationVersion("0.1.0");
    QApplication::setWindowIcon(QIcon(":/icons/app.ico"));

    MainWindow window;
    window.show();

    // Allows a quick startup check without leaving a window running.
    if (app.arguments().contains("--smoke-test")) {
        QTimer::singleShot(200, &app, &QApplication::quit);
    }
    return app.exec();
}

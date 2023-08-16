#include <QApplication>
#include <QMessageBox>

#include "mainwindow.h"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    if (!QSystemTrayIcon::isSystemTrayAvailable())
    {
        QMessageBox::critical(nullptr, QObject::tr("ClipNet"), QObject::tr("I couldn't detect any system tray on this system."));
        return 1;
    }

    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);

    MainWindow window;
    //    window.hide();

    return app.exec();
}

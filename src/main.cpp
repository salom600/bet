#include <QApplication>
#include <QFile>
#include <QStyleFactory>
#include "ui/MainWindow.h"
#include "utils/GenTime.h"

int main(int argc, char* argv[]) {
    QApplication::setStyle(QStyleFactory::create("Fusion"));

    QApplication app(argc, argv);
    app.setApplicationName("VideoEditor");
    app.setOrganizationName("VideoEditor");
    app.setApplicationVersion("0.2.0");

    QFile qss(":/styles/dark.qss");
    if (qss.open(QIODevice::ReadOnly | QIODevice::Text)) {
        app.setStyleSheet(QString::fromUtf8(qss.readAll()));
    }

    ve::MainWindow w;
    w.resize(1600, 1000);
    w.show();
    return app.exec();
}

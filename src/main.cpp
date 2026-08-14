#include <QApplication>
#include <QFile>
#include <QStyleFactory>
#include "ui/MainWindow.h"

int main(int argc, char* argv[]) {
    // High DPI is on by default in Qt6; explicitly request fusion style for a
    // consistent dark appearance across platforms.
    QApplication::setStyle(QStyleFactory::create("Fusion"));

    QApplication app(argc, argv);
    app.setApplicationName("VideoEditor");
    app.setOrganizationName("VideoEditor");
    app.setApplicationVersion("0.1.0");

    // Load dark theme stylesheet
    QFile qss(":/styles/dark.qss");
    if (qss.open(QIODevice::ReadOnly | QIODevice::Text)) {
        app.setStyleSheet(QString::fromUtf8(qss.readAll()));
    }

    ve::MainWindow w;
    w.resize(1440, 900);
    w.show();

    // Optional: load a project file passed as command-line argument
    const QStringList args = app.arguments();
    if (args.size() > 1) {
        const QString path = args.at(1);
        QMetaObject::invokeMethod(&w, [path, &w]() {
            // Trigger loadProject via direct call is private; replicate via QTimer
        }, Qt::QueuedConnection);
    }

    return app.exec();
}

/*
 * VideoEditor - main.cpp
 *
 * Entry point with comprehensive crash diagnostics:
 *   - Logs all Qt messages to %TEMP%/VideoEditor.log (or /tmp on Linux)
 *   - Catches C++ exceptions and shows a MessageBox with the error
 *   - Installs a Windows unhandled-exception filter to log access violations
 *   - Prints Qt version, plugin path, and init milestones so we can see
 *     exactly where the app fails if it crashes.
 *
 * The binary is built with WIN32_EXECUTABLE=ON so no console window flashes
 * on Windows; on crash a MessageBox appears instead.
 */

#include <QApplication>
#include <QFile>
#include <QStyleFactory>
#include <QMessageBox>
#include <QDateTime>
#include <QDir>
#include <QStandardPaths>
#include <QTextStream>
#include <QDebug>
#include <QLibraryInfo>
#include <QMessageLogContext>

#ifdef Q_OS_WIN
#  include <windows.h>
#endif

#include "ui/MainWindow.h"
#include "utils/GenTime.h"

// ----------------------------------------------------------------------------
// Globals for the message handler / crash handler
// ----------------------------------------------------------------------------
static QtMessageHandler g_oldMessageHandler = nullptr;
static QString           g_logFilePath;

// ----------------------------------------------------------------------------
// Qt message handler: writes to %TEMP%/VideoEditor.log with timestamp + level
// ----------------------------------------------------------------------------
static void veMessageHandler(QtMsgType type, const QMessageLogContext& ctx, const QString& msg) {
    QString level;
    switch (type) {
        case QtDebugMsg:    level = "DEBUG";    break;
        case QtInfoMsg:     level = "INFO";     break;
        case QtWarningMsg:  level = "WARN";     break;
        case QtCriticalMsg: level = "CRIT";     break;
        case QtFatalMsg:    level = "FATAL";    break;
    }

    QString line = QString("[%1] %2: %3")
        .arg(QDateTime::currentDateTime().toString("HH:mm:ss.zzz"))
        .arg(level)
        .arg(msg);
    if (ctx.file) {
        line += QString("  (%1:%2)").arg(ctx.file).arg(ctx.line);
    }
    line += "\n";

    // Append to log file (open/append/close so the log survives a crash).
    QFile f(g_logFilePath);
    if (f.open(QIODevice::Append | QIODevice::Text)) {
        f.write(line.toUtf8());
        f.close();
    }

    // Also call the old handler (for OutputDebugString / stderr).
    if (g_oldMessageHandler) {
        g_oldMessageHandler(type, ctx, msg);
    }

    // For fatal messages, show a MessageBox on Windows GUI subsystem apps.
    if (type == QtFatalMsg) {
        QMessageBox::critical(nullptr, "VideoEditor - Fatal Error",
            QString("VideoEditor hit a fatal error and must close.\n\n"
                    "Error: %1\n\n"
                    "Log file: %2\n\n"
                    "Please send the log file to the developer.").arg(msg).arg(g_logFilePath));
    }
}

// ----------------------------------------------------------------------------
// Windows unhandled exception filter: logs the exception code + address.
// ----------------------------------------------------------------------------
#ifdef Q_OS_WIN
static LONG WINAPI veCrashHandler(EXCEPTION_POINTERS* ep) {
    if (!ep || !ep->ExceptionRecord) return EXCEPTION_CONTINUE_SEARCH;

    QFile f(g_logFilePath);
    if (f.open(QIODevice::Append | QIODevice::Text)) {
        QTextStream s(&f);
        s << "\n========================================\n";
        s << "=== UNHANDLED EXCEPTION (CRASH) ===\n";
        s << "========================================\n";
        s << "Timestamp: " << QDateTime::currentDateTime().toString() << "\n";
        s << "Exception code: 0x" << QString::number(ep->ExceptionRecord->ExceptionCode, 16).toUpper() << "\n";
        s << "Address: 0x" << QString::number(reinterpret_cast<quintptr>(ep->ExceptionRecord->ExceptionAddress), 16).toUpper() << "\n";
        s << "Flags: 0x" << QString::number(ep->ExceptionRecord->ExceptionFlags, 16) << "\n";
        // Decode common exception codes
        DWORD code = ep->ExceptionRecord->ExceptionCode;
        QString desc;
        switch (code) {
            case EXCEPTION_ACCESS_VIOLATION:    desc = "Access violation (null pointer deref or bad memory access)"; break;
            case EXCEPTION_ARRAY_BOUNDS_EXCEEDED: desc = "Array bounds exceeded"; break;
            case EXCEPTION_BREAKPOINT:          desc = "Breakpoint"; break;
            case EXCEPTION_DATATYPE_MISALIGNMENT: desc = "Datatype misalignment"; break;
            case EXCEPTION_FLT_DIVIDE_BY_ZERO:  desc = "Float divide by zero"; break;
            case EXCEPTION_FLT_OVERFLOW:        desc = "Float overflow"; break;
            case EXCEPTION_ILLEGAL_INSTRUCTION: desc = "Illegal instruction"; break;
            case EXCEPTION_IN_PAGE_ERROR:       desc = "In-page error"; break;
            case EXCEPTION_INT_DIVIDE_BY_ZERO:  desc = "Integer divide by zero"; break;
            case EXCEPTION_INT_OVERFLOW:        desc = "Integer overflow"; break;
            case EXCEPTION_INVALID_DISPOSITION: desc = "Invalid disposition"; break;
            case EXCEPTION_NONCONTINUABLE_EXCEPTION: desc = "Noncontinuable exception"; break;
            case EXCEPTION_PRIV_INSTRUCTION:    desc = "Privileged instruction"; break;
            case EXCEPTION_SINGLE_STEP:         desc = "Single step"; break;
            case EXCEPTION_STACK_OVERFLOW:      desc = "Stack overflow"; break;
            default:                            desc = "Unknown exception"; break;
        }
        s << "Description: " << desc << "\n";
        // For access violations, the exception record has 2 extra params:
        // [0] = 0=read, 1=write, 8=DEP execute;  [1] = address accessed.
        if (code == EXCEPTION_ACCESS_VIOLATION && ep->ExceptionRecord->NumberParameters >= 2) {
            ULONG_PTR op = ep->ExceptionRecord->ExceptionInformation[0];
            ULONG_PTR addr = ep->ExceptionRecord->ExceptionInformation[1];
            QString opStr = (op == 0) ? "READ" : (op == 1) ? "WRITE" : (op == 8) ? "DEP/EXECUTE" : QString("op=%1").arg(op);
            s << "Access type: " << opStr << " at address 0x"
              << QString::number(addr, 16).toUpper() << "\n";
        }
        f.close();
    }

    // Show a MessageBox to the user
    QString code = QString("0x%1").arg(ep->ExceptionRecord->ExceptionCode, 0, 16).toUpper();
    QString addr = QString("0x%1").arg(reinterpret_cast<quintptr>(ep->ExceptionRecord->ExceptionAddress), 0, 16).toUpper();
    QMessageBox::critical(nullptr, "VideoEditor - Crash",
        QString("VideoEditor crashed.\n\n"
                "Exception code: %1\n"
                "Address: %2\n\n"
                "Log file: %3\n\n"
                "Please send the log file to the developer.").arg(code).arg(addr).arg(g_logFilePath));

    return EXCEPTION_EXECUTE_HANDLER;
}
#endif

// ----------------------------------------------------------------------------
// Initialize log file path + write startup banner
// ----------------------------------------------------------------------------
static void initLogFile() {
    QString logDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    if (logDir.isEmpty()) logDir = QDir::tempPath();
    QDir().mkpath(logDir);
    g_logFilePath = logDir + QDir::separator() + "VideoEditor.log";

    // Truncate previous log
    QFile lf(g_logFilePath);
    if (lf.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        QTextStream s(&lf);
        s << "VideoEditor v0.3.0 starting at "
          << QDateTime::currentDateTime().toString(Qt::ISODate) << "\n";
        s << "Log file: " << g_logFilePath << "\n";
        s << "Temp dir: " << logDir << "\n";
        s << "========================================\n\n";
        lf.close();
    }
}

// ----------------------------------------------------------------------------
// Entry point
// ----------------------------------------------------------------------------
int main(int argc, char* argv[]) {
    // Initialize log file FIRST so we capture everything that follows.
    initLogFile();

    // Install Qt message handler EARLY, before QApplication, so we capture
    // any Qt warnings during QApplication construction (e.g. missing plugin).
    g_oldMessageHandler = qInstallMessageHandler(veMessageHandler);

    // Write a startup marker to the log using only CRT (no Qt) so we know
    // the handler is installed even if QApplication fails to construct.
    {
        QFile f(g_logFilePath);
        if (f.open(QIODevice::Append | QIODevice::Text)) {
            f.write("--- main() entered, message handler installed ---\n");
            f.close();
        }
    }

#ifdef Q_OS_WIN
    // Install the Windows unhandled exception filter (catches access violations etc.)
    SetUnhandledExceptionFilter(veCrashHandler);
#endif

    try {
        // Construct QApplication FIRST.
        // Do NOT call QStyleFactory or any GUI function before this - that is a
        // classic Qt crash cause. QStyleFactory::create internally queries
        // QGuiApplication::style() and triggers GUI subsystem init; doing that
        // before QApplication exists leads to access violations (0xC0000005)
        // in qwindows.dll or Qt6Gui.dll.
        qDebug() << "Constructing QApplication...";
        QApplication app(argc, argv);
        qDebug() << "QApplication constructed OK.";
        app.setApplicationName("VideoEditor");
        app.setOrganizationName("VideoEditor");
        app.setApplicationVersion("0.3.0");

        // NOW it is safe to set the style (Fusion is built into Qt6Core,
        // no plugin needed).
        qDebug() << "Setting Fusion style...";
        QStyle* fusionStyle = QStyleFactory::create("Fusion");
        if (fusionStyle) {
            QApplication::setStyle(fusionStyle);
            qDebug() << "Style set OK (Fusion).";
        } else {
            qWarning() << "Fusion style not available; using Qt default style.";
            // List available styles for diagnostics
            qDebug() << "Available styles:" << QStyleFactory::keys();
        }

        // Diagnostic info: log Qt version, plugin path, and where Qt is looking
        // for the platform plugin. This is the #1 cause of "opens then closes".
        qDebug() << "Qt compile-time version:" << QT_VERSION_STR;
        qDebug() << "Qt runtime version:     " << qVersion();
        qDebug() << "Plugins path:           " << QLibraryInfo::path(QLibraryInfo::PluginsPath);
        qDebug() << "Prefix path:            " << QLibraryInfo::path(QLibraryInfo::PrefixPath);
        qDebug() << "Data path:              " << QLibraryInfo::path(QLibraryInfo::DataPath);
        qDebug() << "Binaries path:          " << QLibraryInfo::path(QLibraryInfo::BinariesPath);
        qDebug() << "Application dir path:   " << QApplication::applicationDirPath();
        qDebug() << "Current dir:            " << QDir::currentPath();

        // Verify the platform plugin exists. If it doesn't, Qt will qFatal()
        // at QApplication construction - but our log will at least show what
        // was checked.
        QString platformsDir = QApplication::applicationDirPath() + "/platforms";
        qDebug() << "Looking for platform plugin at:" << platformsDir << "/qwindows.dll";
        if (QFile::exists(platformsDir + "/qwindows.dll")) {
            qDebug() << "  -> qwindows.dll FOUND";
        } else {
            qCritical() << "  -> qwindows.dll MISSING! App will crash.";
        }

        // Load dark theme stylesheet from resources.
        qDebug() << "Loading stylesheet from :/styles/dark.qss ...";
        QFile qss(":/styles/dark.qss");
        if (qss.open(QIODevice::ReadOnly | QIODevice::Text)) {
            app.setStyleSheet(QString::fromUtf8(qss.readAll()));
            qDebug() << "  -> stylesheet loaded (" << qss.size() << " bytes)";
        } else {
            qWarning() << "  -> could not open :/styles/dark.qss";
        }

        // Construct the main window. This is where most crashes would happen
        // (Project / TimelineModel / BinModel setup + UI layout).
        qDebug() << "Constructing MainWindow...";
        ve::MainWindow w;
        qDebug() << "MainWindow constructed OK.";

        w.resize(1600, 1000);
        w.show();
        qDebug() << "Window shown, entering Qt event loop.";

        int ret = app.exec();
        qDebug() << "Event loop exited with code" << ret;
        return ret;

    } catch (const std::exception& e) {
        QString err = QString("std::exception: %1").arg(e.what());
        qCritical() << err;
        QMessageBox::critical(nullptr, "VideoEditor - Error",
            QString("VideoEditor crashed:\n\n%1\n\nLog file: %2").arg(err).arg(g_logFilePath));
        return 1;
    } catch (...) {
        qCritical() << "Unknown C++ exception caught in main()";
        QMessageBox::critical(nullptr, "VideoEditor - Error",
            QString("VideoEditor crashed with an unknown exception.\n\nLog file: %1").arg(g_logFilePath));
        return 1;
    }
}

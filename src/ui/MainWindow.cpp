/*
 * VideoEditor - MainWindow.cpp
 */
#include "ui/MainWindow.h"
#include "ui/tabs/TopTabBar.h"
#include "ui/tools/ToolStrip.h"
#include "ui/transport/TransportBar.h"
#include "ui/adjust/AdjustPanel.h"
#include "ui/bin/BinWidget.h"
#include "ui/monitor/ProjectMonitor.h"
#include "ui/timeline2/TimelineWidget.h"
// Legacy (kept for menu actions, will be removed)
#include "ui/toolbar/Toolbar.h"
#include "ui/bin/ClipMonitorWidget.h"
#include "ui/monitor/MonitorManager.h"
#include "ui/effects/EffectStackView.h"
#include "ui/properties/PropertiesPanel.h"
#include "project/Project.h"
#include "project/ProjectSerializer.h"
#include "project/ProfileRepository.h"
#include "model/TimelineModel.h"
#include "model/TrackModel.h"
#include "model/ClipModel.h"
#include "model/BinClip.h"

#include <QApplication>
#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
#include <QAction>
#include <QKeySequence>
#include <QFileDialog>
#include <QMessageBox>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QUrl>
#include <QFileInfo>
#include <QSplitter>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QProcess>
#include <QStandardPaths>
#include <QDir>
#include <QDateTime>
#include <QDebug>

namespace ve {

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , project_(new Project(this))
{
    qDebug() << "MainWindow: setting minimum size...";
    setMinimumSize(1440, 900);
    qDebug() << "MainWindow: calling setupUi()...";
    setupUi();        // creates child widgets first
    qDebug() << "MainWindow: setupUi() done.";
    qDebug() << "MainWindow: calling setupActions()...";
    setupActions();   // then wires actions to those widgets
    qDebug() << "MainWindow: setupActions() done.";
    qDebug() << "MainWindow: calling setupMenus()...";
    setupMenus();
    qDebug() << "MainWindow: setupMenus() done.";
    qDebug() << "MainWindow: calling updateWindowTitle()...";
    updateWindowTitle();
    qDebug() << "MainWindow: construction complete.";
}

void MainWindow::setupActions() {
    actNew_ = new QAction(QIcon::fromTheme(QStringLiteral("document-new")), "&New Project", this);
    actNew_->setShortcut(QKeySequence::New);
    connect(actNew_, &QAction::triggered, this, &MainWindow::newProject);

    actOpen_ = new QAction(QIcon::fromTheme(QStringLiteral("document-open")), "&Open Project...", this);
    actOpen_->setShortcut(QKeySequence::Open);
    connect(actOpen_, &QAction::triggered, this, &MainWindow::loadProject);

    actSave_ = new QAction(QIcon::fromTheme(QStringLiteral("document-save")), "&Save", this);
    actSave_->setShortcut(QKeySequence::Save);
    connect(actSave_, &QAction::triggered, this, &MainWindow::saveProject);

    actSaveAs_ = new QAction(QIcon::fromTheme(QStringLiteral("document-save-as")), "Save &As...", this);
    connect(actSaveAs_, &QAction::triggered, this, &MainWindow::saveProjectAs);

    actImport_ = new QAction(QIcon::fromTheme(QStringLiteral("list-add")), "&Import Media...", this);
    actImport_->setShortcut(QKeySequence("Ctrl+I"));
    connect(actImport_, &QAction::triggered, this, [this]() {
        const QStringList files = QFileDialog::getOpenFileNames(
            this, "Import Media", QString(),
            "Media Files (*.mp4 *.mov *.mkv *.avi *.webm *.m4v "
            "*.png *.jpg *.jpeg *.bmp *.webp "
            "*.mp3 *.wav *.aac *.flac *.ogg *.m4a)");
        if (!files.isEmpty()) importFiles(files);
    });

    actExport_ = new QAction(QIcon::fromTheme(QStringLiteral("document-send")), "&Export / Render...", this);
    actExport_->setShortcut(QKeySequence("Ctrl+E"));
    connect(actExport_, &QAction::triggered, this, &MainWindow::exportProject);

    actUndo_ = project_->undoStack()->createUndoAction(this, "&Undo");
    actUndo_->setShortcut(QKeySequence::Undo);
    actRedo_ = project_->undoStack()->createRedoAction(this, "&Redo");
    actRedo_->setShortcut(QKeySequence::Redo);

    actDelete_ = new QAction("&Delete Selected Clip", this);
    actDelete_->setShortcut(QKeySequence::Delete);
    actDelete_->setShortcutContext(Qt::ApplicationShortcut);
    connect(actDelete_, &QAction::triggered, timeline_, &TimelineWidget::deleteSelectedClip);

    actPlay_ = new QAction(QIcon::fromTheme(QStringLiteral("media-playback-start")), "Play/Pause", this);
    actPlay_->setShortcut(QKeySequence(Qt::Key_Space));
    actPlay_->setShortcutContext(Qt::ApplicationShortcut);
    connect(actPlay_, &QAction::triggered, projectMonitor_, &ProjectMonitor::togglePlay);
}

void MainWindow::setupUi() {
    qDebug() << "setupUi: creating TopTabBar...";
    topTabBar_ = new TopTabBar(this);
    qDebug() << "setupUi: creating ToolStrip...";
    toolStrip_ = new ToolStrip(this);
    qDebug() << "setupUi: creating BinWidget...";
    binWidget_ = new BinWidget(project_->bin(), this);
    qDebug() << "setupUi: creating ProjectMonitor (single center)...";
    projectMonitor_ = new ProjectMonitor(project_, this);
    qDebug() << "setupUi: creating TransportBar...";
    transportBar_ = new TransportBar(this);
    qDebug() << "setupUi: creating AdjustPanel...";
    adjustPanel_ = new AdjustPanel(this);
    qDebug() << "setupUi: creating TimelineWidget...";
    timeline_ = new TimelineWidget(project_, this);
    qDebug() << "setupUi: all child widgets created.";

    // --- Center column: preview + transport ---
    auto* centerCol = new QWidget;
    auto* centerLayout = new QVBoxLayout(centerCol);
    centerLayout->setContentsMargins(0, 0, 0, 0);
    centerLayout->setSpacing(0);
    centerLayout->addWidget(projectMonitor_, 1);
    centerLayout->addWidget(transportBar_);

    // --- Main row: tool strip | bin | center | adjust ---
    auto* mainSplit = new QSplitter(Qt::Horizontal);
    mainSplit->addWidget(toolStrip_);
    mainSplit->addWidget(binWidget_);
    mainSplit->addWidget(centerCol);
    mainSplit->addWidget(adjustPanel_);
    mainSplit->setStretchFactor(0, 0);   // tool strip fixed
    mainSplit->setStretchFactor(1, 1);   // bin
    mainSplit->setStretchFactor(2, 3);   // center preview
    mainSplit->setStretchFactor(3, 1);   // adjust
    mainSplit->setSizes({48, 250, 800, 300});

    // --- Vertical split: main row on top, timeline on bottom ---
    auto* verticalSplit = new QSplitter(Qt::Vertical);
    verticalSplit->addWidget(mainSplit);
    verticalSplit->addWidget(timeline_);
    verticalSplit->setStretchFactor(0, 3);
    verticalSplit->setStretchFactor(1, 2);
    verticalSplit->setSizes({600, 400});

    auto* central = new QWidget;
    auto* v = new QVBoxLayout(central);
    v->setContentsMargins(0, 0, 0, 0);
    v->setSpacing(0);
    v->addWidget(topTabBar_);
    v->addWidget(verticalSplit, 1);

    setCentralWidget(central);
    qDebug() << "setupUi: layout assembled, central widget set.";

    // --- Wire signals ---

    // Top tab bar → switch panel content (future: per-tab panel switching)
    connect(topTabBar_, &TopTabBar::tabChanged, this, [this](TopTabBar::Tab t) {
        onTabChanged(static_cast<int>(t));
    });

    // Tool strip → change cursor / behavior
    connect(toolStrip_, &ToolStrip::toolChanged, this, [this](ToolStrip::Tool t) {
        onToolChanged(static_cast<int>(t));
    });

    // Transport bar → playback
    connect(transportBar_, &TransportBar::playClicked, projectMonitor_, &ProjectMonitor::togglePlay);
    connect(transportBar_, &TransportBar::skipStartClicked, projectMonitor_, &ProjectMonitor::skipToStart);
    connect(transportBar_, &TransportBar::skipEndClicked, projectMonitor_, &ProjectMonitor::skipToEnd);
    connect(transportBar_, &TransportBar::playheadMoved, projectMonitor_, &ProjectMonitor::setPlayhead);

    // Project monitor → transport bar (timecode update)
    connect(projectMonitor_, &ProjectMonitor::playheadMoved, transportBar_, &TransportBar::setPlayhead);
    connect(projectMonitor_, &ProjectMonitor::playheadMoved, timeline_, &TimelineWidget::setPlayhead);
    connect(projectMonitor_, &ProjectMonitor::playbackTicked, timeline_, &TimelineWidget::setPlayhead);

    // Timeline → project monitor (playhead sync)
    connect(timeline_, &TimelineWidget::playheadChanged, projectMonitor_, &ProjectMonitor::setPlayhead);
    connect(timeline_, &TimelineWidget::playheadChanged, transportBar_, &TransportBar::setPlayhead);

    // Project monitor → transport bar (playing state)
    connect(projectMonitor_, &ProjectMonitor::playheadMoved, this, [this](double) {
        transportBar_->setPlaying(projectMonitor_->isPlaying());
    });

    // Project monitor → adjust panel (update scopes when frame changes)
    connect(projectMonitor_, &ProjectMonitor::frameRendered, adjustPanel_, &AdjustPanel::updateScopes);

    // Adjust panel → project monitor (re-render with new color grade)
    connect(adjustPanel_, &AdjustPanel::gradeChanged, this, [this]() {
        projectMonitor_->setColorGrade(adjustPanel_->grade());
    });

    // Set initial duration on transport bar
    transportBar_->setDuration(projectMonitor_->playhead() > 0 ? projectMonitor_->playhead() : 0);
    // Update duration when timeline changes
    connect(project_->timeline().get(), &TimelineModel::structureChanged, this, [this]() {
        double dur = project_->timeline()->framesToSeconds(project_->timeline()->duration());
        transportBar_->setDuration(dur);
    });

    // Undo stack → dirty + window title
    connect(project_->undoStack(), &QUndoStack::indexChanged, this, [this]() {
        project_->setDirty(true);
        updateWindowTitle();
    });

    setAcceptDrops(true);

    // Status bar with selection info (left) and timecode (right)
    auto* statusLeft = new QLabel("Ready. Drag media files here or use File → Import.");
    statusBar()->addWidget(statusLeft);
    auto* timecodeLabel = new QLabel("00:00:00.00");
    timecodeLabel->setStyleSheet("color: #5ac8fa; font-family: monospace; font-size: 10pt; padding: 0 12px;");
    statusBar()->addPermanentWidget(timecodeLabel);
    connect(timeline_, &TimelineWidget::playheadChanged, this, [timecodeLabel](double t) {
        int total = static_cast<int>(t);
        int h = total / 3600;
        int m = (total / 60) % 60;
        int s = total % 60;
        int f = static_cast<int>((t - total) * 30);
        timecodeLabel->setText(QString("%1:%2:%3.%4")
            .arg(h, 2, 10, QChar('0'))
            .arg(m, 2, 10, QChar('0'))
            .arg(s, 2, 10, QChar('0'))
            .arg(f, 2, 10, QChar('0')));
    });
    connect(timeline_, &TimelineWidget::clipSelected, this, [statusLeft](ClipModel* clip) {
        if (clip) {
            auto bc = clip->binClip();
            if (bc) statusLeft->setText(QString("Selected: %1").arg(bc->name()));
        } else {
            statusLeft->setText("No clip selected.");
        }
    });

    qDebug() << "setupUi: signals wired.";
}

void MainWindow::onTabChanged(int tabId) {
    qDebug() << "Tab changed to:" << tabId;
    // For now, the tab determines what the LEFT panel shows.
    // In a future iteration we'll swap binWidget_ for a different widget
    // per tab (Text library, Transitions library, etc.).
    // For v0.4.0 we keep the bin visible but log the tab change.
}

void MainWindow::onToolChanged(int toolId) {
    qDebug() << "Tool changed to:" << toolId;
    // The tool determines cursor and click behavior on the timeline.
    // For v0.4.0 we just set the cursor; razor tool will split clips in
    // a future iteration.
    switch (static_cast<ToolStrip::Tool>(toolId)) {
        case ToolStrip::Tool::Select:
            timeline_->setCursor(Qt::ArrowCursor);
            break;
        case ToolStrip::Tool::Move:
            timeline_->setCursor(Qt::OpenHandCursor);
            break;
        case ToolStrip::Tool::Razor:
            timeline_->setCursor(Qt::IBeamCursor);
            break;
        default:
            timeline_->setCursor(Qt::ArrowCursor);
            break;
    }
}

void MainWindow::setupMenus() {
    auto* fileMenu = menuBar()->addMenu("&File");
    fileMenu->addAction(actNew_);
    fileMenu->addAction(actOpen_);
    fileMenu->addAction(actSave_);
    fileMenu->addAction(actSaveAs_);
    fileMenu->addSeparator();
    fileMenu->addAction(actImport_);
    fileMenu->addSeparator();
    fileMenu->addAction(actExport_);
    fileMenu->addSeparator();
    fileMenu->addAction(QIcon::fromTheme(QStringLiteral("application-exit")), "&Quit",
                        qApp, &QApplication::quit, QKeySequence::Quit);

    auto* editMenu = menuBar()->addMenu("&Edit");
    editMenu->addAction(actUndo_);
    editMenu->addAction(actRedo_);
    editMenu->addSeparator();
    editMenu->addAction(actDelete_);

    auto* viewMenu = menuBar()->addMenu("&View");
    viewMenu->addAction("Toggle &Properties Panel", properties_, &PropertiesPanel::toggleVisible,
                        QKeySequence("Ctrl+P"));

    auto* helpMenu = menuBar()->addMenu("&Help");
    helpMenu->addAction("&About", this, [this]() {
        QMessageBox::information(this, "About VideoEditor",
            "<b>VideoEditor 0.2</b><br><br>"
            "Cross-platform C++/Qt6/FFmpeg video editor, redesigned with "
            "Kdenlive-inspired architecture: Bin/Timeline split, "
            "EffectStackModel, lambda undo, XML project format.<br><br>"
            "Shortcuts:<br>"
            "Space - play/pause<br>"
            "Delete - delete selected clip<br>"
            "Ctrl+Z / Ctrl+Y - undo/redo<br>"
            "Ctrl+S - save project<br>"
            "Ctrl+I - import media<br>"
            "Ctrl+E - export render");
    });
}

void MainWindow::updateWindowTitle() {
    QString name = project_->filePath().isEmpty() ? "Untitled"
        : QFileInfo(project_->filePath()).completeBaseName();
    setWindowTitle(QStringLiteral("%1%2 - VideoEditor")
        .arg(name, project_->isDirty() ? " *" : ""));
}

void MainWindow::newProject() {
    if (project_->isDirty()) {
        auto r = QMessageBox::question(this, "New Project", "Discard unsaved changes?");
        if (r != QMessageBox::Yes) return;
    }
    delete project_;
    project_ = new Project(this);
    timeline_->setProject(project_);
    binWidget_->setBin(project_->bin());
    projectMonitor_->setProject(project_);
    properties_->setClip(nullptr);
    effectStack_->setClip(nullptr);
    updateWindowTitle();
}

void MainWindow::loadProject() {
    const QString path = QFileDialog::getOpenFileName(
        this, "Open Project", QString(), "VideoEditor Project (*.veproj)");
    if (path.isEmpty()) return;
    QString err;
    if (!ProjectSerializer::load(project_, path, &err)) {
        QMessageBox::warning(this, "Open failed", err);
        return;
    }
    timeline_->setProject(project_);
    binWidget_->setBin(project_->bin());
    projectMonitor_->setProject(project_);
    properties_->setClip(nullptr);
    effectStack_->setClip(nullptr);
    project_->setDirty(false);
    updateWindowTitle();
}

void MainWindow::saveProject() {
    if (project_->filePath().isEmpty()) { saveProjectAs(); return; }
    QString err;
    if (!ProjectSerializer::save(project_, project_->filePath(), &err)) {
        QMessageBox::warning(this, "Save failed", err);
        return;
    }
    project_->setDirty(false);
    updateWindowTitle();
}

void MainWindow::saveProjectAs() {
    const QString path = QFileDialog::getSaveFileName(
        this, "Save Project As", QString(), "VideoEditor Project (*.veproj)");
    if (path.isEmpty()) return;
    QString p = path;
    if (!p.endsWith(".veproj", Qt::CaseInsensitive)) p += ".veproj";
    project_->setFilePath(p);
    QString err;
    if (!ProjectSerializer::save(project_, p, &err)) {
        QMessageBox::warning(this, "Save failed", err);
        return;
    }
    project_->setDirty(false);
    updateWindowTitle();
}

void MainWindow::importFiles(const QStringList& files) {
    for (const QString& f : files) {
        project_->bin()->addClip(f);
    }
    project_->setDirty(true);
    updateWindowTitle();
    statusBar()->showMessage(QStringLiteral("Imported %1 file(s).").arg(files.size()), 4000);
}

void MainWindow::exportProject() {
    const QString defaultName = QStringLiteral("export_%1.mp4")
        .arg(QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss"));
    QString defaultDir = QStandardPaths::writableLocation(QStandardPaths::MoviesLocation);
    if (defaultDir.isEmpty() || !QDir(defaultDir).exists()) {
        QDir().mkpath(QDir::homePath() + "/VideoEditorExports");
        defaultDir = QDir::homePath() + "/VideoEditorExports";
    }
    const QString out = QFileDialog::getSaveFileName(
        this, "Export Video", defaultDir + "/" + defaultName, "MP4 Video (*.mp4)");
    if (out.isEmpty()) return;

    // Build ffmpeg command: concat clips from the first video track.
    auto tl = project_->timeline();
    auto bin = project_->bin();
    QStringList args;
    args << "-y";

    // Collect unique source files
    QMap<QString, int> sourceIndex;
    QStringList sourcesInOrder;
    for (ObjectId tid : tl->trackIds()) {
        auto t = tl->track(tid);
        if (!t || t->type() == TrackType::Audio) continue;
        for (ObjectId clipId : t->clipIds()) {
            auto c = tl->clip(clipId);
            if (!c) continue;
            QString path = c->binClip() ? c->binClip()->sourcePath() : QString();
            if (path.isEmpty() || sourceIndex.contains(path)) continue;
            sourceIndex[path] = sourcesInOrder.size();
            sourcesInOrder.append(path);
            args << "-i" << path;
        }
    }
    if (sourcesInOrder.isEmpty()) {
        QMessageBox::information(this, "Export", "No video/image clips on the timeline.");
        return;
    }

    // Build filter_complex: concat in timeline order
    QStringList filters;
    QStringList concatInputs;
    int streamIdx = 0;
    for (ObjectId tid : tl->trackIds()) {
        auto t = tl->track(tid);
        if (!t || t->type() == TrackType::Audio) continue;
        for (ObjectId clipId : t->clipsSorted()) {
            auto c = tl->clip(clipId);
            if (!c) continue;
            auto bc = c->binClip();
            if (!bc) continue;
            int src = sourceIndex[bc->sourcePath()];
            double in  = tl->framesToSeconds(c->getIn());
            double dur = tl->framesToSeconds(c->getPlaytime());
            if (bc->type() == ClipType::Image) {
                filters << QStringLiteral("[%1:v]trim=duration=%2,setpts=PTS-STARTPTS[v%3];")
                            .arg(src).arg(dur, 0, 'f', 3).arg(streamIdx);
            } else {
                filters << QStringLiteral("[%1:v]trim=start=%2:end=%3,setpts=PTS-STARTPTS[v%4];")
                            .arg(src).arg(in, 0, 'f', 3).arg(in + dur, 0, 'f', 3).arg(streamIdx);
            }
            concatInputs << QStringLiteral("[v%1]").arg(streamIdx);
            streamIdx++;
        }
    }
    if (concatInputs.isEmpty()) {
        QMessageBox::information(this, "Export", "No video/image clips on the timeline.");
        return;
    }
    QString filter = filters.join("") +
        QStringLiteral("%1concat=n=%2:v=1:a=0[outv]")
            .arg(concatInputs.join("")).arg(concatInputs.size());
    args << "-filter_complex" << filter << "-map" << "[outv]";

    args << "-c:v" << "libx264"
         << "-pix_fmt" << "yuv420p"
         << "-r" << QString::number(project_->profile().fps())
         << "-b:v" << "8000k"
         << "-s" << QStringLiteral("%1x%2").arg(project_->profile().width).arg(project_->profile().height)
         << out;

    auto* proc = new QProcess(this);
    proc->setProcessChannelMode(QProcess::MergedChannels);
    connect(proc, qOverload<int, QProcess::ExitStatus>(&QProcess::finished), this,
            [this, proc, out](int code, QProcess::ExitStatus) {
        if (code == 0) {
            QMessageBox::information(this, "Export complete",
                QStringLiteral("Saved to:\n%1").arg(out));
        } else {
            QMessageBox::warning(this, "Export failed",
                QStringLiteral("ffmpeg exited with code %1.\nOutput:\n%2")
                    .arg(code).arg(QString::fromUtf8(proc->readAllStandardOutput())));
        }
        proc->deleteLater();
    });
    QString ffmpegExe = qEnvironmentVariable("FFMPEG_BINARY", "ffmpeg");
    proc->start(ffmpegExe, args);
}

void MainWindow::dragEnterEvent(QDragEnterEvent* e) {
    if (e->mimeData()->hasUrls()) e->acceptProposedAction();
}

void MainWindow::dropEvent(QDropEvent* e) {
    QStringList files;
    for (const QUrl& u : e->mimeData()->urls()) {
        if (u.isLocalFile()) files << u.toLocalFile();
    }
    if (!files.isEmpty()) importFiles(files);
    e->acceptProposedAction();
}

void MainWindow::closeEvent(QCloseEvent* e) {
    if (project_->isDirty()) {
        auto r = QMessageBox::question(this, "Quit", "Discard unsaved changes and quit?");
        if (r != QMessageBox::Yes) { e->ignore(); return; }
    }
    e->accept();
}

} // namespace ve

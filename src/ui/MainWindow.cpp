#include "ui/MainWindow.h"
#include "ui/Toolbar.h"
#include "ui/PreviewWidget.h"
#include "ui/TimelineWidget.h"
#include "ui/PropertiesPanel.h"
#include "core/Command.h"
#include "core/Project.h"
#include "core/Timeline.h"
#include "core/Track.h"
#include "core/Clip.h"
#include "media/MediaDecoder.h"
#include "utils/JsonSerializer.h"

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
#include <QProgressDialog>
#include <QFutureWatcher>
#include <QtConcurrent/QtConcurrentRun>
#include <QStandardPaths>
#include <QDir>
#include <QDateTime>
#include <QDebug>
#include <QPainter>
#include <QProcess>

namespace ve {

static const QStringList VIDEO_EXTS = { "mp4", "mov", "mkv", "avi", "webm", "m4v" };
static const QStringList IMAGE_EXTS = { "png", "jpg", "jpeg", "bmp", "webp" };
static const QStringList AUDIO_EXTS = { "mp3", "wav", "aac", "flac", "ogg", "m4a" };

static bool hasExt(const QString& path, const QStringList& exts) {
    const QString ext = QFileInfo(path).suffix().toLower();
    return exts.contains(ext);
}

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , project_(new Project(this))
    , undoStack_(new QUndoStack(this))
{
    setMinimumSize(1280, 800);
    setupActions();
    setupUi();
    setupMenus();
    updateWindowTitle();

    // Wire undo stack → dirty + window title
    connect(undoStack_, &QUndoStack::indexChanged, this, [this]() {
        project_->setDirty(true);
        updateWindowTitle();
    });

    // Wire timeline selection → properties panel
    connect(timeline_, &TimelineWidget::clipSelected, properties_, &PropertiesPanel::setClip);
    connect(timeline_, &TimelineWidget::clipSelected, preview_, &PreviewWidget::setSelectedClip);

    // Wire timeline playhead → preview
    connect(timeline_, &TimelineWidget::playheadChanged, preview_, &PreviewWidget::setPlayhead);
    connect(preview_, &PreviewWidget::playheadMoved, timeline_, &TimelineWidget::setPlayhead);

    // Playback sync
    connect(preview_, &PreviewWidget::playbackTicked, timeline_, &TimelineWidget::setPlayhead);

    // Keyboard shortcuts handled by actions below.
    setAcceptDrops(true);
    statusBar()->showMessage("Ready. Drag media files here, or use File → Import.");
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

    actUndo_ = undoStack_->createUndoAction(this, "&Undo");
    actUndo_->setShortcut(QKeySequence::Undo);
    actRedo_ = undoStack_->createRedoAction(this, "&Redo");
    actRedo_->setShortcut(QKeySequence::Redo);

    actDelete_ = new QAction("&Delete Selected Clip", this);
    actDelete_->setShortcut(QKeySequence::Delete);
    actDelete_->setShortcutContext(Qt::ApplicationShortcut);
    connect(actDelete_, &QAction::triggered, timeline_, &TimelineWidget::deleteSelectedClip);

    actPlay_ = new QAction(QIcon::fromTheme(QStringLiteral("media-playback-start")), "Play/Pause", this);
    actPlay_->setShortcut(QKeySequence(Qt::Key_Space));
    actPlay_->setShortcutContext(Qt::ApplicationShortcut);
    connect(actPlay_, &QAction::triggered, preview_, &PreviewWidget::togglePlay);
}

void MainWindow::setupUi() {
    toolbar_    = new Toolbar(this);
    preview_    = new PreviewWidget(project_, this);
    timeline_   = new TimelineWidget(project_, undoStack_, this);
    properties_ = new PropertiesPanel(undoStack_, this);

    auto* rightSplit = new QSplitter(Qt::Vertical);
    rightSplit->addWidget(preview_);
    rightSplit->addWidget(properties_);
    rightSplit->setStretchFactor(0, 3);
    rightSplit->setStretchFactor(1, 1);

    auto* mainSplit = new QSplitter(Qt::Horizontal);
    mainSplit->addWidget(rightSplit);
    mainSplit->addWidget(timeline_);
    mainSplit->setStretchFactor(0, 3);
    mainSplit->setStretchFactor(1, 5);

    auto* central = new QWidget;
    auto* v = new QVBoxLayout(central);
    v->setContentsMargins(0, 0, 0, 0);
    v->setSpacing(0);
    v->addWidget(toolbar_);
    v->addWidget(mainSplit, 1);

    setCentralWidget(central);

    // Wire toolbar buttons
    connect(toolbar_, &Toolbar::importClicked, actImport_, &QAction::trigger);
    connect(toolbar_, &Toolbar::exportClicked, actExport_, &QAction::trigger);
    connect(toolbar_, &Toolbar::playClicked,    preview_, &PreviewWidget::togglePlay);
    connect(toolbar_, &Toolbar::stopClicked,    preview_, &PreviewWidget::stop);
    connect(toolbar_, &Toolbar::skipStartClicked, preview_, &PreviewWidget::skipToStart);
    connect(toolbar_, &Toolbar::skipEndClicked,   preview_, &PreviewWidget::skipToEnd);
    connect(toolbar_, &Toolbar::undoClicked, actUndo_, &QAction::trigger);
    connect(toolbar_, &Toolbar::redoClicked, actRedo_, &QAction::trigger);
    connect(toolbar_, &Toolbar::addVideoTrack, this, [this]() {
        undoStack_->push(new LambdaCommand(
            "Add Video Track",
            [this]() { project_->timeline()->addTrack(Track::Kind::Video); },
            [this]() {
                auto& ts = project_->timeline()->tracks();
                if (!ts.isEmpty()) project_->timeline()->removeTrack(ts.last());
            }
        ));
    });
    connect(toolbar_, &Toolbar::addAudioTrack, this, [this]() {
        undoStack_->push(new LambdaCommand(
            "Add Audio Track",
            [this]() { project_->timeline()->addTrack(Track::Kind::Audio); },
            [this]() {
                auto& ts = project_->timeline()->tracks();
                if (!ts.isEmpty()) project_->timeline()->removeTrack(ts.last());
            }
        ));
    });
    connect(toolbar_, &Toolbar::zoomIn,  timeline_, &TimelineWidget::zoomIn);
    connect(toolbar_, &Toolbar::zoomOut, timeline_, &TimelineWidget::zoomOut);
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
    fileMenu->addAction(QIcon::fromTheme(QStringLiteral("application-exit")), "&Quit", qApp, &QApplication::quit,
                        QKeySequence::Quit);

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
            "<b>VideoEditor 0.1</b><br><br>"
            "Cross-platform C++/Qt6/FFmpeg video editor.<br><br>"
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
    QString name = project_->filePath().isEmpty()
        ? "Untitled"
        : QFileInfo(project_->filePath()).completeBaseName();
    setWindowTitle(QStringLiteral("%1%2 - VideoEditor")
                       .arg(name, project_->isDirty() ? " *" : ""));
}

// ---------------------------------------------------------------------------
// Project file ops
// ---------------------------------------------------------------------------
void MainWindow::newProject() {
    if (project_->isDirty()) {
        auto r = QMessageBox::question(this, "New Project",
            "Discard unsaved changes?");
        if (r != QMessageBox::Yes) return;
    }
    delete project_;
    project_ = new Project(this);
    undoStack_->clear();
    timeline_->setProject(project_);
    preview_->setProject(project_);
    properties_->setClip(nullptr);
    updateWindowTitle();
}

void MainWindow::loadProject() {
    const QString path = QFileDialog::getOpenFileName(
        this, "Open Project", QString(), "VideoEditor Project (*.veproj)");
    if (path.isEmpty()) return;
    QString err;
    if (!JsonSerializer::load(project_, path, &err)) {
        QMessageBox::warning(this, "Open failed", err);
        return;
    }
    undoStack_->clear();
    timeline_->setProject(project_);
    preview_->setProject(project_);
    properties_->setClip(nullptr);
    project_->setDirty(false);
    updateWindowTitle();
}

void MainWindow::saveProject() {
    if (project_->filePath().isEmpty()) {
        saveProjectAs();
        return;
    }
    QString err;
    if (!JsonSerializer::save(project_, project_->filePath(), &err)) {
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
    if (!QFileInfo(p).suffix().isEmpty() && QFileInfo(p).suffix().toLower() != "veproj") {
        // keep extension as-is
    } else if (!p.endsWith(".veproj", Qt::CaseInsensitive)) {
        p += ".veproj";
    }
    project_->setFilePath(p);
    QString err;
    if (!JsonSerializer::save(project_, p, &err)) {
        QMessageBox::warning(this, "Save failed", err);
        return;
    }
    project_->setDirty(false);
    updateWindowTitle();
}

// ---------------------------------------------------------------------------
// Media import
// ---------------------------------------------------------------------------
void MainWindow::importFiles(const QStringList& files) {
    for (const QString& f : files) {
        const QString ext = QFileInfo(f).suffix().toLower();
        MediaType type = MediaType::Video;
        Track::Kind targetKind = Track::Kind::Video;
        if (VIDEO_EXTS.contains(ext)) {
            type = MediaType::Video;
            targetKind = Track::Kind::Video;
        } else if (IMAGE_EXTS.contains(ext)) {
            type = MediaType::Image;
            targetKind = Track::Kind::Image;
        } else if (AUDIO_EXTS.contains(ext)) {
            type = MediaType::Audio;
            targetKind = Track::Kind::Audio;
        } else {
            qWarning() << "Unknown extension, skipping:" << f;
            continue;
        }

        // Probe for duration / dimensions
        MediaInfo info = MediaDecoder::probe(f);
        double dur = info.duration > 0 ? info.duration : 5.0;
        if (type == MediaType::Image) dur = 5.0; // default 5s stills

        // Find first track of the matching kind
        Track* target = nullptr;
        for (Track* t : project_->timeline()->tracks()) {
            if (t->kind() == targetKind) { target = t; break; }
        }
        if (!target) {
            target = project_->timeline()->addTrack(targetKind);
        }

        // Place clip at end of track
        double placeAt = 0.0;
        for (Clip* c : target->clips()) {
            double end = c->timelineStart() + c->duration();
            if (end > placeAt) placeAt = end;
        }

        auto* clip = new Clip;
        clip->setSourcePath(f);
        clip->setType(type);
        clip->setSourceIn(0.0);
        clip->setSourceOut(dur);
        clip->setTimelineStart(placeAt);
        target->addClip(clip);

        maybeGenerateThumbnailsForNewClip(clip);
        project_->setDirty(true);
    }
    updateWindowTitle();
    statusBar()->showMessage(QStringLiteral("Imported %1 file(s).").arg(files.size()), 4000);
}

void MainWindow::maybeGenerateThumbnailsForNewClip(Clip* clip) {
    // Generate thumbnail off the UI thread
    const QString path = clip->sourcePath();
    const MediaType type = clip->type();
    Clip* clipPtr = clip;
    auto* watcher = new QFutureWatcher<QImage>(this);
    connect(watcher, &QFutureWatcher<QImage>::finished, this, [watcher, clipPtr, type]() {
        QImage img = watcher->result();
        if (!img.isNull()) clipPtr->setThumbnail(img);
        if (type == MediaType::Audio) {
            // Waveform thumb can be generated separately; for now leave empty
        }
        watcher->deleteLater();
    });
    if (type == MediaType::Video) {
        watcher->setFuture(QtConcurrent::run([path]() {
            MediaDecoder dec;
            if (!dec.open(path)) return QImage();
            return dec.grabFrame(0.5, 160, 90);
        }));
    } else if (type == MediaType::Image) {
        watcher->setFuture(QtConcurrent::run([path]() {
            QImage img(path);
            if (!img.isNull()) return img.scaled(160, 90, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            return QImage();
        }));
    } else {
        // Audio: render waveform
        watcher->setFuture(QtConcurrent::run([path]() -> QImage {
            MediaDecoder dec;
            if (!dec.open(path)) return QImage();
            auto peaks = dec.audioPeaks(200);
            QImage img(200, 60, QImage::Format_ARGB32);
            img.fill(Qt::transparent);
            QPainter p(&img);
            p.setPen(QColor(80, 200, 255));
            for (int i = 0; i < (int)peaks.size(); ++i) {
                int h = std::clamp(static_cast<int>(peaks[i] * img.height()), 0, img.height());
                int y0 = (img.height() - h) / 2;
                p.drawLine(i, y0, i, y0 + h);
            }
            return img;
        }));
    }
}

// ---------------------------------------------------------------------------
// Export / Render
// ---------------------------------------------------------------------------
void MainWindow::exportProject() {
    const QString defaultName = QStringLiteral("export_%1.mp4")
        .arg(QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss"));
    const QString defaultDir = QStandardPaths::writableLocation(QStandardPaths::MoviesLocation);
    if (defaultDir.isEmpty() || !QDir(defaultDir).exists()) {
        QDir().mkpath(QDir::homePath() + "/VideoEditorExports");
    }
    const QString startDir = (defaultDir.isEmpty() ? QDir::homePath() + "/VideoEditorExports" : defaultDir)
        + "/" + defaultName;
    const QString out = QFileDialog::getSaveFileName(
        this, "Export Video", startDir, "MP4 Video (*.mp4)");
    if (out.isEmpty()) return;

    // Use ffmpeg CLI as a simple, robust render path. Build an edit decision
    // list from the timeline and pass it to ffmpeg via filter_complex.
    QStringList args;
    args << "-y";

    // Collect unique source files
    QMap<QString, int> sourceIndex;
    QList<QString> sourcesInOrder;
    for (Track* t : project_->timeline()->tracks()) {
        for (Clip* c : t->clips()) {
            if (!sourceIndex.contains(c->sourcePath())) {
                sourceIndex[c->sourcePath()] = sourcesInOrder.size();
                sourcesInOrder.append(c->sourcePath());
                args << "-i" << c->sourcePath();
            }
        }
    }
    if (sourcesInOrder.isEmpty()) {
        QMessageBox::information(this, "Export", "Timeline is empty; nothing to export.");
        return;
    }

    // Build filter_complex: for each clip, trim from source and place on a
    // numbered video/audio stream. Concatenate them in timeline order on
    // a per-track basis, then overlay / mix across tracks.
    // For "basics" we render the first video-track chain concatenated.
    QStringList filters;
    QStringList concatInputs;
    int streamIdx = 0;
    for (Track* t : project_->timeline()->tracks()) {
        if (t->kind() == Track::Kind::Audio) continue;
        for (Clip* c : t->clips()) {
            int src = sourceIndex[c->sourcePath()];
            double in  = c->sourceIn();
            double dur = c->duration();
            if (c->type() == MediaType::Image) {
                filters << QStringLiteral("[%1:d]trim=duration=%2,setpts=PTS-STARTPTS[v%3];")
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

    // Encoding settings
    args << "-c:v" << "libx264"
         << "-pix_fmt" << "yuv420p"
         << "-r" << QString::number(project_->exportFps())
         << "-b:v" << QStringLiteral("%1k").arg(project_->exportBitrateKbps())
         << "-s" << QStringLiteral("%1x%2").arg(project_->exportWidth()).arg(project_->exportHeight())
         << out;

    // Show progress dialog while running
    auto* prog = new QProgressDialog("Rendering…", "Cancel", 0, 0, this);
    prog->setWindowModality(Qt::ApplicationModal);
    prog->setCancelButton(nullptr);
    prog->setMinimumDuration(0);
    prog->setValue(0);
    prog->setLabelText(QStringLiteral("Running: ffmpeg %1").arg(args.join(" ")));

    auto* proc = new QProcess(this);
    proc->setProcessChannelMode(QProcess::MergedChannels);
    connect(proc, &QProcess::readyReadStandardOutput, this, [prog, proc]() {
        prog->setLabelText(proc->readAllStandardOutput().trimmed().mid(0, 200));
    });
    connect(proc, qOverload<int, QProcess::ExitStatus>(&QProcess::finished), this,
            [this, prog, proc, out](int code, QProcess::ExitStatus) {
        prog->close();
        prog->deleteLater();
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

// ---------------------------------------------------------------------------
// Drag & drop
// ---------------------------------------------------------------------------
void MainWindow::dragEnterEvent(QDragEnterEvent* e) {
    if (e->mimeData()->hasUrls()) {
        e->acceptProposedAction();
    }
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
        auto r = QMessageBox::question(this, "Quit",
            "Discard unsaved changes and quit?");
        if (r != QMessageBox::Yes) { e->ignore(); return; }
    }
    e->accept();
}

} // namespace ve

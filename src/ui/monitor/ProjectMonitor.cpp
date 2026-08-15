#include "ui/monitor/ProjectMonitor.h"
#include "project/Project.h"
#include "model/TimelineModel.h"
#include "model/TrackModel.h"
#include "model/ClipModel.h"
#include "model/BinClip.h"
#include "media/MediaBackend.h"
#include "media/ColorGrader.h"

#include <QPainter>
#include <QPaintEvent>
#include <QLinearGradient>

namespace ve {

ProjectMonitor::ProjectMonitor(Project* project, QWidget* parent)
    : QWidget(parent)
    , project_(project)
{
    setMinimumSize(480, 270);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setAttribute(Qt::WA_OpaquePaintEvent, true);

    connect(&tickTimer_, &QTimer::timeout, this, [this]() {
        if (!playing_) return;
        double elapsed = elapsed_.elapsed() / 1000.0;
        double t = playStartPlayhead_ + elapsed;
        double dur = timelineDuration();
        if (t >= dur) { t = dur; stop(); }
        setPlayhead(t);
        emit playbackTicked(t);
    });
    tickTimer_.setInterval(33);
}

void ProjectMonitor::setProject(Project* p) {
    project_ = p;
    playhead_ = 0.0;
    renderCurrentFrame();
    update();
}

void ProjectMonitor::setPlayhead(double t) {
    if (t < 0) t = 0;
    playhead_ = t;
    renderCurrentFrame();
    update();
}

void ProjectMonitor::togglePlay() {
    if (playing_) stop();
    else {
        if (playhead_ >= timelineDuration() - 0.05) playhead_ = 0;
        playing_ = true;
        playStartPlayhead_ = playhead_;
        elapsed_.start();
        tickTimer_.start();
    }
    update();
}

void ProjectMonitor::stop() {
    playing_ = false;
    tickTimer_.stop();
    update();
}

void ProjectMonitor::skipToStart() { setPlayhead(0); }
void ProjectMonitor::skipToEnd()   { setPlayhead(timelineDuration()); }

void ProjectMonitor::setColorGrade(const ColorGrade& g) {
    grade_ = g;
    renderCurrentFrame();
    update();
}

double ProjectMonitor::timelineDuration() const {
    return project_ ? project_->timeline()->framesToSeconds(project_->timeline()->duration()) : 0.0;
}

void ProjectMonitor::renderCurrentFrame() {
    if (!project_) return;
    auto tl = project_->timeline();
    int playheadFrame = tl->secondsToFrames(playhead_);
    ClipModel* best = nullptr;
    for (ObjectId tid : tl->trackIds()) {
        auto t = tl->track(tid);
        if (!t || !t->isVisible() || t->type() == TrackType::Audio) continue;
        for (ObjectId cid : t->clipIds()) {
            auto c = tl->clip(cid);
            if (!c) continue;
            int start = c->getPosition();
            int end   = start + c->getPlaytime();
            if (playheadFrame >= start && playheadFrame < end) {
                best = c.get();
                break;
            }
        }
        if (best) break;
    }
    if (!best) { currentFrame_ = QImage(); rawFrame_ = QImage(); emit frameRendered(currentFrame_); return; }

    auto bc = best->binClip();
    if (!bc) { currentFrame_ = QImage(); rawFrame_ = QImage(); emit frameRendered(currentFrame_); return; }

    double sourceTime = bc->duration() > 0
        ? (playhead_ - tl->framesToSeconds(best->getPosition()))
        : 0.0;

    if (bc->type() == ClipType::Image) {
        rawFrame_ = QImage(bc->sourcePath());
    } else {
        auto backend = createDefaultBackend();
        if (backend->open(bc->sourcePath())) {
            rawFrame_ = backend->grabFrame(sourceTime, width(), height());
        }
    }

    // Apply color grade
    currentFrame_ = rawFrame_;
    if (!currentFrame_.isNull() && !grade_.isDefault()) {
        ColorGrader::apply(currentFrame_, grade_);
    }

    emit frameRendered(currentFrame_);
}

void ProjectMonitor::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    // Background gradient
    QLinearGradient bg(rect().topLeft(), rect().bottomLeft());
    bg.setColorAt(0, QColor(0x0d, 0x0e, 0x12));
    bg.setColorAt(1, QColor(0x16, 0x18, 0x1c));
    p.fillRect(rect(), bg);

    if (!currentFrame_.isNull()) {
        QSize sz = currentFrame_.size();
        sz.scale(width() - 20, height() - 40, Qt::KeepAspectRatio);
        QRect dst(QPoint((width() - sz.width()) / 2, 20), sz);
        p.drawImage(dst, currentFrame_);

        // Draw subtle border around video frame
        p.setPen(QColor(0, 0, 0, 180));
        p.setBrush(Qt::NoBrush);
        p.drawRect(dst.adjusted(-1, -1, 0, 0));
    } else {
        p.setPen(QColor(120, 120, 130));
        QFont f = p.font(); f.setPointSize(12); p.setFont(f);
        p.drawText(rect(), Qt::AlignCenter,
                   "No preview available.\nImport media and place clips on the timeline.");
    }

    // Top overlay bar (monitor type + timecode)
    QRect topBar(0, 0, width(), 20);
    p.fillRect(topBar, QColor(0x1c, 0x1f, 0x25, 200));
    p.setPen(QColor(0x5a, 0xc8, 0xfa));
    QFont f = p.font(); f.setPointSize(9); f.setBold(true); p.setFont(f);
    p.drawText(topBar.adjusted(8, 0, -8, 0), Qt::AlignLeft | Qt::AlignVCenter, "PROJECT MONITOR");

    // Timecode on the right of top bar
    QString state = playing_ ? QStringLiteral("\u25B6 ") : QStringLiteral("\u23F8 ");
    QString tc = QString("%1%2s / %3s")
        .arg(state)
        .arg(playhead_, 0, 'f', 2)
        .arg(timelineDuration(), 0, 'f', 2);
    p.setPen(QColor(0xe0, 0xe0, 0xe6));
    p.drawText(topBar.adjusted(8, 0, -8, 0), Qt::AlignRight | Qt::AlignVCenter, tc);

    // Bottom overlay: frame info
    if (project_) {
        QRect bottomBar(0, height() - 20, width(), 20);
        p.fillRect(bottomBar, QColor(0x1c, 0x1f, 0x25, 200));
        auto tl = project_->timeline();
        int playheadFrame = tl->secondsToFrames(playhead_);
        int totalFrames = tl->duration();
        p.setPen(QColor(0x8a, 0x8d, 0x96));
        QFont f2 = p.font(); f2.setPointSize(9); p.setFont(f2);
        p.drawText(bottomBar.adjusted(8, 0, -8, 0), Qt::AlignLeft | Qt::AlignVCenter,
                   QString("Frame: %1 / %2").arg(playheadFrame).arg(totalFrames));
        // FPS on the right
        p.drawText(bottomBar.adjusted(8, 0, -8, 0), Qt::AlignRight | Qt::AlignVCenter,
                   QString("%1 fps").arg(tl->fps(), 0, 'f', 2));
    }
}

} // namespace ve

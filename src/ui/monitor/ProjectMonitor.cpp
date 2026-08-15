#include "ui/monitor/ProjectMonitor.h"
#include "project/Project.h"
#include "model/TimelineModel.h"
#include "model/TrackModel.h"
#include "model/ClipModel.h"
#include "model/BinClip.h"
#include "media/MediaBackend.h"

#include <QPainter>
#include <QPaintEvent>

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
    if (!best) { currentFrame_ = QImage(); return; }

    auto bc = best->binClip();
    if (!bc) { currentFrame_ = QImage(); return; }

    double sourceTime = bc->duration() > 0
        ? (playhead_ - tl->framesToSeconds(best->getPosition()))
        : 0.0;

    if (bc->type() == ClipType::Image) {
        currentFrame_ = QImage(bc->sourcePath());
    } else {
        auto backend = createDefaultBackend();
        if (backend->open(bc->sourcePath())) {
            currentFrame_ = backend->grabFrame(sourceTime, width(), height());
        }
    }
}

void ProjectMonitor::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.fillRect(rect(), QColor(15, 15, 18));
    if (!currentFrame_.isNull()) {
        QSize sz = currentFrame_.size();
        sz.scale(width() - 20, height() - 40, Qt::KeepAspectRatio);
        QRect dst(QPoint((width() - sz.width()) / 2, 20), sz);
        p.drawImage(dst, currentFrame_);
    } else {
        p.setPen(QColor(120, 120, 130));
        QFont f = p.font(); f.setPointSize(14); p.setFont(f);
        p.drawText(rect(), Qt::AlignCenter,
                   "No preview available.\nImport media and place clips on the timeline.");
    }
    p.setPen(QColor(220, 220, 230));
    QFont f = p.font(); f.setPointSize(10); p.setFont(f);
    QString state = playing_ ? QStringLiteral("\u25B6") : QStringLiteral("\u23F8");
    p.drawText(10, 16, QStringLiteral("PROJECT MONITOR  %1  %2s / %3s")
        .arg(state)
        .arg(playhead_, 0, 'f', 2)
        .arg(timelineDuration(), 0, 'f', 2));
}

} // namespace ve

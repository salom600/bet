#include "ui/PreviewWidget.h"
#include "core/Project.h"
#include "core/Timeline.h"
#include "core/Track.h"
#include "core/Clip.h"
#include "media/MediaDecoder.h"

#include <QPainter>
#include <QPaintEvent>
#include <QMouseEvent>
#include <QApplication>
#include <QDebug>

namespace ve {

PreviewWidget::PreviewWidget(Project* project, QWidget* parent)
    : QWidget(parent)
    , project_(project)
{
    setMinimumSize(480, 270);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setAttribute(Qt::WA_OpaquePaintEvent, true);
    setMouseTracking(true);

    connect(&tickTimer_, &QTimer::timeout, this, [this]() {
        if (!playing_) return;
        const double elapsed = elapsed_.elapsed() / 1000.0;
        double t = playStartPlayhead_ + elapsed;
        const double dur = timelineDuration();
        if (t >= dur) {
            t = dur;
            stop();
        }
        setPlayhead(t);
        emit playbackTicked(t);
    });
    tickTimer_.setInterval(33); // ~30fps refresh
}

void PreviewWidget::setProject(Project* p) {
    project_ = p;
    playhead_ = 0.0;
    renderCurrentFrame();
    update();
}

void PreviewWidget::setSelectedClip(Clip* c) {
    selectedClip_ = c;
    update();
}

void PreviewWidget::setPlayhead(double t) {
    if (t < 0) t = 0;
    playhead_ = t;
    renderCurrentFrame();
    update();
}

void PreviewWidget::togglePlay() {
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

void PreviewWidget::stop() {
    playing_ = false;
    tickTimer_.stop();
    update();
}

void PreviewWidget::skipToStart() { setPlayhead(0); }
void PreviewWidget::skipToEnd()   { setPlayhead(timelineDuration()); }

double PreviewWidget::timelineDuration() const {
    return project_ ? project_->timeline()->duration() : 0.0;
}

void PreviewWidget::renderCurrentFrame() {
    if (!project_) return;
    // Find the top-most visible video/image clip at the playhead
    Clip* best = nullptr;
    for (Track* t : project_->timeline()->tracks()) {
        if (!t->isVisible()) continue;
        if (t->kind() == Track::Kind::Audio) continue;
        for (Clip* c : t->clips()) {
            double start = c->timelineStart();
            double end   = start + c->duration();
            if (playhead_ >= start && playhead_ < end) {
                best = c;
                break;
            }
        }
        if (best) break;
    }
    if (!best) { currentFrame_ = QImage(); return; }

    const double sourceTime = best->sourceIn() + (playhead_ - best->timelineStart());

    if (best->type() == MediaType::Image) {
        QImage img(best->sourcePath());
        if (!img.isNull()) currentFrame_ = img;
    } else if (best->type() == MediaType::Video) {
        MediaDecoder dec;
        if (dec.open(best->sourcePath())) {
            currentFrame_ = dec.grabFrame(sourceTime, width(), height());
        }
    }
}

void PreviewWidget::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.fillRect(rect(), QColor(15, 15, 18));

    if (!currentFrame_.isNull()) {
        // Maintain aspect ratio, scale to fit
        QSize sz = currentFrame_.size();
        sz.scale(width() - 20, height() - 20, Qt::KeepAspectRatio);
        QRect dst((width() - sz.width()) / 2, (height() - sz.height()) / 2, sz);
        p.drawImage(dst, currentFrame_);
    } else {
        p.setPen(QColor(120, 120, 130));
        QFont f = p.font();
        f.setPointSize(14);
        p.setFont(f);
        p.drawText(rect(), Qt::AlignCenter,
                   "No preview available.\nImport media and place clips on the timeline.");
    }

    // Top-left overlay: playhead time + state
    p.setPen(QColor(220, 220, 230));
    QFont f = p.font();
    f.setPointSize(10);
    p.setFont(f);
    QString state = playing_ ? "▶" : "⏸";
    p.drawText(10, 18, QStringLiteral("%1  %2s / %3s")
        .arg(state)
        .arg(playhead_, 0, 'f', 2)
        .arg(timelineDuration(), 0, 'f', 2));
}

void PreviewWidget::mousePressEvent(QMouseEvent* e) {
    // Clicking the preview seeks the playhead proportionally to the click x
    // only if it's within the rendered frame area.
    if (e->button() == Qt::LeftButton) {
        // Treat as no-op to avoid accidental seek; user controls playhead via timeline
        e->accept();
    }
}

void PreviewWidget::resizeEvent(QResizeEvent*) {
    renderCurrentFrame();
}

} // namespace ve

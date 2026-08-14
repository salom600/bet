#include "ui/ClipItem.h"
#include "ui/TimelineWidget.h"
#include "core/Clip.h"
#include "core/Track.h"
#include "core/Timeline.h"
#include "core/Command.h"

#include <QPainter>
#include <QPaintEvent>
#include <QMouseEvent>
#include <QDebug>
#include <cmath>
#include <algorithm>

namespace ve {

ClipItem::ClipItem(Clip* clip, TimelineWidget* owner, QWidget* parent)
    : QWidget(parent)
    , clip_(clip)
    , owner_(owner)
{
    setMouseTracking(true);
    setAttribute(Qt::WA_OpaquePaintEvent, false);
    refreshGeometry();
    connect(clip_, &Clip::changed, this, [this]() { refreshGeometry(); update(); });
}

void ClipItem::refreshGeometry() {
    int x = owner_->timeToX(clip_->timelineStart());
    int w = std::max(12, owner_->timeToX(clip_->duration()));
    if (parentWidget()) {
        setGeometry(x, 2, w, parentWidget()->height() - 4);
    }
}

void ClipItem::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    // Background color depends on type
    QColor bg;
    switch (clip_->type()) {
        case MediaType::Video: bg = QColor(72, 130, 200); break;
        case MediaType::Image: bg = QColor(160, 110, 200); break;
        case MediaType::Audio: bg = QColor(80, 180, 130); break;
    }
    if (clip_->opacity() < 1.0) bg.setAlphaF(0.5 + 0.5 * clip_->opacity());

    p.fillRect(rect(), bg);

    // Draw thumbnail if available
    if (!clip_->thumbnail().isNull()) {
        QImage thumb = clip_->thumbnail();
        QRect thumbRect = rect().adjusted(2, 2, -2, -16);
        p.drawImage(thumbRect, thumb.scaled(thumbRect.size(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
    }

    // Trim edges
    p.setPen(QPen(QColor(255, 255, 255, 180), 1));
    p.setBrush(QColor(255, 255, 255, 60));
    p.drawRect(0, 0, edgeWidth_, height());
    p.drawRect(width() - edgeWidth_, 0, edgeWidth_, height());

    // Border
    p.setPen(QColor(0, 0, 0, 120));
    p.drawRect(0, 0, width() - 1, height() - 1);

    // Label
    p.setPen(QColor(255, 255, 255, 230));
    QFont f = p.font();
    f.setPointSize(8);
    f.setBold(true);
    p.setFont(f);
    QString name = clip_->sourcePath().section('/', -1);
    if (name.isEmpty()) name = "clip";
    QRect labelRect(8, height() - 14, width() - 16, 12);
    p.drawText(labelRect, Qt::AlignLeft | Qt::AlignVCenter,
               name.left(30) + QStringLiteral("  %1s")
                   .arg(clip_->duration(), 0, 'f', 1));
}

void ClipItem::mousePressEvent(QMouseEvent* e) {
    pressPos_ = e->position().toPoint();
    pressTimelineStart_ = clip_->timelineStart();
    pressSourceIn_  = clip_->sourceIn();
    pressSourceOut_ = clip_->sourceOut();

    if (e->button() != Qt::LeftButton) return;

    // Check if click is on left/right trim handle
    if (pressPos_.x() <= edgeWidth_) {
        drag_ = DragMode::TrimLeft;
    } else if (pressPos_.x() >= width() - edgeWidth_) {
        drag_ = DragMode::TrimRight;
    } else {
        drag_ = DragMode::Move;
    }
    emit selected(clip_);
    grabMouse();
}

void ClipItem::mouseMoveEvent(QMouseEvent* e) {
    if (drag_ == DragMode::None) return;

    // Find parent track; if locked, ignore moves
    Track* track = qobject_cast<Track*>(clip_->parent());
    if (track && track->isLocked()) return;

    int dx = e->position().toPoint().x() - pressPos_.x();
    double dt = owner_->xToTime(dx);

    if (drag_ == DragMode::Move) {
        double newStart = pressTimelineStart_ + dt;
        newStart = std::max(0.0, owner_->snap(newStart));
        // Apply directly (move uses undo via TimelineWidget on release for simplicity)
        clip_->setTimelineStart(newStart);
        refreshGeometry();
        emit moved(clip_, newStart);
    } else if (drag_ == DragMode::TrimLeft) {
        double newStart  = pressTimelineStart_ + dt;
        double newIn     = pressSourceIn_ + dt;
        if (newIn < 0)            { newIn = 0; newStart = pressTimelineStart_ - pressSourceIn_; }
        if (newIn >= pressSourceOut_ - 0.05) return;
        newStart = std::max(0.0, owner_->snap(newStart));
        double inDelta = newStart - clip_->timelineStart();
        clip_->setSourceIn(pressSourceIn_ + inDelta);
        clip_->setTimelineStart(newStart);
        refreshGeometry();
        emit trimmed(clip_, clip_->sourceIn(), clip_->sourceOut());
    } else if (drag_ == DragMode::TrimRight) {
        double newOut = pressSourceOut_ + dt;
        if (newOut <= clip_->sourceIn() + 0.05) return;
        // Snap right edge to playhead / neighbor if close
        double endT = clip_->timelineStart() + (newOut - clip_->sourceIn());
        endT = owner_->snap(endT);
        newOut = clip_->sourceIn() + (endT - clip_->timelineStart());
        if (newOut <= clip_->sourceIn() + 0.05) return;
        clip_->setSourceOut(newOut);
        refreshGeometry();
        emit trimmed(clip_, clip_->sourceIn(), clip_->sourceOut());
    }
}

void ClipItem::mouseReleaseEvent(QMouseEvent*) {
    if (drag_ != DragMode::None) {
        releaseMouse();
        // Push undo via TimelineWidget's undo stack using current state
        auto* stack = owner_->undoStack();
        double startBefore = pressTimelineStart_;
        double inBefore    = pressSourceIn_;
        double outBefore   = pressSourceOut_;
        double startAfter  = clip_->timelineStart();
        double inAfter     = clip_->sourceIn();
        double outAfter    = clip_->sourceOut();

        if (drag_ == DragMode::Move) {
            if (std::fabs(startAfter - startBefore) > 1e-6) {
                stack->push(new LambdaCommand(
                    "Move clip",
                    [this, startAfter]() { clip_->setTimelineStart(startAfter); refreshGeometry(); },
                    [this, startBefore]() { clip_->setTimelineStart(startBefore); refreshGeometry(); }
                ));
                // The push already called redo() which set startAfter (already current).
                // Undo stack will replay correctly.
            }
        } else {
            // Trim
            stack->push(new LambdaCommand(
                "Trim clip",
                [this, inAfter, outAfter, startAfter]() {
                    clip_->setSourceIn(inAfter);
                    clip_->setSourceOut(outAfter);
                    clip_->setTimelineStart(startAfter);
                    refreshGeometry();
                },
                [this, inBefore, outBefore, startBefore]() {
                    clip_->setSourceIn(inBefore);
                    clip_->setSourceOut(outBefore);
                    clip_->setTimelineStart(startBefore);
                    refreshGeometry();
                }
            ));
        }
        drag_ = DragMode::None;
    }
}

} // namespace ve

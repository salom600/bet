#include "ui/timeline2/ClipItem.h"
#include "ui/timeline2/TimelineWidget.h"
#include "model/ClipModel.h"
#include "model/BinClip.h"
#include "model/TimelineModel.h"
#include "model/TrackModel.h"
#include "project/Project.h"

#include <QPainter>
#include <QPaintEvent>
#include <QMouseEvent>
#include <QDebug>
#include <algorithm>

namespace ve {

ClipItem::ClipItem(ClipModel* clip, TimelineWidget* owner, QWidget* parent)
    : QWidget(parent)
    , clip_(clip)
    , owner_(owner)
{
    setMouseTracking(true);
    refreshGeometry();
    connect(clip_, &ClipModel::changed, this, [this]() { refreshGeometry(); update(); });
}

void ClipItem::refreshGeometry() {
    auto tl = owner_->project()->timeline();
    int x = owner_->timeToX(tl->framesToSeconds(clip_->getPosition()));
    int w = std::max(12, owner_->timeToX(tl->framesToSeconds(clip_->getPlaytime())));
    if (parentWidget()) {
        setGeometry(x, 2, w, parentWidget()->height() - 4);
    }
}

void ClipItem::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    QColor bg;
    auto bc = clip_->binClip();
    if (bc) {
        switch (bc->type()) {
            case ClipType::Video:
            case ClipType::AV:     bg = QColor(72, 130, 200); break;
            case ClipType::Image:  bg = QColor(160, 110, 200); break;
            case ClipType::Audio:  bg = QColor(80, 180, 130); break;
            default:               bg = QColor(120, 120, 120); break;
        }
    } else {
        bg = QColor(120, 120, 120);
    }
    p.fillRect(rect(), bg);

    if (bc) {
        QImage thumb = bc->thumbnail();
        if (!thumb.isNull()) {
            QRect thumbRect = rect().adjusted(2, 2, -2, -16);
            p.drawImage(thumbRect, thumb.scaled(thumbRect.size(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
        }
    }

    p.setPen(QPen(QColor(255, 255, 255, 180), 1));
    p.setBrush(QColor(255, 255, 255, 60));
    p.drawRect(0, 0, edgeWidth_, height());
    p.drawRect(width() - edgeWidth_, 0, edgeWidth_, height());

    p.setPen(QColor(0, 0, 0, 120));
    p.drawRect(0, 0, width() - 1, height() - 1);

    p.setPen(QColor(255, 255, 255, 230));
    QFont f = p.font();
    f.setPointSize(8);
    f.setBold(true);
    p.setFont(f);
    QString name = bc ? bc->name() : "clip";
    QRect labelRect(8, height() - 14, width() - 16, 12);
    p.drawText(labelRect, Qt::AlignLeft | Qt::AlignVCenter,
               name.left(30) + QStringLiteral("  %1f")
                   .arg(clip_->getPlaytime()));
}

void ClipItem::mousePressEvent(QMouseEvent* e) {
    pressPos_ = e->position().toPoint();
    pressPosition_ = clip_->getPosition();
    pressIn_  = clip_->getIn();
    pressOut_ = clip_->getOut();

    if (e->button() != Qt::LeftButton) return;

    if (pressPos_.x() <= edgeWidth_)             drag_ = DragMode::TrimLeft;
    else if (pressPos_.x() >= width() - edgeWidth_) drag_ = DragMode::TrimRight;
    else                                        drag_ = DragMode::Move;

    owner_->project()->timeline()->setSelected(clip_->getId());
    emit selected(clip_->getId());
    grabMouse();
}

void ClipItem::mouseMoveEvent(QMouseEvent* e) {
    if (drag_ == DragMode::None) return;
    auto tl = owner_->project()->timeline();
    auto track = tl->track(clip_->getCurrentTrackId());
    if (track && track->isLocked()) return;

    int dx = e->position().toPoint().x() - pressPos_.x();
    int dtFrames = tl->secondsToFrames(owner_->xToTime(dx));

    if (drag_ == DragMode::Move) {
        int newPos = std::max(0, pressPosition_ + dtFrames);
        // Snapping
        int snapped = tl->snap(newPos, static_cast<int>(owner_->snapTolerance() * tl->fps()));
        if (snapped >= 0) newPos = snapped;
        tl->requestClipMove(clip_->getId(), clip_->getCurrentTrackId(), newPos);
        refreshGeometry();
    } else if (drag_ == DragMode::TrimLeft) {
        int newIn  = pressIn_ + dtFrames;
        int newPos = pressPosition_ + dtFrames;
        if (newIn < 0) { newIn = 0; newPos = pressPosition_ - pressIn_; }
        int curLen = pressOut_ - newIn;
        if (curLen < 1) return;
        // Use requestClipResize with fromStart=true
        int snapped = tl->snap(newPos, static_cast<int>(owner_->snapTolerance() * tl->fps()));
        if (snapped >= 0) newPos = snapped;
        // Adjust: clip->setIn + clip->setPosition atomically
        int delta = newPos - clip_->getPosition();
        clip_->setIn(clip_->getIn() + delta);
        clip_->setPosition(newPos);
        refreshGeometry();
    } else if (drag_ == DragMode::TrimRight) {
        int newOut = pressOut_ + dtFrames;
        if (newOut <= clip_->getIn() + 1) return;
        int endFrame = clip_->getPosition() + (newOut - clip_->getIn());
        int snapped = tl->snap(endFrame, static_cast<int>(owner_->snapTolerance() * tl->fps()));
        if (snapped >= 0) newOut = clip_->getIn() + (snapped - clip_->getPosition());
        if (newOut <= clip_->getIn() + 1) return;
        clip_->setOut(newOut);
        refreshGeometry();
    }
}

void ClipItem::mouseReleaseEvent(QMouseEvent*) {
    if (drag_ != DragMode::None) {
        releaseMouse();
        drag_ = DragMode::None;
    }
}

} // namespace ve

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
#include <QLinearGradient>
#include <algorithm>
#include <cmath>

namespace ve {

// Kdenlive-inspired color palette for clips
static const QColor VIDEO_COLOR   = QColor( 56, 116, 173);  // cool blue
static const QColor VIDEO_COLOR_DARK = QColor( 32,  72, 120);
static const QColor IMAGE_COLOR   = QColor(149, 102, 184);  // purple
static const QColor AUDIO_COLOR   = QColor( 73, 175, 130);  // green
static const QColor AUDIO_COLOR_DARK = QColor( 42, 110,  82);
static const QColor SELECTED_BORDER = QColor(255, 200,  80);  // Kdenlive's selection yellow
static const QColor TRIM_HANDLE    = QColor(255, 255, 255, 200);

ClipItem::ClipItem(ClipModel* clip, TimelineWidget* owner, QWidget* parent)
    : QWidget(parent)
    , clip_(clip)
    , owner_(owner)
{
    setMouseTracking(true);
    setCursor(Qt::OpenHandCursor);
    refreshGeometry();
    connect(clip_, &ClipModel::changed, this, [this]() { refreshGeometry(); update(); });
}

void ClipItem::refreshGeometry() {
    if (!clip_ || !owner_ || !owner_->project()) return;
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

    auto bc = clip_ ? clip_->binClip() : nullptr;
    if (!bc) return;

    // Determine clip colors based on type
    QColor bgColor, bgColorDark;
    bool isAudio = false;
    switch (bc->type()) {
        case ClipType::Audio:
            bgColor = AUDIO_COLOR; bgColorDark = AUDIO_COLOR_DARK; isAudio = true; break;
        case ClipType::Image:
            bgColor = IMAGE_COLOR; bgColorDark = IMAGE_COLOR.darker(150); break;
        case ClipType::Video:
        case ClipType::AV:
        default:
            bgColor = VIDEO_COLOR; bgColorDark = VIDEO_COLOR_DARK; break;
    }

    // Draw clip background with gradient (top = bright, bottom = dark)
    QRect r = rect();
    QLinearGradient gradient(r.topLeft(), r.bottomLeft());
    gradient.setColorAt(0, bgColor);
    gradient.setColorAt(1, bgColorDark);
    p.fillRect(r, gradient);

    // Draw thumbnail for video/image clips
    if (!isAudio && width() > 30) {
        QImage thumb = bc->thumbnail();
        if (!thumb.isNull()) {
            int thumbH = height() - 20;  // leave room for label
            int thumbW = std::min(width() - 4, thumbH * 16 / 9);
            QRect thumbRect(2, 2, thumbW, thumbH);
            p.drawImage(thumbRect, thumb.scaled(thumbRect.size(), Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
        }
    }

    // Draw audio waveform for audio clips
    if (isAudio && width() > 10) {
        auto peaks = bc->audioPeaks();
        if (!peaks.empty()) {
            int availableWidth = width() - 4;
            int centerY = height() / 2;
            int maxH = (height() - 20) / 2;
            p.setPen(QColor(255, 255, 255, 180));
            for (int i = 0; i < availableWidth && i < (int)peaks.size(); ++i) {
                int h = std::clamp(static_cast<int>(peaks[i] * maxH), 0, maxH);
                p.drawLine(i + 2, centerY - h, i + 2, centerY + h);
            }
        }
    }

    // Top accent strip (bright color)
    p.fillRect(QRect(0, 0, width(), 3), bgColor.lighter(120));

    // Trim handles (left and right edges)
    p.setBrush(TRIM_HANDLE);
    p.setPen(Qt::NoPen);
    p.drawRect(QRect(0, 0, edgeWidth_, height()));
    p.drawRect(QRect(width() - edgeWidth_, 0, edgeWidth_, height()));
    // Trim handle grip lines
    p.setPen(QColor(0, 0, 0, 120));
    for (int i = 0; i < 3; ++i) {
        int y = (i + 1) * height() / 4;
        p.drawLine(2, y, edgeWidth_ - 2, y);
        p.drawLine(width() - edgeWidth_ + 2, y, width() - 2, y);
    }

    // Clip name label at bottom
    p.setPen(QColor(255, 255, 255, 230));
    QFont f = p.font();
    f.setPointSize(8);
    f.setBold(true);
    p.setFont(f);
    QString name = bc->name();
    QRect labelRect(edgeWidth_ + 4, height() - 16, width() - 2 * (edgeWidth_ + 4), 14);
    QString labelText = name.left(30);
    // Add duration suffix if there's room
    if (width() > 120) {
        double dur = owner_->project()->timeline()->framesToSeconds(clip_->getPlaytime());
        labelText += QString("  %1s").arg(dur, 0, 'f', 1);
    }
    p.drawText(labelRect, Qt::AlignLeft | Qt::AlignVCenter, labelText);

    // Selection border
    if (clip_->isSelected()) {
        p.setBrush(Qt::NoBrush);
        p.setPen(QPen(SELECTED_BORDER, 2));
        p.drawRect(QRect(1, 1, width() - 2, height() - 2));
    } else {
        // Normal border
        p.setBrush(Qt::NoBrush);
        p.setPen(QColor(0, 0, 0, 100));
        p.drawRect(0, 0, width() - 1, height() - 1);
    }
}

void ClipItem::mousePressEvent(QMouseEvent* e) {
    pressPos_ = e->position().toPoint();
    if (clip_) {
        pressPosition_ = clip_->getPosition();
        pressIn_  = clip_->getIn();
        pressOut_ = clip_->getOut();
    }

    if (e->button() != Qt::LeftButton) return;

    if (pressPos_.x() <= edgeWidth_)             drag_ = DragMode::TrimLeft;
    else if (pressPos_.x() >= width() - edgeWidth_) drag_ = DragMode::TrimRight;
    else                                        drag_ = DragMode::Move;

    if (clip_ && owner_ && owner_->project()) {
        owner_->project()->timeline()->setSelected(clip_->getId());
    }
    emit selected(clip_ ? clip_->getId() : -1);
    grabMouse();
    setCursor(Qt::ClosedHandCursor);
}

void ClipItem::mouseMoveEvent(QMouseEvent* e) {
    if (drag_ == DragMode::None) return;
    if (!clip_ || !owner_ || !owner_->project()) return;
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
        int snapped = tl->snap(newPos, static_cast<int>(owner_->snapTolerance() * tl->fps()));
        if (snapped >= 0) newPos = snapped;
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
        setCursor(Qt::OpenHandCursor);
        drag_ = DragMode::None;
    }
}

void ClipItem::enterEvent(QEnterEvent* e) {
    QWidget::enterEvent(e);
    setCursor(drag_ == DragMode::None ? Qt::OpenHandCursor : Qt::ClosedHandCursor);
}

void ClipItem::leaveEvent(QEvent* e) {
    QWidget::leaveEvent(e);
    setCursor(Qt::ArrowCursor);
}

} // namespace ve

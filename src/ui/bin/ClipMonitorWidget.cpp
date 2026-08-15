#include "ui/bin/ClipMonitorWidget.h"
#include "model/BinClip.h"
#include "model/BinModel.h"
#include "media/MediaBackend.h"

#include <QPainter>
#include <QPaintEvent>
#include <QDebug>

namespace ve {

ClipMonitorWidget::ClipMonitorWidget(std::shared_ptr<BinModel> bin, QWidget* parent)
    : QWidget(parent)
    , bin_(bin)
{
    setMinimumSize(320, 240);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setAttribute(Qt::WA_OpaquePaintEvent, true);
}

void ClipMonitorWidget::loadBinClip(const QString& binClipId) {
    currentBinClipId_ = binClipId;
    playhead_ = 0.0;
    if (!bin_) return;
    auto bc = bin_->clip(binClipId);
    if (!bc) return;
    if (bc->type() == ClipType::Image) {
        currentFrame_ = QImage(bc->sourcePath());
    } else {
        auto backend = createDefaultBackend();
        if (backend->open(bc->sourcePath())) {
            currentFrame_ = backend->grabFrame(0.5, width(), height());
        }
    }
    update();
}

void ClipMonitorWidget::setPlayhead(double t) {
    playhead_ = t;
    if (!bin_ || currentBinClipId_.isEmpty()) { update(); return; }
    auto bc = bin_->clip(currentBinClipId_);
    if (!bc) { update(); return; }
    if (bc->type() == ClipType::Image) {
        currentFrame_ = QImage(bc->sourcePath());
    } else if (bc->type() == ClipType::Video || bc->type() == ClipType::AV) {
        auto backend = createDefaultBackend();
        if (backend->open(bc->sourcePath())) {
            currentFrame_ = backend->grabFrame(t, width(), height());
        }
    }
    update();
}

void ClipMonitorWidget::togglePlay() { playing_ = !playing_; update(); }
void ClipMonitorWidget::stop()       { playing_ = false; update(); }

void ClipMonitorWidget::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.fillRect(rect(), QColor(15, 15, 18));
    if (!currentFrame_.isNull()) {
        QSize sz = currentFrame_.size();
        sz.scale(width() - 20, height() - 40, Qt::KeepAspectRatio);
        QRect dst(QPoint((width() - sz.width()) / 2, 20), sz);
        p.drawImage(dst, currentFrame_);
    } else {
        p.setPen(QColor(120, 120, 130));
        QFont f = p.font(); f.setPointSize(12); p.setFont(f);
        p.drawText(rect(), Qt::AlignCenter, "Clip Monitor\n(double-click a bin clip)");
    }
    p.setPen(QColor(220, 220, 230));
    QFont f = p.font(); f.setPointSize(10); p.setFont(f);
    p.drawText(10, 16, QStringLiteral("CLIP MONITOR  %1s").arg(playhead_, 0, 'f', 2));
}

} // namespace ve

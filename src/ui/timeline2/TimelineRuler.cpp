#include "ui/timeline2/TimelineRuler.h"
#include "ui/timeline2/TimelineWidget.h"
#include <QPainter>
#include <QPaintEvent>
#include <QMouseEvent>

namespace ve {

TimelineRuler::TimelineRuler(TimelineWidget* owner, QWidget* parent)
    : QWidget(parent), owner_(owner)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setMinimumWidth(800);
    setMouseTracking(true);
}

void TimelineRuler::setPlayhead(double t) {
    playhead_ = t;
    update();
}

void TimelineRuler::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    // Background gradient (subtle)
    QLinearGradient bg(rect().topLeft(), rect().bottomLeft());
    bg.setColorAt(0, QColor(0x14, 0x16, 0x1a));
    bg.setColorAt(1, QColor(0x10, 0x12, 0x16));
    p.fillRect(rect(), bg);

    const double pps = owner_->pixelsPerSecond();
    if (pps <= 0) return;

    // Choose a tick step that yields ~80px between major ticks
    static const double candidates[] = {0.1, 0.25, 0.5, 1, 2, 5, 10, 15, 30, 60, 120, 300, 600};
    double step = 1.0;
    for (double c : candidates) {
        if (c * pps >= 80) { step = c; break; }
    }

    const int h = height();
    const double totalSec = width() / pps;

    // Minor ticks (every 1/5 of major step)
    double minorStep = step / 5.0;
    p.setPen(QColor(0x3a, 0x3f, 0x4a));
    for (double t = 0; t <= totalSec; t += minorStep) {
        int x = static_cast<int>(t * pps);
        p.drawLine(x, h - 4, x, h - 1);
    }

    // Major ticks + labels
    p.setPen(QColor(0x8a, 0x8d, 0x96));
    QFont f = p.font();
    f.setPointSize(8);
    p.setFont(f);
    for (double t = 0; t <= totalSec; t += step) {
        int x = static_cast<int>(t * pps);
        p.drawLine(x, h - 10, x, h - 1);

        // Format as HH:MM:SS or MM:SS depending on duration
        int total = static_cast<int>(t);
        int secs = total % 60;
        int mins = (total / 60) % 60;
        int hours = total / 3600;
        QString label;
        if (hours > 0) {
            label = QString("%1:%2:%3")
                .arg(hours).arg(mins, 2, 10, QChar('0')).arg(secs, 2, 10, QChar('0'));
        } else {
            label = QString("%1:%2")
                .arg(mins).arg(secs, 2, 10, QChar('0'));
        }
        // Add frame number if zoom is high enough
        if (pps > 100) {
            int frameFraction = static_cast<int>((t - total) * 30); // approximate fps
            label += QString(".%1").arg(frameFraction, 2, 10, QChar('0'));
        }
        p.drawText(x + 4, h - 14, label);
    }

    // Bottom border line
    p.setPen(QColor(0x2a, 0x2d, 0x33));
    p.drawLine(0, h - 1, width(), h - 1);

    // Playhead (red triangle + vertical line)
    int px = static_cast<int>(playhead_ * pps);
    if (px >= 0 && px <= width()) {
        // Vertical line
        p.setPen(QPen(QColor(255, 80, 80), 1));
        p.drawLine(px, 0, px, h);
        // Triangle at top
        p.setBrush(QColor(255, 80, 80));
        p.setPen(Qt::NoPen);
        QPolygon tri;
        tri << QPoint(px - 6, 0) << QPoint(px + 6, 0) << QPoint(px, 8);
        p.drawPolygon(tri);
    }
}

void TimelineRuler::mousePressEvent(QMouseEvent* e) {
    if (e->button() == Qt::LeftButton) {
        owner_->setPlayhead(owner_->xToTime(e->position().toPoint().x()));
    }
}

void TimelineRuler::mouseMoveEvent(QMouseEvent* e) {
    if (e->buttons() & Qt::LeftButton) {
        owner_->setPlayhead(owner_->xToTime(e->position().toPoint().x()));
    }
}

} // namespace ve

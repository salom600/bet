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
}

void TimelineRuler::setPlayhead(double t) {
    playhead_ = t;
    update();
}

void TimelineRuler::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.fillRect(rect(), QColor(0x10, 0x12, 0x16));

    const double pps = owner_->pixelsPerSecond();
    static const double candidates[] = {0.1, 0.25, 0.5, 1, 2, 5, 10, 15, 30, 60, 120, 300, 600};
    double step = 1.0;
    for (double c : candidates) {
        if (c * pps >= 80) { step = c; break; }
    }

    p.setPen(QColor(120, 120, 130));
    QFont f = p.font();
    f.setPointSize(8);
    p.setFont(f);

    const int h = height();
    const double totalSec = width() / pps;
    for (double t = 0; t <= totalSec; t += step) {
        int x = static_cast<int>(t * pps);
        p.drawLine(x, h - 8, x, h);
        int total = static_cast<int>(t);
        int m = total / 60;
        int s = total % 60;
        QString label = QStringLiteral("%1:%2").arg(m, 2, 10, QChar('0')).arg(s, 2, 10, QChar('0'));
        p.drawText(x + 2, h - 10, label);
    }

    int px = static_cast<int>(playhead_ * pps);
    p.setPen(QColor(255, 80, 80));
    p.drawLine(px, 0, px, h);
    p.fillRect(px - 4, 0, 8, h, QColor(255, 80, 80));
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

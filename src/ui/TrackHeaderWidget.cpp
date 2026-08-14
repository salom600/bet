#include "ui/TrackHeaderWidget.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QPainter>
#include <QPaintEvent>

namespace ve {

TrackHeaderWidget::TrackHeaderWidget(Track* track, QWidget* parent)
    : QWidget(parent)
    , track_(track)
{
    setStyleSheet("background-color: #1a1c20; border-right: 1px solid #2a2d33;");

    auto* v = new QVBoxLayout(this);
    v->setContentsMargins(6, 4, 6, 4);
    v->setSpacing(2);

    name_ = new QLabel(track_->name(), this);
    QFont f = name_->font();
    f.setBold(true);
    name_->setFont(f);
    name_->setStyleSheet("color: #e0e0e6;");

    auto* controls = new QHBoxLayout;
    controls->setSpacing(2);

    auto makeBtn = [this](const QString& text, const QString& tip) {
        auto* b = new QPushButton(text, this);
        b->setToolTip(tip);
        b->setFixedSize(28, 24);
        b->setCursor(Qt::PointingHandCursor);
        b->setCheckable(true);
        b->setStyleSheet(
            "QPushButton { background: #262931; color: #b0b3bb; border: 1px solid #2e323b; border-radius: 3px; }"
            "QPushButton:hover { background: #2e323d; }"
            "QPushButton:checked { background: #5ac8fa; color: #000; }");
        return b;
    };

    btnMute_  = makeBtn("M", "Mute track");
    btnSolo_  = makeBtn("V", "Toggle visibility");
    btnLock_  = makeBtn("L", "Lock track");

    controls->addWidget(btnMute_);
    controls->addWidget(btnSolo_);
    controls->addWidget(btnLock_);
    controls->addStretch(1);

    v->addWidget(name_);
    v->addLayout(controls);

    connect(btnMute_, &QPushButton::toggled, this, [this](bool on) {
        track_->setMuted(on);
        onChanged();
    });
    connect(btnSolo_, &QPushButton::toggled, this, [this](bool on) {
        track_->setVisible(!on); // when checked = hidden
        onChanged();
    });
    connect(btnLock_, &QPushButton::toggled, this, [this](bool on) {
        track_->setLocked(on);
        onChanged();
    });
    connect(track_, &Track::changed, this, &TrackHeaderWidget::onChanged);

    // Initialize visual state
    btnMute_->setChecked(track_->isMuted());
    btnSolo_->setChecked(!track_->isVisible());
    btnLock_->setChecked(track_->isLocked());
}

void TrackHeaderWidget::onChanged() {
    name_->setText(track_->name());
    btnMute_->blockSignals(true);
    btnSolo_->blockSignals(true);
    btnLock_->blockSignals(true);
    btnMute_->setChecked(track_->isMuted());
    btnSolo_->setChecked(!track_->isVisible());
    btnLock_->setChecked(track_->isLocked());
    btnMute_->blockSignals(false);
    btnSolo_->blockSignals(false);
    btnLock_->blockSignals(false);

    // Visually gray out when hidden/locked/muted
    QString color = "#e0e0e6";
    if (track_->isMuted() || !track_->isVisible() || track_->isLocked()) {
        color = "#5a5d65";
    }
    name_->setStyleSheet(QStringLiteral("color: %1;").arg(color));
    update();
}

void TrackHeaderWidget::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.fillRect(rect(), QColor(0x1a, 0x1c, 0x20));
    if (track_->isLocked()) {
        p.fillRect(rect(), QColor(40, 30, 30, 80));
    }
    if (!track_->isVisible()) {
        p.fillRect(rect(), QColor(20, 20, 20, 120));
    }
}

} // namespace ve

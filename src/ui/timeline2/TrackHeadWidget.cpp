#include "ui/timeline2/TrackHeadWidget.h"
#include "model/TrackModel.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QToolButton>
#include <QPainter>
#include <QPaintEvent>
#include <QDebug>

namespace ve {

TrackHeadWidget::TrackHeadWidget(TrackModel* track, QWidget* parent)
    : QWidget(parent)
    , track_(track)
{
    setObjectName("timelineTrackHeader");

    auto* v = new QVBoxLayout(this);
    v->setContentsMargins(6, 4, 6, 4);
    v->setSpacing(2);

    // Track name row
    auto* nameRow = new QHBoxLayout();
    nameRow->setContentsMargins(0, 0, 0, 0);
    nameRow->setSpacing(4);

    // Track type icon
    auto* typeIcon = new QLabel(this);
    typeIcon->setFixedSize(16, 16);
    switch (track_->type()) {
        case TrackType::Video: typeIcon->setPixmap(QIcon(":/icons/video.svg").pixmap(16, 16)); break;
        case TrackType::Image: typeIcon->setPixmap(QIcon(":/icons/image.svg").pixmap(16, 16)); break;
        case TrackType::Audio: typeIcon->setPixmap(QIcon(":/icons/add-audio-track.svg").pixmap(16, 16)); break;
    }
    nameRow->addWidget(typeIcon);

    name_ = new QLabel(track_->name(), this);
    QFont f = name_->font();
    f.setBold(true);
    f.setPointSize(10);
    name_->setFont(f);
    name_->setStyleSheet("color: #e0e0e6;");
    nameRow->addWidget(name_);
    nameRow->addStretch(1);
    v->addLayout(nameRow);

    // Controls row (mute / hide / lock)
    auto* controls = new QHBoxLayout();
    controls->setContentsMargins(0, 0, 0, 0);
    controls->setSpacing(2);

    auto makeBtn = [this](const QString& iconOn, const QString& tooltip) {
        auto* b = new QToolButton(this);
        b->setIcon(QIcon(iconOn));
        b->setIconSize(QSize(14, 14));
        b->setToolTip(tooltip);
        b->setFixedSize(24, 22);
        b->setCursor(Qt::PointingHandCursor);
        b->setCheckable(true);
        b->setAutoRaise(true);
        b->setStyleSheet(
            "QToolButton { background: #262931; color: #b0b3bb; border: 1px solid #2e323b; border-radius: 3px; }"
            "QToolButton:hover { background: #2e323d; }"
            "QToolButton:checked { background: #5ac8fa; color: #000; border-color: #5ac8fa; }");
        return b;
    };

    btnMute_ = makeBtn(":/icons/mute.svg", "Mute track");
    btnHide_ = makeBtn(":/icons/hide.svg", "Toggle visibility");
    btnLock_ = makeBtn(":/icons/lock.svg", "Lock track");

    controls->addWidget(btnMute_);
    controls->addWidget(btnHide_);
    controls->addWidget(btnLock_);
    controls->addStretch(1);
    v->addLayout(controls);

    connect(btnMute_, &QToolButton::toggled, this, [this](bool on) { track_->setMuted(on); onChanged(); });
    connect(btnHide_, &QToolButton::toggled, this, [this](bool on) { track_->setVisible(!on); onChanged(); });
    connect(btnLock_, &QToolButton::toggled, this, [this](bool on) { track_->setLocked(on); onChanged(); });
    connect(track_, &TrackModel::changed, this, &TrackHeadWidget::onChanged);

    btnMute_->setChecked(track_->isMuted());
    btnHide_->setChecked(!track_->isVisible());
    btnLock_->setChecked(track_->isLocked());

    onChanged();
}

void TrackHeadWidget::onChanged() {
    if (!track_) return;
    name_->setText(track_->name());

    btnMute_->blockSignals(true);
    btnHide_->blockSignals(true);
    btnLock_->blockSignals(true);
    btnMute_->setChecked(track_->isMuted());
    btnHide_->setChecked(!track_->isVisible());
    btnLock_->setChecked(track_->isLocked());
    btnMute_->blockSignals(false);
    btnHide_->blockSignals(false);
    btnLock_->blockSignals(false);

    // Visually gray out when hidden/locked/muted
    QString color = "#e0e0e6";
    if (track_->isMuted() || !track_->isVisible() || track_->isLocked()) {
        color = "#5a5d65";
    }
    name_->setStyleSheet(QStringLiteral("color: %1;").arg(color));
    update();
}

void TrackHeadWidget::paintEvent(QPaintEvent*) {
    QPainter p(this);
    // Background color depends on track type
    QColor bg(0x1a, 0x1c, 0x20);
    switch (track_->type()) {
        case TrackType::Video: bg = QColor(0x1a, 0x1f, 0x2a); break;
        case TrackType::Image: bg = QColor(0x22, 0x1a, 0x2a); break;
        case TrackType::Audio: bg = QColor(0x1a, 0x2a, 0x22); break;
    }
    p.fillRect(rect(), bg);

    // Left accent strip
    QColor accent;
    switch (track_->type()) {
        case TrackType::Video: accent = QColor(0x38, 0x74, 0xad); break;
        case TrackType::Image: accent = QColor(0x95, 0x66, 0xb8); break;
        case TrackType::Audio: accent = QColor(0x49, 0xaf, 0x82); break;
    }
    p.fillRect(QRect(0, 0, 3, height()), accent);

    // Locked overlay
    if (track_->isLocked()) {
        p.fillRect(rect(), QColor(40, 30, 30, 80));
    }
    // Hidden overlay
    if (!track_->isVisible()) {
        p.fillRect(rect(), QColor(20, 20, 20, 120));
    }
}

} // namespace ve

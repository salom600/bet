#include "ui/transport/TransportBar.h"

#include <QHBoxLayout>
#include <QToolButton>
#include <QLabel>
#include <QDebug>

namespace ve {

TransportBar::TransportBar(QWidget* parent)
    : QWidget(parent)
{
    setObjectName("transportBar");
    setFixedHeight(40);

    layout_ = new QHBoxLayout(this);
    layout_->setContentsMargins(12, 4, 12, 4);
    layout_->setSpacing(8);

    // Left: timecode
    timecodeLbl_ = new QLabel("00:00:00.00", this);
    timecodeLbl_->setStyleSheet(
        "color: #5ac8fa; font-family: monospace; font-size: 11pt; font-weight: bold; "
        "min-width: 110px;");
    layout_->addWidget(timecodeLbl_);

    // Skip to start
    btnSkipStart_ = new QToolButton(this);
    btnSkipStart_->setIcon(QIcon(":/icons/skip-start.svg"));
    btnSkipStart_->setIconSize(QSize(18, 18));
    btnSkipStart_->setToolTip("Skip to start");
    btnSkipStart_->setAutoRaise(true);
    btnSkipStart_->setFixedSize(32, 32);
    layout_->addWidget(btnSkipStart_);

    // Play/pause
    btnPlay_ = new QToolButton(this);
    btnPlay_->setIcon(QIcon(":/icons/play.svg"));
    btnPlay_->setIconSize(QSize(20, 20));
    btnPlay_->setToolTip("Play / Pause (Space)");
    btnPlay_->setAutoRaise(true);
    btnPlay_->setFixedSize(40, 32);
    btnPlay_->setStyleSheet(
        "QToolButton { background: #5ac8fa; border-radius: 4px; }"
        "QToolButton:hover { background: #7ad8fa; }");
    layout_->addWidget(btnPlay_);

    // Skip to end
    btnSkipEnd_ = new QToolButton(this);
    btnSkipEnd_->setIcon(QIcon(":/icons/skip-end.svg"));
    btnSkipEnd_->setIconSize(QSize(18, 18));
    btnSkipEnd_->setToolTip("Skip to end");
    btnSkipEnd_->setAutoRaise(true);
    btnSkipEnd_->setFixedSize(32, 32);
    layout_->addWidget(btnSkipEnd_);

    // Scrubber
    scrubber_ = new QSlider(Qt::Horizontal, this);
    scrubber_->setRange(0, 1000);
    scrubber_->setValue(0);
    scrubber_->setMinimumWidth(200);
    layout_->addWidget(scrubber_, 1);

    // Right: duration
    durationLbl_ = new QLabel("00:00:00.00", this);
    durationLbl_->setStyleSheet(
        "color: #8a8d96; font-family: monospace; font-size: 11pt; "
        "min-width: 110px;");
    durationLbl_->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    layout_->addWidget(durationLbl_);

    // Wire
    connect(btnSkipStart_, &QToolButton::clicked, this, &TransportBar::skipStartClicked);
    connect(btnPlay_,      &QToolButton::clicked, this, &TransportBar::playClicked);
    connect(btnSkipEnd_,   &QToolButton::clicked, this, &TransportBar::skipEndClicked);
    connect(scrubber_, &QSlider::sliderMoved, this, &TransportBar::onSliderMoved);
    connect(scrubber_, &QSlider::sliderPressed, this, [this]() {
        onSliderMoved(scrubber_->value());
    });
}

void TransportBar::setPlayhead(double t) {
    playhead_ = t;
    // Update timecode label
    int total = static_cast<int>(t);
    int h = total / 3600;
    int m = (total / 60) % 60;
    int s = total % 60;
    int f = static_cast<int>((t - total) * 30);
    timecodeLbl_->setText(QString("%1:%2:%3.%4")
        .arg(h, 2, 10, QChar('0'))
        .arg(m, 2, 10, QChar('0'))
        .arg(s, 2, 10, QChar('0'))
        .arg(f, 2, 10, QChar('0')));
    // Update scrubber position
    if (duration_ > 0 && !scrubber_->isSliderDown()) {
        int v = static_cast<int>((t / duration_) * 1000);
        QSignalBlocker b(scrubber_);
        scrubber_->setValue(v);
    }
}

void TransportBar::setDuration(double d) {
    duration_ = d;
    int total = static_cast<int>(d);
    int h = total / 3600;
    int m = (total / 60) % 60;
    int s = total % 60;
    int f = static_cast<int>((d - total) * 30);
    durationLbl_->setText(QString("%1:%2:%3.%4")
        .arg(h, 2, 10, QChar('0'))
        .arg(m, 2, 10, QChar('0'))
        .arg(s, 2, 10, QChar('0'))
        .arg(f, 2, 10, QChar('0')));
}

void TransportBar::setPlaying(bool playing) {
    playing_ = playing;
    btnPlay_->setIcon(QIcon(playing ? ":/icons/pause.svg" : ":/icons/play.svg"));
}

void TransportBar::onSliderMoved(int v) {
    if (duration_ <= 0) return;
    double t = (v / 1000.0) * duration_;
    emit playheadMoved(t);
}

} // namespace ve

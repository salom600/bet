#pragma once

#include <QWidget>
#include <QSlider>

class QToolButton;
class QLabel;
class QHBoxLayout;

namespace ve {

/// Transport bar shown BELOW the preview monitor.
/// Contains: timecode (left), scrubber (center), play/pause/skip buttons,
/// resolution dropdown (right).
class TransportBar : public QWidget {
    Q_OBJECT
public:
    explicit TransportBar(QWidget* parent = nullptr);

    void setPlayhead(double t);
    void setDuration(double d);
    void setPlaying(bool playing);

signals:
    void playheadMoved(double t);
    void playClicked();
    void stopClicked();
    void skipStartClicked();
    void skipEndClicked();

private slots:
    void onSliderMoved(int v);

private:
    QHBoxLayout* layout_ = nullptr;
    QLabel*      timecodeLbl_ = nullptr;
    QSlider*     scrubber_ = nullptr;
    QToolButton* btnSkipStart_ = nullptr;
    QToolButton* btnPlay_ = nullptr;
    QToolButton* btnSkipEnd_ = nullptr;
    QLabel*      durationLbl_ = nullptr;
    double playhead_ = 0.0;
    double duration_ = 0.0;
    bool   playing_ = false;
};

} // namespace ve

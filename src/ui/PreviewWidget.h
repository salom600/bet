#pragma once

#include <QWidget>
#include <QImage>
#include <QTimer>
#include <QElapsedTimer>
#include <memory>
#include "media/MediaDecoder.h"

namespace ve {

class Project;
class Clip;

/// Preview widget: displays the current frame at the playhead position.
/// Playback is driven by a QTimer that advances the playhead and re-renders.
class PreviewWidget : public QWidget {
    Q_OBJECT
public:
    explicit PreviewWidget(Project* project, QWidget* parent = nullptr);

    void setProject(Project* p);
    void setSelectedClip(Clip* c);

    double playhead() const { return playhead_; }
    bool isPlaying() const { return playing_; }

public slots:
    void setPlayhead(double t);
    void togglePlay();
    void stop();
    void skipToStart();
    void skipToEnd();

signals:
    void playheadMoved(double t);
    void playbackTicked(double t);

protected:
    void paintEvent(QPaintEvent*) override;
    void mousePressEvent(QMouseEvent*) override;
    void resizeEvent(QResizeEvent*) override;

private:
    void renderCurrentFrame();
    double timelineDuration() const;

    Project* project_ = nullptr;
    Clip*    selectedClip_ = nullptr;

    double   playhead_ = 0.0;
    bool     playing_  = false;

    QImage   currentFrame_;
    QTimer   tickTimer_;
    QElapsedTimer elapsed_;
    double   playStartPlayhead_ = 0.0;
};

} // namespace ve

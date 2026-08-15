#pragma once

#include <QWidget>
#include <QImage>
#include <QTimer>
#include <QElapsedTimer>
#include "model/ColorGrade.h"

namespace ve {

class Project;

/// Project Monitor: shows the rendered timeline frame at the playhead position.
/// Single large center preview (CapCut-style). Applies the color grade from
/// AdjustPanel before displaying.
class ProjectMonitor : public QWidget {
    Q_OBJECT
public:
    explicit ProjectMonitor(Project* project, QWidget* parent = nullptr);

    void setProject(Project* p);

    double playhead() const { return playhead_; }
    bool isPlaying() const { return playing_; }

    /// Set the color grade to apply to preview frames.
    void setColorGrade(const ColorGrade& g);

    /// Get the last rendered frame (for scopes).
    QImage currentFrame() const { return currentFrame_; }

public slots:
    void setPlayhead(double t);
    void togglePlay();
    void stop();
    void skipToStart();
    void skipToEnd();

signals:
    void playheadMoved(double t);
    void playbackTicked(double t);
    void frameRendered(const QImage& frame);

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void renderCurrentFrame();
    double timelineDuration() const;

    Project* project_ = nullptr;
    double playhead_ = 0.0;
    bool   playing_  = false;
    QImage currentFrame_;
    QImage rawFrame_;          // before color grade
    ColorGrade grade_;
    QTimer tickTimer_;
    QElapsedTimer elapsed_;
    double playStartPlayhead_ = 0.0;
};

} // namespace ve

#pragma once

#include <QWidget>
#include <QImage>
#include <QTimer>
#include <QElapsedTimer>

namespace ve {

class Project;

/// Project Monitor: shows the rendered timeline frame at the playhead position.
class ProjectMonitor : public QWidget {
    Q_OBJECT
public:
    explicit ProjectMonitor(Project* project, QWidget* parent = nullptr);

    void setProject(Project* p);

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

private:
    void renderCurrentFrame();
    double timelineDuration() const;

    Project* project_ = nullptr;
    double playhead_ = 0.0;
    bool   playing_  = false;
    QImage currentFrame_;
    QTimer tickTimer_;
    QElapsedTimer elapsed_;
    double playStartPlayhead_ = 0.0;
};

} // namespace ve

#pragma once

#include <QWidget>
#include "core/Clip.h"

namespace ve {

class TimelineWidget;

/// Visual representation of a clip on the timeline. Supports:
///  - drag to move along the timeline (snapping enabled)
///  - drag left/right edges to trim start/end
///  - click to select
class ClipItem : public QWidget {
    Q_OBJECT
public:
    enum class DragMode {
        None,
        Move,
        TrimLeft,
        TrimRight
    };

    ClipItem(Clip* clip, TimelineWidget* owner, QWidget* parent = nullptr);

    Clip* clip() const { return clip_; }

    void refreshGeometry();

signals:
    void selected(Clip* clip);
    void moved(Clip* clip, double newStart);
    void trimmed(Clip* clip, double newIn, double newOut);

protected:
    void paintEvent(QPaintEvent*) override;
    void mousePressEvent(QMouseEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;
    void mouseReleaseEvent(QMouseEvent*) override;

private:
    int  edgeWidth_ = 6;
    Clip* clip_;
    TimelineWidget* owner_;
    DragMode drag_ = DragMode::None;
    QPoint  pressPos_;
    double  pressTimelineStart_ = 0.0;
    double  pressSourceIn_ = 0.0;
    double  pressSourceOut_ = 0.0;
};

} // namespace ve

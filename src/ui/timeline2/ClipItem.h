#pragma once

#include "../../definitions.h"
#include <QWidget>

namespace ve {

class TimelineWidget;
class ClipModel;

class ClipItem : public QWidget {
    Q_OBJECT
public:
    enum class DragMode { None, Move, TrimLeft, TrimRight };

    ClipItem(ClipModel* clip, TimelineWidget* owner, QWidget* parent = nullptr);

    ClipModel* clip() const { return clip_; }
    void refreshGeometry();

signals:
    void selected(int clipId);

protected:
    void paintEvent(QPaintEvent*) override;
    void mousePressEvent(QMouseEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;
    void mouseReleaseEvent(QMouseEvent*) override;

private:
    int edgeWidth_ = 6;
    ClipModel* clip_;
    TimelineWidget* owner_;
    DragMode drag_ = DragMode::None;
    QPoint  pressPos_;
    int     pressPosition_ = 0;
    int     pressIn_  = 0;
    int     pressOut_ = 0;
};

} // namespace ve

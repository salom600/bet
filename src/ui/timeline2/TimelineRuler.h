#pragma once

#include <QWidget>

namespace ve {

class TimelineWidget;

class TimelineRuler : public QWidget {
    Q_OBJECT
public:
    explicit TimelineRuler(TimelineWidget* owner, QWidget* parent = nullptr);
    void setPlayhead(double t);

protected:
    void paintEvent(QPaintEvent*) override;
    void mousePressEvent(QMouseEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;

private:
    TimelineWidget* owner_;
    double playhead_ = 0.0;
};

} // namespace ve

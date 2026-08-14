#pragma once

#include <QWidget>
#include "core/Track.h"

class QPushButton;
class QLabel;

namespace ve {

class Track;

class TrackHeaderWidget : public QWidget {
    Q_OBJECT
public:
    explicit TrackHeaderWidget(Track* track, QWidget* parent = nullptr);

protected:
    void paintEvent(QPaintEvent*) override;

private slots:
    void onChanged();

private:
    Track* track_;
    QLabel* name_;
    QPushButton* btnMute_;
    QPushButton* btnSolo_;  // implemented as "hide"
    QPushButton* btnLock_;
};

} // namespace ve

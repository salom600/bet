#pragma once

#include "../../definitions.h"
#include <QWidget>

namespace ve {

class TrackModel;

class TrackHeadWidget : public QWidget {
    Q_OBJECT
public:
    explicit TrackHeadWidget(TrackModel* track, QWidget* parent = nullptr);

protected:
    void paintEvent(QPaintEvent*) override;

private slots:
    void onChanged();

private:
    TrackModel* track_;
    class QLabel*     name_;
    class QToolButton* btnMute_;
    class QToolButton* btnHide_;
    class QToolButton* btnLock_;
};

} // namespace ve

#pragma once

#include "../../definitions.h"
#include <QWidget>
#include <QLabel>
#include <QToolButton>

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
    TrackModel*   track_;
    QLabel*       name_;
    QToolButton*  btnMute_;
    QToolButton*  btnHide_;
    QToolButton*  btnLock_;
};

} // namespace ve

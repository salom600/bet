#pragma once

#include "../../definitions.h"
#include <QWidget>
#include <QLabel>
#include <QPushButton>

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
    QPushButton*  btnMute_;
    QPushButton*  btnHide_;
    QPushButton*  btnLock_;
};

} // namespace ve

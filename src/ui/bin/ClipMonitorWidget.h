#pragma once

#include <QWidget>
#include <QImage>
#include <QString>
#include <QTimer>
#include <memory>

namespace ve {

class BinModel;
class BinClip;

/// Clip Monitor: shows the source clip from the bin (scrub the original file).
/// One of the two Kdenlive-style monitors.
class ClipMonitorWidget : public QWidget {
    Q_OBJECT
public:
    explicit ClipMonitorWidget(std::shared_ptr<BinModel> bin, QWidget* parent = nullptr);

public slots:
    void loadBinClip(const QString& binClipId);
    void setPlayhead(double t);
    void togglePlay();
    void stop();

signals:
    void playheadChanged(double t);

protected:
    void paintEvent(QPaintEvent*) override;

private:
    std::shared_ptr<BinModel> bin_;
    QString currentBinClipId_;
    QImage  currentFrame_;
    double  playhead_ = 0.0;
    bool    playing_  = false;
};

} // namespace ve

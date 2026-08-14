#pragma once

#include <QWidget>

class QPushButton;
class QSlider;

namespace ve {

class Toolbar : public QWidget {
    Q_OBJECT
public:
    explicit Toolbar(QWidget* parent = nullptr);

signals:
    void importClicked();
    void exportClicked();
    void playClicked();
    void stopClicked();
    void skipStartClicked();
    void skipEndClicked();
    void undoClicked();
    void redoClicked();
    void addVideoTrack();
    void addAudioTrack();
    void zoomIn();
    void zoomOut();

private:
    QPushButton* makeBtn(const QString& text, const QString& tooltip, const char* signal);
};

} // namespace ve

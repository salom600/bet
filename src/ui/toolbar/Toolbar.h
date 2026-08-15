#pragma once

#include <QWidget>

class QPushButton;
class QToolButton;
class QLabel;
class QFrame;

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
    void zoomIn();
    void zoomOut();
    void addVideoTrack();
    void addAudioTrack();
    void deleteClicked();

private:
    QToolButton* makeIconButton(const QString& iconPath, const QString& tooltip,
                                 const QString& text = QString());
    QFrame* makeDivider();
};

} // namespace ve

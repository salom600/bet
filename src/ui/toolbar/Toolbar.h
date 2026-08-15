#pragma once

#include <QWidget>

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
};

} // namespace ve

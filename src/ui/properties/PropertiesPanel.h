#pragma once

#include <QWidget>

namespace ve {

class ClipModel;

class PropertiesPanel : public QWidget {
    Q_OBJECT
public:
    explicit PropertiesPanel(QWidget* parent = nullptr);

    void toggleVisible() { setVisible(!isVisible()); }

public slots:
    void setClip(ClipModel* clip);

private:
    ClipModel* clip_ = nullptr;
    class QLabel* header_;
    class QFormLayout* form_;
    class QWidget* content_;
    class QScrollArea* scroll_;
    class QVBoxLayout* layout_;
    class QDoubleSpinBox* start_;
    class QDoubleSpinBox* duration_;
};

} // namespace ve

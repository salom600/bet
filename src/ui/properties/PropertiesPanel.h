#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QScrollArea>
#include <QDoubleSpinBox>

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
    QLabel*           header_;
    QFormLayout*      form_;
    QWidget*          content_;
    QScrollArea*      scroll_;
    QVBoxLayout*      layout_;
    QDoubleSpinBox*   start_;
    QDoubleSpinBox*   duration_;
};

} // namespace ve

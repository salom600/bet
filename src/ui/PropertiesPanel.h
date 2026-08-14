#pragma once

#include <QWidget>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QLabel>
#include <QScrollArea>

class QUndoStack;

namespace ve {

class Clip;

/// Right-side properties inspector. Shows editable fields for the currently
/// selected clip. Changes are pushed onto the undo stack.
class PropertiesPanel : public QWidget {
    Q_OBJECT
public:
    explicit PropertiesPanel(QUndoStack* undoStack, QWidget* parent = nullptr);

    void toggleVisible() { setVisible(!isVisible()); }

public slots:
    void setClip(Clip* clip);

private:
    void rebuildForClip(Clip* clip);
    void clearLayout(QLayout* l);

    QUndoStack* undoStack_;
    Clip* clip_ = nullptr;

    QVBoxLayout* layout_ = nullptr;
    QScrollArea* scroll_ = nullptr;
    QWidget*     content_ = nullptr;

    // Cached editors
    QDoubleSpinBox* posX_   = nullptr;
    QDoubleSpinBox* posY_   = nullptr;
    QDoubleSpinBox* scale_  = nullptr;
    QDoubleSpinBox* opacity_= nullptr;
    QDoubleSpinBox* volume_ = nullptr;
    QDoubleSpinBox* pan_    = nullptr;
    QDoubleSpinBox* start_  = nullptr;
    QDoubleSpinBox* duration_ = nullptr;
};

} // namespace ve

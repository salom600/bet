#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QScrollArea>
#include <QSlider>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QPushButton>
#include <QHBoxLayout>
#include "model/ColorGrade.h"

namespace ve {

class ClipModel;

/// Right-side panel shown when the "Adjust" tab is active.
/// Contains REAL color grading sliders (temperature, tint, exposure,
/// contrast, brightness, highlights, shadows, saturation, vibrance, hue,
/// sharpness, vignette) + a histogram + vectorscope + waveform rendered
/// from the current preview frame.
class AdjustPanel : public QWidget {
    Q_OBJECT
public:
    explicit AdjustPanel(QWidget* parent = nullptr);

    /// Set the clip whose color grade we're editing.
    void setClip(ClipModel* clip);

    /// Update the scopes from the given frame.
    void updateScopes(const QImage& frame);

    /// Get the current color grade (applied to preview frames).
    const ColorGrade& grade() const { return grade_; }

signals:
    /// Emitted when any color parameter changes (preview should re-render).
    void gradeChanged();

private slots:
    void onReset();
    void onSliderChanged(int paramId, int value);

private:
    struct ParamRow {
        int          id;
        QString      name;
        QSlider*     slider   = nullptr;
        QDoubleSpinBox* spin  = nullptr;
        QLabel*      valueLbl = nullptr;
        double       minVal;
        double       maxVal;
        double       defaultVal;
    };

    void buildParamRows();
    ParamRow* makeRow(const QString& name, double minV, double maxV, double defV, int id);
    QHBoxLayout* makeRowLayout(ParamRow* r);
    void updateGradeFromRow(ParamRow& row);

    QVBoxLayout*  layout_       = nullptr;
    QScrollArea*  scroll_       = nullptr;
    QWidget*      content_      = nullptr;
    QVBoxLayout*  contentLayout_ = nullptr;

    // Scope widgets
    QLabel*       histogramLbl_ = nullptr;
    QLabel*       vectorscopeLbl_ = nullptr;
    QLabel*       waveformLbl_  = nullptr;

    // Buttons
    QPushButton*  btnReset_     = nullptr;
    QPushButton*  btnApply_     = nullptr;

    QList<ParamRow> rows_;
    ColorGrade    grade_;
    ClipModel*    clip_ = nullptr;
};

} // namespace ve

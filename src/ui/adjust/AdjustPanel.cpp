#include "ui/adjust/AdjustPanel.h"
#include "model/ClipModel.h"
#include "media/ColorGrader.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QSlider>
#include <QDoubleSpinBox>
#include <QPushButton>
#include <QScrollArea>
#include <QFrame>
#include <QPainter>
#include <QDebug>

namespace ve {

AdjustPanel::AdjustPanel(QWidget* parent)
    : QWidget(parent)
{
    setMinimumWidth(280);
    setMaximumWidth(340);

    layout_ = new QVBoxLayout(this);
    layout_->setContentsMargins(0, 0, 0, 0);
    layout_->setSpacing(0);

    // Header
    auto* headerLayout = new QHBoxLayout();
    headerLayout->setContentsMargins(8, 6, 8, 6);
    auto* titleLabel = new QLabel("Adjust", this);
    titleLabel->setStyleSheet("font-weight: bold; color: #5ac8fa; font-size: 11pt;");
    headerLayout->addWidget(titleLabel);
    headerLayout->addStretch(1);

    btnReset_ = new QPushButton("Reset", this);
    btnReset_->setStyleSheet(
        "QPushButton { background: #262931; color: #b0b3bb; border: 1px solid #2e323b; border-radius: 3px; padding: 4px 12px; }"
        "QPushButton:hover { background: #2e323d; }");
    btnReset_->setFixedHeight(24);
    headerLayout->addWidget(btnReset_);

    btnApply_ = new QPushButton("Apply", this);
    btnApply_->setStyleSheet(
        "QPushButton { background: #5ac8fa; color: #000; border: 1px solid #5ac8fa; border-radius: 3px; padding: 4px 12px; font-weight: bold; }"
        "QPushButton:hover { background: #7ad8fa; }");
    btnApply_->setFixedHeight(24);
    headerLayout->addWidget(btnApply_);

    auto* headerWidget = new QWidget(this);
    headerWidget->setStyleSheet("background-color: #1c1f25; border-bottom: 1px solid #2a2d33;");
    headerWidget->setLayout(headerLayout);
    layout_->addWidget(headerWidget);

    // Scrollable content
    scroll_ = new QScrollArea(this);
    scroll_->setWidgetResizable(true);
    scroll_->setStyleSheet("QScrollArea { border: none; }");
    content_ = new QWidget;
    contentLayout_ = new QVBoxLayout(content_);
    contentLayout_->setContentsMargins(8, 8, 8, 8);
    contentLayout_->setSpacing(8);

    // --- Scopes section ---
    auto* scopesTitle = new QLabel("Scopes", content_);
    scopesTitle->setStyleSheet("font-weight: bold; color: #b0b3bb; font-size: 10pt; padding: 4px 0;");
    contentLayout_->addWidget(scopesTitle);

    // Histogram
    histogramLbl_ = new QLabel(content_);
    histogramLbl_->setMinimumHeight(80);
    histogramLbl_->setStyleSheet("background-color: #0d0e12; border: 1px solid #2a2d33; border-radius: 2px;");
    histogramLbl_->setAlignment(Qt::AlignCenter);
    histogramLbl_->setText("Histogram");
    histogramLbl_->setStyleSheet("background-color: #0d0e12; border: 1px solid #2a2d33; border-radius: 2px; color: #5a5d65;");
    contentLayout_->addWidget(histogramLbl_);

    // Vectorscope + Waveform side-by-side
    auto* scopesRow = new QHBoxLayout();
    scopesRow->setSpacing(4);
    vectorscopeLbl_ = new QLabel(content_);
    vectorscopeLbl_->setFixedSize(120, 120);
    vectorscopeLbl_->setStyleSheet("background-color: #0d0e12; border: 1px solid #2a2d33; border-radius: 2px; color: #5a5d65;");
    vectorscopeLbl_->setAlignment(Qt::AlignCenter);
    vectorscopeLbl_->setText("Vector");
    scopesRow->addWidget(vectorscopeLbl_);

    waveformLbl_ = new QLabel(content_);
    waveformLbl_->setFixedSize(120, 120);
    waveformLbl_->setStyleSheet("background-color: #0d0e12; border: 1px solid #2a2d33; border-radius: 2px; color: #5a5d65;");
    waveformLbl_->setAlignment(Qt::AlignCenter);
    waveformLbl_->setText("Waveform");
    scopesRow->addWidget(waveformLbl_);
    scopesRow->addStretch(1);
    contentLayout_->addLayout(scopesRow);

    // Separator
    auto* sep1 = new QFrame(content_);
    sep1->setFrameShape(QFrame::HLine);
    sep1->setStyleSheet("color: #2a2d33;");
    contentLayout_->addWidget(sep1);

    // --- Color section ---
    auto* colorTitle = new QLabel("White Balance", content_);
    colorTitle->setStyleSheet("font-weight: bold; color: #b0b3bb; font-size: 10pt; padding: 4px 0;");
    contentLayout_->addWidget(colorTitle);

    // Param rows: id, name, min, max, default
    // 0=temperature, 1=tint
    auto* r1 = makeRow("Temperature", -100, 100, 0, 0); contentLayout_->addLayout(makeRowLayout(r1));
    auto* r2 = makeRow("Tint",        -100, 100, 0, 1); contentLayout_->addLayout(makeRowLayout(r2));

    // Tone section
    auto* sep2 = new QFrame(content_);
    sep2->setFrameShape(QFrame::HLine);
    sep2->setStyleSheet("color: #2a2d33;");
    contentLayout_->addWidget(sep2);

    auto* toneTitle = new QLabel("Tone", content_);
    toneTitle->setStyleSheet("font-weight: bold; color: #b0b3bb; font-size: 10pt; padding: 4px 0;");
    contentLayout_->addWidget(toneTitle);

    // 2=exposure, 3=contrast, 4=brightness, 5=highlights, 6=shadows
    auto* r3 = makeRow("Exposure",   -1.0, 1.0, 0.0, 2); contentLayout_->addLayout(makeRowLayout(r3));
    auto* r4 = makeRow("Contrast",   -1.0, 1.0, 0.0, 3); contentLayout_->addLayout(makeRowLayout(r4));
    auto* r5 = makeRow("Brightness", -1.0, 1.0, 0.0, 4); contentLayout_->addLayout(makeRowLayout(r5));
    auto* r6 = makeRow("Highlights", -1.0, 1.0, 0.0, 5); contentLayout_->addLayout(makeRowLayout(r6));
    auto* r7 = makeRow("Shadows",    -1.0, 1.0, 0.0, 6); contentLayout_->addLayout(makeRowLayout(r7));

    // Color section
    auto* sep3 = new QFrame(content_);
    sep3->setFrameShape(QFrame::HLine);
    sep3->setStyleSheet("color: #2a2d33;");
    contentLayout_->addWidget(sep3);

    auto* satTitle = new QLabel("Color", content_);
    satTitle->setStyleSheet("font-weight: bold; color: #b0b3bb; font-size: 10pt; padding: 4px 0;");
    contentLayout_->addWidget(satTitle);

    // 7=saturation, 8=vibrance, 9=hue
    auto* r8  = makeRow("Saturation", -1.0, 1.0,  0.0, 7); contentLayout_->addLayout(makeRowLayout(r8));
    auto* r9  = makeRow("Vibrance",   -1.0, 1.0,  0.0, 8); contentLayout_->addLayout(makeRowLayout(r9));
    auto* r10 = makeRow("Hue",       -180,  180,   0,  9); contentLayout_->addLayout(makeRowLayout(r10));

    // Effects section
    auto* sep4 = new QFrame(content_);
    sep4->setFrameShape(QFrame::HLine);
    sep4->setStyleSheet("color: #2a2d33;");
    contentLayout_->addWidget(sep4);

    auto* fxTitle = new QLabel("Effects", content_);
    fxTitle->setStyleSheet("font-weight: bold; color: #b0b3bb; font-size: 10pt; padding: 4px 0;");
    contentLayout_->addWidget(fxTitle);

    // 10=sharpness, 11=blur, 12=vignette
    auto* r11 = makeRow("Sharpness", 0.0, 1.0, 0.0, 10); contentLayout_->addLayout(makeRowLayout(r11));
    auto* r12 = makeRow("Blur",      0.0, 1.0, 0.0, 11); contentLayout_->addLayout(makeRowLayout(r12));
    auto* r13 = makeRow("Vignette",  0.0, 1.0, 0.0, 12); contentLayout_->addLayout(makeRowLayout(r13));

    contentLayout_->addStretch(1);
    scroll_->setWidget(content_);
    layout_->addWidget(scroll_, 1);

    // Wire buttons
    connect(btnReset_, &QPushButton::clicked, this, &AdjustPanel::onReset);
    connect(btnApply_, &QPushButton::clicked, this, [this]() {
        // "Apply" stores the grade on the clip (already live, so just emit)
        emit gradeChanged();
    });
}

QHBoxLayout* AdjustPanel::makeRowLayout(ParamRow* r) {
    auto* row = new QHBoxLayout();
    row->setSpacing(4);
    row->setContentsMargins(0, 0, 0, 0);

    auto* name = new QLabel(r->name, content_);
    name->setMinimumWidth(80);
    name->setStyleSheet("color: #b0b3bb; font-size: 10pt;");
    row->addWidget(name);

    row->addWidget(r->slider, 1);

    r->valueLbl->setMinimumWidth(50);
    r->valueLbl->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    r->valueLbl->setStyleSheet("color: #5ac8fa; font-family: monospace; font-size: 10pt;");
    row->addWidget(r->valueLbl);

    return row;
}

AdjustPanel::ParamRow* AdjustPanel::makeRow(const QString& name, double minV, double maxV, double defV, int id) {
    ParamRow r;
    r.id = id;
    r.name = name;
    r.minVal = minV;
    r.maxVal = maxV;
    r.defaultVal = defV;

    r.slider = new QSlider(Qt::Horizontal, content_);
    // Scale to 0..1000 for fine control
    int sliderMin = static_cast<int>(minV * 1000);
    int sliderMax = static_cast<int>(maxV * 1000);
    r.slider->setRange(sliderMin, sliderMax);
    r.slider->setValue(static_cast<int>(defV * 1000));
    r.slider->setStyleSheet(
        "QSlider::groove:horizontal { border: none; height: 4px; background: #2a2d33; border-radius: 2px; }"
        "QSlider::sub-page:horizontal { background: #5ac8fa; border-radius: 2px; }"
        "QSlider::handle:horizontal { background: #e0e0e6; border: 2px solid #5ac8fa; width: 12px; height: 12px; margin: -6px 0; border-radius: 8px; }");

    r.valueLbl = new QLabel(QString::number(defV, 'f', 2), content_);

    // Wire slider
    connect(r.slider, &QSlider::valueChanged, this, [this, id](int v) {
        onSliderChanged(id, v);
    });

    rows_.append(r);
    return &rows_.last();
}

void AdjustPanel::setClip(ClipModel* clip) {
    clip_ = clip;
    // For now we use a global grade; per-clip grade storage is a future
    // improvement. The grade is reset when switching clips.
    // (Could also load from clip's effect stack if a color_grade effect exists.)
}

void AdjustPanel::updateScopes(const QImage& frame) {
    if (frame.isNull()) return;

    // Histogram
    int binsR[256], binsG[256], binsB[256];
    ColorGrader::histogram(frame, binsR, binsG, binsB);
    int maxBin = 1;
    for (int i = 0; i < 256; ++i) {
        maxBin = std::max({maxBin, binsR[i], binsG[i], binsB[i]});
    }
    QImage histImg(256, 80, QImage::Format_ARGB32);
    histImg.fill(QColor(0x0d, 0x0e, 0x12));
    QPainter p(&histImg);
    for (int i = 0; i < 256; ++i) {
        int hR = (binsR[i] * 80) / maxBin;
        int hG = (binsG[i] * 80) / maxBin;
        int hB = (binsB[i] * 80) / maxBin;
        p.setPen(QColor(255, 80, 80, 180));
        p.drawLine(i, 80 - hR, i, 80);
        p.setPen(QColor(80, 255, 80, 180));
        p.drawLine(i, 80 - hG, i, 80);
        p.setPen(QColor(80, 130, 255, 180));
        p.drawLine(i, 80 - hB, i, 80);
    }
    histogramLbl_->setPixmap(QPixmap::fromImage(histImg.scaled(histogramLbl_->size(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation)));

    // Vectorscope
    QImage vs = ColorGrader::vectorscope(frame, 120);
    vectorscopeLbl_->setPixmap(QPixmap::fromImage(vs));

    // Waveform
    QImage wf = ColorGrader::waveform(frame, 120);
    waveformLbl_->setPixmap(QPixmap::fromImage(wf));
}

void AdjustPanel::onSliderChanged(int paramId, int value) {
    double dv = value / 1000.0;
    switch (paramId) {
        case 0:  grade_.temperature = dv; break;
        case 1:  grade_.tint        = dv; break;
        case 2:  grade_.exposure    = dv; break;
        case 3:  grade_.contrast    = dv; break;
        case 4:  grade_.brightness  = dv; break;
        case 5:  grade_.highlights  = dv; break;
        case 6:  grade_.shadows     = dv; break;
        case 7:  grade_.saturation  = dv; break;
        case 8:  grade_.vibrance    = dv; break;
        case 9:  grade_.hue         = dv * 180.0; break;  // slider -1..1 -> -180..180
        case 10: grade_.sharpness   = dv; break;
        case 11: grade_.blur        = dv; break;
        case 12: grade_.vignette    = dv; break;
    }
    // Update value label
    for (auto& r : rows_) {
        if (r.id == paramId) {
            double displayVal = (paramId == 9) ? dv * 180.0 : dv;
            r.valueLbl->setText(QString::number(displayVal, 'f', 1));
            break;
        }
    }
    emit gradeChanged();
}

void AdjustPanel::onReset() {
    grade_.reset();
    for (auto& r : rows_) {
        QSignalBlocker b(r.slider);
        r.slider->setValue(static_cast<int>(r.defaultVal * 1000));
        double displayVal = (r.id == 9) ? r.defaultVal * 180.0 : r.defaultVal;
        r.valueLbl->setText(QString::number(displayVal, 'f', 1));
    }
    emit gradeChanged();
}

} // namespace ve

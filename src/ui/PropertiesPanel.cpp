#include "ui/PropertiesPanel.h"
#include "core/Clip.h"
#include "core/Command.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QGroupBox>
#include <QScrollArea>
#include <QFileInfo>
#include <QDebug>
#include <functional>

namespace ve {

PropertiesPanel::PropertiesPanel(QUndoStack* undoStack, QWidget* parent)
    : QWidget(parent)
    , undoStack_(undoStack)
{
    setMinimumWidth(280);
    setMaximumWidth(380);
    setStyleSheet("background-color: #1a1c20; color: #e0e0e6;");

    auto* outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->setSpacing(0);

    auto* title = new QLabel("Properties", this);
    QFont f = title->font();
    f.setPointSize(11);
    f.setBold(true);
    title->setFont(f);
    title->setStyleSheet("background-color: #262931; padding: 8px; color: #5ac8fa;");
    outer->addWidget(title);

    scroll_ = new QScrollArea(this);
    scroll_->setWidgetResizable(true);
    content_ = new QWidget;
    layout_ = new QVBoxLayout(content_);
    layout_->setContentsMargins(8, 8, 8, 8);
    layout_->setSpacing(8);
    layout_->addStretch(1);
    scroll_->setWidget(content_);
    outer->addWidget(scroll_, 1);

    setClip(nullptr);
}

void PropertiesPanel::setClip(Clip* clip) {
    clip_ = clip;
    rebuildForClip(clip);
}

void PropertiesPanel::clearLayout(QLayout* l) {
    QLayoutItem* it;
    while ((it = l->takeAt(0)) != nullptr) {
        if (it->widget()) { it->widget()->deleteLater(); }
        else if (it->layout()) { clearLayout(it->layout()); delete it->layout(); }
        else { delete it; }
    }
}

void PropertiesPanel::rebuildForClip(Clip* clip) {
    // Reset editors (clearLayout will delete the actual widgets)
    posX_ = posY_ = scale_ = opacity_ = nullptr;
    volume_ = pan_ = start_ = duration_ = nullptr;

    clearLayout(layout_);
    layout_->addStretch(1); // ensure stretch at end

    if (!clip) {
        auto* lbl = new QLabel("No clip selected.\n\nClick a clip on the timeline to edit its properties.", content_);
        lbl->setWordWrap(true);
        lbl->setStyleSheet("color: #8a8d96; padding: 12px;");
        layout_->insertWidget(0, lbl);
        return;
    }

    // Clip header
    auto* hdr = new QLabel(QStringLiteral("<b>%1</b>")
        .arg(QFileInfo(clip->sourcePath()).fileName()), content_);
    hdr->setWordWrap(true);
    hdr->setStyleSheet("color: #5ac8fa; padding: 4px;");
    layout_->insertWidget(0, hdr);

    auto* form = new QFormLayout;
    form->setLabelAlignment(Qt::AlignLeft);
    form->setFormAlignment(Qt::AlignLeft | Qt::AlignTop);
    form->setContentsMargins(8, 8, 8, 8);
    form->setSpacing(6);

    auto* grp = new QGroupBox("Clip", content_);
    grp->setLayout(form);
    layout_->insertWidget(1, grp);

    auto makeDouble = [this](double val, double minV, double maxV, double step, int decimals = 2) {
        auto* sb = new QDoubleSpinBox(content_);
        sb->setRange(minV, maxV);
        sb->setSingleStep(step);
        sb->setDecimals(decimals);
        sb->setValue(val);
        sb->setStyleSheet(
            "QDoubleSpinBox { background: #262931; color: #e0e0e6; border: 1px solid #2e323b; padding: 2px; }"
            "QDoubleSpinBox::up-button, QDoubleSpinBox::down-button { background: #2e323d; }");
        return sb;
    };

    // Common fields
    start_ = makeDouble(clip->timelineStart(), 0.0, 86400.0, 0.1);
    duration_ = makeDouble(clip->duration(), 0.05, 86400.0, 0.1);
    form->addRow("Start (s):", start_);
    form->addRow("Duration (s):", duration_);

    if (clip->type() == MediaType::Video || clip->type() == MediaType::Image) {
        posX_     = makeDouble(clip->posX(),    -10000, 10000, 1.0);
        posY_     = makeDouble(clip->posY(),    -10000, 10000, 1.0);
        scale_    = makeDouble(clip->scale(),    0.01,  10.0,  0.05);
        opacity_  = makeDouble(clip->opacity(),  0.0,   1.0,   0.05);
        form->addRow("Position X:", posX_);
        form->addRow("Position Y:", posY_);
        form->addRow("Scale:",       scale_);
        form->addRow("Opacity:",     opacity_);
    }

    if (clip->type() == MediaType::Audio) {
        volume_ = makeDouble(clip->volume(), 0.0,  4.0, 0.05);
        pan_    = makeDouble(clip->pan(),   -1.0,  1.0, 0.05);
        form->addRow("Volume:", volume_);
        form->addRow("Pan:",     pan_);
    }

    // Wire change handlers that push undo commands
    auto wireDouble = [this](QDoubleSpinBox* sb, double oldVal, std::function<void(double)> setter) {
        connect(sb, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
            [this, sb, setter](double newVal) {
                if (!clip_) return;
                double prev = sb->property("prev").toDouble();
                undoStack_->push(new LambdaCommand(
                    "Edit property",
                    [setter, newVal]() { setter(newVal); },
                    [setter, prev]()   { setter(prev);   }
                ));
                sb->setProperty("prev", newVal);
            });
        sb->setProperty("prev", oldVal);
    };

    wireDouble(start_,     clip->timelineStart(), [c = clip](double v) { c->setTimelineStart(v); });
    wireDouble(duration_,  clip->duration(),      [c = clip](double v) { c->setSourceOut(c->sourceIn() + v); });

    if (posX_)     wireDouble(posX_,     clip->posX(),     [c = clip](double v) { c->setPosX(v); });
    if (posY_)     wireDouble(posY_,     clip->posY(),     [c = clip](double v) { c->setPosY(v); });
    if (scale_)    wireDouble(scale_,    clip->scale(),    [c = clip](double v) { c->setScale(v); });
    if (opacity_)  wireDouble(opacity_,  clip->opacity(),  [c = clip](double v) { c->setOpacity(v); });
    if (volume_)   wireDouble(volume_,   clip->volume(),   [c = clip](double v) { c->setVolume(v); });
    if (pan_)      wireDouble(pan_,      clip->pan(),      [c = clip](double v) { c->setPan(v); });

    // Listen for external changes (e.g. undo) to update the editors
    connect(clip, &Clip::changed, this, [this, clip]() {
        if (start_     && !start_->hasFocus())     start_->setValue(clip->timelineStart());
        if (duration_  && !duration_->hasFocus())  duration_->setValue(clip->duration());
        if (posX_      && !posX_->hasFocus())      posX_->setValue(clip->posX());
        if (posY_      && !posY_->hasFocus())      posY_->setValue(clip->posY());
        if (scale_     && !scale_->hasFocus())     scale_->setValue(clip->scale());
        if (opacity_   && !opacity_->hasFocus())   opacity_->setValue(clip->opacity());
        if (volume_    && !volume_->hasFocus())    volume_->setValue(clip->volume());
        if (pan_       && !pan_->hasFocus())       pan_->setValue(clip->pan());
    });
}

} // namespace ve

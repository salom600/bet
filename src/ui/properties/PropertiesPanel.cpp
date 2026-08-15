#include "ui/properties/PropertiesPanel.h"
#include "model/ClipModel.h"
#include "model/BinClip.h"
#include "model/TimelineModel.h"
#include "project/Project.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QGroupBox>
#include <QScrollArea>
#include <QFileInfo>
#include <QDoubleSpinBox>

namespace ve {

PropertiesPanel::PropertiesPanel(QWidget* parent)
    : QWidget(parent)
{
    setMinimumWidth(280);
    setMaximumWidth(380);
    setStyleSheet("background-color: #1a1c20; color: #e0e0e6;");

    layout_ = new QVBoxLayout(this);
    layout_->setContentsMargins(0, 0, 0, 0);
    layout_->setSpacing(0);

    auto* title = new QLabel("Properties", this);
    QFont f = title->font();
    f.setPointSize(11);
    f.setBold(true);
    title->setFont(f);
    title->setStyleSheet("background-color: #262931; padding: 8px; color: #5ac8fa;");
    layout_->addWidget(title);

    scroll_ = new QScrollArea(this);
    scroll_->setWidgetResizable(true);
    content_ = new QWidget;
    auto* outer = new QVBoxLayout(content_);
    outer->setContentsMargins(8, 8, 8, 8);
    outer->setSpacing(8);

    header_ = new QLabel("No clip selected.", content_);
    header_->setWordWrap(true);
    header_->setStyleSheet("color: #8a8d96; padding: 4px;");
    outer->addWidget(header_);

    auto* form = new QFormLayout;
    form->setContentsMargins(8, 8, 8, 8);
    form->setSpacing(6);
    auto* grp = new QGroupBox("Clip", content_);
    grp->setLayout(form);

    auto makeDouble = [this](double val, double minV, double maxV, double step) {
        auto* sb = new QDoubleSpinBox(content_);
        sb->setRange(minV, maxV);
        sb->setSingleStep(step);
        sb->setDecimals(2);
        sb->setValue(val);
        sb->setStyleSheet(
            "QDoubleSpinBox { background: #262931; color: #e0e0e6; border: 1px solid #2e323b; padding: 2px; }"
            "QDoubleSpinBox::up-button, QDoubleSpinBox::down-button { background: #2e323d; }");
        return sb;
    };

    start_    = makeDouble(0, 0, 86400, 0.1);
    duration_ = makeDouble(0, 0.05, 86400, 0.1);
    form->addRow("Start (s):",    start_);
    form->addRow("Duration (s):", duration_);

    outer->addWidget(grp);
    outer->addStretch(1);
    scroll_->setWidget(content_);
    layout_->addWidget(scroll_, 1);
}

void PropertiesPanel::setClip(ClipModel* clip) {
    clip_ = clip;
    if (!clip_) {
        header_->setText("No clip selected.");
        header_->setStyleSheet("color: #8a8d96; padding: 4px;");
        start_->setValue(0);
        duration_->setValue(0);
        return;
    }
    auto bc = clip_->binClip();
    QString name = bc ? bc->name() : "clip";
    header_->setText(QStringLiteral("<b>%1</b>").arg(name));
    header_->setStyleSheet("color: #5ac8fa; padding: 4px;");

    // Display values (read-only-ish; we don't wire undo here for brevity)
    start_->blockSignals(true);
    duration_->blockSignals(true);
    if (bc) {
        // We need timeline's fps for frame->second conversion; we'll approximate
        // using GenTime's static fps.
        start_->setValue(clip_->getPosition() / GenTime::fps());
        duration_->setValue(clip_->getPlaytime() / GenTime::fps());
    }
    start_->blockSignals(false);
    duration_->blockSignals(false);
}

} // namespace ve

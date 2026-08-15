#include "ui/effects/EffectStackView.h"
#include "assets/EffectStackModel.h"
#include "assets/EffectItemModel.h"
#include "assets/EffectsRepository.h"
#include "assets/EffectDescription.h"
#include "model/ClipModel.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QToolButton>
#include <QMenu>
#include <QAction>
#include <QInputDialog>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QSlider>
#include <QFormLayout>
#include <QScrollArea>
#include <QFrame>
#include <QDebug>

namespace ve {

EffectStackView::EffectStackView(QUndoStack* undoStack, QWidget* parent)
    : QWidget(parent)
    , undoStack_(undoStack)
{
    setMinimumWidth(280);
    setMaximumWidth(380);

    auto* outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->setSpacing(0);

    // Header
    auto* headerLayout = new QHBoxLayout();
    headerLayout->setContentsMargins(8, 6, 8, 6);
    headerLayout->setSpacing(4);
    titleLabel_ = new QLabel("Effect Stack", this);
    titleLabel_->setStyleSheet("font-weight: bold; color: #5ac8fa; font-size: 11pt;");
    headerLayout->addWidget(titleLabel_);
    headerLayout->addStretch(1);

    btnAdd_ = new QToolButton(this);
    btnAdd_->setIcon(QIcon(":/icons/import.svg"));
    btnAdd_->setIconSize(QSize(16, 16));
    btnAdd_->setToolTip("Add effect");
    btnAdd_->setAutoRaise(true);
    btnAdd_->setFixedSize(28, 28);

    btnRemove_ = new QToolButton(this);
    btnRemove_->setIcon(QIcon(":/icons/delete.svg"));
    btnRemove_->setIconSize(QSize(16, 16));
    btnRemove_->setToolTip("Remove selected effect");
    btnRemove_->setAutoRaise(true);
    btnRemove_->setFixedSize(28, 28);

    btnUp_ = new QToolButton(this);
    btnUp_->setText("\u2191");
    btnUp_->setToolTip("Move up");
    btnUp_->setAutoRaise(true);
    btnUp_->setFixedSize(28, 28);

    btnDown_ = new QToolButton(this);
    btnDown_->setText("\u2193");
    btnDown_->setToolTip("Move down");
    btnDown_->setAutoRaise(true);
    btnDown_->setFixedSize(28, 28);

    headerLayout->addWidget(btnAdd_);
    headerLayout->addWidget(btnRemove_);
    headerLayout->addWidget(btnUp_);
    headerLayout->addWidget(btnDown_);

    auto* headerWidget = new QWidget(this);
    headerWidget->setStyleSheet("background-color: #1c1f25; border-bottom: 1px solid #2a2d33;");
    headerWidget->setLayout(headerLayout);
    outer->addWidget(headerWidget);

    // Effect list
    list_ = new QListWidget(this);
    list_->setMaximumHeight(140);
    list_->setIconSize(QSize(16, 16));
    outer->addWidget(list_);

    // Separator
    auto* sep = new QFrame(this);
    sep->setFrameShape(QFrame::HLine);
    sep->setStyleSheet("color: #2a2d33;");
    outer->addWidget(sep);

    // Parameter editor area (scrollable)
    scroll_ = new QScrollArea(this);
    scroll_->setWidgetResizable(true);
    scroll_->setStyleSheet("QScrollArea { border: none; }");
    paramsContent_ = new QWidget;
    paramsLayout_ = new QVBoxLayout(paramsContent_);
    paramsLayout_->setContentsMargins(8, 8, 8, 8);
    paramsLayout_->setSpacing(6);
    paramsLayout_->addStretch(1);
    scroll_->setWidget(paramsContent_);
    outer->addWidget(scroll_, 1);

    // Empty state label
    emptyLabel_ = new QLabel("Select a clip to edit its effects.", this);
    emptyLabel_->setAlignment(Qt::AlignCenter);
    emptyLabel_->setStyleSheet("color: #8a8d96; padding: 20px; font-size: 10pt;");
    paramsLayout_->insertWidget(0, emptyLabel_);

    connect(btnAdd_,    &QToolButton::clicked, this, &EffectStackView::onAddEffect);
    connect(btnRemove_, &QToolButton::clicked, this, &EffectStackView::onRemoveEffect);
    connect(btnUp_,     &QToolButton::clicked, this, &EffectStackView::onMoveUp);
    connect(btnDown_,   &QToolButton::clicked, this, &EffectStackView::onMoveDown);
    connect(list_, &QListWidget::currentRowChanged, this, [this](int) { rebuildParams(); });
}

void EffectStackView::setClip(ClipModel* clip) {
    clip_ = clip;
    stack_ = clip ? clip->effectStack() : nullptr;
    rebuildList();
    rebuildParams();
}

void EffectStackView::rebuildList() {
    list_->clear();
    if (!stack_) {
        emptyLabel_->setText("Select a clip to edit its effects.");
        emptyLabel_->show();
        return;
    }
    emptyLabel_->hide();
    for (int i = 0; i < stack_->rowCount(); ++i) {
        auto item = stack_->at(i);
        if (!item) continue;
        QListWidgetItem* li = new QListWidgetItem(item->name(), list_);
        li->setData(Qt::UserRole, i);
        if (!item->isEnabled()) {
            li->setForeground(QColor(120, 120, 120));
        }
    }
    if (list_->count() > 0 && list_->currentRow() < 0) {
        list_->setCurrentRow(0);
    }
}

void EffectStackView::rebuildParams() {
    // Clear existing param widgets
    QLayoutItem* it;
    while ((it = paramsLayout_->takeAt(0)) != nullptr) {
        if (it->widget()) { it->widget()->deleteLater(); }
        delete it;
    }
    paramsLayout_->addStretch(1);

    if (!stack_ || list_->currentRow() < 0) {
        emptyLabel_ = new QLabel("Select an effect to edit its parameters.", paramsContent_);
        emptyLabel_->setAlignment(Qt::AlignCenter);
        emptyLabel_->setStyleSheet("color: #8a8d96; padding: 20px; font-size: 10pt;");
        paramsLayout_->insertWidget(0, emptyLabel_);
        return;
    }

    auto item = stack_->at(list_->currentRow());
    if (!item) return;

    // Effect name header
    auto* nameLbl = new QLabel(item->name(), paramsContent_);
    nameLbl->setStyleSheet("font-weight: bold; color: #5ac8fa; font-size: 11pt; padding: 4px 0;");
    paramsLayout_->insertWidget(paramsLayout_->count() - 1, nameLbl);

    // Enabled checkbox
    auto* enabledCb = new QCheckBox("Enabled", paramsContent_);
    enabledCb->setChecked(item->isEnabled());
    connect(enabledCb, &QCheckBox::toggled, this, [item](bool on) { item->setEnabled(on); });
    paramsLayout_->insertWidget(paramsLayout_->count() - 1, enabledCb);

    // Separator
    auto* sep = new QFrame(paramsContent_);
    sep->setFrameShape(QFrame::HLine);
    sep->setStyleSheet("color: #2a2d33;");
    paramsLayout_->insertWidget(paramsLayout_->count() - 1, sep);

    // Parameter editors
    for (const auto& param : item->description().parameters) {
        auto* paramRow = new QWidget(paramsContent_);
        auto* paramLayout = new QVBoxLayout(paramRow);
        paramLayout->setContentsMargins(0, 0, 0, 0);
        paramLayout->setSpacing(2);

        auto* labelRow = new QHBoxLayout();
        labelRow->setContentsMargins(0, 0, 0, 0);
        auto* paramName = new QLabel(param.displayName, paramRow);
        paramName->setStyleSheet("color: #b0b3bb; font-size: 10pt;");
        labelRow->addWidget(paramName);
        labelRow->addStretch(1);

        // Current value display
        auto* valueLbl = new QLabel(paramRow);
        valueLbl->setStyleSheet("color: #5ac8fa; font-size: 10pt; font-family: monospace;");
        QVariant currentVal = item->parameter(param.name);
        if (currentVal.isValid()) {
            valueLbl->setText(QString::number(currentVal.toDouble(), 'f', 2));
        }
        labelRow->addWidget(valueLbl);
        paramLayout->addLayout(labelRow);

        // Slider + spinbox
        auto* sliderRow = new QHBoxLayout();
        sliderRow->setContentsMargins(0, 0, 0, 0);
        sliderRow->setSpacing(4);

        auto* slider = new QSlider(Qt::Horizontal, paramRow);
        slider->setRange(static_cast<int>(param.minVal * 100),
                         static_cast<int>(param.maxVal * 100));
        double initVal = currentVal.isValid() ? currentVal.toDouble() : param.defaultVal;
        slider->setValue(static_cast<int>(initVal * 100));
        sliderRow->addWidget(slider, 1);

        auto* spin = new QDoubleSpinBox(paramRow);
        spin->setRange(param.minVal, param.maxVal);
        spin->setDecimals(2);
        spin->setSingleStep(0.05);
        spin->setValue(initVal);
        spin->setFixedWidth(80);
        sliderRow->addWidget(spin);

        paramLayout->addLayout(sliderRow);
        paramsLayout_->insertWidget(paramsLayout_->count() - 1, paramRow);

        // Wire slider <-> spinbox <-> model
        connect(slider, &QSlider::valueChanged, this, [spin, valueLbl, item, param](int v) {
            double dv = v / 100.0;
            QSignalBlocker b(spin);
            spin->setValue(dv);
            valueLbl->setText(QString::number(dv, 'f', 2));
            item->setParameter(param.name, dv);
        });
        connect(spin, qOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
                [slider, valueLbl, item, param](double v) {
            QSignalBlocker b(slider);
            slider->setValue(static_cast<int>(v * 100));
            valueLbl->setText(QString::number(v, 'f', 2));
            item->setParameter(param.name, v);
        });
    }
}

void EffectStackView::onAddEffect() {
    if (!stack_ || !clip_) return;
    QMenu menu(this);
    for (const auto& e : EffectsRepository::self().all()) {
        auto* a = menu.addAction(e.name);
        a->setData(e.id);
    }
    QAction* sel = menu.exec(QCursor::pos());
    if (!sel) return;
    QString id = sel->data().toString();
    stack_->appendEffect(id);
    rebuildList();
    rebuildParams();
}

void EffectStackView::onRemoveEffect() {
    if (!stack_) return;
    int row = list_->currentRow();
    if (row < 0) return;
    auto item = stack_->at(row);
    if (!item) return;
    stack_->removeEffect(item);
    rebuildList();
    rebuildParams();
}

void EffectStackView::onMoveUp() {
    if (!stack_) return;
    int row = list_->currentRow();
    if (row <= 0) return;
    auto item = stack_->at(row);
    stack_->moveEffect(item, row - 1);
    rebuildList();
    rebuildParams();
}

void EffectStackView::onMoveDown() {
    if (!stack_) return;
    int row = list_->currentRow();
    if (row < 0 || row >= stack_->rowCount() - 1) return;
    auto item = stack_->at(row);
    stack_->moveEffect(item, row + 1);
    rebuildList();
    rebuildParams();
}

} // namespace ve

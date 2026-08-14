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
#include <QPushButton>
#include <QMenu>
#include <QAction>
#include <QInputDialog>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QFormLayout>
#include <QScrollArea>
#include <QDebug>

namespace ve {

EffectStackView::EffectStackView(QUndoStack* undoStack, QWidget* parent)
    : QWidget(parent)
    , undoStack_(undoStack)
{
    setMinimumWidth(280);
    setMaximumWidth(380);
    setStyleSheet("background-color: #1a1c20; color: #e0e0e6;");

    auto* outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->setSpacing(0);

    auto* title = new QLabel("Effect Stack", this);
    QFont f = title->font();
    f.setPointSize(11);
    f.setBold(true);
    title->setFont(f);
    title->setStyleSheet("background-color: #262931; padding: 8px; color: #5ac8fa;");
    outer->addWidget(title);

    auto* controls = new QHBoxLayout;
    controls->setContentsMargins(6, 6, 6, 6);
    btnAdd_    = new QPushButton("+ Add", this);
    btnRemove_ = new QPushButton("- Remove", this);
    btnUp_     = new QPushButton("\u2191", this);
    btnDown_   = new QPushButton("\u2193", this);
    btnAdd_->setToolTip("Add effect");
    btnRemove_->setToolTip("Remove selected effect");
    btnUp_->setToolTip("Move up");
    btnDown_->setToolTip("Move down");
    controls->addWidget(btnAdd_);
    controls->addWidget(btnRemove_);
    controls->addStretch(1);
    controls->addWidget(btnUp_);
    controls->addWidget(btnDown_);
    outer->addLayout(controls);

    list_ = new QListWidget(this);
    list_->setMaximumHeight(120);
    outer->addWidget(list_);

    auto* scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    auto* content = new QWidget;
    paramsLayout_ = new QVBoxLayout(content);
    paramsLayout_->setContentsMargins(8, 8, 8, 8);
    paramsLayout_->setSpacing(4);
    paramsLayout_->addStretch(1);
    scroll->setWidget(content);
    outer->addWidget(scroll, 1);

    connect(btnAdd_,    &QPushButton::clicked, this, &EffectStackView::onAddEffect);
    connect(btnRemove_, &QPushButton::clicked, this, &EffectStackView::onRemoveEffect);
    connect(btnUp_,     &QPushButton::clicked, this, &EffectStackView::onMoveUp);
    connect(btnDown_,   &QPushButton::clicked, this, &EffectStackView::onMoveDown);
    connect(list_, &QListWidget::currentRowChanged, this, [this](int) { rebuildList(); });
}

void EffectStackView::setClip(ClipModel* clip) {
    clip_ = clip;
    stack_ = clip ? clip->effectStack() : nullptr;
    rebuildList();
}

void EffectStackView::rebuildList() {
    list_->clear();
    if (!stack_) return;
    for (int i = 0; i < stack_->rowCount(); ++i) {
        auto item = stack_->at(i);
        if (!item) continue;
        QListWidgetItem* li = new QListWidgetItem(item->name(), list_);
        li->setData(Qt::UserRole, i);
        if (!item->isEnabled()) {
            li->setForeground(QColor(120, 120, 120));
        }
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
}

void EffectStackView::onRemoveEffect() {
    if (!stack_) return;
    int row = list_->currentRow();
    if (row < 0) return;
    auto item = stack_->at(row);
    if (!item) return;
    stack_->removeEffect(item);
    rebuildList();
}

void EffectStackView::onMoveUp() {
    if (!stack_) return;
    int row = list_->currentRow();
    if (row <= 0) return;
    auto item = stack_->at(row);
    stack_->moveEffect(item, row - 1);
    rebuildList();
}

void EffectStackView::onMoveDown() {
    if (!stack_) return;
    int row = list_->currentRow();
    if (row < 0 || row >= stack_->rowCount() - 1) return;
    auto item = stack_->at(row);
    stack_->moveEffect(item, row + 1);
    rebuildList();
}

} // namespace ve

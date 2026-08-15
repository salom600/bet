#pragma once

#include <QWidget>
#include <QList>
#include <memory>

class QUndoStack;
class QVBoxLayout;
class QToolButton;
class QListWidget;
class QLabel;
class QScrollArea;

namespace ve {

class ClipModel;
class EffectStackModel;
class EffectItemModel;

class EffectStackView : public QWidget {
    Q_OBJECT
public:
    explicit EffectStackView(QUndoStack* undoStack, QWidget* parent = nullptr);

public slots:
    void setClip(ClipModel* clip);

private slots:
    void onAddEffect();
    void onRemoveEffect();
    void onMoveUp();
    void onMoveDown();

private:
    void rebuildList();
    void rebuildParams();

    QUndoStack* undoStack_;
    ClipModel* clip_ = nullptr;
    std::shared_ptr<EffectStackModel> stack_;

    QLabel*       titleLabel_  = nullptr;
    QListWidget*  list_        = nullptr;
    QToolButton*  btnAdd_      = nullptr;
    QToolButton*  btnRemove_   = nullptr;
    QToolButton*  btnUp_       = nullptr;
    QToolButton*  btnDown_     = nullptr;
    QScrollArea*  scroll_      = nullptr;
    QWidget*      paramsContent_ = nullptr;
    QVBoxLayout*  paramsLayout_ = nullptr;
    QLabel*       emptyLabel_  = nullptr;
};

} // namespace ve

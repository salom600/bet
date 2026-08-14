#pragma once

#include <QWidget>
#include <QList>
#include <memory>

class QUndoStack;
class QVBoxLayout;
class QPushButton;
class QListWidget;

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

    QUndoStack* undoStack_;
    ClipModel* clip_ = nullptr;
    std::shared_ptr<EffectStackModel> stack_;

    QListWidget* list_ = nullptr;
    QPushButton* btnAdd_ = nullptr;
    QPushButton* btnRemove_ = nullptr;
    QPushButton* btnUp_ = nullptr;
    QPushButton* btnDown_ = nullptr;
    QVBoxLayout* paramsLayout_ = nullptr;
};

} // namespace ve

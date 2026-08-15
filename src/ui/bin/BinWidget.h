#pragma once

#include <QWidget>
#include <QListView>
#include <QTreeView>
#include <QStackedWidget>
#include <QToolButton>
#include <memory>

namespace ve {

class BinModel;
class BinClip;
class BinItemDelegate;

class BinWidget : public QWidget {
    Q_OBJECT
public:
    enum class ViewMode { IconView, ListView };

    explicit BinWidget(std::shared_ptr<BinModel> bin, QWidget* parent = nullptr);
    void setBin(std::shared_ptr<BinModel> bin);

    void setViewMode(ViewMode mode);

signals:
    void clipActivated(const QString& binClipId);
    void clipDroppedOnTimeline(const QString& binClipId, int positionFrames);

private slots:
    void onAddFolder();
    void onRemoveItem();
    void onToggleViewMode();

private:
    std::shared_ptr<BinModel> bin_;
    QStackedWidget* stack_       = nullptr;
    QListView*      iconView_    = nullptr;
    QTreeView*      treeView_    = nullptr;
    BinItemDelegate* iconDelegate_ = nullptr;
    QToolButton* btnIconView_ = nullptr;
    QToolButton* btnListView_ = nullptr;
    QToolButton* btnAddFolder_ = nullptr;
    QToolButton* btnRemove_ = nullptr;
};

} // namespace ve

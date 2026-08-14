#pragma once

#include <QTreeView>
#include <memory>

namespace ve {

class BinModel;

/// Project Bin widget: QTreeView of the BinModel with drag-to-timeline support.
class BinWidget : public QWidget {
    Q_OBJECT
public:
    explicit BinWidget(std::shared_ptr<BinModel> bin, QWidget* parent = nullptr);

    void setBin(std::shared_ptr<BinModel> bin);

signals:
    void clipActivated(const QString& binClipId);
    void clipDroppedOnTimeline(const QString& binClipId, int positionFrames);

private:
    QTreeView* tree_ = nullptr;
    std::shared_ptr<BinModel> bin_;
};

} // namespace ve

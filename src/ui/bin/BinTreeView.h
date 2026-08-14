#pragma once

#include <QTreeView>

namespace ve {

/// QTreeView subclass that emits custom signals on double-click and
/// supports dragging bin clips onto the timeline.
class BinTreeView : public QTreeView {
    Q_OBJECT
public:
    explicit BinTreeView(QWidget* parent = nullptr);

signals:
    void clipActivated(const QString& binClipId);
    void clipDroppedOnTimeline(const QString& binClipId, int positionFrames);

protected:
    void mouseDoubleClickEvent(QMouseEvent* e) override;
    void startDrag(Qt::DropActions supportedActions) override;
};

} // namespace ve

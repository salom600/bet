#pragma once

#include <QListView>

namespace ve {

/// QListView subclass for the Bin's icon view mode.
/// Supports drag of bin clips onto the timeline.
class BinIconView : public QListView {
    Q_OBJECT
public:
    explicit BinIconView(QWidget* parent = nullptr);

signals:
    void clipActivated(const QString& binClipId);

protected:
    void mouseDoubleClickEvent(QMouseEvent* e) override;
    void startDrag(Qt::DropActions supportedActions) override;
};

} // namespace ve

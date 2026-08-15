#pragma once

#include <QStyledItemDelegate>

namespace ve {

/// Custom delegate for the Bin's icon view.
/// Renders: thumbnail (centered), name (below), duration badge (overlay on thumbnail).
class BinItemDelegate : public QStyledItemDelegate {
    Q_OBJECT
public:
    explicit BinItemDelegate(QObject* parent = nullptr);

    void paint(QPainter* painter, const QStyleOptionViewItem& option,
               const QModelIndex& index) const override;
    QSize sizeHint(const QStyleOptionViewItem& option,
                   const QModelIndex& index) const override;
};

} // namespace ve

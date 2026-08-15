#include "ui/bin/BinItemDelegate.h"
#include "model/BinModel.h"
#include "model/BinClip.h"

#include <QPainter>
#include <QStyle>
#include <QApplication>
#include <QFileInfo>
#include <QDebug>

namespace ve {

BinItemDelegate::BinItemDelegate(QObject* parent)
    : QStyledItemDelegate(parent)
{
}

void BinItemDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option,
                             const QModelIndex& index) const {
    if (!index.isValid()) return;

    painter->save();

    // Draw selection background
    if (option.state & QStyle::State_Selected) {
        painter->fillRect(option.rect, QColor(0x5a, 0xc8, 0xfa));
    } else if (option.state & QStyle::State_MouseOver) {
        painter->fillRect(option.rect, QColor(0x2a, 0x2d, 0x33));
    }

    QString type = index.data(BinModel::TypeRole).toString();
    QRect rect = option.rect.adjusted(4, 4, -4, -4);

    if (type == "folder") {
        // Draw folder icon (simple drawn rectangle for now)
        QRect folderRect = QRect(rect.center().x() - 24, rect.center().y() - 16, 48, 32);
        painter->setBrush(QColor(0x5a, 0x8f, 0xc8));
        painter->setPen(QColor(0x2a, 0x4a, 0x6a));
        painter->drawRoundedRect(folderRect, 4, 4);
        // Folder tab
        QRect tabRect = QRect(folderRect.x(), folderRect.y() - 6, 18, 8);
        painter->drawRoundedRect(tabRect, 2, 2);
        // Folder name
        QString name = index.data(BinModel::NameRole).toString();
        painter->setPen(option.state & QStyle::State_Selected ? QColor(0,0,0) : QColor(0xe0, 0xe0, 0xe6));
        painter->drawText(QRect(rect.x(), folderRect.bottom() + 4, rect.width(), 20),
                          Qt::AlignHCenter | Qt::AlignTop, name);
    } else if (type == "clip") {
        // Thumbnail area: 120x80 centered horizontally, top-aligned
        QRect thumbRect = QRect(rect.center().x() - 60, rect.y(), 120, 80);

        // Draw thumbnail background (dark)
        painter->setBrush(QColor(0x1a, 0x1c, 0x20));
        painter->setPen(QColor(0x2a, 0x2d, 0x33));
        painter->drawRoundedRect(thumbRect, 4, 4);

        // Draw actual thumbnail if available
        QVariant thumbVar = index.data(BinModel::ThumbnailRole);
        if (thumbVar.isValid() && thumbVar.canConvert<QImage>()) {
            QImage img = thumbVar.value<QImage>();
            if (!img.isNull()) {
                QImage scaled = img.scaled(thumbRect.size(), Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
                // Crop to thumbnail rect
                QRect crop(QPoint((scaled.width() - thumbRect.width()) / 2,
                                  (scaled.height() - thumbRect.height()) / 2),
                           thumbRect.size());
                painter->drawImage(thumbRect, scaled, crop);
            }
        } else {
            // Draw type icon placeholder
            painter->setPen(QColor(0x8a, 0x8d, 0x96));
            QFont iconFont = painter->font();
            iconFont.setPointSize(20);
            painter->setFont(iconFont);
            QString placeholder;
            QString path = index.data(BinModel::PathRole).toString();
            QString ext = QFileInfo(path).suffix().toLower();
            if (ext == "mp3" || ext == "wav" || ext == "aac" || ext == "flac" || ext == "ogg") {
                placeholder = "♪";
                painter->setPen(QColor(0x50, 0xb4, 0x82));
            } else {
                placeholder = "🎬";
                painter->setPen(QColor(0x5a, 0xc8, 0xfa));
            }
            painter->drawText(thumbRect, Qt::AlignCenter, placeholder);
        }

        // Duration badge (bottom-right of thumbnail)
        double duration = index.data(BinModel::DurationRole).toDouble();
        if (duration > 0) {
            int mins = static_cast<int>(duration) / 60;
            int secs = static_cast<int>(duration) % 60;
            QString durText = QString("%1:%2").arg(mins).arg(secs, 2, 10, QChar('0'));
            QFont badgeFont = painter->font();
            badgeFont.setPointSize(8);
            badgeFont.setBold(true);
            painter->setFont(badgeFont);
            QRect badgeRect = QRect(thumbRect.right() - 50, thumbRect.bottom() - 18, 46, 14);
            painter->setBrush(QColor(0, 0, 0, 180));
            painter->setPen(Qt::NoPen);
            painter->drawRoundedRect(badgeRect, 3, 3);
            painter->setPen(QColor(0xff, 0xff, 0xff));
            painter->drawText(badgeRect, Qt::AlignCenter, durText);
        }

        // Selection border around thumbnail
        if (option.state & QStyle::State_Selected) {
            painter->setPen(QPen(QColor(0x5a, 0xc8, 0xfa), 2));
            painter->setBrush(Qt::NoBrush);
            painter->drawRoundedRect(thumbRect, 4, 4);
        }

        // Clip name (below thumbnail)
        QString name = index.data(BinModel::NameRole).toString();
        QFont nameFont = painter->font();
        nameFont.setPointSize(9);
        painter->setFont(nameFont);
        painter->setPen(option.state & QStyle::State_Selected ? QColor(0, 0, 0) : QColor(0xe0, 0xe0, 0xe6));
        QRect nameRect(rect.x(), thumbRect.bottom() + 4, rect.width(), 28);
        painter->drawText(nameRect, Qt::AlignHCenter | Qt::AlignTop | Qt::TextWordWrap, name);
    }

    painter->restore();
}

QSize BinItemDelegate::sizeHint(const QStyleOptionViewItem& option,
                                 const QModelIndex& index) const {
    Q_UNUSED(option);
    Q_UNUSED(index);
    return QSize(140, 120);
}

} // namespace ve

#include "ui/bin/BinIconView.h"
#include "model/BinModel.h"
#include <QMouseEvent>
#include <QDrag>
#include <QMimeData>
#include <QDebug>

namespace ve {

BinIconView::BinIconView(QWidget* parent) : QListView(parent) {
    setViewMode(QListView::IconMode);
    setGridSize(QSize(140, 120));
    setIconSize(QSize(120, 80));
    setMovement(QListView::Static);
    setResizeMode(QListView::Adjust);
    setUniformItemSizes(true);
    setWordWrap(true);
    setDragEnabled(true);
    setAcceptDrops(false);
    setSelectionMode(QAbstractItemView::SingleSelection);
    setSelectionBehavior(QAbstractItemView::SelectRows);
}

void BinIconView::mouseDoubleClickEvent(QMouseEvent* e) {
    QModelIndex ix = indexAt(e->pos());
    if (ix.isValid() && ix.data(BinModel::TypeRole).toString() == "clip") {
        emit clipActivated(ix.data(BinModel::IdRole).toString());
    }
    QListView::mouseDoubleClickEvent(e);
}

void BinIconView::startDrag(Qt::DropActions supportedActions) {
    QModelIndex ix = currentIndex();
    if (!ix.isValid()) return;
    if (ix.data(BinModel::TypeRole).toString() != "clip") return;
    QString clipId = ix.data(BinModel::IdRole).toString();

    auto* mime = new QMimeData;
    mime->setData("application/x-ve-binclip", clipId.toUtf8());
    auto* drag = new QDrag(this);
    drag->setMimeData(mime);
    // Set a drag pixmap from the clip thumbnail if available
    QVariant thumbVar = ix.data(BinModel::ThumbnailRole);
    if (thumbVar.isValid() && thumbVar.canConvert<QImage>()) {
        QImage img = thumbVar.value<QImage>();
        if (!img.isNull()) {
            QPixmap pm = QPixmap::fromImage(img).scaled(120, 80, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            drag->setPixmap(pm);
            drag->setHotSpot(QPoint(pm.width()/2, pm.height()/2));
        }
    }
    drag->exec(Qt::CopyAction);
    Q_UNUSED(supportedActions);
}

} // namespace ve

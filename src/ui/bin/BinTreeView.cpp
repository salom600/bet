#include "ui/bin/BinTreeView.h"
#include "model/BinModel.h"
#include <QDrag>
#include <QMimeData>
#include <QMouseEvent>
#include <QModelIndex>
#include <QHeaderView>

namespace ve {

BinTreeView::BinTreeView(QWidget* parent)
    : QTreeView(parent)
{
    setDragEnabled(true);
    setAcceptDrops(true);
    setDefaultDropAction(Qt::CopyAction);
    setSelectionMode(QAbstractItemView::SingleSelection);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setRootIsDecorated(true);
    setUniformRowHeights(true);
    setHeaderHidden(false);
    header()->setStretchLastSection(true);
    header()->setSectionResizeMode(0, QHeaderView::Stretch);
}

void BinTreeView::mouseDoubleClickEvent(QMouseEvent* e) {
    QModelIndex ix = indexAt(e->pos());
    if (ix.isValid() && ix.data(BinModel::TypeRole).toString() == "clip") {
        emit clipActivated(ix.data(BinModel::IdRole).toString());
    }
    QTreeView::mouseDoubleClickEvent(e);
}

void BinTreeView::startDrag(Qt::DropActions supportedActions) {
    QModelIndex ix = currentIndex();
    if (!ix.isValid()) return;
    if (ix.data(BinModel::TypeRole).toString() != "clip") return;
    QString clipId = ix.data(BinModel::IdRole).toString();

    auto* mime = new QMimeData;
    mime->setData("application/x-ve-binclip", clipId.toUtf8());
    auto* drag = new QDrag(this);
    drag->setMimeData(mime);
    drag->exec(Qt::CopyAction);
    Q_UNUSED(supportedActions);
}

} // namespace ve
